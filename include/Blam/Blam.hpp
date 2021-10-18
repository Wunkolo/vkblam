#pragma once

#include <cstddef>
#include <cstdint>

namespace Blam
{

enum CacheVersion : std::uint32_t
{
	Xbox          = 0x5,
	Demo          = 0x6,
	Retail        = 0x7,
	H1A           = 0xD,
	CustomEdition = 0x261
};

enum ScenarioType : std::uint16_t
{
	SinglePlayer  = 0x0,
	MultiPlayer   = 0x1,
	UserInterface = 0x2,
};

enum ResourceMapType : std::uint32_t
{
	Bitmaps = 0x0,
	Sounds  = 0x1,
	Loc     = 0x2,
};

#pragma pack(push, 1)

struct MapHeader
{
	std::uint32_t MagicHead; // 'head'
	CacheVersion  Version;
	std::uint32_t FileSize;
	std::uint32_t PaddingLength; // Xbox Only
	std::uint32_t TagIndexOffset;
	std::uint32_t TagIndexSize;
	std::byte     Pad18[8];
	char          ScenarioName[32];
	char          BuildVersion[32];
	ScenarioType  Type;
	std::byte     Pad64[2];
	std::uint32_t Checksum;
	std::uint32_t H1AFlags; // Todo

	std::byte     Pad6C[1936];
	std::uint32_t MagicFoot; // 'foot'
};

struct TagIndexHeader
{
	std::uint32_t TagArrayOffset;
	std::uint32_t Checksum;
	std::uint32_t ScenarioTagID;
	std::uint32_t TagCount;
	std::uint32_t ModelPartCount;
	std::uint32_t ModelDataOffset;
	std::uint32_t ModelPartCount_1;
	std::uint32_t VertexDataSize;
	std::uint32_t Magic; // 'tags'
};

struct ResourceMapHeader
{
	ResourceMapType Type;
	std::uint32_t   TagPathsOffset;
	std::uint32_t   ResourceOffset;
	std::uint32_t   ResourceCount;
};

#pragma pack(pop)

} // namespace Blam