#include <algorithm>
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

void HexDump(const std::span<const std::byte>& Data, std::uint8_t Columns = 16)
{

	for( std::size_t CurOffset = 0; CurOffset < Data.size();
		 CurOffset += Columns )
	{
		std::printf("0x%08X:", CurOffset);
		for( const auto& Byte : Data.subspan(
				 CurOffset,
				 std::min<std::size_t>(Data.size() - CurOffset, Columns)) )
		{
			std::printf(" %02X", Byte);
		}
		std::printf("\n");
	}
}

std::string FormatTagClass(Blam::TagClass Class)
{
	std::uint32_t TagStr = static_cast<std::uint32_t>(Class);
	if( Class == Blam::TagClass::None )
	{
		TagStr = '-' * 0x01010101;
	}
	TagStr = __builtin_bswap32(TagStr);
	return std::string(reinterpret_cast<const char*>(&TagStr), 4);
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
	// TagID -> TagIndexEntry
	std::map<std::uint32_t, const Blam::TagIndexEntry*> TagIndexLUT;

	for( const auto& CurTag : TagArray )
	{
		TagIndexLUT[CurTag.TagID] = &CurTag;
	}

	// Find the base-tag
	if( const auto CurTagIt = TagIndexLUT.find(0xEA7B0905);
		CurTagIt != TagIndexLUT.end() )
	{
		const auto& CurTag  = *CurTagIt->second;
		const auto& NextTag = *std::next(CurTagIt)->second;
		const char* Name = MapFile.data() + (CurTag.TagPathOffset - MapMagic);
		std::printf(
			"%08X {%.4s %.4s %.4s} \"%s\"\n", CurTag.TagID,
			FormatTagClass(CurTag.ClassPrimary).c_str(),
			FormatTagClass(CurTag.ClassSecondary).c_str(),
			FormatTagClass(CurTag.ClassTertiary).c_str(), Name);

		if( !CurTag.IsExternal )
		{
			const std::span<const std::byte> TagData(
				reinterpret_cast<const std::byte*>(
					MapFile.data() + (CurTag.TagDataOffset - MapMagic)),
				NextTag.TagDataOffset - CurTag.TagDataOffset);
			HexDump(TagData);

			const auto& TestWind
				= *reinterpret_cast<const Blam::Tag<Blam::TagClass::Wind>*>(
					TagData.data());
			std::printf(
				"\tMinWindSpeed: %f\n"
				"\tMaxWindSpeed: %f\n"
				"\tVariationYaw: %f\n"
				"\tVariationPitch: %f\n"
				"\tLocalVariationWeight: %f\n"
				"\tLocalVariationRate: %f\n"
				"\tDamping: %f\n",
				TestWind.MinWindSpeed, TestWind.MaxWindSpeed,
				TestWind.VariationYaw, TestWind.VariationPitch,
				TestWind.LocalVariationWeight, TestWind.LocalVariationRate,
				TestWind.Damping);
		}
	}

	return EXIT_SUCCESS;
}