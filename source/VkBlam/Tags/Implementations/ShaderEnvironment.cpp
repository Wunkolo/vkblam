#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/ShaderEnvironment.hpp>

#include <VkBlam/Tags/Implementations/Bitmap.hpp>
#include <VkBlam/Tags/Implementations/Globals.hpp>
#include <VkBlam/Tags/TagPool.hpp>

#include <Vulkan/Memory.hpp>

#include <tuple>

namespace
{

std::tuple<vk::UniquePipeline, vk::UniquePipelineLayout> CreateGraphicsPipeline(
	vk::Device Device, std::span<const vk::PushConstantRange> PushConstants,
	std::span<const vk::DescriptorSetLayout> SetLayouts,
	vk::ShaderModule VertModule, vk::ShaderModule FragModule,
	std::span<const vk::VertexInputBindingDescription>
		VertexBindingDescriptions,
	std::span<const vk::VertexInputAttributeDescription>
				   VertexAttributeDescriptions,
	vk::RenderPass RenderPass, vk::SampleCountFlagBits RenderSamples,
	vk::PolygonMode PolygonMode
)
{
	// Create Pipeline Layout
	const vk::PipelineLayoutCreateInfo GraphicsPipelineLayoutInfo = {
		.setLayoutCount = static_cast<std::uint32_t>(SetLayouts.size()),
		.pSetLayouts    = SetLayouts.data(),
		.pushConstantRangeCount
		= static_cast<std::uint32_t>(PushConstants.size()),
		.pPushConstantRanges = PushConstants.data(),
	};

	vk::UniquePipelineLayout GraphicsPipelineLayout = {};
	if( auto CreateResult
		= Device.createPipelineLayoutUnique(GraphicsPipelineLayoutInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		GraphicsPipelineLayout = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating pipeline layout: %s\n",
			vk::to_string(CreateResult.result).c_str()
		);
		return {};
	}

	// Describe the stage and entry point of each shader
	const vk::PipelineShaderStageCreateInfo ShaderStagesInfo[2] = {
		vk::PipelineShaderStageCreateInfo{
			.stage  = vk::ShaderStageFlagBits::eVertex,
			.module = VertModule,
			.pName  = "main",
		},
		vk::PipelineShaderStageCreateInfo{
			.stage  = vk::ShaderStageFlagBits::eFragment,
			.module = FragModule,
			.pName  = "main",
		},
	};

	const vk::PipelineVertexInputStateCreateInfo VertexInputState = {
		.vertexBindingDescriptionCount
		= static_cast<std::uint32_t>(VertexBindingDescriptions.size()),
		.pVertexBindingDescriptions = VertexBindingDescriptions.data(),
		.vertexAttributeDescriptionCount
		= static_cast<std::uint32_t>(VertexAttributeDescriptions.size()),
		.pVertexAttributeDescriptions = VertexAttributeDescriptions.data(),
	};

	const vk::PipelineInputAssemblyStateCreateInfo InputAssemblyState = {
		.topology               = vk::PrimitiveTopology::eTriangleList,
		.primitiveRestartEnable = VK_FALSE,
	};

	static const vk::Viewport DefaultViewport = {0, 0, 16, 16, 0.0f, 1.0f};
	static const vk::Rect2D   DefaultScissor  = {{0, 0}, {16, 16}};
	static const vk::PipelineViewportStateCreateInfo ViewportState = {
		.viewportCount = 1,
		.pViewports    = &DefaultViewport,
		.scissorCount  = 1,
		.pScissors     = &DefaultScissor,
	};

	const vk::PipelineRasterizationStateCreateInfo RasterizationState = {
		.depthClampEnable        = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode             = PolygonMode,
		.cullMode                = vk::CullModeFlagBits::eBack,
		.frontFace               = vk::FrontFace::eClockwise,
		.depthBiasEnable         = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp          = 0.0f,
		.depthBiasSlopeFactor    = 0.0,
		.lineWidth               = 1.0f,
	};

	const vk::PipelineMultisampleStateCreateInfo MultisampleState = {
		.rasterizationSamples  = RenderSamples,
		.sampleShadingEnable   = VK_FALSE,
		.minSampleShading      = 1.0f,
		.pSampleMask           = nullptr,
		.alphaToCoverageEnable = VK_TRUE,
		.alphaToOneEnable      = VK_FALSE,
	};

	static const vk::PipelineDepthStencilStateCreateInfo DepthStencilState = {
		.depthTestEnable       = VK_TRUE,
		.depthWriteEnable      = VK_TRUE,
		.depthCompareOp        = vk::CompareOp::eLessOrEqual,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable     = VK_FALSE,
		.front                 = {},
		.back                  = {},
		.minDepthBounds        = 0.0f,
		.maxDepthBounds        = 1.0f,
	};

	static const vk::PipelineColorBlendAttachmentState BlendAttachmentState = {
		.blendEnable         = VK_FALSE,
		.srcColorBlendFactor = vk::BlendFactor::eZero,
		.dstColorBlendFactor = vk::BlendFactor::eZero,
		.colorBlendOp        = vk::BlendOp::eAdd,
		.srcAlphaBlendFactor = vk::BlendFactor::eZero,
		.dstAlphaBlendFactor = vk::BlendFactor::eZero,
		.alphaBlendOp        = vk::BlendOp::eAdd,
		.colorWriteMask
		= vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
		| vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
	};

	static const vk::PipelineColorBlendStateCreateInfo ColorBlendState = {
		.logicOpEnable   = VK_FALSE,
		.logicOp         = vk::LogicOp::eClear,
		.attachmentCount = 1,
		.pAttachments    = &BlendAttachmentState,
	};

	static const vk::DynamicState DynamicStates[] = {
		// The viewport and scissor of the framebuffer will be dynamic at
		// run-time
		// so we definately add these
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	static const vk::PipelineDynamicStateCreateInfo DynamicState = {
		.dynamicStateCount = std::size(DynamicStates),
		.pDynamicStates    = DynamicStates,
	};

	const vk::GraphicsPipelineCreateInfo RenderPipelineInfo = {
		.stageCount          = 2,
		.pStages             = ShaderStagesInfo,
		.pVertexInputState   = &VertexInputState,
		.pInputAssemblyState = &InputAssemblyState,
		.pViewportState      = &ViewportState,
		.pRasterizationState = &RasterizationState,
		.pMultisampleState   = &MultisampleState,
		.pDepthStencilState  = &DepthStencilState,
		.pColorBlendState    = &ColorBlendState,
		.pDynamicState       = &DynamicState,
		.layout              = GraphicsPipelineLayout.get(),
		.renderPass          = RenderPass,
		.subpass             = 0,
	};

	// Create Pipeline
	vk::UniquePipeline Pipeline
		= Device.createGraphicsPipelineUnique({}, RenderPipelineInfo).value;
	return std::make_tuple(
		std::move(Pipeline), std::move(GraphicsPipelineLayout)
	);
}

const auto [VertexBindingDescriptions, VertexAttributeDescriptions]
	= VkBlam::GetVertexInputDescriptions({{
		Blam::VertexFormat::SBSPVertexUncompressed,
		Blam::VertexFormat::SBSPLightmapVertexUncompressed,
	}});

vk::DescriptorSetLayoutBinding ShaderEnvironmentBindings[] = {
	{// Basemap
	 0, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// PrimaryDetailMap
	 1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// SecondaryDetailMap
	 2, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// MicroDetailMap
	 3, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// BumpMap
	 4, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// GlowMap
	 5, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// ReflectionCubeMap
	 6, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
};

static vk::ImageViewType ImageTypes[] = {
	// BaseMap
	vk::ImageViewType::e2D,
	// BumpMap
	vk::ImageViewType::e2D,
	// PrimaryDetailMap
	vk::ImageViewType::e2D,
	// SecondaryDetailMap
	vk::ImageViewType::e2D,
	// MicroDetailMap
	vk::ImageViewType::e2D,
	// GlowMap
	vk::ImageViewType::e2D,
	// ReflectionCubeMap
	vk::ImageViewType::eCube,
};

} // namespace

namespace VkBlam::Tags
{

ShaderEnvironment::ShaderEnvironment(
	const Blam::TagIndexEntry&                          TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ShaderEnvironment>& Tag
)
	: TagImplementation<Blam::TagClass::ShaderEnvironment>(TagIndexEntry, Tag)
{
}

ShaderEnvironment::~ShaderEnvironment()
{
}

vk::DescriptorSet ShaderEnvironment::GetDescriptorSet() const
{
	return DescriptorSet;
}

ShaderEnvironmentSubsystem::ShaderEnvironmentSubsystem(
	TagPool& Pool, VkBlam::Rasterizer& Rasterizer
)
	: TagSubsystem<Blam::TagClass::ShaderEnvironment, ShaderEnvironment>(Pool),
	  Rasterizer(Rasterizer)
{
	const auto& VulkanContext = Rasterizer.GetVulkanContext();

	ShaderEnvironmentDescriptorPool = std::make_unique<Vulkan::DescriptorHeap>(
		Vulkan::DescriptorHeap::Create(
			Rasterizer.GetVulkanContext(), ShaderEnvironmentBindings
		)
			.value()
	);
	const auto ShaderEnvironmentVertShaderData
		= VkBlam::OpenResource("shaders/Default.vert.spv").value();
	const auto ShaderEnvironmentFragShaderData
		= VkBlam::OpenResource("shaders/Default.frag.spv").value();

	std::hash<std::string> StringHasher = {};

	ShaderEnvironmentVertexShaderModule
		= Rasterizer.GetShaderModuleCache()
			  .GetShaderModule(
				  StringHasher("shaders/Default.vert.spv"),
				  ShaderEnvironmentVertShaderData
			  )
			  .value();
	ShaderEnvironmentFragmentShaderModule
		= Rasterizer.GetShaderModuleCache()
			  .GetShaderModule(
				  StringHasher("shaders/Default.frag.spv"),
				  ShaderEnvironmentFragShaderData
			  )
			  .value();

	const vk::RenderPass RenderPass
		= Rasterizer.GetDefaultRenderPass(RenderSamples);

	// std::tie(ShaderEnvironmentPipeline, ShaderEnvironmentPipelineLayout)
	// 	= CreateGraphicsPipeline(
	// 		VulkanContext.LogicalDevice,
	// 		{{vk::PushConstantRange{
	// 			.stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
	// 			.offset     = 0,
	// 			.size       = sizeof(VkBlam::CameraGlobals),
	// 		}}},
	// 		{{SceneDescriptorPool->GetDescriptorSetLayout(),
	// 		  ShaderEnvironmentDescriptorPool->GetDescriptorSetLayout(),
	// 		  DebugDrawDescriptorPool->GetDescriptorSetLayout()}},
	// 		ShaderEnvironmentVertexShaderModule,
	// 		ShaderEnvironmentFragmentShaderModule, VertexBindingDescriptions,
	// 		VertexAttributeDescriptions, RenderPass, RenderSamples,
	// 		vk::PolygonMode::eFill
	// 	);
}

ShaderEnvironmentSubsystem::~ShaderEnvironmentSubsystem()
{
}

std::vector<DependentTag> ShaderEnvironmentSubsystem::GetDependentTags(
	const Blam::TagIndexEntry&                          TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ShaderEnvironment>& Tag,
	const Blam::MapFile&                                MapFile
) const
{
	std::vector<DependentTag> DependentTags;

	// Default2D/3D/Cube textures
	if( const Blam::TagIndexEntry* GlobalsTagEntry
		= MapFile.FindTagIndexEntry("globals\\globals");
		GlobalsTagEntry )
	{
		DependentTags.push_back({GlobalsTagEntry->TagID});
	}

	if( Tag.BaseMap.Valid() )
	{
		DependentTags.push_back({Tag.BaseMap.TagID});
	}
	if( Tag.PrimaryDetailMap.Valid() )
	{
		DependentTags.push_back({Tag.PrimaryDetailMap.TagID});
	}
	if( Tag.SecondaryDetailMap.Valid() )
	{
		DependentTags.push_back({Tag.SecondaryDetailMap.TagID});
	}
	if( Tag.MicroDetailMap.Valid() )
	{
		DependentTags.push_back({Tag.MicroDetailMap.TagID});
	}
	if( Tag.BumpMap.Valid() )
	{
		DependentTags.push_back({Tag.BumpMap.TagID});
	}
	if( Tag.GlowMap.Valid() )
	{
		DependentTags.push_back({Tag.GlowMap.TagID});
	}
	if( Tag.ReflectionCubeMap.Valid() )
	{
		DependentTags.push_back({Tag.ReflectionCubeMap.TagID});
	}

	return DependentTags;
}

ShaderEnvironment* ShaderEnvironmentSubsystem::LoadTag(
	const Blam::TagIndexEntry&                          TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ShaderEnvironment>& Tag, Scene& TargetScene
)
{
	std::unique_ptr<ShaderEnvironment> NewShaderEnvironment(
		new ShaderEnvironment(TagIndexEntry, Tag)
	);

	// Create descriptor set

	if( auto AllocResult
		= ShaderEnvironmentDescriptorPool->AllocateDescriptorSet();
		AllocResult.has_value() )
	{
		NewShaderEnvironment->DescriptorSet = AllocResult.value();
	}
	else
	{
		// Error allocating new descriptor set
		return nullptr;
	}

	Vulkan::SetObjectName(
		Rasterizer.GetVulkanContext().LogicalDevice,
		NewShaderEnvironment->DescriptorSet, "ShaderEnvironment[{:08X}]: {}",
		TagIndexEntry.TagID,
		TargetScene.GetMapFile().GetTagPath(TagIndexEntry.TagID)
	);

	// Get default rasterizer images from globals;
	const Blam::TagIndexEntry* GlobalsTagEntry
		= TargetScene.GetMapFile().FindTagIndexEntry("globals\\globals");

	if( GlobalsTagEntry == nullptr )
	{
		// Error getting globals tag
		return nullptr;
	}

	const Blam::Tag<Blam::TagClass::Globals>* Globals
		= TargetScene.GetMapFile().GetTag<Blam::TagClass::Globals>(
			GlobalsTagEntry->TagID
		);

	if( Globals == nullptr )
	{
		// Error loading globals tag
		return nullptr;
	}

	const auto RasterizerData
		= TargetScene.GetMapFile().TagHeap.GetBlock(Globals->RasterizerData)[0];

	const auto WriteImageTag
		= [&](std::uint8_t Binding, std::uint32_t TagID,
			  Blam::DefaultTextureIndex DefaultIndex) -> void {
		if( TagID == 0xFFFFFFFF )
		{
			std::uint32_t DefaultImageTag;
			switch( ImageTypes[Binding] )
			{
			default:
			case vk::ImageViewType::e2D:
				DefaultImageTag = RasterizerData.Default2D.TagID;
				break;
			case vk::ImageViewType::e3D:
				DefaultImageTag = RasterizerData.Default3D.TagID;
				break;
			case vk::ImageViewType::eCube:
				DefaultImageTag = RasterizerData.DefaultCube.TagID;
				break;
			}
			Rasterizer.GetDescriptorUpdateBatch().AddImage(
				NewShaderEnvironment->DescriptorSet, Binding,
				GetPool()
					.LoadTag<Tags::Bitmap>(DefaultImageTag)
					->GetBitmap(std::uint16_t(DefaultIndex))
					.View.get()
			);
			return;
		}
		Rasterizer.GetDescriptorUpdateBatch().AddImage(
			NewShaderEnvironment->DescriptorSet, Binding,
			GetPool().LoadTag<Tags::Bitmap>(TagID)->GetBitmap(0).View.get()
		);
	};

	WriteImageTag(0, Tag.BaseMap.TagID, Blam::DefaultTextureIndex::Additive);
	WriteImageTag(1, Tag.BumpMap.TagID, Blam::DefaultTextureIndex::Vector);
	WriteImageTag(
		2, Tag.PrimaryDetailMap.TagID, Blam::DefaultTextureIndex::SignedAdditive
	);
	WriteImageTag(
		3, Tag.SecondaryDetailMap.TagID,
		Blam::DefaultTextureIndex::SignedAdditive
	);
	WriteImageTag(
		4, Tag.MicroDetailMap.TagID, Blam::DefaultTextureIndex::SignedAdditive
	);
	WriteImageTag(5, Tag.GlowMap.TagID, Blam::DefaultTextureIndex::Additive);
	WriteImageTag(
		6, Tag.ReflectionCubeMap.TagID, Blam::DefaultTextureIndex::Additive
	);

	Rasterizer.GetDescriptorUpdateBatch().Flush();

	return ShaderEnvironments.emplace_back(std::move(NewShaderEnvironment))
		.get();
}
} // namespace VkBlam::Tags