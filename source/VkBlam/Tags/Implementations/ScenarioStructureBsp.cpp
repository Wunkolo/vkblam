#include <Common/Format.hpp>
#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/Bitmap.hpp>
#include <VkBlam/Tags/Implementations/ScenarioStructureBsp.hpp>
#include <VkBlam/Tags/Implementations/ShaderEnvironment.hpp>
#include <VkBlam/Tags/TagPool.hpp>
#include <Vulkan/Memory.hpp>

namespace
{
vk::DescriptorSetLayoutBinding LightmapBindings[] = {
	{// LightmapImage
	 0, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
};
}

namespace VkBlam::Tags
{

ScenarioStructureBsp::ScenarioStructureBsp(
	const Blam::TagIndexEntry&                               TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>&   Tag,
	const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP& SBSPData
)
	: TagImplementation<Blam::TagClass::ScenarioStructureBsp>(
		  TagIndexEntry, Tag
	  ),
	  SBSPData(SBSPData)
{
}

ScenarioStructureBsp::~ScenarioStructureBsp()
{
}

ScenarioStructureBspSubsystem::ScenarioStructureBspSubsystem(
	TagPool& Pool, Rasterizer& TargetRasterizer
)
	: TagSubsystem<Blam::TagClass::ScenarioStructureBsp, ScenarioStructureBsp>(
		  Pool
	  ),
	  TargetRasterizer(TargetRasterizer)
{
	LightmapDescriptorPool = std::make_unique<Vulkan::DescriptorHeap>(
		Vulkan::DescriptorHeap::Create(
			TargetRasterizer.GetVulkanContext(), LightmapBindings
		)
			.value()
	);
}

ScenarioStructureBspSubsystem::~ScenarioStructureBspSubsystem()
{
}

namespace
{

const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP*
	FindSBSP(std::uint32_t TagID, const Blam::MapFile& MapFile)
{

	for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP& CurSBSP :
		 MapFile.GetScenarioBSPs() )
	{
		if( CurSBSP.BSP.TagID == TagID )
		{
			// Found!
			return &CurSBSP;
		}
	}
	// Could not find associated SBSP
	return nullptr;
}
} // namespace

std::vector<DependentTag> ScenarioStructureBspSubsystem::GetDependentTags(
	const Blam::TagIndexEntry&                             TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& Tag,
	const Blam::MapFile&                                   MapFile
) const
{
	std::vector<DependentTag> DependentTags;

	// The BSP's main lightmap
	DependentTags.push_back({Tag.LightmapTexture.TagID});

	// Find the Scenario::StructureBSP associated with this tag
	// This sucks.
	const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP* SBSP
		= FindSBSP(TagIndexEntry.TagID, MapFile);

	if( SBSP == nullptr )
	{
		// Could not find SBSP
		return DependentTags;
	}

	const Blam::VirtualHeap SBSPHeap = SBSP->GetSBSPHeap(MapFile.GetMapData());

	const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& ScenarioBSP
		= SBSP->GetSBSP(SBSPHeap);

	const auto Lightmaps = SBSPHeap.GetBlock(ScenarioBSP.Lightmaps);

	for( const auto& CurLightmap : Lightmaps )
	{
		for( const auto& CurMaterial :
			 SBSPHeap.GetBlock(CurLightmap.Materials) )
		{
			// Only supports shader-environment for now - 9/23/2024
			if( CurMaterial.Shader.Class == Blam::TagClass::ShaderEnvironment )
			{
				DependentTags.push_back({CurMaterial.Shader.TagID});
			}
		}
	}

	return DependentTags;
}

ScenarioStructureBsp* ScenarioStructureBspSubsystem::LoadTag(
	const Blam::TagIndexEntry&                             TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& Tag,
	Scene&                                                 TargetScene
)
{
	// Find the Scenario::StructureBSP associated with this tag
	// This sucks.
	const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP* SBSP
		= FindSBSP(TagIndexEntry.TagID, TargetScene.GetMapFile());

	if( SBSP == nullptr )
	{
		// Could not find SBSP
		return nullptr;
	}

	const Blam::VirtualHeap SBSPHeap
		= SBSP->GetSBSPHeap(TargetScene.GetMapFile().GetMapData());

	const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& ScenarioBSP
		= SBSP->GetSBSP(SBSPHeap);

	const char* BSPName = &TargetScene.GetMapFile().TagHeap.Read<char>(
		SBSP->BSP.PathVirtualOffset
	);

	std::unique_ptr<ScenarioStructureBsp> NewScenarioStructureBsp(
		new ScenarioStructureBsp(TagIndexEntry, Tag, *SBSP)
	);

	// Load BSP
	const auto Surfaces  = SBSPHeap.GetBlock(ScenarioBSP.Surfaces);
	const auto Lightmaps = SBSPHeap.GetBlock(ScenarioBSP.Lightmaps);

	const Tags::Bitmap* LightmapBitmap
		= GetPool().GetTag<Tags::Bitmap>(Tag.LightmapTexture.TagID);

	LightmapDescriptorSets.resize(Lightmaps.size());

	// Get default lightmap image
	vk::ImageView DefaultLightmapImageView;
	{
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
			= TargetScene.GetMapFile().TagHeap.GetBlock(Globals->RasterizerData
			)[0];
		DefaultLightmapImageView
			= GetPool()
				  .LoadTag<Tags::Bitmap>(RasterizerData.Default2D.TagID)
				  ->GetBitmap(
					  std::uint16_t(Blam::DefaultTextureIndex::Multiplicative)
				  )
				  .View.get();
	}

	// Index in elements, not bytes
	std::uint32_t VertexHeapIndexEnd = 0;
	std::uint32_t IndexOffsetEnd     = 0;

	std::size_t LightmapIndex = 0;
	for( const auto& CurLightmap : Lightmaps )
	{
		// Allocate a descriptor set for this span of lightmaps
		const vk::DescriptorSet& CurLightmapDescriptorSet
			= LightmapDescriptorSets[LightmapIndex]
			= LightmapDescriptorPool->AllocateDescriptorSet().value();

		Vulkan::SetObjectName(
			TargetRasterizer.GetVulkanContext().LogicalDevice,
			CurLightmapDescriptorSet, "Lightmap: {}", LightmapIndex
		);

		// If this is -1, then there is no lightmap
		const std::int16_t LightmapTextureIndex = CurLightmap.LightmapIndex;
		if( LightmapTextureIndex >= 0 )
		{
			TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
				CurLightmapDescriptorSet, 0,
				LightmapBitmap->GetBitmap(LightmapTextureIndex).View.get()
			);
		}
		else
		{
			// Default lightmap texture
			TargetRasterizer.GetDescriptorUpdateBatch().AddImage(
				CurLightmapDescriptorSet, 0, DefaultLightmapImageView
			);
		}

		for( const auto& CurMaterial :
			 SBSPHeap.GetBlock(CurLightmap.Materials) )
		{
			auto& CurLightmapMesh
				= NewScenarioStructureBsp->LightmapMeshs.emplace_back();
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

				CurLightmapMesh.LightmapDescriptorSet
					= CurLightmapDescriptorSet;

				//// Lightmap vertex buffer data
				{
					const std::span<const Blam::LightmapVertex>
						CurLightmapVertexData
						= CurMaterial.GetLightmapVertices(SBSPHeap);
					CurLightmapMesh.LightmapVertexData = CurLightmapVertexData;
				}

				VertexHeapIndexEnd += CurVertexData.size();
			}

			//// Index Buffer dataxiv
			CurLightmapMesh.IndexOffset = IndexOffsetEnd;
			CurLightmapMesh.IndexCount  = CurMaterial.SurfacesCount * 3;

			IndexOffsetEnd += CurLightmapMesh.IndexCount;
		}

		++LightmapIndex;
	}

	//// Create Vertex buffer heap
	const vk::BufferCreateInfo BSPVertexBufferInfo = {
		.size  = VertexHeapIndexEnd * sizeof(Blam::Vertex),
		.usage = vk::BufferUsageFlagBits::eVertexBuffer
			   | vk::BufferUsageFlagBits::eTransferDst,
	};

	if( auto CreateResult
		= TargetRasterizer.GetVulkanContext().LogicalDevice.createBufferUnique(
			BSPVertexBufferInfo
		);
		CreateResult.result == vk::Result::eSuccess )
	{
		NewScenarioStructureBsp->BSPVertexBuffer
			= std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating vertex buffer: %s\n",
			vk::to_string(CreateResult.result).c_str()
		);
		return nullptr;
	}

	Vulkan::SetObjectName(
		TargetRasterizer.GetVulkanContext().LogicalDevice,
		NewScenarioStructureBsp->BSPVertexBuffer.get(),
		"ScenarioStructureBsp[{:08X}]: BSP Vertex Buffer({}) | {}",
		TagIndexEntry.TagID, Common::FormatByteCount(BSPVertexBufferInfo.size),
		BSPName
	);

	//// Create Vertex buffer heap
	const vk::BufferCreateInfo BSPLightmapVertexBufferInfo = {
		.size  = VertexHeapIndexEnd * sizeof(Blam::LightmapVertex),
		.usage = vk::BufferUsageFlagBits::eVertexBuffer
			   | vk::BufferUsageFlagBits::eTransferDst,
	};

	if( auto CreateResult
		= TargetRasterizer.GetVulkanContext().LogicalDevice.createBufferUnique(
			BSPLightmapVertexBufferInfo
		);
		CreateResult.result == vk::Result::eSuccess )
	{
		NewScenarioStructureBsp->BSPLightmapVertexBuffer
			= std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating lightmap vertex buffer: %s\n",
			vk::to_string(CreateResult.result).c_str()
		);
		return nullptr;
	}

	Vulkan::SetObjectName(
		TargetRasterizer.GetVulkanContext().LogicalDevice,
		NewScenarioStructureBsp->BSPLightmapVertexBuffer.get(),
		"ScenarioStructureBsp[{:08X}]: BSP Lightmap Vertex Buffer({}) | {}",
		TagIndexEntry.TagID,
		Common::FormatByteCount(BSPLightmapVertexBufferInfo.size), BSPName
	);

	//// Create Index buffer heap
	const vk::BufferCreateInfo BSPIndexBufferInfo = {
		.size  = ScenarioBSP.Surfaces.Count * 3ULL * sizeof(std::uint16_t),
		.usage = vk::BufferUsageFlagBits::eIndexBuffer
			   | vk::BufferUsageFlagBits::eTransferDst,
	};

	if( auto CreateResult
		= TargetRasterizer.GetVulkanContext().LogicalDevice.createBufferUnique(
			BSPIndexBufferInfo
		);
		CreateResult.result == vk::Result::eSuccess )
	{
		NewScenarioStructureBsp->BSPIndexBuffer = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating Index buffer: %s\n",
			vk::to_string(CreateResult.result).c_str()
		);
		return nullptr;
	}
	Vulkan::SetObjectName(
		TargetRasterizer.GetVulkanContext().LogicalDevice,
		NewScenarioStructureBsp->BSPIndexBuffer.get(),
		"ScenarioStructureBsp[{:08X}]: BSP Index Buffer({}) | {}",
		TagIndexEntry.TagID, Common::FormatByteCount(BSPIndexBufferInfo.size),
		BSPName
	);

	// Create singular allocation of device memory for all vertex and index
	// data
	if( auto [Result, Value] = Vulkan::CommitBufferHeap(
			TargetRasterizer.GetVulkanContext().LogicalDevice,
			TargetRasterizer.GetVulkanContext().PhysicalDevice,
			std::array{
				NewScenarioStructureBsp->BSPVertexBuffer.get(),
				NewScenarioStructureBsp->BSPIndexBuffer.get(),
				NewScenarioStructureBsp->BSPLightmapVertexBuffer.get()
			}
		);
		Result == vk::Result::eSuccess )
	{
		NewScenarioStructureBsp->BSPGeometryMemory = std::move(Value);
	}
	else
	{
		std::fprintf(
			stderr, "Error committing vertex/index memory: %s\n",
			vk::to_string(Result).c_str()
		);
		return nullptr;
	}
	Vulkan::SetObjectName(
		TargetRasterizer.GetVulkanContext().LogicalDevice,
		NewScenarioStructureBsp->BSPGeometryMemory.get(),
		"ScenarioStructureBsp[{:08X}]: BSP Geometry Device Memory({}) | {}",
		TagIndexEntry.TagID, Common::FormatByteCount(BSPIndexBufferInfo.size),
		BSPName
	);

	// Buffers are all now binded to device memory, begin streaming
	for( const auto& CurLightmapMesh : NewScenarioStructureBsp->LightmapMeshs )
	{
		TargetScene.GetRasterizer().GetStreamBuffer().QueueBufferUpload(
			std::as_bytes(CurLightmapMesh.VertexData),
			NewScenarioStructureBsp->BSPVertexBuffer.get(),
			CurLightmapMesh.VertexIndexOffset * sizeof(Blam::Vertex)
		);

		TargetScene.GetRasterizer().GetStreamBuffer().QueueBufferUpload(
			std::as_bytes(CurLightmapMesh.LightmapVertexData),
			NewScenarioStructureBsp->BSPLightmapVertexBuffer.get(),
			CurLightmapMesh.VertexIndexOffset * sizeof(Blam::LightmapVertex)
		);
	}

	// Index Buffer
	{
		TargetScene.GetRasterizer().GetStreamBuffer().QueueBufferUpload(
			std::as_bytes(Surfaces),
			NewScenarioStructureBsp->BSPIndexBuffer.get(), 0
		);
	}

	return ScenarioStructureBsps
		.emplace_back(std::move(NewScenarioStructureBsp))
		.get();
}

void ScenarioStructureBspSubsystem::Draw(
	ScenarioStructureBsp& ScenarioStructureBsp, const SceneView& View,
	vk::CommandBuffer CommandBuffer
)
{
	const std::string_view ScenarioStructureBspName
		= GetMapFile().GetTagPath(ScenarioStructureBsp.GetTagIndexEntry().TagID
		);

	Vulkan::DebugLabelScope DebugScope(
		CommandBuffer, {0.0, 0.0, 0.5, 1.0}, "ScenarioStructureBsp: {}",
		ScenarioStructureBspName
	);

	// Bind geometry
	CommandBuffer.bindVertexBuffers(
		0,
		{ScenarioStructureBsp.BSPVertexBuffer.get(),
		 ScenarioStructureBsp.BSPLightmapVertexBuffer.get()},
		{0, 0}
	);

	CommandBuffer.bindIndexBuffer(
		ScenarioStructureBsp.BSPIndexBuffer.get(), 0, vk::IndexType::eUint16
	);

	// Todo: Shader base-class needed here
	const ShaderEnvironmentSubsystem* ShaderEnvironmentSubsystem
		= GetPool().GetTagSubsystem<Tags::ShaderEnvironmentSubsystem>(
			Blam::TagClass::ShaderEnvironment
		);

	CommandBuffer.bindPipeline(
		vk::PipelineBindPoint::eGraphics,
		ShaderEnvironmentSubsystem->GetPipeline()
	);

	CommandBuffer.pushConstants<VkBlam::CameraGlobals>(
		ShaderEnvironmentSubsystem->GetPipelineLayout(),
		vk::ShaderStageFlagBits::eAllGraphics, 0, {View.CameraGlobalsData}
	);

	for( std::size_t i = 0; i < ScenarioStructureBsp.LightmapMeshs.size(); ++i )
	{
		const auto& CurLightmapMesh = ScenarioStructureBsp.LightmapMeshs[i];
		Vulkan::InsertDebugLabel(
			CommandBuffer, {0.5, 0.5, 0.5, 1.0}, "BSP Draw: {}", i
		);

		Tags::ShaderEnvironment* CurShaderEnvironment
			= GetPool().GetTag<Tags::ShaderEnvironment>(
				CurLightmapMesh.ShaderTag
			);

		if( CurShaderEnvironment == nullptr )
		{
			// Unknown shader type
			continue;
		}

		// Bind shader descriptor
		CommandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			ShaderEnvironmentSubsystem->GetPipelineLayout(), 1,
			{CurShaderEnvironment->GetDescriptorSet()}, {}
		);

		// Bind lightmap texture
		CommandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			ShaderEnvironmentSubsystem->GetPipelineLayout(), 2,
			{CurLightmapMesh.LightmapDescriptorSet}, {}
		);

		CommandBuffer.drawIndexed(
			CurLightmapMesh.IndexCount, 1, CurLightmapMesh.IndexOffset,
			CurLightmapMesh.VertexIndexOffset, 0
		);
	}
}

} // namespace VkBlam::Tags