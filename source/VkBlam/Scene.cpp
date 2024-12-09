#include <VkBlam/Format.hpp>
#include <VkBlam/Scene.hpp>
#include <VkBlam/Tags/TagImplementations.hpp>
#include <VkBlam/Tags/TagPool.hpp>

#include <Blam/TagVisitor.hpp>

#include <Vulkan/Memory.hpp>
#include <Vulkan/Pipeline.hpp>

#include <Common/Format.hpp>

namespace
{

vk::DescriptorSetLayoutBinding SceneBindings[] = {
	{// Default2DSamplerFiltered
	 0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// Default2DSamplerUnfiltered
	 1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// DefaultCubeSampler
	 2, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment
	},
};
} // namespace

namespace VkBlam
{
Scene::Scene(Rasterizer& TargetRasterizer, const World& TargetWorld)
	: TargetWorld(TargetWorld), TargetRasterizer(TargetRasterizer)
{
}

Scene::~Scene()
{
}

void Scene::Render(const SceneView& View, vk::CommandBuffer CommandBuffer)
{

	const vk::Viewport Viewport = {
		.x        = 0.0f,
		.y        = float(View.Viewport.y),
		.width    = float(View.Viewport.x),
		.height   = -float(View.Viewport.y),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	CommandBuffer.setViewport(0, {Viewport});

	// Scissor
	const vk::Rect2D Scissor = {
		.offset = {},
		.extent = {
			.width  = View.Viewport.x,
			.height = View.Viewport.y,
		},
	};

	CommandBuffer.setScissor(0, {Scissor});

	// Draw the scenario!
	auto* Scenario
		= Pool->GetTag<Tags::Scenario>(GetMapFile().TagIndexHeader.BaseTag);

	auto* ScenarioSubsystem = Pool->GetTagSubsystem<Tags::ScenarioSubsystem>(
		Blam::TagClass::Scenario
	);

	if( (Scenario != nullptr) && (ScenarioSubsystem != nullptr) )
	{
		ScenarioSubsystem->Draw(*Scenario, View, CommandBuffer);
	}

	// CommandBuffer.bindPipeline(
	// 	vk::PipelineBindPoint::eGraphics, DebugDrawPipeline.get()
	// );

	// Bing Scene globals
	// CommandBuffer.bindDescriptorSets(
	// 	vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(), 0,
	// 	{CurSceneDescriptor}, {}
	// );

	// CommandBuffer.pushConstants<VkBlam::CameraGlobals>(
	// 	DebugDrawPipelineLayout.get(), vk::ShaderStageFlagBits::eAllGraphics, 0,
	// 	{View.CameraGlobalsData}
	// );

	// CommandBuffer.bindVertexBuffers(
	// 	0, {BSPVertexBuffer.get(), BSPLightmapVertexBuffer.get()}, {0, 0}
	// );

	// CommandBuffer.bindIndexBuffer(
	// 	BSPIndexBuffer.get(), 0, vk::IndexType::eUint16
	// );

	// for( std::size_t i = 0; i < LightmapMeshs.size(); ++i )
	// {
	// 	const auto& CurLightmapMesh = LightmapMeshs[i];
	// 	Vulkan::InsertDebugLabel(
	// 		CommandBuffer, {0.5, 0.5, 0.5, 1.0}, "BSP Draw: {}", i
	// 	);

	// 	// Bind Shader descriptors
	// 	if( ShaderEnvironmentDescriptors.contains(CurLightmapMesh.ShaderTag) )
	// 	{
	// 		CommandBuffer.bindDescriptorSets(
	// 			vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
	// 			1, {ShaderEnvironmentDescriptors.at(CurLightmapMesh.ShaderTag)},
	// 			{}
	// 		);
	// 	}

	// 	// Bind Mesh descriptors
	// 	if( CurLightmapMesh.LightmapTag.has_value()
	// 		&& CurLightmapMesh.LightmapIndex.has_value() )
	// 	{
	// 		CommandBuffer.bindDescriptorSets(
	// 			vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
	// 			2,
	// 			{BitmapHeap.Sets.at(CurLightmapMesh.LightmapTag.value())
	// 				 .at(CurLightmapMesh.LightmapIndex.value())},
	// 			{}
	// 		);
	// 	}
	// 	else
	// 	{
	// 		CommandBuffer.bindDescriptorSets(
	// 			vk::PipelineBindPoint::eGraphics, DebugDrawPipelineLayout.get(),
	// 			2,
	// 			{BitmapHeap.Sets.at(BitmapHeap.Default2D)
	// 				 .at(std::uint32_t(Blam::DefaultTextureIndex::Multiplicative
	// 				 ))},
	// 			{}
	// 		);
	// 	}

	// 	CommandBuffer.drawIndexed(
	// 		CurLightmapMesh.IndexCount, 1, CurLightmapMesh.IndexOffset,
	// 		CurLightmapMesh.VertexIndexOffset, 0
	// 	);
	// }
}

std::unique_ptr<Scene>
	Scene::Create(Rasterizer& TargetRasterizer, const World& TargetWorld)
{
	std::unique_ptr<Scene> NewScene(new Scene(TargetRasterizer, TargetWorld));

	const Vulkan::Context& VulkanContext = TargetRasterizer.GetVulkanContext();

	// Descriptor pools
	{
		// NewScene->DebugDrawDescriptorPool
		// 	= std::make_unique<Vulkan::DescriptorHeap>(
		// 		Vulkan::DescriptorHeap::Create(
		// 			VulkanContext,
		// 			{{vk::DescriptorSetLayoutBinding{
		// 				.binding         = 0,
		// 				.descriptorType  = vk::DescriptorType::eSampledImage,
		// 				.descriptorCount = 1,
		// 				.stageFlags      = vk::ShaderStageFlagBits::eFragment,
		// 			}}}
		// 		).value()
		// 	);

		// NewScene->UnlitDescriptorPool =
		// std::make_unique<Vulkan::DescriptorHeap>(
		// 	Vulkan::DescriptorHeap::Create(
		// 		VulkanContext,
		// 		{{vk::DescriptorSetLayoutBinding{
		// 			.binding         = 0,
		// 			.descriptorType  = vk::DescriptorType::eSampledImage,
		// 			.descriptorCount = 1,
		// 			.stageFlags      = vk::ShaderStageFlagBits::eFragment,
		// 		}}}
		// 	).value()
		// );
		NewScene->SceneDescriptorPool
			= std::make_unique<Vulkan::DescriptorHeap>(
				Vulkan::DescriptorHeap::Create(VulkanContext, SceneBindings)
					.value()
			);
	}

	// Scene Descriptor
	{
		NewScene->CurSceneDescriptor
			= NewScene->SceneDescriptorPool->AllocateDescriptorSet().value();
		// Default2DSamplerFiltered
		TargetRasterizer.GetDescriptorUpdateBatch().AddSampler(
			NewScene->CurSceneDescriptor, 0,
			TargetRasterizer.GetSamplerCache().GetSampler(Sampler2D())
		);

		// Default2DSamplerUnfiltered
		TargetRasterizer.GetDescriptorUpdateBatch().AddSampler(
			NewScene->CurSceneDescriptor, 1,
			TargetRasterizer.GetSamplerCache().GetSampler(Sampler2D(false))
		);

		// Default2DSamplerUnfiltered
		TargetRasterizer.GetDescriptorUpdateBatch().AddSampler(
			NewScene->CurSceneDescriptor, 2,
			TargetRasterizer.GetSamplerCache().GetSampler(SamplerCube())
		);
	}

	{
		// Main Shader modules
		// const auto DefaultVertShaderData
		// 	= VkBlam::OpenResource("shaders/Default.vert.spv").value();
		// const auto DefaultFragShaderData
		// 	= VkBlam::OpenResource("shaders/Default.frag.spv").value();
		// const auto UnlitFragShaderData
		// 	= VkBlam::OpenResource("shaders/Unlit.frag.spv").value();

		// std::hash<std::string> StringHasher = {};

		// NewScene->UnlitFragmentShaderModule
		// 	= TargetRasterizer.GetShaderModuleCache()
		// 		  .GetShaderModule(
		// 			  StringHasher("shaders/Unlit.frag.spv"),
		// 			  UnlitFragShaderData
		// 		  )
		// 		  .value();

		const vk::RenderPass RenderPass
			= TargetRasterizer.GetDefaultRenderPass(RenderSamples);

		const auto [VertexBindingDescriptions, VertexAttributeDescriptions]
			= VkBlam::GetVertexInputDescriptions({{
				Blam::VertexFormat::SBSPVertexUncompressed,
				Blam::VertexFormat::SBSPLightmapVertexUncompressed,
			}});

		// std::tie(NewScene->UnlitDrawPipeline,
		// NewScene->UnlitDrawPipelineLayout) 	= CreateGraphicsPipeline(
		// 		VulkanContext.LogicalDevice,
		// 		{{vk::PushConstantRange{
		// 			.stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
		// 			.offset     = 0,
		// 			.size       = sizeof(VkBlam::CameraGlobals),
		// 		}}},
		// 		{{NewScene->UnlitDescriptorPool->GetDescriptorSetLayout(),
		// 		  NewScene->UnlitDescriptorPool->GetDescriptorSetLayout()}},
		// 		NewScene->DefaultVertexShaderModule,
		// 		NewScene->UnlitFragmentShaderModule, VertexBindingDescriptions,
		// 		VertexAttributeDescriptions, RenderPass, RenderSamples,
		// 		vk::PolygonMode::eLine
		// 	);
	}

	// Register each of the tag systems
	NewScene->Pool = std::make_unique<TagPool>(*NewScene);

	NewScene->Pool->RegisterTagSubsystem(
		Blam::TagClass::Bitmap,
		std::make_unique<Tags::BitmapSubsystem>(
			*NewScene->Pool, TargetRasterizer.GetVulkanContext()
		)
	);
	NewScene->Pool->RegisterTagSubsystem(
		Blam::TagClass::Globals,
		std::make_unique<Tags::GlobalsSubsystem>(*NewScene->Pool)
	);
	NewScene->Pool->RegisterTagSubsystem(
		Blam::TagClass::Scenario,
		std::make_unique<Tags::ScenarioSubsystem>(*NewScene->Pool)
	);
	NewScene->Pool->RegisterTagSubsystem(
		Blam::TagClass::ScenarioStructureBsp,
		std::make_unique<Tags::ScenarioStructureBspSubsystem>(
			*NewScene->Pool, TargetRasterizer
		)
	);
	NewScene->Pool->RegisterTagSubsystem(
		Blam::TagClass::ShaderEnvironment,
		std::make_unique<Tags::ShaderEnvironmentSubsystem>(
			*NewScene->Pool, TargetRasterizer
		)
	);

	// Load Scenario!
	const auto Scenario = NewScene->Pool->LoadTag<Tags::Scenario>(
		TargetWorld.GetMapFile().TagIndexHeader.BaseTag
	);

	if( Scenario == nullptr )
	{
		// Error loading scenario
		return nullptr;
	}

	// std::vector<Blam::TagVisitorProc> TagVisitors = {};

	// // Load BSP
	// {
	// 	// Index in elements, not bytes
	// 	std::uint32_t VertexHeapIndexEnd = 0;
	// 	std::uint32_t IndexHeapIndexEnd  = 0;

	// 	for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP& CurSBSP :
	// 		 TargetWorld.GetMapFile().GetScenarioBSPs() )
	// 	{
	// 		const Blam::VirtualHeap SBSPHeap
	// 			= CurSBSP.GetSBSPHeap(TargetWorld.GetMapFile().GetMapData());

	// 		const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& ScenarioBSP
	// 			= CurSBSP.GetSBSP(SBSPHeap);

	// 		const auto Surfaces = SBSPHeap.GetBlock(ScenarioBSP.Surfaces);

	// 		std::uint32_t SBSPIndexHeapEnd = IndexHeapIndexEnd;

	// 		// Lightmap
	// 		for( const auto& CurLightmap :
	// 			 SBSPHeap.GetBlock(ScenarioBSP.Lightmaps) )
	// 		{
	// 			const auto& LightmapTextureTag
	// 				= TargetWorld.GetMapFile().GetTag<Blam::TagClass::Bitmap>(
	// 					ScenarioBSP.LightmapTexture.TagID
	// 				);
	// 			const std::int16_t LightmapTextureIndex
	// 				= CurLightmap.LightmapIndex;

	// 			for( const auto& CurMaterial :
	// 				 SBSPHeap.GetBlock(CurLightmap.Materials) )
	// 			{

	// 				std::printf(
	// 					"Shader(%s): %s | Permutation: %04X | Surfaces: "
	// 					"[%04d,%04d)\n",
	// 					Blam::FormatTagClass(CurMaterial.Shader.Class).c_str(),
	// 					TargetWorld.GetMapFile()
	// 						.GetTagPath(CurMaterial.Shader.TagID)
	// 						.data(),
	// 					CurMaterial.ShaderPermutation,
	// 					CurMaterial.SurfacesIndexStart,
	// 					CurMaterial.SurfacesIndexStart
	// 						+ CurMaterial.SurfacesCount
	// 				);

	// 				auto& CurLightmapMesh
	// 					= NewScene->LightmapMeshs.emplace_back();
	// 				//// Vertex Buffer data
	// 				{
	// 					// Copy vertex data into the staging buffer
	// 					const std::span<const Blam::Vertex> CurVertexData
	// 						= CurMaterial.GetVertices(SBSPHeap);

	// 					CurLightmapMesh.VertexData = CurVertexData;

	// 					// Add the offset needed to begin indexing into
	// 					// this particular part of the vertex buffer,
	// 					// used when drawing
	// 					CurLightmapMesh.VertexIndexOffset = VertexHeapIndexEnd;

	// 					CurLightmapMesh.ShaderTag = CurMaterial.Shader.TagID;

	// 					if( ScenarioBSP.LightmapTexture.Valid()
	// 						&& LightmapTextureIndex != -1 )
	// 					{
	// 						CurLightmapMesh.LightmapTag
	// 							= ScenarioBSP.LightmapTexture.TagID;
	// 						CurLightmapMesh.LightmapIndex
	// 							= LightmapTextureIndex;
	// 					}

	// 					//// Lightmap vertex buffer data
	// 					{
	// 						const std::span<const Blam::LightmapVertex>
	// 							CurLightmapVertexData
	// 							= CurMaterial.GetLightmapVertices(SBSPHeap);
	// 						CurLightmapMesh.LightmapVertexData
	// 							= CurLightmapVertexData;
	// 					}

	// 					VertexHeapIndexEnd += CurVertexData.size();
	// 				}

	// 				//// Index Buffer data
	// 				CurLightmapMesh.IndexOffset = SBSPIndexHeapEnd;
	// 				CurLightmapMesh.IndexCount  = CurMaterial.SurfacesCount * 3;
	// 				SBSPIndexHeapEnd += CurMaterial.SurfacesCount * 3;
	// 			}
	// 		}

	// 		IndexHeapIndexEnd += ScenarioBSP.Surfaces.Count * 3;
	// 	}

	// 	//// Create Vertex buffer heap
	// 	const vk::BufferCreateInfo BSPVertexBufferInfo = {
	// 		.size  = VertexHeapIndexEnd * sizeof(Blam::Vertex),
	// 		.usage = vk::BufferUsageFlagBits::eVertexBuffer
	// 			   | vk::BufferUsageFlagBits::eTransferDst,
	// 	};

	// 	if( auto CreateResult
	// 		= VulkanContext.LogicalDevice.createBufferUnique(BSPVertexBufferInfo
	// 		);
	// 		CreateResult.result == vk::Result::eSuccess )
	// 	{
	// 		NewScene->BSPVertexBuffer = std::move(CreateResult.value);
	// 	}
	// 	else
	// 	{
	// 		std::fprintf(
	// 			stderr, "Error creating vertex buffer: %s\n",
	// 			vk::to_string(CreateResult.result).c_str()
	// 		);
	// 		return {};
	// 	}

	// 	Vulkan::SetObjectName(
	// 		VulkanContext.LogicalDevice, NewScene->BSPVertexBuffer.get(),
	// 		"VkBlam::Scene: BSP Vertex Buffer( {} )",
	// 		Common::FormatByteCount(BSPVertexBufferInfo.size)
	// 	);

	// 	//// Create Vertex buffer heap
	// 	const vk::BufferCreateInfo BSPLightmapVertexBufferInfo = {
	// 		.size  = VertexHeapIndexEnd * sizeof(Blam::LightmapVertex),
	// 		.usage = vk::BufferUsageFlagBits::eVertexBuffer
	// 			   | vk::BufferUsageFlagBits::eTransferDst,
	// 	};

	// 	if( auto CreateResult = VulkanContext.LogicalDevice.createBufferUnique(
	// 			BSPLightmapVertexBufferInfo
	// 		);
	// 		CreateResult.result == vk::Result::eSuccess )
	// 	{
	// 		NewScene->BSPLightmapVertexBuffer = std::move(CreateResult.value);
	// 	}
	// 	else
	// 	{
	// 		std::fprintf(
	// 			stderr, "Error creating lightmap vertex buffer: %s\n",
	// 			vk::to_string(CreateResult.result).c_str()
	// 		);
	// 		return {};
	// 	}

	// 	Vulkan::SetObjectName(
	// 		VulkanContext.LogicalDevice,
	// NewScene->BSPLightmapVertexBuffer.get(), 		"VkBlam::Scene: BSP
	// Lightmap Vertex Buffer( {} )",
	// 		Common::FormatByteCount(BSPLightmapVertexBufferInfo.size)
	// 	);

	// 	//// Create Index buffer heap
	// 	const vk::BufferCreateInfo BSPIndexBufferInfo = {
	// 		.size  = IndexHeapIndexEnd * sizeof(std::uint16_t),
	// 		.usage = vk::BufferUsageFlagBits::eIndexBuffer
	// 			   | vk::BufferUsageFlagBits::eTransferDst,
	// 	};

	// 	if( auto CreateResult
	// 		= VulkanContext.LogicalDevice.createBufferUnique(BSPIndexBufferInfo
	// 		);
	// 		CreateResult.result == vk::Result::eSuccess )
	// 	{
	// 		NewScene->BSPIndexBuffer = std::move(CreateResult.value);
	// 	}
	// 	else
	// 	{
	// 		std::fprintf(
	// 			stderr, "Error creating Index buffer: %s\n",
	// 			vk::to_string(CreateResult.result).c_str()
	// 		);
	// 		return {};
	// 	}
	// 	Vulkan::SetObjectName(
	// 		VulkanContext.LogicalDevice, NewScene->BSPIndexBuffer.get(),
	// 		"VkBlam::Scene: BSP Index Buffer( {} )",
	// 		Common::FormatByteCount(BSPIndexBufferInfo.size)
	// 	);

	// 	// Create singular allocation of device memory for all vertex and index
	// 	// data
	// 	if( auto [Result, Value] = Vulkan::CommitBufferHeap(
	// 			VulkanContext.LogicalDevice, VulkanContext.PhysicalDevice,
	// 			std::array{
	// 				NewScene->BSPVertexBuffer.get(),
	// 				NewScene->BSPIndexBuffer.get(),
	// 				NewScene->BSPLightmapVertexBuffer.get()
	// 			}
	// 		);
	// 		Result == vk::Result::eSuccess )
	// 	{
	// 		NewScene->BSPGeometryMemory = std::move(Value);
	// 	}
	// 	else
	// 	{
	// 		std::fprintf(
	// 			stderr, "Error committing vertex/index memory: %s\n",
	// 			vk::to_string(Result).c_str()
	// 		);
	// 		return {};
	// 	}
	// 	Vulkan::SetObjectName(
	// 		VulkanContext.LogicalDevice, NewScene->BSPGeometryMemory.get(),
	// 		"VkBlam::Scene: BSP Geometry Device Memory( {} )",
	// 		Common::FormatByteCount(BSPIndexBufferInfo.size)
	// 	);

	// 	// Buffers are all now binded to device memory, begin streaming
	// 	for( const auto& CurLightmapMesh : NewScene->LightmapMeshs )
	// 	{
	// 		TargetRasterizer.GetStreamBuffer().QueueBufferUpload(
	// 			std::as_bytes(CurLightmapMesh.VertexData),
	// 			NewScene->BSPVertexBuffer.get(),
	// 			CurLightmapMesh.VertexIndexOffset * sizeof(Blam::Vertex)
	// 		);

	// 		TargetRasterizer.GetStreamBuffer().QueueBufferUpload(
	// 			std::as_bytes(CurLightmapMesh.LightmapVertexData),
	// 			NewScene->BSPLightmapVertexBuffer.get(),
	// 			CurLightmapMesh.VertexIndexOffset * sizeof(Blam::LightmapVertex)
	// 		);
	// 	}

	// 	// Index Buffer
	// 	{
	// 		std::uint32_t IndexOffset = 0;
	// 		for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP&
	// 				 CurSBSP : TargetWorld.GetMapFile().GetScenarioBSPs() )
	// 		{
	// 			const Blam::VirtualHeap SBSPHeap
	// 				= CurSBSP.GetSBSPHeap(TargetWorld.GetMapFile().GetMapData()
	// 				);

	// 			const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>&
	// 				ScenarioBSP
	// 				= CurSBSP.GetSBSP(SBSPHeap);

	// 			const auto Surfaces = SBSPHeap.GetBlock(ScenarioBSP.Surfaces);

	// 			TargetRasterizer.GetStreamBuffer().QueueBufferUpload(
	// 				std::as_bytes(Surfaces), NewScene->BSPIndexBuffer.get(),
	// 				IndexOffset
	// 			);
	// 			IndexOffset += Surfaces.size_bytes();
	// 		}
	// 	}
	// }

	// std::vector<Blam::TagVisitorProc> TagPoolVisitors = {};
	// Blam::TagVisitorProc& TagPoolVisitor = TagVisitors.emplace_back();
	// TagPoolVisitor.VisitClass            = Blam::TagClass::Bitmap;

	// TagPoolVisitor.VisitTags
	// 	= [&](std::span<const Blam::TagIndexEntry> TagIndexEntries,
	// 		  const Blam::MapFile&                 Map) -> void {
	// 	for( const auto& TagIndexEntry : TagIndexEntries )
	// 	{
	// 		const auto CurBitmap
	// 			= Map.GetTag<Blam::TagClass::Bitmap>(TagIndexEntry.TagID);
	// 		const auto NewBitmap
	// 			= NewScene->Pool->LoadTag<Tags::Bitmap>(TagIndexEntry.TagID);
	// 	}
	// };

	// Blam::DispatchTagVisitors(TagPoolVisitors, TargetWorld.GetMapFile());

	// // Load bitmaps
	// {

	// 	// Create image handles
	// 	const auto CreateBitmapImage
	// 		= [&](VkBlam::BitmapHeapT::Bitmap&                   TargetBitmap,
	// 			  Blam::Tag<Blam::TagClass::Bitmap>::BitmapEntry BitmapEntry
	// 		  ) -> bool {
	// 		vk::ImageCreateFlags Flags       = {};
	// 		std::uint32_t        ArrayLayers = 1u;

	// 		const std::uint32_t MipLevels
	// 			= std::max<std::uint16_t>(BitmapEntry.MipmapCount, 1);

	// 		if( BitmapEntry.Type == Blam::BitmapEntryType::CubeMap )
	// 		{
	// 			Flags       = vk::ImageCreateFlagBits::eCubeCompatible;
	// 			ArrayLayers = 6u;
	// 		}

	// 		const vk::ImageCreateInfo ImageInfo = {
	// 			.flags = Flags,
	// 			.imageType = VkBlam::BlamToVk(BitmapEntry.Type),
	// 			.format    = VkBlam::BlamToVk(BitmapEntry.Format),
	// 			.extent    = vk::Extent3D{
	//                 .width = BitmapEntry.Width,
	// 				.height = BitmapEntry.Height,
	// 				.depth = BitmapEntry.Depth,
	// 			},
	// 			.mipLevels = MipLevels,
	// 			.arrayLayers = ArrayLayers,
	// 			.samples = vk::SampleCountFlagBits::e1,
	// 			.tiling  = vk::ImageTiling::eOptimal,
	// 			.usage   = vk::ImageUsageFlagBits::eSampled
	// 				   | vk::ImageUsageFlagBits::eTransferDst
	// 				   | vk::ImageUsageFlagBits::eTransferSrc,
	// 			.sharingMode   = vk::SharingMode::eExclusive,
	// 			.initialLayout = vk::ImageLayout::eUndefined,
	// 		};

	// 		auto& ImageDest = TargetBitmap.Image;

	// 		if( auto CreateResult
	// 			= VulkanContext.LogicalDevice.createImageUnique(ImageInfo);
	// 			CreateResult.result == vk::Result::eSuccess )
	// 		{
	// 			ImageDest = std::move(CreateResult.value);
	// 		}
	// 		else
	// 		{
	// 			std::fprintf(
	// 				stderr, "Error creating image: %s\n",
	// 				vk::to_string(CreateResult.result).c_str()
	// 			);
	// 			return false;
	// 		}
	// 		return true;
	// 	};

	// 	const auto CreateBitmap
	// 		= [&, CreateBitmapImage](
	// 			  const Blam::TagIndexEntry&               TagEntry,
	// 			  const Blam::Tag<Blam::TagClass::Bitmap>& Bitmap,
	// 			  const Blam::MapFile&                     Map
	// 		  ) -> void {
	// 		// std::printf("%s\n",
	// 		// CurWorld.GetMapFile().GetTagPath(TagEntry.TagID).data());
	// 		for( std::size_t CurSubTextureIdx = 0;
	// 			 CurSubTextureIdx < Bitmap.Bitmaps.Count; ++CurSubTextureIdx )
	// 		{
	// 			const auto& CurSubTexture
	// 				= Map.TagHeap.GetBlock(Bitmap.Bitmaps)[CurSubTextureIdx];

	// 			// Create bitmap and descriptor set
	// 			auto& BitmapDest
	// 				= NewScene->BitmapHeap
	// 					  .Bitmaps[TagEntry.TagID][CurSubTextureIdx];

	// 			auto& BitmapDescriptorDest
	// 				= NewScene->BitmapHeap
	// 					  .Sets[TagEntry.TagID][CurSubTextureIdx];

	// 			CreateBitmapImage(BitmapDest, CurSubTexture);

	// 			Vulkan::SetObjectName(
	// 				VulkanContext.LogicalDevice, BitmapDest.Image.get(),
	// 				"VkBlam::Scene: Bitmap {:08X}[{:2}] | {}", TagEntry.TagID,
	// 				CurSubTextureIdx, Map.GetTagPath(TagEntry.TagID)
	// 			);
	// 		}
	// 	};

	// 	Blam::TagVisitorProc& BitmapLoader = TagVisitors.emplace_back();

	// 	BitmapLoader.VisitClass = Blam::TagClass::Bitmap;

	// 	BitmapLoader.VisitTags
	// 		= [CreateBitmap](
	// 			  std::span<const Blam::TagIndexEntry> TagIndexEntries,
	// 			  const Blam::MapFile&                 Map
	// 		  ) -> void {
	// 		for( const auto& TagIndexEntry : TagIndexEntries )
	// 		{
	// 			const auto& CurBitmap
	// 				= Map.GetTag<Blam::TagClass::Bitmap>(TagIndexEntry.TagID);
	// 			CreateBitmap(TagIndexEntry, *CurBitmap, Map);
	// 		}
	// 	};

	// 	BitmapLoader.EndVisits = [&](const Blam::MapFile& Map) -> void {
	// 		Map.VisitTagClass<Blam::TagClass::Globals>(
	// 			[&](const Blam::TagIndexEntry&                TagEntry,
	// 				const Blam::Tag<Blam::TagClass::Globals>& Globals) -> void {
	// 				const auto& CurGlobal = Globals;
	// 				for( const auto& RasterData :
	// 					 TargetWorld.GetMapFile().TagHeap.GetBlock(
	// 						 CurGlobal.RasterizerData
	// 					 ) )
	// 				{
	// 					NewScene->BitmapHeap.Default2D
	// 						= RasterData.Default2D.TagID;
	// 					NewScene->BitmapHeap.Default3D
	// 						= RasterData.Default3D.TagID;
	// 					NewScene->BitmapHeap.DefaultCube
	// 						= RasterData.DefaultCube.TagID;
	// 				}
	// 			}
	// 		);
	// 	};

	// 	Blam::TagVisitorProc& BitmapCommitter = TagVisitors.emplace_back();

	// 	BitmapCommitter.Parallel   = true;
	// 	BitmapCommitter.VisitClass = Blam::TagClass::Bitmap;

	// 	// Allocate and bind memory for all bitmaps
	// 	BitmapCommitter.BeginVisits = [&](const Blam::MapFile& Map) -> void {
	// 		std::vector<vk::Image> Bitmaps;
	// 		for( const auto& CurBitmap : NewScene->BitmapHeap.Bitmaps )
	// 		{
	// 			for( const auto& CurSubBitmap : CurBitmap.second )
	// 			{
	// 				Bitmaps.emplace_back(CurSubBitmap.second.Image.get());
	// 			}
	// 		}

	// 		if( auto [Result, Value] = Vulkan::CommitImageHeap(
	// 				VulkanContext.LogicalDevice, VulkanContext.PhysicalDevice,
	// 				Bitmaps
	// 			);
	// 			Result == vk::Result::eSuccess )
	// 		{
	// 			NewScene->BitmapHeapMemory = std::move(Value);
	// 		}
	// 		else
	// 		{
	// 			std::fprintf(
	// 				stderr, "Error committing bitmap memory: %s\n",
	// 				vk::to_string(Result).c_str()
	// 			);
	// 			return;
	// 		}
	// 	};

	// 	// All images are now created and binded to memory
	// 	{
	// 		// Todo: This would be the draft of a bitmap manager's stream
	// 		// function
	// 		const auto StreamBitmapImage =
	// 			[&TargetRasterizer](
	// 				VkBlam::BitmapHeapT::Bitmap&                   TargetBitmap,
	// 				Blam::Tag<Blam::TagClass::Bitmap>::BitmapEntry BitmapEntry,
	// 				std::span<const std::byte>                     PixelData
	// 			) -> bool {
	// 			// Upload image data
	// 			const std::uint16_t MipCount
	// 				= std::max<std::uint16_t>(BitmapEntry.MipmapCount, 1);
	// 			const std::uint16_t LayerCount
	// 				= BitmapEntry.Type == Blam::BitmapEntryType::CubeMap ? 6
	// 																	 : 1;

	// 			const std::size_t BlockSize
	// 				= vk::blockSize(VkBlam::BlamToVk(BitmapEntry.Format));
	// 			const std::array<std::uint8_t, 3> BlockExtent
	// 				= vk::blockExtent(VkBlam::BlamToVk(BitmapEntry.Format));

	// 			std::size_t PixelDataOff = 0;

	// 			auto CurExtent = vk::Extent3D{
	// 				.width  = BitmapEntry.Width,
	// 				.height = BitmapEntry.Height,
	// 				.depth  = BitmapEntry.Depth
	// 			};
	// 			for( std::uint16_t CurMip = 0; CurMip < MipCount; ++CurMip )
	// 			{
	// 				for( std::uint16_t CurLayer = 0; CurLayer < LayerCount;
	// 					 ++CurLayer )
	// 				{
	// 					const std::array<std::uint32_t, 3> CurBlockCount
	// 						= {std::max(1u, CurExtent.width / BlockExtent[0]),
	// 						   std::max(1u, CurExtent.height / BlockExtent[1]),
	// 						   std::max(1u, CurExtent.depth / BlockExtent[2])};

	// 					const std::size_t CurPixelDataSize
	// 						= CurBlockCount[0] * CurBlockCount[1]
	// 						* CurBlockCount[2] * BlockSize;

	// 					TargetRasterizer.GetStreamBuffer().QueueImageUpload(
	// 						PixelData.subspan(PixelDataOff, CurPixelDataSize),
	// 						TargetBitmap.Image.get(), vk::Offset3D{0, 0, 0},
	// 						CurExtent,
	// 						vk::ImageSubresourceLayers{
	// 							.aspectMask = vk::ImageAspectFlagBits::eColor,
	// 							.mipLevel   = CurMip,
	// 							.baseArrayLayer = CurLayer,
	// 							.layerCount     = 1,
	// 						}
	// 					);

	// 					PixelDataOff += CurPixelDataSize;
	// 				}

	// 				CurExtent.width  = std::max(1u, CurExtent.width / 2);
	// 				CurExtent.height = std::max(1u, CurExtent.height / 2);
	// 				CurExtent.depth  = std::max(1u, CurExtent.depth / 2);
	// 			}

	// 			// Create image view
	// 			vk::ImageViewType ViewType = {};
	// 			switch( BitmapEntry.Type )
	// 			{
	// 			default:
	// 			case Blam::BitmapEntryType::Texture2D:
	// 			{
	// 				ViewType = vk::ImageViewType::e2D;
	// 				break;
	// 			}
	// 			case Blam::BitmapEntryType::Texture3D:
	// 			{
	// 				ViewType = vk::ImageViewType::e3D;
	// 				break;
	// 			}
	// 			case Blam::BitmapEntryType::CubeMap:
	// 			{
	// 				ViewType = vk::ImageViewType::eCube;
	// 				break;
	// 			}
	// 			}

	// 			const vk::ImageViewCreateInfo BitmapImageViewInfo = {
	// 				.image            = TargetBitmap.Image.get(),
	// 				.viewType         = ViewType,
	// 				.format           = VkBlam::BlamToVk(BitmapEntry.Format),
	// 				.subresourceRange = vk::
	// 					ImageSubresourceRange{vk::ImageAspectFlagBits::eColor,
	// 0, MipCount, 0, LayerCount},
	// 			};

	// 			if( auto CreateResult
	// 				= TargetRasterizer.GetVulkanContext()
	// 					  .LogicalDevice.createImageViewUnique(
	// 						  BitmapImageViewInfo
	// 					  );
	// 				CreateResult.result == vk::Result::eSuccess )
	// 			{
	// 				TargetBitmap.View = std::move(CreateResult.value);
	// 			}
	// 			else
	// 			{
	// 				std::fprintf(
	// 					stderr, "Error creating bitmap view: %s\n",
	// 					vk::to_string(CreateResult.result).c_str()
	// 				);
	// 				return false;
	// 			}
	// 			return true;
	// 		};

	// 		const auto StreamBitmap
	// 			= [&NewScene, &TargetWorld, &TargetRasterizer,
	// StreamBitmapImage]( 				  const Blam::TagIndexEntry& TagEntry,
	// 				  const Blam::Tag<Blam::TagClass::Bitmap>& Bitmap
	// 			  ) -> void {
	// 			for( std::size_t CurSubTextureIdx = 0;
	// 				 CurSubTextureIdx < Bitmap.Bitmaps.Count;
	// 				 ++CurSubTextureIdx )
	// 			{
	// 				const auto& CurSubTexture
	// 					= TargetWorld.GetMapFile().TagHeap.GetBlock(
	// 						Bitmap.Bitmaps
	// 					)[CurSubTextureIdx];
	// 				const auto PixelData = std::span<const std::byte>(
	// 					reinterpret_cast<const std::byte*>(
	// 						TargetWorld.GetMapFile().GetBitmapData().data()
	// 					) + CurSubTexture.PixelDataOffset,
	// 					CurSubTexture.PixelDataSize
	// 				);

	// 				auto& BitmapDest
	// 					= NewScene->BitmapHeap.Bitmaps.at(TagEntry.TagID)
	// 						  .at(CurSubTextureIdx);

	// 				StreamBitmapImage(BitmapDest, CurSubTexture, PixelData);

	// 				Vulkan::SetObjectName(
	// 					TargetRasterizer.GetVulkanContext().LogicalDevice,
	// 					BitmapDest.View.get(),
	// 					"VkBlam::Scene: Bitmap View {:08X}[{:2}] | {}",
	// 					TagEntry.TagID, CurSubTextureIdx,
	// 					TargetWorld.GetMapFile().GetTagPath(TagEntry.TagID)
	// 				);

	// 				// Create descriptor set
	// 				vk::DescriptorSet& TargetSet
	// 					= NewScene->BitmapHeap.Sets.at(TagEntry.TagID)
	// 						  .at(CurSubTextureIdx);

	// 				if( auto NewSet = NewScene->DebugDrawDescriptorPool
	// 									  ->AllocateDescriptorSet();
	// 					NewSet )
	// 				{
	// 					TargetSet = NewSet.value();
	// 				}
	// 				else
	// 				{
	// 					std::fprintf(
	// 						stderr, "Error allocating bitmap descriptor set\n"
	// 					);
	// 					return;
	// 				}

	// 				Vulkan::SetObjectName(
	// 					TargetRasterizer.GetVulkanContext().LogicalDevice,
	// 					TargetSet,
	// 					"VkBlam::Scene: Bitmap Descriptor Set {:08X}[{:2}] | "
	// 					"{}",
	// 					TagEntry.TagID, CurSubTextureIdx,
	// 					TargetWorld.GetMapFile().GetTagPath(TagEntry.TagID)
	// 				);

	// 				TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
	// 					TargetSet, 0, BitmapDest.View.get(),
	// 					vk::ImageLayout::eShaderReadOnlyOptimal
	// 				);
	// 			}
	// 		};

	// 		BitmapCommitter.VisitTags
	// 			= [StreamBitmap](
	// 				  std::span<const Blam::TagIndexEntry> TagIndexEntries,
	// 				  const Blam::MapFile&                 Map
	// 			  ) -> void {
	// 			for( const auto& TagIndexEntry : TagIndexEntries )
	// 			{
	// 				const auto CurBitmap
	// 					= Map.GetTag<Blam::TagClass::Bitmap>(TagIndexEntry.TagID
	// 					);
	// 				StreamBitmap(TagIndexEntry, *CurBitmap);
	// 			}
	// 		};

	// 		BitmapCommitter.EndVisits = [&](const Blam::MapFile& Map) -> void {
	// 			TargetRasterizer.GetDescriptorUpdateBatch().Flush();
	// 		};
	// 	}
	// }

	// // Create Shader-Environment descriptor sets
	// {
	// 	const auto CreateShaderEnvironmentDescriptor
	// 		= [&](const Blam::TagIndexEntry& TagEntry,
	// 			  const Blam::Tag<Blam::TagClass::ShaderEnvironment>&
	// 				  ShaderEnvironment) -> void {
	// 		const vk::DescriptorSet NewSet
	// 			= NewScene->ShaderEnvironmentDescriptorPool
	// 				  ->AllocateDescriptorSet()
	// 				  .value();
	// 		NewScene->ShaderEnvironmentDescriptors[TagEntry.TagID] = NewSet;

	// 		Vulkan::SetObjectName(
	// 			VulkanContext.LogicalDevice, NewSet,
	// 			"senv: {:08X} \'{}\' Descriptor Set", TagEntry.TagID,
	// 			TargetWorld.GetMapFile().GetTagPath(TagEntry.TagID)
	// 		);

	// 		const vk::ImageView BaseMapView
	// 			= (ShaderEnvironment.BaseMap.Valid()
	// 				   ? NewScene->BitmapHeap.Bitmaps
	// 						 .at(ShaderEnvironment.BaseMap.TagID)
	// 						 .at(0)
	// 				   : NewScene->BitmapHeap.Bitmaps
	// 						 .at(NewScene->BitmapHeap.Default2D)
	// 						 .at(std::uint32_t(
	// 							 Blam::DefaultTextureIndex::Multiplicative
	// 						 )))
	// 				  .View.get();
	// 		const vk::ImageView PrimaryDetailMapView
	// 			= (ShaderEnvironment.PrimaryDetailMap.Valid()
	// 				   ? NewScene->BitmapHeap.Bitmaps
	// 						 .at(ShaderEnvironment.PrimaryDetailMap.TagID)
	// 						 .at(0)
	// 				   : NewScene->BitmapHeap.Bitmaps
	// 						 .at(NewScene->BitmapHeap.Default2D)
	// 						 .at(0))
	// 				  .View.get();
	// 		const vk::ImageView SecondaryDetailMapView
	// 			= (ShaderEnvironment.SecondaryDetailMap.Valid()
	// 				   ? NewScene->BitmapHeap.Bitmaps
	// 						 .at(ShaderEnvironment.SecondaryDetailMap.TagID)
	// 						 .at(0)
	// 				   : NewScene->BitmapHeap.Bitmaps
	// 						 .at(NewScene->BitmapHeap.Default2D)
	// 						 .at(0))
	// 				  .View.get();
	// 		const vk::ImageView MicroDetailMapView
	// 			= (ShaderEnvironment.MicroDetailMap.Valid()
	// 				   ? NewScene->BitmapHeap.Bitmaps
	// 						 .at(ShaderEnvironment.MicroDetailMap.TagID)
	// 						 .at(0)
	// 				   : NewScene->BitmapHeap.Bitmaps
	// 						 .at(NewScene->BitmapHeap.Default2D)
	// 						 .at(0))
	// 				  .View.get();
	// 		const vk::ImageView BumpMapView
	// 			= (ShaderEnvironment.BumpMap.Valid()
	// 				   ? NewScene->BitmapHeap.Bitmaps
	// 						 .at(ShaderEnvironment.BumpMap.TagID)
	// 						 .at(0)
	// 				   : NewScene->BitmapHeap.Bitmaps
	// 						 .at(NewScene->BitmapHeap.Default2D)
	// 						 .at(std::uint32_t(Blam::DefaultTextureIndex::Vector
	// 						 )))
	// 				  .View.get();
	// 		const vk::ImageView GlowMapView
	// 			= (ShaderEnvironment.GlowMap.Valid()
	// 				   ? NewScene->BitmapHeap.Bitmaps
	// 						 .at(ShaderEnvironment.GlowMap.TagID)
	// 						 .at(0)
	// 				   : NewScene->BitmapHeap.Bitmaps
	// 						 .at(NewScene->BitmapHeap.Default2D)
	// 						 .at(std::uint32_t(
	// 							 Blam::DefaultTextureIndex::Additive
	// 						 )))
	// 				  .View.get();
	// 		const vk::ImageView ReflectionCubeMapView
	// 			= (ShaderEnvironment.ReflectionCubeMap.Valid()
	// 				   ? NewScene->BitmapHeap.Bitmaps
	// 						 .at(ShaderEnvironment.ReflectionCubeMap.TagID)
	// 						 .at(0)
	// 				   : NewScene->BitmapHeap.Bitmaps
	// 						 .at(NewScene->BitmapHeap.DefaultCube)
	// 						 .at(0))
	// 				  .View.get();

	// 		TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
	// 			NewSet, 0, BaseMapView, vk::ImageLayout::eShaderReadOnlyOptimal
	// 		);
	// 		TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
	// 			NewSet, 1, PrimaryDetailMapView,
	// 			vk::ImageLayout::eShaderReadOnlyOptimal
	// 		);
	// 		TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
	// 			NewSet, 2, SecondaryDetailMapView,
	// 			vk::ImageLayout::eShaderReadOnlyOptimal
	// 		);
	// 		TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
	// 			NewSet, 3, MicroDetailMapView,
	// 			vk::ImageLayout::eShaderReadOnlyOptimal
	// 		);
	// 		TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
	// 			NewSet, 4, BumpMapView, vk::ImageLayout::eShaderReadOnlyOptimal
	// 		);
	// 		TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
	// 			NewSet, 5, GlowMapView, vk::ImageLayout::eShaderReadOnlyOptimal
	// 		);
	// 		TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
	// 			NewSet, 6, ReflectionCubeMapView,
	// 			vk::ImageLayout::eShaderReadOnlyOptimal
	// 		);
	// 	};

	// 	Blam::TagVisitorProc& ShaderEnvironmentProc
	// 		= TagVisitors.emplace_back();

	// 	ShaderEnvironmentProc.VisitClass = Blam::TagClass::ShaderEnvironment;

	// 	ShaderEnvironmentProc.DependClasses
	// 		= {// Wait for bitmap loaders to finish
	// 		   Blam::TagClass::Bitmap
	// 		};

	// 	ShaderEnvironmentProc.VisitTags
	// 		= [CreateShaderEnvironmentDescriptor](
	// 			  std::span<const Blam::TagIndexEntry> TagIndexEntries,
	// 			  const Blam::MapFile&                 Map
	// 		  ) -> void {
	// 		for( const auto& TagIndexEntry : TagIndexEntries )
	// 		{
	// 			const auto CurShader
	// 				= Map.GetTag<Blam::TagClass::ShaderEnvironment>(
	// 					TagIndexEntry.TagID
	// 				);
	// 			CreateShaderEnvironmentDescriptor(TagIndexEntry, *CurShader);
	// 		}
	// 	};
	// }

	// Blam::DispatchTagVisitors(TagVisitors, TargetWorld.GetMapFile());

	return {std::move(NewScene)};
}

} // namespace VkBlam