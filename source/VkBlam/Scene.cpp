#include <VkBlam/Scene.hpp>

#include <Vulkan/Memory.hpp>
#include <Vulkan/Pipeline.hpp>

#include <Common/Format.hpp>

std::tuple<vk::UniquePipeline, vk::UniquePipelineLayout> CreateGraphicsPipeline(
	vk::Device Device, std::span<const vk::PushConstantRange> PushConstants,
	std::span<const vk::DescriptorSetLayout> SetLayouts,
	vk::ShaderModule VertModule, vk::ShaderModule FragModule,
	vk::RenderPass RenderPass, vk::SampleCountFlagBits RenderSamples,
	vk::PolygonMode PolygonMode)
{
	// Create Pipeline Layout
	vk::PipelineLayoutCreateInfo GraphicsPipelineLayoutInfo = {};

	GraphicsPipelineLayoutInfo.pSetLayouts            = SetLayouts.data();
	GraphicsPipelineLayoutInfo.setLayoutCount         = SetLayouts.size();
	GraphicsPipelineLayoutInfo.pPushConstantRanges    = PushConstants.data();
	GraphicsPipelineLayoutInfo.pushConstantRangeCount = PushConstants.size();

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
			vk::to_string(CreateResult.result).c_str());
		return {};
	}

	// Describe the stage and entry point of each shader
	const vk::PipelineShaderStageCreateInfo ShaderStagesInfo[2] = {
		vk::PipelineShaderStageCreateInfo(
			{},                               // Flags
			vk::ShaderStageFlagBits::eVertex, // Shader Stage
			VertModule,                       // Shader Module
			"main", // Shader entry point function name
			{}      // Shader specialization info
			),
		vk::PipelineShaderStageCreateInfo(
			{},                                 // Flags
			vk::ShaderStageFlagBits::eFragment, // Shader Stage
			FragModule,                         // Shader Module
			"main", // Shader entry point function name
			{}      // Shader specialization info
			),
	};

	vk::PipelineVertexInputStateCreateInfo VertexInputState = {};

	static std::array<vk::VertexInputBindingDescription, 2>
		VertexBindingDescriptions
		= {Vulkan::CreateVertexInputBinding<Blam::Vertex>(0),
		   Vulkan::CreateVertexInputBinding<Blam::LightmapVertex>(1)};

	VertexInputState.vertexBindingDescriptionCount
		= std::size(VertexBindingDescriptions);
	VertexInputState.pVertexBindingDescriptions
		= VertexBindingDescriptions.data();

	static std::array<vk::VertexInputAttributeDescription, 7>
		AttributeDescriptions = {};
	// Position
	AttributeDescriptions[0].binding  = 0;
	AttributeDescriptions[0].location = 0;
	AttributeDescriptions[0].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[0].offset   = offsetof(Blam::Vertex, Position);
	// Normal
	AttributeDescriptions[1].binding  = 0;
	AttributeDescriptions[1].location = 1;
	AttributeDescriptions[1].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[1].offset   = offsetof(Blam::Vertex, Normal);
	// Binormal
	AttributeDescriptions[2].binding  = 0;
	AttributeDescriptions[2].location = 2;
	AttributeDescriptions[2].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[2].offset   = offsetof(Blam::Vertex, Binormal);
	// Tangent
	AttributeDescriptions[3].binding  = 0;
	AttributeDescriptions[3].location = 3;
	AttributeDescriptions[3].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[3].offset   = offsetof(Blam::Vertex, Tangent);
	// UV
	AttributeDescriptions[4].binding  = 0;
	AttributeDescriptions[4].location = 4;
	AttributeDescriptions[4].format   = vk::Format::eR32G32Sfloat;
	AttributeDescriptions[4].offset   = offsetof(Blam::Vertex, UV);

	// Normal-Lightmap
	AttributeDescriptions[5].binding  = 1;
	AttributeDescriptions[5].location = 5;
	AttributeDescriptions[5].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[5].offset   = offsetof(Blam::LightmapVertex, Normal);
	// UV-Lightmap
	AttributeDescriptions[6].binding  = 1;
	AttributeDescriptions[6].location = 6;
	AttributeDescriptions[6].format   = vk::Format::eR32G32Sfloat;
	AttributeDescriptions[6].offset   = offsetof(Blam::LightmapVertex, UV);

	VertexInputState.vertexAttributeDescriptionCount
		= AttributeDescriptions.size();
	VertexInputState.pVertexAttributeDescriptions
		= AttributeDescriptions.data();

	vk::PipelineInputAssemblyStateCreateInfo InputAssemblyState = {};
	InputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
	InputAssemblyState.primitiveRestartEnable = false;

	vk::PipelineViewportStateCreateInfo ViewportState = {};

	static const vk::Viewport DefaultViewport = {0, 0, 16, 16, 0.0f, 1.0f};
	static const vk::Rect2D   DefaultScissor  = {{0, 0}, {16, 16}};
	ViewportState.viewportCount               = 1;
	ViewportState.pViewports                  = &DefaultViewport;
	ViewportState.scissorCount                = 1;
	ViewportState.pScissors                   = &DefaultScissor;

	vk::PipelineRasterizationStateCreateInfo RasterizationState = {};

	RasterizationState.depthClampEnable        = false;
	RasterizationState.rasterizerDiscardEnable = false;
	RasterizationState.polygonMode             = PolygonMode;
	RasterizationState.cullMode                = vk::CullModeFlagBits::eBack;
	RasterizationState.frontFace               = vk::FrontFace::eClockwise;
	RasterizationState.depthBiasEnable         = false;
	RasterizationState.depthBiasConstantFactor = 0.0f;
	RasterizationState.depthBiasClamp          = 0.0f;
	RasterizationState.depthBiasSlopeFactor    = 0.0;
	RasterizationState.lineWidth               = 1.0f;

	vk::PipelineMultisampleStateCreateInfo MultisampleState = {};

	MultisampleState.rasterizationSamples  = RenderSamples;
	MultisampleState.sampleShadingEnable   = true;
	MultisampleState.minSampleShading      = 1.0f;
	MultisampleState.pSampleMask           = nullptr;
	MultisampleState.alphaToCoverageEnable = false;
	MultisampleState.alphaToOneEnable      = false;

	vk::PipelineDepthStencilStateCreateInfo DepthStencilState = {};

	DepthStencilState.depthTestEnable       = true;
	DepthStencilState.depthWriteEnable      = true;
	DepthStencilState.depthCompareOp        = vk::CompareOp::eLessOrEqual;
	DepthStencilState.depthBoundsTestEnable = false;
	DepthStencilState.stencilTestEnable     = false;
	DepthStencilState.front                 = vk::StencilOp::eKeep;
	DepthStencilState.back                  = vk::StencilOp::eKeep;
	DepthStencilState.minDepthBounds        = 0.0f;
	DepthStencilState.maxDepthBounds        = 1.0f;

	vk::PipelineColorBlendStateCreateInfo ColorBlendState = {};

	ColorBlendState.logicOpEnable   = false;
	ColorBlendState.logicOp         = vk::LogicOp::eClear;
	ColorBlendState.attachmentCount = 1;

	vk::PipelineColorBlendAttachmentState BlendAttachmentState = {};

	BlendAttachmentState.blendEnable         = false;
	BlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eZero;
	BlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eZero;
	BlendAttachmentState.colorBlendOp        = vk::BlendOp::eAdd;
	BlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eZero;
	BlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	BlendAttachmentState.alphaBlendOp        = vk::BlendOp::eAdd;
	BlendAttachmentState.colorWriteMask
		= vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
		| vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	ColorBlendState.pAttachments = &BlendAttachmentState;

	vk::PipelineDynamicStateCreateInfo DynamicState = {};
	vk::DynamicState                   DynamicStates[]
		= {// The viewport and scissor of the framebuffer will be dynamic at
		   // run-time
		   // so we definately add these
		   vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	DynamicState.dynamicStateCount = std::size(DynamicStates);
	DynamicState.pDynamicStates    = DynamicStates;

	vk::GraphicsPipelineCreateInfo RenderPipelineInfo = {};

	RenderPipelineInfo.stageCount          = 2; // Vert + Frag stages
	RenderPipelineInfo.pStages             = ShaderStagesInfo;
	RenderPipelineInfo.pVertexInputState   = &VertexInputState;
	RenderPipelineInfo.pInputAssemblyState = &InputAssemblyState;
	RenderPipelineInfo.pViewportState      = &ViewportState;
	RenderPipelineInfo.pRasterizationState = &RasterizationState;
	RenderPipelineInfo.pMultisampleState   = &MultisampleState;
	RenderPipelineInfo.pDepthStencilState  = &DepthStencilState;
	RenderPipelineInfo.pColorBlendState    = &ColorBlendState;
	RenderPipelineInfo.pDynamicState       = &DynamicState;
	RenderPipelineInfo.subpass             = 0;
	RenderPipelineInfo.renderPass          = RenderPass;
	RenderPipelineInfo.layout              = GraphicsPipelineLayout.get();

	// Create Pipeline
	vk::UniquePipeline Pipeline
		= Device.createGraphicsPipelineUnique({}, RenderPipelineInfo).value;
	return std::make_tuple(
		std::move(Pipeline), std::move(GraphicsPipelineLayout));
}

static vk::DescriptorSetLayoutBinding SceneBindings[] = {
	{// Default2DSamplerFiltered
	 0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment},
	{// Default2DSamplerUnfiltered
	 1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment},
};

namespace VkBlam
{
Scene::Scene(Renderer& TargetRenderer, const World& TargetWorld)
	: TargetRenderer(TargetRenderer), TargetWorld(TargetWorld)
{
}

Scene::~Scene()
{
}

void Scene::Render(const SceneView& View, vk::CommandBuffer CommandBuffer)
{

	vk::Viewport Viewport = {};
	Viewport.width        = float(View.Viewport.x);
	Viewport.height       = -float(View.Viewport.y);
	Viewport.x            = 0.0f;
	Viewport.y            = float(View.Viewport.y);
	Viewport.minDepth     = 0.0f;
	Viewport.maxDepth     = 1.0f;
	CommandBuffer.setViewport(0, {Viewport});
	// Scissor
	vk::Rect2D Scissor    = {};
	Scissor.extent.width  = View.Viewport.x;
	Scissor.extent.height = View.Viewport.y;
	CommandBuffer.setScissor(0, {Scissor});

	CommandBuffer.bindPipeline(
		vk::PipelineBindPoint::eGraphics, DebugDrawPipeline.get());

	CommandBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(), 0,
		{CurSceneDescriptor}, {});

	CommandBuffer.pushConstants<VkBlam::CameraGlobals>(
		DebugDrawPipelineLayout.get(), vk::ShaderStageFlagBits::eAllGraphics, 0,
		{View.CameraGlobalsData});

	CommandBuffer.bindVertexBuffers(
		0, {BSPVertexBuffer.get(), BSPLightmapVertexBuffer.get()}, {0, 0});

	CommandBuffer.bindIndexBuffer(
		BSPIndexBuffer.get(), 0, vk::IndexType::eUint16);

	for( std::size_t i = 0; i < LightmapMeshs.size(); ++i )
	{
		const auto& CurLightmapMesh = LightmapMeshs[i];
		Vulkan::InsertDebugLabel(
			CommandBuffer, {0.5, 0.5, 0.5, 1.0}, "BSP Draw: %zu", i);
		if( CurLightmapMesh.LightmapTag.has_value()
			&& CurLightmapMesh.LightmapIndex.has_value() )
		{
			CommandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
				1,
				{BitmapHeap.Sets.at(CurLightmapMesh.LightmapTag.value())
					 .at(CurLightmapMesh.LightmapIndex.value())},
				{});
		}
		else
		{
			CommandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
				1,
				{BitmapHeap.Sets.at(BitmapHeap.Default2D)
					 .at(std::uint32_t(
						 Blam::DefaultTextureIndex::Multiplicative))},
				{});
		}
		if( CurLightmapMesh.BasemapTag.has_value() )
		{
			CommandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
				2,
				{BitmapHeap.Sets.at(CurLightmapMesh.BasemapTag.value()).at(0)},
				{});
		}
		else
		{
			CommandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
				2,
				{BitmapHeap.Sets.at(BitmapHeap.Default2D)
					 .at(std::uint32_t(Blam::DefaultTextureIndex::Additive))},
				{});
		}
		CommandBuffer.drawIndexed(
			CurLightmapMesh.IndexCount, 1, CurLightmapMesh.IndexOffset,
			CurLightmapMesh.VertexIndexOffset, 0);
	}
}

std::optional<Scene>
	Scene::Create(Renderer& TargetRenderer, const World& TargetWorld)
{
	Scene NewScene(TargetRenderer, TargetWorld);

	const Vulkan::Context& VulkanContext = TargetRenderer.GetVulkanContext();

	// Descriptor pools
	{
		NewScene.DebugDrawDescriptorPool
			= std::make_unique<Vulkan::DescriptorHeap>(
				Vulkan::DescriptorHeap::Create(
					VulkanContext, {{vk::DescriptorSetLayoutBinding(
									   0, vk::DescriptorType::eSampledImage, 1,
									   vk::ShaderStageFlagBits::eFragment)}})
					.value());

		NewScene.UnlitDescriptorPool = std::make_unique<Vulkan::DescriptorHeap>(
			Vulkan::DescriptorHeap::Create(
				VulkanContext, {{vk::DescriptorSetLayoutBinding(
								   0, vk::DescriptorType::eSampledImage, 1,
								   vk::ShaderStageFlagBits::eFragment)}})
				.value());
		NewScene.SceneDescriptorPool = std::make_unique<Vulkan::DescriptorHeap>(
			Vulkan::DescriptorHeap::Create(VulkanContext, SceneBindings)
				.value());
	}

	// Scene Descriptor
	{
		NewScene.CurSceneDescriptor
			= NewScene.SceneDescriptorPool->AllocateDescriptorSet().value();

		TargetRenderer.GetDescriptorUpdateBatch().AddSampler(
			NewScene.CurSceneDescriptor, 0,
			TargetRenderer.GetSamplerCache().GetSampler(Sampler2D()));
		TargetRenderer.GetDescriptorUpdateBatch().AddSampler(
			NewScene.CurSceneDescriptor, 1,
			TargetRenderer.GetSamplerCache().GetSampler(Sampler2D(false)));
	}

	{
		// Main Shader modules
		const auto DefaultVertShaderData
			= VkBlam::OpenResource("shaders/Default.vert.spv").value();
		const auto DefaultFragShaderData
			= VkBlam::OpenResource("shaders/Default.frag.spv").value();
		const auto UnlitFragShaderData
			= VkBlam::OpenResource("shaders/Unlit.frag.spv").value();

		NewScene.DefaultVertexShaderModule = Vulkan::CreateShaderModule(
			VulkanContext.LogicalDevice,
			std::span<const std::uint32_t>(
				reinterpret_cast<const std::uint32_t*>(
					DefaultVertShaderData.data()),
				DefaultVertShaderData.size() / sizeof(std::uint32_t)));
		NewScene.DefaultFragmentShaderModule = Vulkan::CreateShaderModule(
			VulkanContext.LogicalDevice,
			std::span<const std::uint32_t>(
				reinterpret_cast<const std::uint32_t*>(
					DefaultFragShaderData.data()),
				DefaultFragShaderData.size() / sizeof(std::uint32_t)));
		NewScene.UnlitFragmentShaderModule = Vulkan::CreateShaderModule(
			VulkanContext.LogicalDevice,
			std::span<const std::uint32_t>(
				reinterpret_cast<const std::uint32_t*>(
					UnlitFragShaderData.data()),
				UnlitFragShaderData.size() / sizeof(std::uint32_t)));

		const vk::RenderPass RenderPass
			= TargetRenderer.GetDefaultRenderPass(RenderSamples);

		std::tie(NewScene.DebugDrawPipeline, NewScene.DebugDrawPipelineLayout)
			= CreateGraphicsPipeline(
				VulkanContext.LogicalDevice,
				{{vk::PushConstantRange(
					vk::ShaderStageFlagBits::eAllGraphics, 0,
					sizeof(VkBlam::CameraGlobals))}},
				{{NewScene.SceneDescriptorPool->GetDescriptorSetLayout(),
				  NewScene.DebugDrawDescriptorPool->GetDescriptorSetLayout(),
				  NewScene.DebugDrawDescriptorPool->GetDescriptorSetLayout()}},
				NewScene.DefaultVertexShaderModule.get(),
				NewScene.DefaultFragmentShaderModule.get(), RenderPass,
				RenderSamples, vk::PolygonMode::eFill);

		std::tie(NewScene.UnlitDrawPipeline, NewScene.UnlitDrawPipelineLayout)
			= CreateGraphicsPipeline(
				VulkanContext.LogicalDevice,
				{{vk::PushConstantRange(
					vk::ShaderStageFlagBits::eAllGraphics, 0,
					sizeof(VkBlam::CameraGlobals))}},
				{{NewScene.UnlitDescriptorPool->GetDescriptorSetLayout(),
				  NewScene.UnlitDescriptorPool->GetDescriptorSetLayout()}},
				NewScene.DefaultVertexShaderModule.get(),
				NewScene.UnlitFragmentShaderModule.get(), RenderPass,
				RenderSamples, vk::PolygonMode::eLine);
	}

	// Load BSP
	{
		// Offset is in elements, not bytes
		std::uint32_t VertexHeapIndexOffset = 0;
		std::uint32_t IndexHeapIndexOffset  = 0;

		for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP&
				 CurBSPEntry : TargetWorld.GetMapFile().GetScenarioBSPs() )
		{
			const std::span<const std::byte> BSPData = CurBSPEntry.GetSBSPData(
				TargetWorld.GetMapFile().GetMapData().data());
			const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& ScenarioBSP
				= CurBSPEntry.GetSBSP(
					TargetWorld.GetMapFile().GetMapData().data());

			// Lightmap
			for( const auto& CurLightmap : ScenarioBSP.Lightmaps.GetSpan(
					 BSPData.data(), CurBSPEntry.BSPVirtualBase) )
			{
				const auto& LightmapTextureTag
					= TargetWorld.GetMapFile().GetTag<Blam::TagClass::Bitmap>(
						ScenarioBSP.LightmapTexture.TagID);
				const std::int16_t LightmapTextureIndex
					= CurLightmap.LightmapIndex;

				const auto Surfaces = ScenarioBSP.Surfaces.GetSpan(
					BSPData.data(), CurBSPEntry.BSPVirtualBase);
				for( const auto& CurMaterial : CurLightmap.Materials.GetSpan(
						 BSPData.data(), CurBSPEntry.BSPVirtualBase) )
				{

					std::printf(
						"Shader(%s): %s | Permutation: %04X\n",
						Blam::FormatTagClass(CurMaterial.Shader.Class).c_str(),
						TargetWorld.GetMapFile()
							.GetTagName(CurMaterial.Shader.TagID)
							.data(),
						CurMaterial.ShaderPermutation);

					auto& CurLightmapMesh
						= NewScene.LightmapMeshs.emplace_back();
					//// Vertex Buffer data
					{
						// Copy vertex data into the staging buffer
						const std::span<const Blam::Vertex> CurVertexData
							= CurMaterial.GetVertices(
								BSPData.data(), CurBSPEntry.BSPVirtualBase);

						CurLightmapMesh.VertexData = CurVertexData;

						// Add the offset needed to begin indexing into
						// this particular part of the vertex buffer,
						// used when drawing
						CurLightmapMesh.VertexIndexOffset
							= VertexHeapIndexOffset;

						if( CurMaterial.Shader.Class
							== Blam::TagClass::ShaderEnvironment )
						{
							auto* BasemapTag
								= TargetWorld.GetMapFile()
									  .GetTag<
										  Blam::TagClass::ShaderEnvironment>(
										  CurMaterial.Shader.TagID);
							if( BasemapTag->BaseMap.TagID != -1 )
							{
								CurLightmapMesh.BasemapTag
									= BasemapTag->BaseMap.TagID;
							}
						}

						if( ScenarioBSP.LightmapTexture.TagID != -1
							&& LightmapTextureIndex != -1 )
						{
							CurLightmapMesh.LightmapTag
								= ScenarioBSP.LightmapTexture.TagID;
							CurLightmapMesh.LightmapIndex
								= LightmapTextureIndex;
						}

						//// Lightmap vertex buffer data
						{
							const std::span<const Blam::LightmapVertex>
								CurLightmapVertexData
								= CurMaterial.GetLightmapVertices(
									BSPData.data(), CurBSPEntry.BSPVirtualBase);
							CurLightmapMesh.LightmapVertexData
								= CurLightmapVertexData;
						}

						VertexHeapIndexOffset += CurVertexData.size();
					}

					//// Index Buffer data
					{
						const std::span<const std::byte> CurIndexData
							= std::as_bytes(Surfaces.subspan(
								CurMaterial.SurfacesIndexStart,
								CurMaterial.SurfacesCount));

						CurLightmapMesh.IndexData = CurIndexData;

						CurLightmapMesh.IndexCount
							= CurMaterial.SurfacesCount * 3;

						CurLightmapMesh.IndexOffset = IndexHeapIndexOffset;

						// Increment offsets
						IndexHeapIndexOffset += CurMaterial.SurfacesCount * 3;
					}
				}
			}
		}

		//// Create Vertex buffer heap
		vk::BufferCreateInfo BSPVertexBufferInfo = {};
		BSPVertexBufferInfo.size = VertexHeapIndexOffset * sizeof(Blam::Vertex);
		BSPVertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer
								  | vk::BufferUsageFlagBits::eTransferDst;

		if( auto CreateResult = VulkanContext.LogicalDevice.createBufferUnique(
				BSPVertexBufferInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewScene.BSPVertexBuffer = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating vertex buffer: %s\n",
				vk::to_string(CreateResult.result).c_str());
			return {};
		}

		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewScene.BSPVertexBuffer.get(),
			"VkBlam::Scene: BSP Vertex Buffer( %s )",
			Common::FormatByteCount(BSPVertexBufferInfo.size).c_str());

		//// Create Vertex buffer heap
		vk::BufferCreateInfo BSPLightmapVertexBufferInfo = {};

		BSPLightmapVertexBufferInfo.size
			= VertexHeapIndexOffset * sizeof(Blam::LightmapVertex);
		BSPLightmapVertexBufferInfo.usage
			= vk::BufferUsageFlagBits::eVertexBuffer
			| vk::BufferUsageFlagBits::eTransferDst;

		if( auto CreateResult = VulkanContext.LogicalDevice.createBufferUnique(
				BSPLightmapVertexBufferInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewScene.BSPLightmapVertexBuffer = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating lightmap vertex buffer: %s\n",
				vk::to_string(CreateResult.result).c_str());
			return {};
		}

		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewScene.BSPLightmapVertexBuffer.get(),
			"VkBlam::Scene: BSP Lightmap Vertex Buffer( %s )",
			Common::FormatByteCount(BSPLightmapVertexBufferInfo.size).c_str());

		//// Create Index buffer heap
		vk::BufferCreateInfo BSPIndexBufferInfo = {};
		BSPIndexBufferInfo.size  = IndexHeapIndexOffset * sizeof(std::uint16_t);
		BSPIndexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer
								 | vk::BufferUsageFlagBits::eTransferDst;

		if( auto CreateResult = VulkanContext.LogicalDevice.createBufferUnique(
				BSPIndexBufferInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewScene.BSPIndexBuffer = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating Index buffer: %s\n",
				vk::to_string(CreateResult.result).c_str());
			return {};
		}
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewScene.BSPIndexBuffer.get(),
			"VkBlam::Scene: BSP Index Buffer( %s )",
			Common::FormatByteCount(BSPIndexBufferInfo.size).c_str());

		// Create singular allocation of device memory for all vertex and index
		// data
		if( auto [Result, Value] = Vulkan::CommitBufferHeap(
				VulkanContext.LogicalDevice, VulkanContext.PhysicalDevice,
				std::array{
					NewScene.BSPVertexBuffer.get(),
					NewScene.BSPIndexBuffer.get(),
					NewScene.BSPLightmapVertexBuffer.get()});
			Result == vk::Result::eSuccess )
		{
			NewScene.BSPGeometryMemory = std::move(Value);
		}
		else
		{
			std::fprintf(
				stderr, "Error committing vertex/index memory: %s\n",
				vk::to_string(Result).c_str());
			return {};
		}
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewScene.BSPGeometryMemory.get(),
			"VkBlam::Scene: BSP Geometry Device Memory( %s )",
			Common::FormatByteCount(BSPIndexBufferInfo.size).c_str());

		// Buffers are all now binded to device memory, begin streaming
		for( const auto& CurLightmapMesh : NewScene.LightmapMeshs )
		{
			TargetRenderer.GetStreamBuffer().QueueBufferUpload(
				std::as_bytes(CurLightmapMesh.VertexData),
				NewScene.BSPVertexBuffer.get(),
				CurLightmapMesh.VertexIndexOffset * sizeof(Blam::Vertex));

			TargetRenderer.GetStreamBuffer().QueueBufferUpload(
				std::as_bytes(CurLightmapMesh.LightmapVertexData),
				NewScene.BSPLightmapVertexBuffer.get(),
				CurLightmapMesh.VertexIndexOffset
					* sizeof(Blam::LightmapVertex));

			TargetRenderer.GetStreamBuffer().QueueBufferUpload(
				std::as_bytes(CurLightmapMesh.IndexData),
				NewScene.BSPIndexBuffer.get(),
				CurLightmapMesh.IndexOffset * sizeof(std::uint16_t));
		}
	}

	// Load bitmaps
	{

		// Create image handles
		const auto CreateBitmapImage
			= [&](VkBlam::BitmapHeapT::Bitmap&                   TargetBitmap,
				  Blam::Tag<Blam::TagClass::Bitmap>::BitmapEntry BitmapEntry)
			-> bool {
			vk::ImageCreateInfo ImageInfo = {};
			ImageInfo.imageType           = VkBlam::BlamToVk(BitmapEntry.Type);
			ImageInfo.format = VkBlam::BlamToVk(BitmapEntry.Format);
			ImageInfo.extent = vk::Extent3D(
				BitmapEntry.Width, BitmapEntry.Height, BitmapEntry.Depth);
			ImageInfo.mipLevels
				= std::max<std::uint16_t>(BitmapEntry.MipmapCount, 1);
			ImageInfo.arrayLayers
				= BitmapEntry.Type == Blam::BitmapEntryType::CubeMap ? 6 : 1;
			ImageInfo.samples = vk::SampleCountFlagBits::e1;
			ImageInfo.tiling  = vk::ImageTiling::eOptimal;
			ImageInfo.usage   = vk::ImageUsageFlagBits::eSampled
							| vk::ImageUsageFlagBits::eTransferDst
							| vk::ImageUsageFlagBits::eTransferSrc;
			ImageInfo.sharingMode   = vk::SharingMode::eExclusive;
			ImageInfo.initialLayout = vk::ImageLayout::eUndefined;

			if( BitmapEntry.Type == Blam::BitmapEntryType::CubeMap )
			{
				ImageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
			}

			auto& ImageDest = TargetBitmap.Image;

			if( auto CreateResult
				= VulkanContext.LogicalDevice.createImageUnique(ImageInfo);
				CreateResult.result == vk::Result::eSuccess )
			{
				ImageDest = std::move(CreateResult.value);
			}
			else
			{
				std::fprintf(
					stderr, "Error creating image: %s\n",
					vk::to_string(CreateResult.result).c_str());
				return false;
			}
			return true;
		};

		const auto CreateBitmap
			= [&](const Blam::TagIndexEntry&               TagEntry,
				  const Blam::Tag<Blam::TagClass::Bitmap>& Bitmap) -> void {
			// std::printf("%s\n",
			// CurWorld.GetMapFile().GetTagName(TagEntry.TagID).data());
			for( std::size_t CurSubTextureIdx = 0;
				 CurSubTextureIdx < Bitmap.Bitmaps.Count; ++CurSubTextureIdx )
			{
				const auto& CurSubTexture = Bitmap.Bitmaps.GetSpan(
					TargetWorld.GetMapFile().GetMapData().data(),
					TargetWorld.GetMapFile()
						.TagHeapVirtualBase)[CurSubTextureIdx];

				auto& BitmapDest
					= NewScene.BitmapHeap
						  .Bitmaps[TagEntry.TagID][CurSubTextureIdx];

				CreateBitmapImage(BitmapDest, CurSubTexture);

				Vulkan::SetObjectName(
					VulkanContext.LogicalDevice, BitmapDest.Image.get(),
					"VkBlam::Scene: Bitmap %08X[%2zu] | %s", TagEntry.TagID,
					CurSubTextureIdx,
					TargetWorld.GetMapFile().GetTagName(TagEntry.TagID).data());
			}
		};

		TargetWorld.GetMapFile().VisitTagClass<Blam::TagClass::Bitmap>(
			CreateBitmap);

		TargetWorld.GetMapFile().VisitTagClass<Blam::TagClass::Globals>(
			[&](const Blam::TagIndexEntry&                TagEntry,
				const Blam::Tag<Blam::TagClass::Globals>& Globals) -> void {
				const auto& CurGlobal = Globals;
				for( const auto& RasterData : CurGlobal.RasterizerData.GetSpan(
						 TargetWorld.GetMapFile().GetMapData().data(),
						 TargetWorld.GetMapFile().TagHeapVirtualBase) )
				{
					NewScene.BitmapHeap.Default2D = RasterData.Default2D.TagID;
					NewScene.BitmapHeap.Default3D = RasterData.Default3D.TagID;
					NewScene.BitmapHeap.DefaultCube
						= RasterData.DefaultCube.TagID;
				}
			});

		// Allocate and bind memory for all bitmaps

		{
			std::vector<vk::Image> Bitmaps;
			for( const auto& CurBitmap : NewScene.BitmapHeap.Bitmaps )
			{
				for( const auto& CurSubBitmap : CurBitmap.second )
				{
					Bitmaps.emplace_back(CurSubBitmap.second.Image.get());
				}
			}

			if( auto [Result, Value] = Vulkan::CommitImageHeap(
					VulkanContext.LogicalDevice, VulkanContext.PhysicalDevice,
					Bitmaps);
				Result == vk::Result::eSuccess )
			{
				NewScene.BitmapHeapMemory = std::move(Value);
			}
			else
			{
				std::fprintf(
					stderr, "Error committing bitmap memory: %s\n",
					vk::to_string(Result).c_str());
				return {};
			}
		}

		// All images are now created and binded to memory
		{
			// Todo: This would be the draft of a bitmap manager's stream
			// function
			const auto StreamBitmapImage =
				[&](VkBlam::BitmapHeapT::Bitmap&                   TargetBitmap,
					Blam::Tag<Blam::TagClass::Bitmap>::BitmapEntry BitmapEntry,
					std::span<const std::byte> PixelData) -> bool {
				// Upload image data
				const std::size_t MipCount
					= std::max<std::uint16_t>(BitmapEntry.MipmapCount, 1);
				const std::size_t LayerCount
					= BitmapEntry.Type == Blam::BitmapEntryType::CubeMap ? 6
																		 : 1;

				const std::size_t BlockSize
					= vk::blockSize(VkBlam::BlamToVk(BitmapEntry.Format));
				const std::array<std::uint8_t, 3> BlockExtent
					= vk::blockExtent(VkBlam::BlamToVk(BitmapEntry.Format));

				std::size_t PixelDataOff = 0;

				auto CurExtent = vk::Extent3D(
					BitmapEntry.Width, BitmapEntry.Height, BitmapEntry.Depth);
				for( std::size_t CurMip = 0; CurMip < MipCount; ++CurMip )
				{
					for( std::size_t CurLayer = 0; CurLayer < LayerCount;
						 ++CurLayer )
					{
						const std::array<std::uint32_t, 3> CurBlockCount
							= {std::max(1u, CurExtent.width / BlockExtent[0]),
							   std::max(1u, CurExtent.height / BlockExtent[1]),
							   std::max(1u, CurExtent.depth / BlockExtent[2])};

						const std::size_t CurPixelDataSize
							= CurBlockCount[0] * CurBlockCount[1]
							* CurBlockCount[2] * BlockSize;

						TargetRenderer.GetStreamBuffer().QueueImageUpload(
							PixelData.subspan(PixelDataOff, CurPixelDataSize),
							TargetBitmap.Image.get(), vk::Offset3D(0, 0, 0),
							CurExtent,
							vk::ImageSubresourceLayers(
								vk::ImageAspectFlagBits::eColor, CurMip,
								CurLayer, 1));

						PixelDataOff += CurPixelDataSize;
					}

					CurExtent.width  = std::max(1u, CurExtent.width / 2);
					CurExtent.height = std::max(1u, CurExtent.height / 2);
					CurExtent.depth  = std::max(1u, CurExtent.depth / 2);
				}

				// Create image view
				vk::ImageViewCreateInfo BitmapImageViewInfo = {};
				BitmapImageViewInfo.image = TargetBitmap.Image.get();
				switch( BitmapEntry.Type )
				{
				default:
				case Blam::BitmapEntryType::Texture2D:
				{
					BitmapImageViewInfo.viewType = vk::ImageViewType::e2D;
					break;
				}
				case Blam::BitmapEntryType::Texture3D:
				{
					BitmapImageViewInfo.viewType = vk::ImageViewType::e3D;
					break;
				}
				case Blam::BitmapEntryType::CubeMap:
				{
					BitmapImageViewInfo.viewType = vk::ImageViewType::eCube;
					break;
				}
				}
				BitmapImageViewInfo.format
					= VkBlam::BlamToVk(BitmapEntry.Format);
				;
				BitmapImageViewInfo.components.r = vk::ComponentSwizzle::eR;
				BitmapImageViewInfo.components.g = vk::ComponentSwizzle::eG;
				BitmapImageViewInfo.components.b = vk::ComponentSwizzle::eB;
				BitmapImageViewInfo.components.a = vk::ComponentSwizzle::eA;
				BitmapImageViewInfo.subresourceRange.aspectMask
					= vk::ImageAspectFlagBits::eColor;
				BitmapImageViewInfo.subresourceRange.baseMipLevel   = 0;
				BitmapImageViewInfo.subresourceRange.levelCount     = MipCount;
				BitmapImageViewInfo.subresourceRange.baseArrayLayer = 0;
				BitmapImageViewInfo.subresourceRange.layerCount = LayerCount;

				if( auto CreateResult
					= VulkanContext.LogicalDevice.createImageViewUnique(
						BitmapImageViewInfo);
					CreateResult.result == vk::Result::eSuccess )
				{
					TargetBitmap.View = std::move(CreateResult.value);
				}
				else
				{
					std::fprintf(
						stderr, "Error bitmap view: %s\n",
						vk::to_string(CreateResult.result).c_str());
					return false;
				}
				return true;
			};

			const auto StreamBitmap
				= [&](const Blam::TagIndexEntry&               TagEntry,
					  const Blam::Tag<Blam::TagClass::Bitmap>& Bitmap) -> void {
				for( std::size_t CurSubTextureIdx = 0;
					 CurSubTextureIdx < Bitmap.Bitmaps.Count;
					 ++CurSubTextureIdx )
				{
					const auto& CurSubTexture = Bitmap.Bitmaps.GetSpan(
						TargetWorld.GetMapFile().GetMapData().data(),
						TargetWorld.GetMapFile()
							.TagHeapVirtualBase)[CurSubTextureIdx];
					const auto PixelData = std::span<const std::byte>(
						reinterpret_cast<const std::byte*>(
							TargetWorld.GetMapFile().GetBitmapData().data())
							+ CurSubTexture.PixelDataOffset,
						CurSubTexture.PixelDataSize);

					auto& BitmapDest
						= NewScene.BitmapHeap
							  .Bitmaps[TagEntry.TagID][CurSubTextureIdx];

					StreamBitmapImage(BitmapDest, CurSubTexture, PixelData);

					Vulkan::SetObjectName(
						VulkanContext.LogicalDevice, BitmapDest.View.get(),
						"VkBlam::Scene: Bitmap View %08X[%2zu] | %s",
						TagEntry.TagID, CurSubTextureIdx,
						TargetWorld.GetMapFile()
							.GetTagName(TagEntry.TagID)
							.data());

					// Create descriptor set
					vk::DescriptorSet& TargetSet
						= NewScene.BitmapHeap
							  .Sets[TagEntry.TagID][CurSubTextureIdx];

					if( auto NewSet = NewScene.DebugDrawDescriptorPool
										  ->AllocateDescriptorSet();
						NewSet )
					{
						TargetSet = NewSet.value();
					}
					else
					{
						std::fprintf(
							stderr, "Error allocating bitmap descriptor set\n");
						return;
					}

					Vulkan::SetObjectName(
						VulkanContext.LogicalDevice, TargetSet,
						"VkBlam::Scene: Bitmap Descriptor Set %08X[%2zu] | %s",
						TagEntry.TagID, CurSubTextureIdx,
						TargetWorld.GetMapFile()
							.GetTagName(TagEntry.TagID)
							.data());

					TargetRenderer.GetDescriptorUpdateBatch().AddImage(
						TargetSet, 0, BitmapDest.View.get(),
						vk::ImageLayout::eShaderReadOnlyOptimal);
				}
			};

			TargetWorld.GetMapFile().VisitTagClass<Blam::TagClass::Bitmap>(
				StreamBitmap);
		}
	}

	return {std::move(NewScene)};
}

} // namespace VkBlam