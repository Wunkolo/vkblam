#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <span>

// #define VULKAN_HPP_NO_EXCEPTIONS
// #include <vulkan/vulkan.hpp>

#include <mio/mmap.hpp>

#include <Blam/Blam.hpp>

std::string FormatTagGroup(std::uint32_t TagClass)
{
	if( TagClass == -1 )
	{
		TagClass = '-' * 0x01010101;
	}
	TagClass = __builtin_bswap32(TagClass);
	std::string Result(reinterpret_cast<const char*>(&TagClass), 4);

	return Result;
}

int main(int argc, char* argv[])
{
	if( argc < 2 )
	{
		// Not enough arguments
		return EXIT_FAILURE;
	}

	auto MapFile = mio::mmap_source(argv[1]);

	const Blam::MapHeader& MapHeader
		= *reinterpret_cast<const Blam::MapHeader*>(MapFile.data());

	std::printf(
		"Map Header:\n"
		" - Version:         0x%08X\n"
		" - FileSize:        0x%08X\n"
		" - TagIndexOffset:  0x%08X\n"
		" - TagIndexSize:    0x%08X\n"
		" - ScenarioName:    \"%.32s\"\n"
		" - BuildVersion:    \"%.32s\"\n"
		" - Type:            0x%08X\n"
		" - Checksum:        0x%08X\n",
		MapHeader.Version, MapHeader.FileSize, MapHeader.TagIndexOffset,
		MapHeader.TagIndexSize, MapHeader.ScenarioName, MapHeader.BuildVersion,
		MapHeader.Type, MapHeader.Checksum);

	const Blam::TagIndexHeader& TagIndexHeader
		= *reinterpret_cast<const Blam::TagIndexHeader*>(
			MapFile.data() + MapHeader.TagIndexOffset);

	std::printf(
		"Tag Index Header:\n"
		" - TagArrayOffset:  0x%08X\n"
		" - BaseTag:         0x%08X\n"
		" - ScenarioTagID:   0x%08X\n"
		" - TagCount:        0x%08X\n"
		" - VertexCount:     0x%08X\n"
		" - VertexOffset:    0x%08X\n"
		" - IndexCount:      0x%08X\n"
		" - IndexOffset:     0x%08X\n"
		" - ModelDataSize:   0x%08X\n",
		TagIndexHeader.TagArrayOffset, TagIndexHeader.BaseTag,
		TagIndexHeader.ScenarioTagID, TagIndexHeader.TagCount,
		TagIndexHeader.VertexCount, TagIndexHeader.VertexOffset,
		TagIndexHeader.IndexCount, TagIndexHeader.IndexOffset,
		TagIndexHeader.ModelDataSize);

	const std::uint32_t MapMagic
		= (TagIndexHeader.TagArrayOffset - sizeof(Blam::TagIndexHeader))
		- MapHeader.TagIndexOffset;

	const std::span<const Blam::TagIndexEntry> TagArray(
		reinterpret_cast<const Blam::TagIndexEntry*>(
			MapFile.data() + MapHeader.TagIndexOffset
			+ sizeof(Blam::TagIndexHeader)),
		TagIndexHeader.TagCount);

	// Acceleration structure for fast tag lookups
	// TagID -> TagArrayEntry
	std::map<std::uint32_t, const Blam::TagIndexEntry*> TagIndexLUT;

	for( const auto& CurTag : TagArray )
	{
		TagIndexLUT[CurTag.TagID] = &CurTag;
	}

	// Find the base-tag
	if( const auto CurTagIt = TagIndexLUT.find(TagIndexHeader.BaseTag);
		CurTagIt != TagIndexLUT.end() )
	{
		const auto& CurTag = *CurTagIt->second;
		const char* Name   = MapFile.data() + (CurTag.TagPathOffset - MapMagic);
		std::printf(
			"%08X {%.4s %.4s %.4s} \"%s\"\n", CurTag.TagID,
			FormatTagGroup(CurTag.GroupPrimary).c_str(),
			FormatTagGroup(CurTag.GroupSecondary).c_str(),
			FormatTagGroup(CurTag.GroupTertiary).c_str(), Name);
	}

	return EXIT_SUCCESS;
}