#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/Scenario.hpp>

namespace VkBlam::Tags
{

Scenario::~Scenario()
{
}

ScenarioSubsystem::ScenarioSubsystem()
{
}

ScenarioSubsystem::~ScenarioSubsystem()
{
}

std::vector<DependentTag> ScenarioSubsystem::GetDependentTags(
	const Blam::TagIndexEntry&                 TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Scenario>& Tag, const Blam::MapFile& MapFile
) const
{
	std::vector<DependentTag> DependentTags;

	// Dependent StructureBSPs, these use a different heap, so the tag data
	// must be explicitly provided
	for( const auto& CurSBSP : MapFile.TagHeap.GetBlock(Tag.StructureBSPs) )
	{
		const Blam::VirtualHeap SBSPHeap
			= CurSBSP.GetSBSPHeap(MapFile.GetMapData());

		const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& ScenarioBSP
			= CurSBSP.GetSBSP(SBSPHeap);
		DependentTag Dependency{
			.TagID   = CurSBSP.BSP.TagID,
			.TagData = reinterpret_cast<const Blam::TagBase*>(&ScenarioBSP),
		};
		DependentTags.push_back(Dependency);
	}

	return DependentTags;
}

Scenario* ScenarioSubsystem::LoadTag(
	const Blam::TagIndexEntry&                 TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Scenario>& Tag, Scene& TargetScene
)
{
	std::unique_ptr<Scenario> NewScenario(new Scenario());

	return Scenarios.emplace_back(std::move(NewScenario)).get();
}
} // namespace VkBlam::Tags