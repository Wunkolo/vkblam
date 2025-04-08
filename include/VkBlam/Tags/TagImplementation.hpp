#pragma once

#include <VkBlam/Scene.hpp>
#include <VkBlam/VkBlam.hpp>

namespace VkBlam
{
class TagPool;

template<typename BaseT, typename T>
concept IsBaseOf = std::is_base_of_v<T, BaseT>;

class TagImplementationBase
{
private:
public:
	virtual ~TagImplementationBase() = 0;

	[[nodiscard]] virtual Blam::TagClass GetTagClass() const = 0;
};

template<Blam::TagClass ClassT>
class TagImplementation : public TagImplementationBase
{
private:
	// The original tag that this implementation is derived from
	const Blam::TagIndexEntry& TagIndexEntry;
	const Blam::Tag<ClassT>&   Tag;

public:
	TagImplementation(
		const Blam::TagIndexEntry& TagIndexEntry, const Blam::Tag<ClassT>& Tag
	)
		: TagIndexEntry(TagIndexEntry), Tag(Tag)
	{
	}

	const Blam::TagIndexEntry& GetTagIndexEntry() const
	{
		return TagIndexEntry;
	}

	const Blam::Tag<ClassT>& GetTag() const
	{
		return Tag;
	}

	// TagImplementationBase
	[[nodiscard]] Blam::TagClass GetTagClass() const override
	{
		return ClassT;
	}
	// TagImplementationBase
};

struct DependentTag
{
	// ID of the dependent tag
	std::uint32_t TagID;
	// Optionally provided tag data
	// Specifically used to handle ScenarioStructureBsp tags which
	// utilize a different heap than the typical TagHeap
	const Blam::TagBase* TagData = nullptr;
};
class TagSubsystemBase
{
private:
	TagPool& Pool;

public:
	TagSubsystemBase(TagPool& Pool);
	virtual ~TagSubsystemBase() = 0;

	[[nodiscard]] TagPool&             GetPool() const;
	[[nodiscard]] const Blam::MapFile& GetMapFile() const;

	[[nodiscard]] virtual Blam::TagClass GetHandledTagClass() const = 0;

	// Return a list of dependent tags that this tag depends on.
	// The tag is not considered "loaded" until this list of tags are also
	// completely loaded
	[[nodiscard]] virtual std::vector<DependentTag> GetDependentTags(
		const Blam::TagIndexEntry& TagIndexEntry, const Blam::TagBase& Tag,
		const Blam::MapFile& MapFile
	) const
		= 0;

	[[nodiscard]] virtual TagImplementationBase* LoadTag(
		const Blam::TagIndexEntry& TagIndexEntry, const Blam::TagBase& Tag,
		Scene& TargetScene
	) = 0;
};

template<Blam::TagClass ClassT, IsBaseOf<TagImplementationBase> ImplementationT>
class TagSubsystem : public TagSubsystemBase
{
private:
public:
	TagSubsystem(TagPool& Pool) : TagSubsystemBase(Pool)
	{
	}
	// Return a list of dependent tags that this tag depends on.
	// The tag is not considered "loaded" until this list of tags are also
	// completely loaded
	[[nodiscard]] virtual std::vector<DependentTag> GetDependentTags(
		const Blam::TagIndexEntry& TagIndexEntry, const Blam::Tag<ClassT>& Tag,
		const Blam::MapFile& MapFile
	) const
	{
		return {};
	};

	[[nodiscard]] virtual ImplementationT* LoadTag(
		const Blam::TagIndexEntry& TagIndexEntry, const Blam::Tag<ClassT>& Tag,
		Scene& TargetScene
	) = 0;

	// TagSubsystemBase
	[[nodiscard]] Blam::TagClass GetHandledTagClass() const override
	{
		return ClassT;
	}

	// Return a list of dependent tags that this tag depends on.
	// The tag is not considered "loaded" until this list of tags are also
	// completely loaded
	[[nodiscard]] std::vector<DependentTag> GetDependentTags(
		const Blam::TagIndexEntry& TagIndexEntry, const Blam::TagBase& Tag,
		const Blam::MapFile& MapFile
	) const override
	{
		return GetDependentTags(
			TagIndexEntry, *reinterpret_cast<const Blam::Tag<ClassT>*>(&Tag),
			MapFile
		);
	};

	[[nodiscard]] TagImplementationBase* LoadTag(
		const Blam::TagIndexEntry& TagIndexEntry, const Blam::TagBase& Tag,
		Scene& TargetScene
	) override
	{
		return LoadTag(
			TagIndexEntry, *reinterpret_cast<const Blam::Tag<ClassT>*>(&Tag),
			TargetScene
		);
	};

	// TagSubsystemBase
};

} // namespace VkBlam