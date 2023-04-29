#pragma once

#include <cstdint>

#include <Blam/Enums.hpp>

namespace Blam
{
#pragma pack(push, 1)
struct TagIndexHeader
{
	std::uint32_t TagIndexVirtualOffset;
	std::uint32_t BaseTag;
	std::uint32_t ScenarioTagID;
	std::uint32_t TagCount;
	std::uint32_t VertexCount;
	std::uint32_t VertexOffset;
	std::uint32_t IndexCount;
	std::uint32_t IndexOffset;
	std::uint32_t ModelDataSize;
	std::uint32_t MagicTags; // 'tags'
};

struct TagIndexEntry
{
	TagClass      ClassPrimary;
	TagClass      ClassSecondary;
	TagClass      ClassTertiary;
	std::uint32_t TagID;
	std::uint32_t TagPathVirtualOffset;
	std::uint32_t TagDataVirtualOffset;
	std::uint32_t IsExternal;
	std::uint32_t Unused;
};
#pragma pack(pop)

static_assert(sizeof(TagIndexHeader) == 40);
static_assert(sizeof(TagIndexEntry) == 32);
} // namespace Blam