#pragma once

#include <VkBlam/Scene.hpp>
#include <VkBlam/VkBlam.hpp>

namespace VkBlam
{

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
public:
	// TagImplementationBase
	[[nodiscard]] Blam::TagClass GetTagClass() const override
	{
		return ClassT;
	}
	// TagImplementationBase
};

class TagSubsystemBase
{
private:
public:
	virtual ~TagSubsystemBase() = 0;

	[[nodiscard]] virtual Blam::TagClass GetHandledTagClass() const = 0;

	[[nodiscard]] virtual TagImplementationBase* LoadTag(
		const Blam::TagIndexEntry& TagIndexEntry, const Blam::TagBase& Tag,
		Scene& TargetScene
	) = 0;
};

template<
	Blam::TagClass                           ClassT,
	std::derived_from<TagImplementationBase> ImplementationT>
class TagSubsystem : public TagSubsystemBase
{
private:
public:
	[[nodiscard]] virtual ImplementationT* LoadTag(
		const Blam::TagIndexEntry& TagIndexEntry, const Blam::Tag<ClassT>& Tag,
		Scene& TargetScene
	) = 0;

	// TagSubsystemBase
	[[nodiscard]] Blam::TagClass GetHandledTagClass() const override
	{
		return ClassT;
	}

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