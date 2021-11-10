#include "Blam/Util.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <span>

// #define VULKAN_HPP_NO_EXCEPTIONS
// #include <vulkan/vulkan.hpp>

#include <mio/mmap.hpp>

#include <Blam/Blam.hpp>

void HexDump(const std::span<const std::byte>& Data, std::uint8_t Columns = 16)
{

	for( std::size_t CurOffset = 0; CurOffset < Data.size();
		 CurOffset += Columns )
	{
		std::printf("0x%08lX:", CurOffset);
		for( const auto& Byte : Data.subspan(
				 CurOffset,
				 std::min<std::size_t>(Data.size() - CurOffset, Columns)) )
		{
			std::printf(" %02hhX", Byte);
		}
		std::printf("\n");
	}
}

int main(int argc, char* argv[])
{
	if( argc < 2 )
	{
		// Not enough arguments
		return EXIT_FAILURE;
	}
	auto MapFile = mio::mmap_source(argv[1]);

	Blam::MapFile CurMap(std::span<const std::byte>(
		reinterpret_cast<const std::byte*>(MapFile.data()), MapFile.size()));

	std::fputs(Blam::ToString(CurMap.MapHeader).c_str(), stdout);
	std::fputs(Blam::ToString(CurMap.TagIndexHeader).c_str(), stdout);

	// Find the base-tag
	if( const auto BaseTagPtr
		= CurMap.GetTagIndexEntry(CurMap.TagIndexHeader.BaseTag);
		BaseTagPtr )
	{
		const auto& CurTag = *BaseTagPtr;
		const char* TagName
			= MapFile.data()
			+ (CurTag.TagPathVirtualOffset - CurMap.TagHeapVirtualBase);

		if( !CurTag.IsExternal )
		{
			const std::byte* TagData(reinterpret_cast<const std::byte*>(
				MapFile.data()
				+ (CurTag.TagDataVirtualOffset - CurMap.TagHeapVirtualBase)));

			const auto& Scenario
				= *reinterpret_cast<const Blam::Tag<Blam::TagClass::Scenario>*>(
					TagData);

			// Iterate BSP
			// std::printf("Iterating BSPs: %s\n", TagName);
			for( const auto& CurBSPEntry : Scenario.StructureBSPs.GetSpan(
					 MapFile.data(), CurMap.TagHeapVirtualBase) )
			{
				const std::span<const std::byte> BSPData(
					reinterpret_cast<const std::byte*>(
						MapFile.data() + CurBSPEntry.BSPStart),
					CurBSPEntry.BSPSize);

				const char* BSPName
					= (MapFile.data()
					   + (CurBSPEntry.BSP.PathVirtualOffset
						  - CurMap.TagHeapVirtualBase));
				std::printf(
					"g %s %s \n",
					CurBSPEntry.BSP.PathVirtualOffset ? BSPName : "", TagName);

				const auto& ScenarioBSP = CurBSPEntry.GetBSP(MapFile.data());

				// std::printf(
				// 	"Bounds:\n"
				// 	"\tX:[%12.4f, %12.4f]\n"
				// 	"\tY:[%12.4f, %12.4f]\n"
				// 	"\tZ:[%12.4f, %12.4f]\n",
				// 	ScenarioBSP.WorldBoundsX[0], ScenarioBSP.WorldBoundsX[1],
				// 	ScenarioBSP.WorldBoundsY[0], ScenarioBSP.WorldBoundsY[1],
				// 	ScenarioBSP.WorldBoundsZ[0], ScenarioBSP.WorldBoundsZ[1]);

				const auto Surfaces = ScenarioBSP.Surfaces.GetSpan(
					BSPData.data(), CurBSPEntry.BSPVirtualBase);

				// Lightmap
				for( const auto& CurLightmap : ScenarioBSP.Lightmaps.GetSpan(
						 BSPData.data(), CurBSPEntry.BSPVirtualBase) )
				{
					for( const auto& CurMaterial :
						 CurLightmap.Materials.GetSpan(
							 BSPData.data(), CurBSPEntry.BSPVirtualBase) )
					{
						struct Vertex
						{
							float Position[3];
							float Normal[3];
							float Binormal[3];
							float Tangent[3];
							float UV[2];
						};
						const std::span<const Vertex> VertexData(
							reinterpret_cast<const Vertex*>(
								BSPData.data()
								+ (CurMaterial.UncompressedVertices
									   .VirtualOffset
								   - CurBSPEntry.BSPVirtualBase)),
							CurMaterial.Geometry.VertexBufferCount);

						for( const auto& Test : VertexData )
						{
							std::printf(
								"v %f %f %f\n"
								"vn %f %f %f\n"
								"vt %f %f\n",
								Test.Position[0], Test.Position[1],
								Test.Position[2], Test.Normal[0],
								Test.Normal[1], Test.Normal[2], Test.UV[0],
								Test.UV[1]);
						}
					}
				}

				std::uint16_t IndexStart = 1;
				for( const auto& CurLightmap : ScenarioBSP.Lightmaps.GetSpan(
						 BSPData.data(), CurBSPEntry.BSPVirtualBase) )
				{
					for( const auto& CurMaterial :
						 CurLightmap.Materials.GetSpan(
							 BSPData.data(), CurBSPEntry.BSPVirtualBase) )
					{
						const auto CurSurfaces = Surfaces.subspan(
							CurMaterial.SurfacesIndexStart,
							CurMaterial.SurfacesCount);

						for( const auto& CurSurface : CurSurfaces )
						{
							std::printf(
								"f %d/%d/%d %d/%d/%d %d/%d/%d\n",
								IndexStart + CurSurface[0],
								IndexStart + CurSurface[0],
								IndexStart + CurSurface[0],
								IndexStart + CurSurface[1],
								IndexStart + CurSurface[1],
								IndexStart + CurSurface[1],
								IndexStart + CurSurface[2],
								IndexStart + CurSurface[2],
								IndexStart + CurSurface[2]);
						}
						IndexStart += CurMaterial.Geometry.VertexBufferCount;
					}
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
