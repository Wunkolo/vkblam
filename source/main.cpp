#include <cstddef>
#include <cstdint>
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

	const Blam::MapHeader& Header
		= *reinterpret_cast<const Blam::MapHeader*>(MapFile.data());

	return EXIT_SUCCESS;
}