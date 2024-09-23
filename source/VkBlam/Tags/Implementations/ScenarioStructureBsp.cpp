#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/ScenarioStructureBsp.hpp>

namespace VkBlam::Tags
{

ScenarioStructureBsp::~ScenarioStructureBsp()
{
}

ScenarioStructureBspSubsystem::ScenarioStructureBspSubsystem()
{
}

ScenarioStructureBspSubsystem::~ScenarioStructureBspSubsystem()
{
}

std::vector<DependentTag> ScenarioStructureBspSubsystem::GetDependentTags(
	const Blam::TagIndexEntry&                             TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& Tag,
	const Blam::MapFile&                                   MapFile
) const
{
	std::vector<DependentTag> DependentTags;

	// The BSP's main lightmap
	DependentTags.push_back({Tag.LightmapTexture.TagID});

	return DependentTags;
}

ScenarioStructureBsp* ScenarioStructureBspSubsystem::LoadTag(
	const Blam::TagIndexEntry&                             TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& Tag,
	Scene&                                                 TargetScene
)
{
	std::unique_ptr<ScenarioStructureBsp> NewScenarioStructureBsp(
		new ScenarioStructureBsp()
	);

	return ScenarioStructureBsps
		.emplace_back(std::move(NewScenarioStructureBsp))
		.get();
}
} // namespace VkBlam::Tags