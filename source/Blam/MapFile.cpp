#include <Blam/MapFile.hpp>

namespace Blam
{

MapFile::MapFile(
	std::span<const std::byte> MapFileData,
	std::span<const std::byte> BitmapFileData
)
	: MapFileData(MapFileData), BitmapFileData(BitmapFileData),
	  MapHeader(*reinterpret_cast<const Blam::MapHeader*>(MapFileData.data())),
	  TagIndexHeader(*reinterpret_cast<const Blam::TagIndexHeader*>(
		  MapFileData.data() + MapHeader.TagIndexOffset
	  )),
	  TagHeap{
		  (TagIndexHeader.TagIndexVirtualOffset
		   - std::uint32_t(sizeof(Blam::TagIndexHeader)))
			  - MapHeader.TagIndexOffset,
		  MapFileData
	  }
{
}

std::span<const TagIndexEntry> MapFile::GetTagIndexArray() const
{
	return std::span<const Blam::TagIndexEntry>(
		reinterpret_cast<const Blam::TagIndexEntry*>(
			MapFileData.data() + MapHeader.TagIndexOffset
			+ sizeof(Blam::TagIndexHeader)
		),
		TagIndexHeader.TagCount
	);
}

const TagIndexEntry* MapFile::GetTagIndexEntry(std::uint16_t TagIndex) const
{
	if( TagIndex >= TagIndexHeader.TagCount )
	{
		return nullptr;
	}
	return &GetTagIndexArray()[TagIndex];
}

const TagIndexEntry* MapFile::FindTagIndexEntry(std::string_view TagPath) const
{
	for( const TagIndexEntry& TagEntry : GetTagIndexArray() )
	{
		const std::string_view CurTagPath = GetTagPath(TagEntry.TagID);
		if( CurTagPath.compare(TagPath) == 0 )
		{
			return &TagEntry;
		}
	}
	return nullptr;
}

const std::span<const std::byte>& MapFile::GetMapData() const
{
	return MapFileData;
}

const std::span<const std::byte>& MapFile::GetBitmapData() const
{
	return BitmapFileData;
}

const TagBase* MapFile::GetTag(std::uint32_t TagID) const
{
	const TagIndexEntry* TagIndexEntryPtr
		= GetTagIndexEntry(std::uint16_t(TagID));
	if( TagIndexEntryPtr == nullptr )
	{
		return nullptr;
	}

	if( TagIndexEntryPtr->TagID != TagID )
	{
		// Salts don't match
		return nullptr;
	}

	return &TagHeap.Read<TagBase>(TagIndexEntryPtr->TagDataVirtualOffset);
}

std::string_view MapFile::GetTagPath(std::uint32_t TagID) const
{
	const TagIndexEntry* TagIndexEntryPtr = GetTagIndexEntry(TagID);
	if( TagIndexEntryPtr == nullptr )
	{
		return {};
	}

	return &TagHeap.Read<char>(TagIndexEntryPtr->TagPathVirtualOffset);
}

const Tag<TagClass::Scenario>* MapFile::GetScenarioTag() const
{
	return GetTag<TagClass::Scenario>(TagIndexHeader.BaseTag);
}

std::span<const Tag<TagClass::Scenario>::StructureBSP>
	MapFile::GetScenarioBSPs() const
{
	if( const auto* ScenarioTag = GetScenarioTag(); ScenarioTag )
	{
		return TagHeap.GetBlock(ScenarioTag->StructureBSPs);
	}
	return {};
}
} // namespace Blam
