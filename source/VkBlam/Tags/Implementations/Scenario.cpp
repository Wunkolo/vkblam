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

std::vector<std::uint32_t> ScenarioSubsystem::GetDependentTags(
	const Blam::TagIndexEntry&                 TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Scenario>& Tag, const Blam::MapFile& MapFile
) const
{
	std::vector<std::uint32_t> DependentTags;

	// Dependent StructureBSP
	for( const auto& CurSBSP : MapFile.TagHeap.GetBlock(Tag.StructureBSPs) )
	{
		DependentTags.push_back(CurSBSP.BSP.TagID);
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