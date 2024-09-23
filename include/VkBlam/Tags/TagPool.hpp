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
	// TagSubsystems own the actual types, these are strictly non-owning
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

	// Load a tag's dependencies, then load the tag itself
	template<std::derived_from<TagImplementationBase> ImplementationT>
	ImplementationT* OpenTag(std::uint32_t TagID)
	{

		const Blam::TagIndexEntry* TagIndexEntryPtr
			= TargetScene.GetWorld().GetMapFile().GetTagIndexEntry(
				std::uint16_t(TagID)
			);

		if( Tags.contains(TagID) )
		{
			ImplementationT* TagImplementation
				= reinterpret_cast<ImplementationT*>(Tags.at(TagID));

			if( TagIndexEntryPtr->ClassPrimary
				!= TagImplementation->GetTagClass() )
			{
				// Type missmatch
				return nullptr;
			}

			return TagImplementation;
		}

		// Get a subsystem that handles this tag
		if( !TagSubsystems.contains(TagIndexEntryPtr->ClassPrimary) )
		{
			// No subsystem handles this tag
			return nullptr;
		}

		TagSubsystemBase* TagSubsystem
			= TagSubsystems.at(TagIndexEntryPtr->ClassPrimary).get();

		const Blam::TagBase* Tag
			= TargetScene.GetWorld().GetMapFile().GetTag(TagID);

		const auto& NewTag
			= (Tags[TagID]
			   = TagSubsystem->LoadTag(*TagIndexEntryPtr, *Tag, TargetScene));

		return reinterpret_cast<ImplementationT*>(NewTag);
	}
};
} // namespace VkBlam