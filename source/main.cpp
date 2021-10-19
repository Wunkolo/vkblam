#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

// #define VULKAN_HPP_NO_EXCEPTIONS
// #include <vulkan/vulkan.hpp>

#include <mio/mmap.hpp>

#include <Blam/Blam.hpp>

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
		"Map Name: %.32s\n"
		"Build Date: %.32s\n",
		MapHeader.ScenarioName, MapHeader.BuildVersion);

	const Blam::TagIndexHeader& TagIndexHeader
		= *reinterpret_cast<const Blam::TagIndexHeader*>(
			MapFile.data() + MapHeader.TagIndexOffset);

	std::printf(
		"Checksum: %08X\n"
		"TagCount: %08X\n"
		"ScenarioTag: %08X\n",
		TagIndexHeader.Checksum, TagIndexHeader.TagCount,
		TagIndexHeader.ScenarioTagID);

	const std::uint32_t MapMagic
		= (TagIndexHeader.TagArrayOffset - 0x28) - MapHeader.TagIndexOffset;

	const Blam::TagArrayEntry* TagArray
		= reinterpret_cast<const Blam::TagArrayEntry*>(
			MapFile.data() + MapHeader.TagIndexOffset
			+ sizeof(Blam::TagIndexHeader));

	for( std::size_t i = 0; i < TagIndexHeader.TagCount; ++i )
	{
		const Blam::TagArrayEntry& CurTag = TagArray[i];

		const char* Name = MapFile.data() + (CurTag.TagPathOffset - MapMagic);
		std::printf(
			"%08X %.4s %s\n", CurTag.TagID, CurTag.TagGroupPrimary, Name);
	}

	return EXIT_SUCCESS;
}