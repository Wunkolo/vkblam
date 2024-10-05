#pragma once

#include <VkBlam/Scene.hpp>
#include <VkBlam/VkBlam.hpp>

#include "TagImplementation.hpp"

#include <concepts>
#include <map>
#include <memory>

namespace VkBlam
{

class TagPool
{
private:
	Scene& TargetScene;

	// TagID -> TagImplementation
	// TagSubsystems own the actual types, this is a strictly non-owning cache
	std::map<std::uint32_t, TagImplementationBase*> Tags;

	// TagClass -> TagSubsystem
	std::map<Blam::TagClass, std::unique_ptr<TagSubsystemBase>> TagSubsystems;

public:
	TagPool(Scene& TargetScene);
	~TagPool();

	bool RegisterTagSubsystem(
		Blam::TagClass TagClass, std::unique_ptr<TagSubsystemBase> TagSubsystem
	)
	{
		if( TagSubsystems.contains(TagClass) )
		{
			// Subsystem already exists
			return false;
		}

		TagSubsystems[TagClass] = std::move(TagSubsystem);

		return true;
	}

	// Get a loaded tag, null otherwise
	TagImplementationBase* GetTag(std::uint32_t TagID) const;
	// Load a tag's dependencies, then load the tag itself
	TagImplementationBase* LoadTag(std::uint32_t TagID);

	// Load a tag's dependencies, then load the tag itself
	// The tag-data is specifically provided rather than loading from the
	// tag-heap. Special-case done for ScenarioStructureBsp
	TagImplementationBase*
		LoadTag(std::uint32_t TagID, const Blam::TagBase* TagData);

	// Get a loaded tag, null otherwise
	template<std::derived_from<TagImplementationBase> ImplementationT>
	ImplementationT* GetTag(std::uint32_t TagID) const
	{
		return reinterpret_cast<ImplementationT*>(GetTag(TagID));
	}
	// Load a tag's dependencies, then load the tag itself
	template<std::derived_from<TagImplementationBase> ImplementationT>
	ImplementationT* LoadTag(std::uint32_t TagID)
	{
		return reinterpret_cast<ImplementationT*>(LoadTag(TagID));
	}
};
} // namespace VkBlam