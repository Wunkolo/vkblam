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
	std::map<std::uint32_t, std::unique_ptr<TagImplementation>> Tags;

public:
	TagPool(Scene& TargetScene);
	~TagPool();

	template<std::derived_from<TagImplementation> ImplementationT>
	ImplementationT* OpenTag(std::uint32_t TagID)
	{
		if( Tags.contains(TagID) )
		{
			return reinterpret_cast<ImplementationT*>(Tags.at(TagID).get());
		}

		const Blam::Tag<ImplementationT::ClassT>* Tag
			= TargetScene.GetWorld()
				  .GetMapFile()
				  .GetTag<ImplementationT::ClassT>(TagID);

		const auto& NewTag
			= (Tags[TagID] = ImplementationT::LoadTag(*Tag, TargetScene));

		return reinterpret_cast<ImplementationT*>(NewTag.get());
	}
};
} // namespace VkBlam