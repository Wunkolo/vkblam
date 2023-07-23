#include <VkBlam/Format.hpp>
#include <VkBlam/Scene.hpp>

#include <Blam/TagVisitor.hpp>

#include <Vulkan/Memory.hpp>
#include <Vulkan/Pipeline.hpp>

#include <Common/Format.hpp>

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
			vk::to_string(CreateResult.result).c_str()
		);
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

	VertexInputState.vertexBindingDescriptionCount
		= VertexBindingDescriptions.size();
	VertexInputState.pVertexBindingDescriptions
		= VertexBindingDescriptions.data();

	VertexInputState.vertexAttributeDescriptionCount
		= VertexAttributeDescriptions.size();
	VertexInputState.pVertexAttributeDescriptions
		= VertexAttributeDescriptions.data();

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
	MultisampleState.alphaToCoverageEnable = true;
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
		std::move(Pipeline), std::move(GraphicsPipelineLayout)
	);
}

static vk::DescriptorSetLayoutBinding SceneBindings[] = {
	{// Default2DSamplerFiltered
	 0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment},
	{// Default2DSamplerUnfiltered
	 1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment},
	{// DefaultCubeSampler
	 2, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment},
};

static vk::DescriptorSetLayoutBinding ShaderEnvironmentBindings[] = {
	{// Basemap
	 0, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// PrimaryDetailMap
	 1, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// SecondaryDetailMap
	 2, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// MicroDetailMap
	 3, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// BumpMap
	 4, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// GlowMap
	 5, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// ReflectionCubeMap
	 6, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
};

namespace VkBlam
{
Scene::Scene(Renderer& TargetRenderer, const World& TargetWorld)
	: TargetWorld(TargetWorld), TargetRenderer(TargetRenderer)
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
		vk::PipelineBindPoint::eGraphics, DebugDrawPipeline.get()
	);

	// Bing Scene globals
	CommandBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(), 0,
		{CurSceneDescriptor}, {}
	);

	CommandBuffer.pushConstants<VkBlam::CameraGlobals>(
		DebugDrawPipelineLayout.get(), vk::ShaderStageFlagBits::eAllGraphics, 0,
		{View.CameraGlobalsData}
	);

	CommandBuffer.bindVertexBuffers(
		0, {BSPVertexBuffer.get(), BSPLightmapVertexBuffer.get()}, {0, 0}
	);

	CommandBuffer.bindIndexBuffer(
		BSPIndexBuffer.get(), 0, vk::IndexType::eUint16
	);

	for( std::size_t i = 0; i < LightmapMeshs.size(); ++i )
	{
		const auto& CurLightmapMesh = LightmapMeshs[i];
		Vulkan::InsertDebugLabel(
			CommandBuffer, {0.5, 0.5, 0.5, 1.0}, "BSP Draw: %zu", i
		);

		// Bind Shader descriptors
		if( ShaderEnvironmentDescriptors.contains(CurLightmapMesh.ShaderTag) )
		{
			CommandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
				1, {ShaderEnvironmentDescriptors.at(CurLightmapMesh.ShaderTag)},
				{}
			);
		}

		// Bind Mesh descriptors
		if( CurLightmapMesh.LightmapTag.has_value()
			&& CurLightmapMesh.LightmapIndex.has_value() )
		{
			CommandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
				2,
				{BitmapHeap.Sets.at(CurLightmapMesh.LightmapTag.value())
					 .at(CurLightmapMesh.LightmapIndex.value())},
				{}
			);
		}
		else
		{
			CommandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
				2,
				{BitmapHeap.Sets.at(BitmapHeap.Default2D)
					 .at(std::uint32_t(Blam::DefaultTextureIndex::Multiplicative
					 ))},
				{}
			);
		}

		CommandBuffer.drawIndexed(
			CurLightmapMesh.IndexCount, 1, CurLightmapMesh.IndexOffset,
			CurLightmapMesh.VertexIndexOffset, 0
		);
	}
}

std::optional<Scene>
	Scene::Create(Renderer& TargetRenderer, const World& TargetWorld)
{
	Scene NewScene(TargetRenderer, TargetWorld);

	const Vulkan::Context& VulkanContext = TargetRenderer.GetVulkanContext();

	// Descriptor pools
	{
		NewScene.ShaderEnvironmentDescriptorPool
			= std::make_unique<Vulkan::DescriptorHeap>(
				Vulkan::DescriptorHeap::Create(
					VulkanContext, ShaderEnvironmentBindings
				)
					.value()
			);
		NewScene.DebugDrawDescriptorPool
			= std::make_unique<Vulkan::DescriptorHeap>(
				Vulkan::DescriptorHeap::Create(
					VulkanContext, {{vk::DescriptorSetLayoutBinding(
									   0, vk::DescriptorType::eSampledImage, 1,
									   vk::ShaderStageFlagBits::eFragment
								   )}}
				).value()
			);

		NewScene.UnlitDescriptorPool = std::make_unique<Vulkan::DescriptorHeap>(
			Vulkan::DescriptorHeap::Create(
				VulkanContext, {{vk::DescriptorSetLayoutBinding(
								   0, vk::DescriptorType::eSampledImage, 1,
								   vk::ShaderStageFlagBits::eFragment
							   )}}
			).value()
		);
		NewScene.SceneDescriptorPool = std::make_unique<Vulkan::DescriptorHeap>(
			Vulkan::DescriptorHeap::Create(VulkanContext, SceneBindings).value()
		);
	}

	// Scene Descriptor
	{
		NewScene.CurSceneDescriptor
			= NewScene.SceneDescriptorPool->AllocateDescriptorSet().value();
		// Default2DSamplerFiltered
		TargetRenderer.GetDescriptorUpdateBatch().AddSampler(
			NewScene.CurSceneDescriptor, 0,
			TargetRenderer.GetSamplerCache().GetSampler(Sampler2D())
		);

		// Default2DSamplerUnfiltered
		TargetRenderer.GetDescriptorUpdateBatch().AddSampler(
			NewScene.CurSceneDescriptor, 1,
			TargetRenderer.GetSamplerCache().GetSampler(Sampler2D(false))
		);

		// Default2DSamplerUnfiltered
		TargetRenderer.GetDescriptorUpdateBatch().AddSampler(
			NewScene.CurSceneDescriptor, 2,
			TargetRenderer.GetSamplerCache().GetSampler(SamplerCube())
		);
	}

	{
		// Main Shader modules
		const auto DefaultVertShaderData
			= VkBlam::OpenResource("shaders/Default.vert.spv").value();
		const auto DefaultFragShaderData
			= VkBlam::OpenResource("shaders/Default.frag.spv").value();
		const auto UnlitFragShaderData
			= VkBlam::OpenResource("shaders/Unlit.frag.spv").value();

		std::hash<std::string> StringHasher = {};

		NewScene.DefaultVertexShaderModule
			= TargetRenderer.GetShaderModuleCache()
				  .GetShaderModule(
					  StringHasher("shaders/Default.vert.spv"),
					  DefaultVertShaderData
				  )
				  .value();
		NewScene.DefaultFragmentShaderModule
			= TargetRenderer.GetShaderModuleCache()
				  .GetShaderModule(
					  StringHasher("shaders/Default.frag.spv"),
					  DefaultFragShaderData
				  )
				  .value();
		NewScene.UnlitFragmentShaderModule
			= TargetRenderer.GetShaderModuleCache()
				  .GetShaderModule(
					  StringHasher("shaders/Unlit.frag.spv"),
					  UnlitFragShaderData
				  )
				  .value();

		const vk::RenderPass RenderPass
			= TargetRenderer.GetDefaultRenderPass(RenderSamples);

		const auto [VertexBindingDescriptions, VertexAttributeDescriptions]
			= VkBlam::GetVertexInputDescriptions({{
				Blam::VertexFormat::SBSPVertexUncompressed,
				Blam::VertexFormat::SBSPLightmapVertexUncompressed,
			}});

		std::tie(NewScene.DebugDrawPipeline, NewScene.DebugDrawPipelineLayout)
			= CreateGraphicsPipeline(
				VulkanContext.LogicalDevice,
				{{vk::PushConstantRange(
					vk::ShaderStageFlagBits::eAllGraphics, 0,
					sizeof(VkBlam::CameraGlobals)
				)}},
				{{NewScene.SceneDescriptorPool->GetDescriptorSetLayout(),
				  NewScene.ShaderEnvironmentDescriptorPool
					  ->GetDescriptorSetLayout(),
				  NewScene.DebugDrawDescriptorPool->GetDescriptorSetLayout()}},
				NewScene.DefaultVertexShaderModule,
				NewScene.DefaultFragmentShaderModule, VertexBindingDescriptions,
				VertexAttributeDescriptions, RenderPass, RenderSamples,
				vk::PolygonMode::eFill
			);

		std::tie(NewScene.UnlitDrawPipeline, NewScene.UnlitDrawPipelineLayout)
			= CreateGraphicsPipeline(
				VulkanContext.LogicalDevice,
				{{vk::PushConstantRange(
					vk::ShaderStageFlagBits::eAllGraphics, 0,
					sizeof(VkBlam::CameraGlobals)
				)}},
				{{NewScene.UnlitDescriptorPool->GetDescriptorSetLayout(),
				  NewScene.UnlitDescriptorPool->GetDescriptorSetLayout()}},
				NewScene.DefaultVertexShaderModule,
				NewScene.UnlitFragmentShaderModule, VertexBindingDescriptions,
				VertexAttributeDescriptions, RenderPass, RenderSamples,
				vk::PolygonMode::eLine
			);
	}

	std::vector<Blam::TagVisitorProc> TagVisitors = {};

	// Load BSP
	{
		// Index in elements, not bytes
		std::uint32_t VertexHeapIndexEnd = 0;
		std::uint32_t IndexHeapIndexEnd  = 0;

		for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP& CurSBSP :
			 TargetWorld.GetMapFile().GetScenarioBSPs() )
		{
			const Blam::VirtualHeap SBSPHeap
				= CurSBSP.GetSBSPHeap(TargetWorld.GetMapFile().GetMapData());

			const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& ScenarioBSP
				= CurSBSP.GetSBSP(SBSPHeap);

			const auto Surfaces = SBSPHeap.GetBlock(ScenarioBSP.Surfaces);

			std::uint32_t SBSPIndexHeapEnd = IndexHeapIndexEnd;

			// Lightmap
			for( const auto& CurLightmap :
				 SBSPHeap.GetBlock(ScenarioBSP.Lightmaps) )
			{
				const auto& LightmapTextureTag
					= TargetWorld.GetMapFile().GetTag<Blam::TagClass::Bitmap>(
						ScenarioBSP.LightmapTexture.TagID
					);
				const std::int16_t LightmapTextureIndex
					= CurLightmap.LightmapIndex;

				for( const auto& CurMaterial :
					 SBSPHeap.GetBlock(CurLightmap.Materials) )
				{

					std::printf(
						"Shader(%s): %s | Permutation: %04X | Surfaces: "
						"[%04d,%04d)\n",
						Blam::FormatTagClass(CurMaterial.Shader.Class).c_str(),
						TargetWorld.GetMapFile()
							.GetTagName(CurMaterial.Shader.TagID)
							.data(),
						CurMaterial.ShaderPermutation,
						CurMaterial.SurfacesIndexStart,
						CurMaterial.SurfacesIndexStart
							+ CurMaterial.SurfacesCount
					);

					auto& CurLightmapMesh
						= NewScene.LightmapMeshs.emplace_back();
					//// Vertex Buffer data
					{
						// Copy vertex data into the staging buffer
						const std::span<const Blam::Vertex> CurVertexData
							= CurMaterial.GetVertices(SBSPHeap);

						CurLightmapMesh.VertexData = CurVertexData;

						// Add the offset needed to begin indexing into
						// this particular part of the vertex buffer,
						// used when drawing
						CurLightmapMesh.VertexIndexOffset = VertexHeapIndexEnd;

						CurLightmapMesh.ShaderTag = CurMaterial.Shader.TagID;

						if( ScenarioBSP.LightmapTexture.Valid()
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
								= CurMaterial.GetLightmapVertices(SBSPHeap);
							CurLightmapMesh.LightmapVertexData
								= CurLightmapVertexData;
						}

						VertexHeapIndexEnd += CurVertexData.size();
					}

					//// Index Buffer data
					CurLightmapMesh.IndexOffset = SBSPIndexHeapEnd;
					CurLightmapMesh.IndexCount  = CurMaterial.SurfacesCount * 3;
					SBSPIndexHeapEnd += CurMaterial.SurfacesCount * 3;
				}
			}

			IndexHeapIndexEnd += ScenarioBSP.Surfaces.Count * 3;
		}

		//// Create Vertex buffer heap
		vk::BufferCreateInfo BSPVertexBufferInfo = {};
		BSPVertexBufferInfo.size  = VertexHeapIndexEnd * sizeof(Blam::Vertex);
		BSPVertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer
								  | vk::BufferUsageFlagBits::eTransferDst;

		if( auto CreateResult
			= VulkanContext.LogicalDevice.createBufferUnique(BSPVertexBufferInfo
			);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewScene.BSPVertexBuffer = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating vertex buffer: %s\n",
				vk::to_string(CreateResult.result).c_str()
			);
			return {};
		}

		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewScene.BSPVertexBuffer.get(),
			"VkBlam::Scene: BSP Vertex Buffer( %s )",
			Common::FormatByteCount(BSPVertexBufferInfo.size).c_str()
		);

		//// Create Vertex buffer heap
		vk::BufferCreateInfo BSPLightmapVertexBufferInfo = {};

		BSPLightmapVertexBufferInfo.size
			= VertexHeapIndexEnd * sizeof(Blam::LightmapVertex);
		BSPLightmapVertexBufferInfo.usage
			= vk::BufferUsageFlagBits::eVertexBuffer
			| vk::BufferUsageFlagBits::eTransferDst;

		if( auto CreateResult = VulkanContext.LogicalDevice.createBufferUnique(
				BSPLightmapVertexBufferInfo
			);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewScene.BSPLightmapVertexBuffer = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating lightmap vertex buffer: %s\n",
				vk::to_string(CreateResult.result).c_str()
			);
			return {};
		}

		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewScene.BSPLightmapVertexBuffer.get(),
			"VkBlam::Scene: BSP Lightmap Vertex Buffer( %s )",
			Common::FormatByteCount(BSPLightmapVertexBufferInfo.size).c_str()
		);

		//// Create Index buffer heap
		vk::BufferCreateInfo BSPIndexBufferInfo = {};
		BSPIndexBufferInfo.size  = IndexHeapIndexEnd * sizeof(std::uint16_t);
		BSPIndexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer
								 | vk::BufferUsageFlagBits::eTransferDst;

		if( auto CreateResult
			= VulkanContext.LogicalDevice.createBufferUnique(BSPIndexBufferInfo
			);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewScene.BSPIndexBuffer = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating Index buffer: %s\n",
				vk::to_string(CreateResult.result).c_str()
			);
			return {};
		}
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewScene.BSPIndexBuffer.get(),
			"VkBlam::Scene: BSP Index Buffer( %s )",
			Common::FormatByteCount(BSPIndexBufferInfo.size).c_str()
		);

		// Create singular allocation of device memory for all vertex and index
		// data
		if( auto [Result, Value] = Vulkan::CommitBufferHeap(
				VulkanContext.LogicalDevice, VulkanContext.PhysicalDevice,
				std::array{
					NewScene.BSPVertexBuffer.get(),
					NewScene.BSPIndexBuffer.get(),
					NewScene.BSPLightmapVertexBuffer.get()}
			);
			Result == vk::Result::eSuccess )
		{
			NewScene.BSPGeometryMemory = std::move(Value);
		}
		else
		{
			std::fprintf(
				stderr, "Error committing vertex/index memory: %s\n",
				vk::to_string(Result).c_str()
			);
			return {};
		}
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewScene.BSPGeometryMemory.get(),
			"VkBlam::Scene: BSP Geometry Device Memory( %s )",
			Common::FormatByteCount(BSPIndexBufferInfo.size).c_str()
		);

		// Buffers are all now binded to device memory, begin streaming
		for( const auto& CurLightmapMesh : NewScene.LightmapMeshs )
		{
			TargetRenderer.GetStreamBuffer().QueueBufferUpload(
				std::as_bytes(CurLightmapMesh.VertexData),
				NewScene.BSPVertexBuffer.get(),
				CurLightmapMesh.VertexIndexOffset * sizeof(Blam::Vertex)
			);

			TargetRenderer.GetStreamBuffer().QueueBufferUpload(
				std::as_bytes(CurLightmapMesh.LightmapVertexData),
				NewScene.BSPLightmapVertexBuffer.get(),
				CurLightmapMesh.VertexIndexOffset * sizeof(Blam::LightmapVertex)
			);
		}

		// Index Buffer
		{
			std::uint32_t IndexOffset = 0;
			for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP&
					 CurSBSP : TargetWorld.GetMapFile().GetScenarioBSPs() )
			{
				const Blam::VirtualHeap SBSPHeap
					= CurSBSP.GetSBSPHeap(TargetWorld.GetMapFile().GetMapData()
					);

				const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>&
					ScenarioBSP
					= CurSBSP.GetSBSP(SBSPHeap);

				const auto Surfaces = SBSPHeap.GetBlock(ScenarioBSP.Surfaces);

				TargetRenderer.GetStreamBuffer().QueueBufferUpload(
					std::as_bytes(Surfaces), NewScene.BSPIndexBuffer.get(),
					IndexOffset
				);
				IndexOffset += Surfaces.size_bytes();
			}
		}
	}

	// Load bitmaps
	{

		// Create image handles
		const auto CreateBitmapImage
			= [&](VkBlam::BitmapHeapT::Bitmap&                   TargetBitmap,
				  Blam::Tag<Blam::TagClass::Bitmap>::BitmapEntry BitmapEntry
			  ) -> bool {
			vk::ImageCreateInfo ImageInfo = {};
			ImageInfo.imageType           = VkBlam::BlamToVk(BitmapEntry.Type);
			ImageInfo.format = VkBlam::BlamToVk(BitmapEntry.Format);
			ImageInfo.extent = vk::Extent3D(
				BitmapEntry.Width, BitmapEntry.Height, BitmapEntry.Depth
			);
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
					vk::to_string(CreateResult.result).c_str()
				);
				return false;
			}
			return true;
		};

		const auto CreateBitmap
			= [&, CreateBitmapImage](
				  const Blam::TagIndexEntry&               TagEntry,
				  const Blam::Tag<Blam::TagClass::Bitmap>& Bitmap,
				  const Blam::MapFile&                     Map
			  ) -> void {
			// std::printf("%s\n",
			// CurWorld.GetMapFile().GetTagName(TagEntry.TagID).data());
			for( std::size_t CurSubTextureIdx = 0;
				 CurSubTextureIdx < Bitmap.Bitmaps.Count; ++CurSubTextureIdx )
			{
				const auto& CurSubTexture
					= Map.TagHeap.GetBlock(Bitmap.Bitmaps)[CurSubTextureIdx];

				// Create bitmap and descriptor set
				auto& BitmapDest
					= NewScene.BitmapHeap
						  .Bitmaps[TagEntry.TagID][CurSubTextureIdx];

				auto& BitmapDescriptorDest
					= NewScene.BitmapHeap
						  .Sets[TagEntry.TagID][CurSubTextureIdx];

				CreateBitmapImage(BitmapDest, CurSubTexture);

				Vulkan::SetObjectName(
					VulkanContext.LogicalDevice, BitmapDest.Image.get(),
					"VkBlam::Scene: Bitmap %08X[%2zu] | %s", TagEntry.TagID,
					CurSubTextureIdx, Map.GetTagName(TagEntry.TagID).data()
				);
			}
		};

		Blam::TagVisitorProc& BitmapLoader = TagVisitors.emplace_back();

		BitmapLoader.VisitClass = Blam::TagClass::Bitmap;

		BitmapLoader.VisitTags
			= [CreateBitmap](
				  std::span<const Blam::TagIndexEntry> TagIndexEntries,
				  const Blam::MapFile&                 Map
			  ) -> void {
			for( const auto& TagIndexEntry : TagIndexEntries )
			{
				const auto& CurBitmap
					= Map.GetTag<Blam::TagClass::Bitmap>(TagIndexEntry.TagID);
				CreateBitmap(TagIndexEntry, *CurBitmap, Map);
			}
		};

		BitmapLoader.EndVisits = [&](const Blam::MapFile& Map) -> void {
			Map.VisitTagClass<Blam::TagClass::Globals>(
				[&](const Blam::TagIndexEntry&                TagEntry,
					const Blam::Tag<Blam::TagClass::Globals>& Globals) -> void {
					const auto& CurGlobal = Globals;
					for( const auto& RasterData :
						 TargetWorld.GetMapFile().TagHeap.GetBlock(
							 CurGlobal.RasterizerData
						 ) )
					{
						NewScene.BitmapHeap.Default2D
							= RasterData.Default2D.TagID;
						NewScene.BitmapHeap.Default3D
							= RasterData.Default3D.TagID;
						NewScene.BitmapHeap.DefaultCube
							= RasterData.DefaultCube.TagID;
					}
				}
			);
		};

		Blam::TagVisitorProc& BitmapCommitter = TagVisitors.emplace_back();

		BitmapCommitter.Parallel   = true;
		BitmapCommitter.VisitClass = Blam::TagClass::Bitmap;

		// Allocate and bind memory for all bitmaps
		BitmapCommitter.BeginVisits = [&](const Blam::MapFile& Map) -> void {
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
					Bitmaps
				);
				Result == vk::Result::eSuccess )
			{
				NewScene.BitmapHeapMemory = std::move(Value);
			}
			else
			{
				std::fprintf(
					stderr, "Error committing bitmap memory: %s\n",
					vk::to_string(Result).c_str()
				);
				return;
			}
		};

		// All images are now created and binded to memory
		{
			// Todo: This would be the draft of a bitmap manager's stream
			// function
			const auto StreamBitmapImage =
				[&TargetRenderer](
					VkBlam::BitmapHeapT::Bitmap&                   TargetBitmap,
					Blam::Tag<Blam::TagClass::Bitmap>::BitmapEntry BitmapEntry,
					std::span<const std::byte>                     PixelData
				) -> bool {
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
					BitmapEntry.Width, BitmapEntry.Height, BitmapEntry.Depth
				);
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
								CurLayer, 1
							)
						);

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
					= TargetRenderer.GetVulkanContext()
						  .LogicalDevice.createImageViewUnique(
							  BitmapImageViewInfo
						  );
					CreateResult.result == vk::Result::eSuccess )
				{
					TargetBitmap.View = std::move(CreateResult.value);
				}
				else
				{
					std::fprintf(
						stderr, "Error creating bitmap view: %s\n",
						vk::to_string(CreateResult.result).c_str()
					);
					return false;
				}
				return true;
			};

			const auto StreamBitmap
				= [&NewScene, &TargetWorld, &TargetRenderer, StreamBitmapImage](
					  const Blam::TagIndexEntry&               TagEntry,
					  const Blam::Tag<Blam::TagClass::Bitmap>& Bitmap
				  ) -> void {
				for( std::size_t CurSubTextureIdx = 0;
					 CurSubTextureIdx < Bitmap.Bitmaps.Count;
					 ++CurSubTextureIdx )
				{
					const auto& CurSubTexture
						= TargetWorld.GetMapFile().TagHeap.GetBlock(
							Bitmap.Bitmaps
						)[CurSubTextureIdx];
					const auto PixelData = std::span<const std::byte>(
						reinterpret_cast<const std::byte*>(
							TargetWorld.GetMapFile().GetBitmapData().data()
						) + CurSubTexture.PixelDataOffset,
						CurSubTexture.PixelDataSize
					);

					auto& BitmapDest
						= NewScene.BitmapHeap.Bitmaps.at(TagEntry.TagID)
							  .at(CurSubTextureIdx);

					StreamBitmapImage(BitmapDest, CurSubTexture, PixelData);

					Vulkan::SetObjectName(
						TargetRenderer.GetVulkanContext().LogicalDevice,
						BitmapDest.View.get(),
						"VkBlam::Scene: Bitmap View %08X[%2zu] | %s",
						TagEntry.TagID, CurSubTextureIdx,
						TargetWorld.GetMapFile()
							.GetTagName(TagEntry.TagID)
							.data()
					);

					// Create descriptor set
					vk::DescriptorSet& TargetSet
						= NewScene.BitmapHeap.Sets.at(TagEntry.TagID)
							  .at(CurSubTextureIdx);

					if( auto NewSet = NewScene.DebugDrawDescriptorPool
										  ->AllocateDescriptorSet();
						NewSet )
					{
						TargetSet = NewSet.value();
					}
					else
					{
						std::fprintf(
							stderr, "Error allocating bitmap descriptor set\n"
						);
						return;
					}

					Vulkan::SetObjectName(
						TargetRenderer.GetVulkanContext().LogicalDevice,
						TargetSet,
						"VkBlam::Scene: Bitmap Descriptor Set %08X[%2zu] | %s",
						TagEntry.TagID, CurSubTextureIdx,
						TargetWorld.GetMapFile()
							.GetTagName(TagEntry.TagID)
							.data()
					);

					TargetRenderer.GetDescriptorUpdateBatch().AddImage(
						TargetSet, 0, BitmapDest.View.get(),
						vk::ImageLayout::eShaderReadOnlyOptimal
					);
				}
			};

			BitmapCommitter.VisitTags
				= [StreamBitmap](
					  std::span<const Blam::TagIndexEntry> TagIndexEntries,
					  const Blam::MapFile&                 Map
				  ) -> void {
				for( const auto& TagIndexEntry : TagIndexEntries )
				{
					const auto CurBitmap
						= Map.GetTag<Blam::TagClass::Bitmap>(TagIndexEntry.TagID
						);
					StreamBitmap(TagIndexEntry, *CurBitmap);
				}
			};

			BitmapCommitter.EndVisits = [&](const Blam::MapFile& Map) -> void {
				TargetRenderer.GetDescriptorUpdateBatch().Flush();
			};
		}
	}

	// Create Shader-Environment descriptor sets
	{
		const auto CreateShaderEnvironmentDescriptor
			= [&](const Blam::TagIndexEntry& TagEntry,
				  const Blam::Tag<Blam::TagClass::ShaderEnvironment>&
					  ShaderEnvironment) -> void {
			const vk::DescriptorSet NewSet
				= NewScene.ShaderEnvironmentDescriptorPool
					  ->AllocateDescriptorSet()
					  .value();
			NewScene.ShaderEnvironmentDescriptors[TagEntry.TagID] = NewSet;

			Vulkan::SetObjectName(
				VulkanContext.LogicalDevice, NewSet,
				"senv: %08X \'%s\' Descriptor Set", TagEntry.TagID,
				TargetWorld.GetMapFile().GetTagName(TagEntry.TagID).data()
			);

			const vk::ImageView BaseMapView
				= (ShaderEnvironment.BaseMap.Valid()
					   ? NewScene.BitmapHeap.Bitmaps
							 .at(ShaderEnvironment.BaseMap.TagID)
							 .at(0)
					   : NewScene.BitmapHeap.Bitmaps
							 .at(NewScene.BitmapHeap.Default2D)
							 .at(std::uint32_t(
								 Blam::DefaultTextureIndex::Multiplicative
							 )))
					  .View.get();
			const vk::ImageView PrimaryDetailMapView
				= (ShaderEnvironment.PrimaryDetailMap.Valid()
					   ? NewScene.BitmapHeap.Bitmaps
							 .at(ShaderEnvironment.PrimaryDetailMap.TagID)
							 .at(0)
					   : NewScene.BitmapHeap.Bitmaps
							 .at(NewScene.BitmapHeap.Default2D)
							 .at(0))
					  .View.get();
			const vk::ImageView SecondaryDetailMapView
				= (ShaderEnvironment.SecondaryDetailMap.Valid()
					   ? NewScene.BitmapHeap.Bitmaps
							 .at(ShaderEnvironment.SecondaryDetailMap.TagID)
							 .at(0)
					   : NewScene.BitmapHeap.Bitmaps
							 .at(NewScene.BitmapHeap.Default2D)
							 .at(0))
					  .View.get();
			const vk::ImageView MicroDetailMapView
				= (ShaderEnvironment.MicroDetailMap.Valid()
					   ? NewScene.BitmapHeap.Bitmaps
							 .at(ShaderEnvironment.MicroDetailMap.TagID)
							 .at(0)
					   : NewScene.BitmapHeap.Bitmaps
							 .at(NewScene.BitmapHeap.Default2D)
							 .at(0))
					  .View.get();
			const vk::ImageView BumpMapView
				= (ShaderEnvironment.BumpMap.Valid()
					   ? NewScene.BitmapHeap.Bitmaps
							 .at(ShaderEnvironment.BumpMap.TagID)
							 .at(0)
					   : NewScene.BitmapHeap.Bitmaps
							 .at(NewScene.BitmapHeap.Default2D)
							 .at(std::uint32_t(Blam::DefaultTextureIndex::Vector
							 )))
					  .View.get();
			const vk::ImageView GlowMapView
				= (ShaderEnvironment.GlowMap.Valid()
					   ? NewScene.BitmapHeap.Bitmaps
							 .at(ShaderEnvironment.GlowMap.TagID)
							 .at(0)
					   : NewScene.BitmapHeap.Bitmaps
							 .at(NewScene.BitmapHeap.Default2D)
							 .at(std::uint32_t(
								 Blam::DefaultTextureIndex::Additive
							 )))
					  .View.get();
			const vk::ImageView ReflectionCubeMapView
				= (ShaderEnvironment.ReflectionCubeMap.Valid()
					   ? NewScene.BitmapHeap.Bitmaps
							 .at(ShaderEnvironment.ReflectionCubeMap.TagID)
							 .at(0)
					   : NewScene.BitmapHeap.Bitmaps
							 .at(NewScene.BitmapHeap.DefaultCube)
							 .at(0))
					  .View.get();

			TargetRenderer.GetDescriptorUpdateBatch().AddImage(
				NewSet, 0, BaseMapView, vk::ImageLayout::eShaderReadOnlyOptimal
			);
			TargetRenderer.GetDescriptorUpdateBatch().AddImage(
				NewSet, 1, PrimaryDetailMapView,
				vk::ImageLayout::eShaderReadOnlyOptimal
			);
			TargetRenderer.GetDescriptorUpdateBatch().AddImage(
				NewSet, 2, SecondaryDetailMapView,
				vk::ImageLayout::eShaderReadOnlyOptimal
			);
			TargetRenderer.GetDescriptorUpdateBatch().AddImage(
				NewSet, 3, MicroDetailMapView,
				vk::ImageLayout::eShaderReadOnlyOptimal
			);
			TargetRenderer.GetDescriptorUpdateBatch().AddImage(
				NewSet, 4, BumpMapView, vk::ImageLayout::eShaderReadOnlyOptimal
			);
			TargetRenderer.GetDescriptorUpdateBatch().AddImage(
				NewSet, 5, GlowMapView, vk::ImageLayout::eShaderReadOnlyOptimal
			);
			TargetRenderer.GetDescriptorUpdateBatch().AddImage(
				NewSet, 6, ReflectionCubeMapView,
				vk::ImageLayout::eShaderReadOnlyOptimal
			);
		};

		Blam::TagVisitorProc& ShaderEnvironmentProc
			= TagVisitors.emplace_back();

		ShaderEnvironmentProc.VisitClass = Blam::TagClass::ShaderEnvironment;

		ShaderEnvironmentProc.DependClasses
			= {// Wait for bitmap loaders to finish
			   Blam::TagClass::Bitmap};

		ShaderEnvironmentProc.VisitTags
			= [CreateShaderEnvironmentDescriptor](
				  std::span<const Blam::TagIndexEntry> TagIndexEntries,
				  const Blam::MapFile&                 Map
			  ) -> void {
			for( const auto& TagIndexEntry : TagIndexEntries )
			{
				const auto CurShader
					= Map.GetTag<Blam::TagClass::ShaderEnvironment>(
						TagIndexEntry.TagID
					);
				CreateShaderEnvironmentDescriptor(TagIndexEntry, *CurShader);
			}
		};
	}

	Blam::DispatchTagVisitors(TagVisitors, TargetWorld.GetMapFile());

	return {std::move(NewScene)};
}

} // namespace VkBlam