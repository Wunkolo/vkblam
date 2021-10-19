#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>

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
		= (TagIndexHeader.TagArrayOffset - sizeof(Blam::TagIndexHeader))
		- MapHeader.TagIndexOffset;

	const std::span<const Blam::TagArrayEntry> TagArray(
		reinterpret_cast<const Blam::TagArrayEntry*>(
			MapFile.data() + MapHeader.TagIndexOffset
			+ sizeof(Blam::TagIndexHeader)),
		TagIndexHeader.TagCount);

	for( const auto& CurTag : TagArray )
	{
		const char* Name = MapFile.data() + (CurTag.TagPathOffset - MapMagic);
		std::printf(
			"%08X %.4s \"%s\"\n", CurTag.TagID, CurTag.TagGroupPrimary, Name);
	}

	return EXIT_SUCCESS;
}