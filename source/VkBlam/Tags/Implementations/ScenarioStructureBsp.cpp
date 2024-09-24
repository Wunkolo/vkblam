#include <Common/Format.hpp>
#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/ScenarioStructureBsp.hpp>
#include <Vulkan/Memory.hpp>

namespace VkBlam::Tags
{

ScenarioStructureBsp::ScenarioStructureBsp(
	const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>&   SBSPTag,
	const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP& SBSPData
)
	: SBSPTag(SBSPTag), SBSPData(SBSPData)
{
}

ScenarioStructureBsp::~ScenarioStructureBsp()
{
}

ScenarioStructureBspSubsystem::ScenarioStructureBspSubsystem(
	TagPool& Pool, const Vulkan::Context& VulkanContext
)
	: TagSubsystem<Blam::TagClass::ScenarioStructureBsp, ScenarioStructureBsp>(
		  Pool
	  ),
	  VulkanContext(VulkanContext)
{
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
			// DependentTags.push_back({CurMaterial.Shader.TagID});
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

	std::unique_ptr<ScenarioStructureBsp> NewScenarioStructureBsp(
		new ScenarioStructureBsp(Tag, *SBSP)
	);

	// Load BSP
	const auto Surfaces  = SBSPHeap.GetBlock(ScenarioBSP.Surfaces);
	const auto Lightmaps = SBSPHeap.GetBlock(ScenarioBSP.Lightmaps);

	// Index in elements, not bytes
	std::uint32_t VertexHeapIndexEnd = 0;
	std::uint32_t IndexHeapIndexEnd  = 0;

	for( const auto& CurLightmap : Lightmaps )
	{
		// If this is -1, then there is no lightmap
		const std::int16_t LightmapTextureIndex = CurLightmap.LightmapIndex;

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

				if( ScenarioBSP.LightmapTexture.Valid()
					&& LightmapTextureIndex != -1 )
				{
					CurLightmapMesh.LightmapTag
						= ScenarioBSP.LightmapTexture.TagID;
					CurLightmapMesh.LightmapIndex = LightmapTextureIndex;
				}

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
			CurLightmapMesh.IndexCount = CurMaterial.SurfacesCount * 3;
		}
	}

	//// Create Vertex buffer heap
	const vk::BufferCreateInfo BSPVertexBufferInfo = {
		.size  = VertexHeapIndexEnd * sizeof(Blam::Vertex),
		.usage = vk::BufferUsageFlagBits::eVertexBuffer
			   | vk::BufferUsageFlagBits::eTransferDst,
	};

	if( auto CreateResult
		= VulkanContext.LogicalDevice.createBufferUnique(BSPVertexBufferInfo);
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
		VulkanContext.LogicalDevice,
		NewScenarioStructureBsp->BSPVertexBuffer.get(),
		"ScenarioStructureBsp: BSP Vertex Buffer( {} )",
		Common::FormatByteCount(BSPVertexBufferInfo.size)
	);

	//// Create Vertex buffer heap
	const vk::BufferCreateInfo BSPLightmapVertexBufferInfo = {
		.size  = VertexHeapIndexEnd * sizeof(Blam::LightmapVertex),
		.usage = vk::BufferUsageFlagBits::eVertexBuffer
			   | vk::BufferUsageFlagBits::eTransferDst,
	};

	if( auto CreateResult = VulkanContext.LogicalDevice.createBufferUnique(
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
		VulkanContext.LogicalDevice,
		NewScenarioStructureBsp->BSPLightmapVertexBuffer.get(),
		"ScenarioStructureBsp: BSP Lightmap Vertex Buffer( {} )",
		Common::FormatByteCount(BSPLightmapVertexBufferInfo.size)
	);

	//// Create Index buffer heap
	const vk::BufferCreateInfo BSPIndexBufferInfo = {
		.size  = ScenarioBSP.Surfaces.Count * 3ULL * sizeof(std::uint16_t),
		.usage = vk::BufferUsageFlagBits::eIndexBuffer
			   | vk::BufferUsageFlagBits::eTransferDst,
	};

	if( auto CreateResult
		= VulkanContext.LogicalDevice.createBufferUnique(BSPIndexBufferInfo);
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
		VulkanContext.LogicalDevice,
		NewScenarioStructureBsp->BSPIndexBuffer.get(),
		"VkBlam::Scene: BSP Index Buffer( {} )",
		Common::FormatByteCount(BSPIndexBufferInfo.size)
	);

	// Create singular allocation of device memory for all vertex and index
	// data
	if( auto [Result, Value] = Vulkan::CommitBufferHeap(
			VulkanContext.LogicalDevice, VulkanContext.PhysicalDevice,
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
		VulkanContext.LogicalDevice,
		NewScenarioStructureBsp->BSPGeometryMemory.get(),
		"ScenarioStructureBsp: BSP Geometry Device Memory( {} )",
		Common::FormatByteCount(BSPIndexBufferInfo.size)
	);

	// Buffers are all now binded to device memory, begin streaming
	for( const auto& CurLightmapMesh : NewScenarioStructureBsp->LightmapMeshs )
	{
		TargetScene.GetRenderer().GetStreamBuffer().QueueBufferUpload(
			std::as_bytes(CurLightmapMesh.VertexData),
			NewScenarioStructureBsp->BSPVertexBuffer.get(),
			CurLightmapMesh.VertexIndexOffset * sizeof(Blam::Vertex)
		);

		TargetScene.GetRenderer().GetStreamBuffer().QueueBufferUpload(
			std::as_bytes(CurLightmapMesh.LightmapVertexData),
			NewScenarioStructureBsp->BSPLightmapVertexBuffer.get(),
			CurLightmapMesh.VertexIndexOffset * sizeof(Blam::LightmapVertex)
		);
	}

	// Index Buffer
	{
		TargetScene.GetRenderer().GetStreamBuffer().QueueBufferUpload(
			std::as_bytes(Surfaces),
			NewScenarioStructureBsp->BSPIndexBuffer.get(), 0
		);
	}

	return ScenarioStructureBsps
		.emplace_back(std::move(NewScenarioStructureBsp))
		.get();
}
} // namespace VkBlam::Tags