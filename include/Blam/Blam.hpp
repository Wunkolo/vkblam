#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <type_traits>

#include "Enums.hpp"
#include "Tags.hpp"
#include "Types.hpp"

#include "Util.hpp"

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

	const std::span<const std::byte>& GetMapData() const
	{
		return MapFileData;
	}

	const std::span<const std::byte>& GetBitmapData() const
	{
		return BitmapFileData;
	}

	std::span<const Blam::TagIndexEntry> GetTagIndexArray() const;

	const TagIndexEntry* GetTagIndexEntry(std::uint16_t TagIndex) const;

	template<TagClass TagClassT>
	void VisitTagClass(
		const std::function<
			void(const Blam::TagIndexEntry, const Blam::Tag<TagClassT>&)>& Func
	) const
	{
		for( const auto& CurTagEntry : GetTagIndexArray() )
		{
			if( CurTagEntry.ClassPrimary == TagClassT )
			{
				const auto& CurTag = TagHeap.Read<Blam::Tag<TagClassT>>(
					CurTagEntry.TagDataVirtualOffset
				);
				Func(CurTagEntry, CurTag);
			}
		}
	}

	template<TagClass TagClassT>
	const Tag<TagClassT>* GetTag(std::uint32_t TagID) const
	{
		const TagIndexEntry* TagIndexEntryPtr
			= GetTagIndexEntry(std::uint16_t(TagID));
		if( !TagIndexEntryPtr )
		{
			return nullptr;
		}

		if( TagIndexEntryPtr->TagID != TagID )
		{
			// Salts don't match
			return nullptr;
		}

		return &TagHeap.Read<Blam::Tag<TagClassT>>(
			TagIndexEntryPtr->TagDataVirtualOffset
		);
	}

	std::string_view GetTagName(std::uint32_t TagID) const
	{
		const TagIndexEntry* TagIndexEntryPtr = GetTagIndexEntry(TagID);
		if( !TagIndexEntryPtr )
		{
			return {};
		}

		return &TagHeap.Read<char>(TagIndexEntryPtr->TagPathVirtualOffset);
	}

	// Helpers
	const Tag<Blam::TagClass::Scenario>* GetScenarioTag() const
	{
		return GetTag<Blam::TagClass::Scenario>(TagIndexHeader.BaseTag);
	}

	std::span<const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP>
		GetScenarioBSPs() const
	{
		if( const auto* ScenarioTag = GetScenarioTag(); ScenarioTag )
		{
			return TagHeap.GetBlock(ScenarioTag->StructureBSPs);
		}
		return {};
	}
};

} // namespace Blam
