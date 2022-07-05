#include "mio/mmap.hpp"
#include <VkBlam/World.hpp>

#include <limits>

namespace VkBlam
{
World::World(const Blam::MapFile& MapFile)
	: MapFile(MapFile), WorldBoundMax(std::numeric_limits<float>::max()),
	  WorldBoundMin(std::numeric_limits<float>::min())
{
}

World::~World()
{
}

std::optional<World> World::Create(
	const Blam::MapFile& MapFile, std::filesystem::path BitmapPath)
{

	auto BitmapFileData = mio::mmap_source(BitmapPath.c_str());

	// Todo: This might be optional depending on the map version
	if( !BitmapFileData.is_open() )
	{
		// Error opening map files
		return {};
	}

	World NewMap(MapFile);

	NewMap.BitmapFileData = std::move(BitmapFileData);

	NewMap.BitmapPath = BitmapPath;

	// Get the total world bounds min/max
	for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP& CurBSPEntry :
		 NewMap.MapFile.GetScenarioBSPs() )
	{
		const std::span<const std::byte> BSPData
			= CurBSPEntry.GetSBSPData(NewMap.MapFile.GetMapData().data());
		const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& ScenarioBSP
			= CurBSPEntry.GetSBSP(NewMap.MapFile.GetMapData().data());

		NewMap.WorldBoundMin.x
			= glm::min(NewMap.WorldBoundMin.x, ScenarioBSP.WorldBoundsX[0]);
		NewMap.WorldBoundMin.y
			= glm::min(NewMap.WorldBoundMin.y, ScenarioBSP.WorldBoundsY[0]);
		NewMap.WorldBoundMin.z
			= glm::min(NewMap.WorldBoundMin.z, ScenarioBSP.WorldBoundsZ[0]);

		NewMap.WorldBoundMax.x
			= glm::max(NewMap.WorldBoundMax.x, ScenarioBSP.WorldBoundsX[1]);
		NewMap.WorldBoundMax.y
			= glm::max(NewMap.WorldBoundMax.y, ScenarioBSP.WorldBoundsY[1]);
		NewMap.WorldBoundMax.z
			= glm::max(NewMap.WorldBoundMax.z, ScenarioBSP.WorldBoundsZ[1]);
	}

	return {std::move(NewMap)};
}
} // namespace VkBlam