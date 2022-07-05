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

std::optional<World> World::Create(const Blam::MapFile& MapFile)
{
	World NewWorld(MapFile);

	// Get the total world bounds min/max
	for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP& CurBSPEntry :
		 NewWorld.MapFile.GetScenarioBSPs() )
	{
		const std::span<const std::byte> BSPData
			= CurBSPEntry.GetSBSPData(NewWorld.MapFile.GetMapData().data());
		const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& ScenarioBSP
			= CurBSPEntry.GetSBSP(NewWorld.MapFile.GetMapData().data());

		NewWorld.WorldBoundMin.x
			= glm::min(NewWorld.WorldBoundMin.x, ScenarioBSP.WorldBoundsX[0]);
		NewWorld.WorldBoundMin.y
			= glm::min(NewWorld.WorldBoundMin.y, ScenarioBSP.WorldBoundsY[0]);
		NewWorld.WorldBoundMin.z
			= glm::min(NewWorld.WorldBoundMin.z, ScenarioBSP.WorldBoundsZ[0]);

		NewWorld.WorldBoundMax.x
			= glm::max(NewWorld.WorldBoundMax.x, ScenarioBSP.WorldBoundsX[1]);
		NewWorld.WorldBoundMax.y
			= glm::max(NewWorld.WorldBoundMax.y, ScenarioBSP.WorldBoundsY[1]);
		NewWorld.WorldBoundMax.z
			= glm::max(NewWorld.WorldBoundMax.z, ScenarioBSP.WorldBoundsZ[1]);
	}

	return {std::move(NewWorld)};
}
} // namespace VkBlam