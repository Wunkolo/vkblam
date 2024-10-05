
#include <VkBlam/VkBlam.hpp>

#include <VkBlam/Tags/TagPool.hpp>

#include <concepts>
#include <map>
#include <memory>

namespace VkBlam
{

TagPool::TagPool(Scene& TargetScene) : TargetScene(TargetScene)
{
}

TagPool::~TagPool()
{
}

TagImplementationBase* TagPool::GetTag(std::uint32_t TagID) const
{
	if( Tags.contains(TagID) )
	{
		return Tags.at(TagID);
	}
	return nullptr;
}

TagImplementationBase* TagPool::LoadTag(std::uint32_t TagID)
{
	const Blam::TagIndexEntry* TagIndexEntryPtr
		= TargetScene.GetWorld().GetMapFile().GetTagIndexEntry(
			std::uint16_t(TagID)
		);

	// Get tag implementation if it already exists
	if( Tags.contains(TagID) )
	{
		TagImplementationBase* TagImplementation = Tags.at(TagID);

		if( TagIndexEntryPtr->ClassPrimary != TagImplementation->GetTagClass() )
		{
			// Type missmatch
			return nullptr;
		}

		return TagImplementation;
	}

	// Tag does not exist, load tag

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

	// Load dependencies
	const std::vector<DependentTag> DependentTags
		= TagSubsystem->GetDependentTags(
			*TagIndexEntryPtr, *Tag, TargetScene.GetMapFile()
		);

	for( const DependentTag& DependentTag : DependentTags )
	{
		TagImplementationBase* DependentTagImplementation = nullptr;

		// If the tag data is immediately provided, prefer loading that rather
		// than using the tag heap. This is a special case done in particular
		// for ScenarioStructureBsp since that tag-data is loaded from a
		// different heap
		if( DependentTag.TagData != nullptr )
		{
			DependentTagImplementation
				= LoadTag(DependentTag.TagID, DependentTag.TagData);
		}
		else
		{
			DependentTagImplementation = LoadTag(DependentTag.TagID);
		}

		if( DependentTagImplementation == nullptr )
		{
			// Error loading dependent tag
			return nullptr;
		}
	}

	const auto& NewTag
		= (Tags[TagID]
		   = TagSubsystem->LoadTag(*TagIndexEntryPtr, *Tag, TargetScene));

	return NewTag;
}

TagImplementationBase*
	TagPool::LoadTag(std::uint32_t TagID, const Blam::TagBase* TagData)
{
	const Blam::TagIndexEntry* TagIndexEntryPtr
		= TargetScene.GetWorld().GetMapFile().GetTagIndexEntry(
			std::uint16_t(TagID)
		);

	// Get tag implementation if it already exists
	if( Tags.contains(TagID) )
	{
		TagImplementationBase* TagImplementation = Tags.at(TagID);

		if( TagIndexEntryPtr->ClassPrimary != TagImplementation->GetTagClass() )
		{
			// Type missmatch
			return nullptr;
		}

		return TagImplementation;
	}

	// Tag does not exist, load tag

	// Get a subsystem that handles this tag
	if( !TagSubsystems.contains(TagIndexEntryPtr->ClassPrimary) )
	{
		// No subsystem handles this tag
		return nullptr;
	}

	TagSubsystemBase* TagSubsystem
		= TagSubsystems.at(TagIndexEntryPtr->ClassPrimary).get();

	const Blam::TagBase* Tag = TagData;

	// Load dependencies
	const std::vector<DependentTag> DependentTags
		= TagSubsystem->GetDependentTags(
			*TagIndexEntryPtr, *Tag, TargetScene.GetMapFile()
		);

	for( const DependentTag& DependentTag : DependentTags )
	{
		if( LoadTag(DependentTag.TagID) == nullptr )
		{
			// Error loading dependent tag
			return nullptr;
		}
	}

	const auto& NewTag
		= (Tags[TagID]
		   = TagSubsystem->LoadTag(*TagIndexEntryPtr, *Tag, TargetScene));

	return NewTag;
}
} // namespace VkBlam