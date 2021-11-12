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

	// std::fputs(Blam::ToString(CurMap.MapHeader).c_str(), stdout);
	// std::fputs(Blam::ToString(CurMap.TagIndexHeader).c_str(), stdout);

	// Find the base-tag
	if( const auto BaseTagPtr
		= CurMap.GetTagIndexEntry(CurMap.TagIndexHeader.BaseTag);
		BaseTagPtr )
	{
		const auto&            CurTag = *BaseTagPtr;
		const std::string_view TagName
			= CurMap.GetTagName(CurMap.TagIndexHeader.BaseTag);

		if( const auto ScenarioPtr = CurMap.GetTag<Blam::TagClass::Scenario>(
				CurMap.TagIndexHeader.BaseTag);
			ScenarioPtr )
		{
			const Blam::Tag<Blam::TagClass::Scenario>& Scenario = *ScenarioPtr;
			// Iterate BSP
			// std::printf("Iterating BSPs: %s\n", TagName);
			std::uint16_t IndexStart = 1;
			for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP&
					 CurBSPEntry : Scenario.StructureBSPs.GetSpan(
						 MapFile.data(), CurMap.TagHeapVirtualBase) )
			{
				const std::span<const std::byte> BSPData
					= CurBSPEntry.GetSBSPData(MapFile.data());

				const char* BSPName
					= (MapFile.data()
					   + (CurBSPEntry.BSP.PathVirtualOffset
						  - CurMap.TagHeapVirtualBase));
				std::printf(
					"g %s %s\n",
					CurBSPEntry.BSP.PathVirtualOffset ? BSPName : "",
					TagName.data());

				const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>&
					ScenarioBSP
					= CurBSPEntry.GetSBSP(MapFile.data());

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
						auto Test = CurMaterial;
						for( const auto& CurVert : CurMaterial.GetVertices(
								 BSPData.data(), CurBSPEntry.BSPVirtualBase) )
						{
							// std::printf(
							//	"v %f %f %f\n"
							//	"vn %f %f %f\n"
							//	"vt %f %f\n",
							//	CurVert.Position[0], CurVert.Position[1],
							//	CurVert.Position[2], CurVert.Normal[0],
							//	CurVert.Normal[1], CurVert.Normal[2],
							//	CurVert.UV[0], CurVert.UV[1]);
						}
					}
				}

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
