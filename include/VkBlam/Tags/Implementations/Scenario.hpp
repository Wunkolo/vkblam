#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

class Scenario final : public TagImplementation<Blam::TagClass::Scenario>
{
private:
public:
	~Scenario();

	friend class ScenarioSubsystem;
};

class ScenarioSubsystem final
	: public TagSubsystem<Blam::TagClass::Scenario, Scenario>
{
private:
	std::vector<std::unique_ptr<Scenario>> Scenarios;

public:
	ScenarioSubsystem(TagPool& Pool);
	~ScenarioSubsystem();

	[[nodiscard]] std::vector<DependentTag> GetDependentTags(
		const Blam::TagIndexEntry&                 TagIndexEntry,
		const Blam::Tag<Blam::TagClass::Scenario>& Tag,
		const Blam::MapFile&                       MapFile
	) const override;

	[[nodiscard]] Scenario* LoadTag(
		const Blam::TagIndexEntry&                 TagIndexEntry,
		const Blam::Tag<Blam::TagClass::Scenario>& Tag, Scene& TargetScene
	) override;
};

} // namespace VkBlam::Tags