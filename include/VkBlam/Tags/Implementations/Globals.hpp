#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

class Globals final : public TagImplementation<Blam::TagClass::Globals>
{
private:
public:
	Globals(
		const Blam::TagIndexEntry&                TagIndexEntry,
		const Blam::Tag<Blam::TagClass::Globals>& Tag
	);
	~Globals();

	friend class GlobalsSubsystem;
};

class GlobalsSubsystem final
	: public TagSubsystem<Blam::TagClass::Globals, Globals>
{
private:
	std::vector<std::unique_ptr<Globals>> Globalss;

public:
	GlobalsSubsystem(TagPool& Pool);
	~GlobalsSubsystem();

	[[nodiscard]] std::vector<DependentTag> GetDependentTags(
		const Blam::TagIndexEntry&                TagIndexEntry,
		const Blam::Tag<Blam::TagClass::Globals>& Tag,
		const Blam::MapFile&                      MapFile
	) const override;

	[[nodiscard]] Globals* LoadTag(
		const Blam::TagIndexEntry&                TagIndexEntry,
		const Blam::Tag<Blam::TagClass::Globals>& Tag, Scene& TargetScene
	) override;
};

} // namespace VkBlam::Tags