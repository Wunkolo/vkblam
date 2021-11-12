#include <Blam/Blam.hpp>

namespace Blam
{

MapFile::MapFile(std::span<const std::byte> MapFileData)
	: MapData(MapFileData),
	  MapHeader(*reinterpret_cast<const Blam::MapHeader*>(MapData.data())),
	  TagIndexHeader(*reinterpret_cast<const Blam::TagIndexHeader*>(
		  MapData.data() + MapHeader.TagIndexOffset)),
	  TagHeapVirtualBase(
		  (TagIndexHeader.TagIndexVirtualOffset - sizeof(Blam::TagIndexHeader))
		  - MapHeader.TagIndexOffset)
{
}

std::span<const TagIndexEntry> MapFile::GetTagIndexArray() const
{
	return std::span<const Blam::TagIndexEntry>(
		reinterpret_cast<const Blam::TagIndexEntry*>(
			MapData.data() + MapHeader.TagIndexOffset
			+ sizeof(Blam::TagIndexHeader)),
		TagIndexHeader.TagCount);
}

const TagIndexEntry* MapFile::GetTagIndexEntry(std::uint16_t TagIndex) const
{
	if( TagIndex >= TagIndexHeader.TagCount )
	{
		return nullptr;
	}
	return &GetTagIndexArray()[TagIndex];
}

} // namespace Blam
