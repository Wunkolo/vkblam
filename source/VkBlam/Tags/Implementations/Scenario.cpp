#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/Scenario.hpp>
#include <VkBlam/Tags/Implementations/ScenarioStructureBsp.hpp>
#include <VkBlam/Tags/TagPool.hpp>

namespace VkBlam::Tags
{

Scenario::Scenario(
	const Blam::TagIndexEntry&                 TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Scenario>& Tag
)
	: TagImplementation<Blam::TagClass::Scenario>(TagIndexEntry, Tag)
{
}

Scenario::~Scenario()
{
}

ScenarioSubsystem::ScenarioSubsystem(TagPool& Pool)
	: TagSubsystem<Blam::TagClass::Scenario, Scenario>(Pool)
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
	std::unique_ptr<Scenario> NewScenario(new Scenario(TagIndexEntry, Tag));

	return Scenarios.emplace_back(std::move(NewScenario)).get();
}

void ScenarioSubsystem::Draw(
	Scenario& Scenario, const SceneView& View, vk::CommandBuffer CommandBuffer
)
{
	const std::string_view ScenarioName
		= GetMapFile().GetTagPath(Scenario.GetTagIndexEntry().TagID);
	;
	Vulkan::DebugLabelScope DebugScope(
		CommandBuffer, {0.0, 0.5, 0.0, 1.0}, "Scenario: {}", ScenarioName
	);

	// Draw each StructureBSP
	auto* ScenarioStructureBspSubsystem
		= GetPool().GetTagSubsystem<Tags::ScenarioStructureBspSubsystem>(
			Blam::TagClass::ScenarioStructureBsp
		);

	if( ScenarioStructureBspSubsystem == nullptr )
	{
		// Errot drawing ScenarioStructureBsps
		return;
	}

	for( const auto& StructureBSP :
		 GetMapFile().TagHeap.GetBlock(Scenario.GetTag().StructureBSPs) )
	{
		ScenarioStructureBsp* CurrentSBSP
			= GetPool().GetTag<Tags::ScenarioStructureBsp>(
				StructureBSP.BSP.TagID
			);

		if( CurrentSBSP != nullptr )
		{
			ScenarioStructureBspSubsystem->Draw(
				*CurrentSBSP, View, CommandBuffer
			);
		}
	}
}

} // namespace VkBlam::Tags