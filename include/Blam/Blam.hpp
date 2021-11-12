#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

#include "Enums.hpp"
#include "Tags.hpp"
#include "Types.hpp"

#include "Util.hpp"

namespace Blam
{

class MapFile
{
private:
	std::span<const std::byte> MapData;

public:
	MapFile(std::span<const std::byte> MapFileData);

	const Blam::MapHeader&      MapHeader;
	const Blam::TagIndexHeader& TagIndexHeader;
	const std::uint32_t         TagHeapVirtualBase;

	std::span<const Blam::TagIndexEntry> GetTagIndexArray() const;

	const TagIndexEntry* GetTagIndexEntry(std::uint16_t TagIndex) const;

	template<TagClass TagClassT>
	const Tag<TagClassT>* GetTag(std::uint32_t TagID) const
	{
		const TagIndexEntry* TagIndexEntryPtr = GetTagIndexEntry(std::uint16_t(TagID));
		if( !TagIndexEntryPtr )
		{
			return nullptr;
		}

		if(TagIndexEntryPtr->TagID != TagID)
		{
			// Salts don't match
			return nullptr;
		}

		return reinterpret_cast<const Blam::Tag<TagClassT>*>(
			MapData.data()
			+ (TagIndexEntryPtr->TagDataVirtualOffset - TagHeapVirtualBase));
	}

	std::string_view GetTagName(std::uint32_t TagID) const
	{
		const TagIndexEntry* TagIndexEntryPtr = GetTagIndexEntry(TagID);
		if( !TagIndexEntryPtr )
		{
			return {};
		}

		return std::string_view(
			reinterpret_cast<const char*>(MapData.data())
			+ (TagIndexEntryPtr->TagPathVirtualOffset - TagHeapVirtualBase));
	}
};

} // namespace Blam
