#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

class ScenarioStructureBsp final
	: public TagImplementation<Blam::TagClass::ScenarioStructureBsp>
{
private:
public:
	~ScenarioStructureBsp();

	[[nodiscard]] Blam::TagClass GetTagClass() const override
	{
		return Blam::TagClass::ScenarioStructureBsp;
	}

	friend class ScenarioStructureBspSubsystem;
};

class ScenarioStructureBspSubsystem final
	: public TagSubsystem<
		  Blam::TagClass::ScenarioStructureBsp, ScenarioStructureBsp>
{
private:
	std::vector<std::unique_ptr<ScenarioStructureBsp>> ScenarioStructureBsps;

public:
	ScenarioStructureBspSubsystem();
	~ScenarioStructureBspSubsystem();

	[[nodiscard]] std::vector<DependentTag> GetDependentTags(
		const Blam::TagIndexEntry&                             TagIndexEntry,
		const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& Tag,
		const Blam::MapFile&                                   MapFile
	) const override;

	[[nodiscard]] ScenarioStructureBsp* LoadTag(
		const Blam::TagIndexEntry&                             TagIndexEntry,
		const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& Tag,
		Scene&                                                 TargetScene
	) override;
};

} // namespace VkBlam::Tags