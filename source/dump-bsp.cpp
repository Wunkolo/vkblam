#include "Blam/Util.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <span>
#include <vector>

#include <mio/mmap.hpp>

#include <Blam/Blam.hpp>

#include <Common/Format.hpp>

int main(int argc, char* argv[])
{
	if( argc < 2 )
	{
		// Not enough arguments
		return EXIT_FAILURE;
	}
	auto MapFile = mio::mmap_source(argv[1]);

	Blam::MapFile CurMap(
		std::span<const std::byte>(
			reinterpret_cast<const std::byte*>(MapFile.data()), MapFile.size()
		),
		{}
	);

	const auto MapData = CurMap.GetMapData();

	if( const auto BaseTagPtr
		= CurMap.GetTagIndexEntry(CurMap.TagIndexHeader.BaseTag);
		BaseTagPtr )
	{
		const auto&            CurTag = *BaseTagPtr;
		const std::string_view TagName
			= CurMap.GetTagName(CurMap.TagIndexHeader.BaseTag);

		if( const auto ScenarioPtr = CurMap.GetTag<Blam::TagClass::Scenario>(
				CurMap.TagIndexHeader.BaseTag
			);
			ScenarioPtr )
		{
			const Blam::Tag<Blam::TagClass::Scenario>& Scenario = *ScenarioPtr;

			std::uint16_t IndexStart = 1;

			for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP&
					 CurSBSP : CurMap.TagHeap.GetBlock(Scenario.StructureBSPs) )
			{
				const Blam::VirtualHeap BSPHeap = CurSBSP.GetSBSPHeap(MapData);

				const char* BSPName
					= &CurMap.TagHeap.Read<char>(CurSBSP.BSP.PathVirtualOffset);

				std::printf(
					"g %s %s\n", CurSBSP.BSP.PathVirtualOffset ? BSPName : "",
					TagName.data()
				);

				const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>&
					ScenarioBSP
					= CurSBSP.GetSBSP(MapData);

				const auto Surfaces = BSPHeap.GetBlock(ScenarioBSP.Surfaces);

				// Lightmap
				for( const auto& CurLightmap :
					 BSPHeap.GetBlock(ScenarioBSP.Lightmaps) )
				{
					for( const auto& CurMaterial :
						 BSPHeap.GetBlock(CurLightmap.Materials) )
					{
						auto Test = CurMaterial;
						for( const auto& CurVert :
							 CurMaterial.GetVertices(BSPHeap) )
						{
							std::printf(
								"v %f %f %f\n"
								"vn %f %f %f\n"
								"vt %f %f\n",
								CurVert.Position[0], CurVert.Position[1],
								CurVert.Position[2], CurVert.Normal[0],
								CurVert.Normal[1], CurVert.Normal[2],
								CurVert.UV[0], CurVert.UV[1]
							);
						}
					}
				}

				for( const auto& CurLightmap :
					 BSPHeap.GetBlock(ScenarioBSP.Lightmaps) )
				{
					for( const auto& CurMaterial :
						 BSPHeap.GetBlock(CurLightmap.Materials) )
					{
						const auto CurSurfaces = Surfaces.subspan(
							CurMaterial.SurfacesIndexStart,
							CurMaterial.SurfacesCount
						);
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
								IndexStart + CurSurface[2]
							);
						}
						IndexStart += CurMaterial.Geometry.VertexBufferCount;
					}
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
