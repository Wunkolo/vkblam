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

	const Blam::TagIndexHeader& TagIndexheader
		= *reinterpret_cast<const Blam::TagIndexHeader*>(
			MapFile.data() + MapHeader.TagIndexOffset);

	std::printf(
		"Checksum: %08X\n"
		"TagCount: %08X\n"
		"ScenarioTag: %08X\n",
		TagIndexheader.Checksum, TagIndexheader.TagCount,
		TagIndexheader.ScenarioTagID);

	return EXIT_SUCCESS;
}