#pragma once

#include "Tags.hpp"
#include "Types.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace Blam
{
// Encapsulates a halo map-cache file
class MapFile
{
private:
	const std::span<const std::byte> MapFileData;
	const std::span<const std::byte> BitmapFileData;

public:
	MapFile(
		std::span<const std::byte> MapFileData,
		std::span<const std::byte> BitmapFileData
	);

	const Blam::MapHeader&      MapHeader;
	const Blam::TagIndexHeader& TagIndexHeader;

	const VirtualHeap TagHeap;

	const std::span<const std::byte>& GetMapData() const;

	const std::span<const std::byte>& GetBitmapData() const;

	std::span<const TagIndexEntry> GetTagIndexArray() const;

	const TagIndexEntry* GetTagIndexEntry(std::uint16_t TagIndex) const;

	template<TagClass TagClassT>
	void VisitTagClass(
		const std::function<void(const TagIndexEntry, const Tag<TagClassT>&)>&
			Func
	) const
	{
		for( const auto& CurTagEntry : GetTagIndexArray() )
		{
			if( CurTagEntry.ClassPrimary == TagClassT )
			{
				const auto& CurTag = TagHeap.Read<Tag<TagClassT>>(
					CurTagEntry.TagDataVirtualOffset
				);
				Func(CurTagEntry, CurTag);
			}
		}
	}

	const TagBase* GetTag(std::uint32_t TagID) const;

	template<TagClass TagClassT>
	const Tag<TagClassT>* GetTag(std::uint32_t TagID) const
	{
		return reinterpret_cast<const Tag<TagClassT>*>(GetTag(TagID));
	}

	std::string_view GetTagPath(std::uint32_t TagID) const;

	// Helpers
	const Tag<TagClass::Scenario>* GetScenarioTag() const;

	std::span<const Tag<TagClass::Scenario>::StructureBSP>
		GetScenarioBSPs() const;
};

} // namespace Blam