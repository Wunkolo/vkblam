#include <VkBlam/Scene.hpp>

#include <Vulkan/Memory.hpp>

#include <Common/Format.hpp>

namespace VkBlam
{
Scene::Scene(Renderer& TargetRenderer, const World& TargetWorld)
	: TargetRenderer(TargetRenderer), TargetWorld(TargetWorld)
{
}

Scene::~Scene()
{
}

std::optional<Scene>
	Scene::Create(Renderer& TargetRenderer, const World& TargetWorld)
{
	Scene NewScene(TargetRenderer, TargetWorld);

	const Vulkan::Context& VulkanContext = TargetRenderer.GetVulkanContext();

	// Load Scenario
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

						// if( CurMaterial.Shader.Class
						// 	== Blam::TagClass::ShaderEnvironment )
						// {
						// 	auto* BasemapTag
						// 		= TargetWorld.GetMapFile()
						// 			  .GetTag<
						// 				  Blam::TagClass::ShaderEnvironment>(
						// 				  CurMaterial.Shader.TagID);
						// 	if( BasemapTag->BaseMap.TagID != -1 )
						// 	{
						// 		CurLightmapMesh.BasemapTag
						// 			= BasemapTag->BaseMap.TagID;
						// 	}
						// }

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

	return {std::move(NewScene)};
}

} // namespace VkBlam