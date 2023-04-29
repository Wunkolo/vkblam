#pragma once

#include <cstdint>

namespace Blam
{
#pragma pack(push, 1)
struct TagDataReference
{
	std::uint32_t Size;
	std::uint32_t IsExternal;
	std::uint32_t Offset;
	std::uint64_t VirtualOffset;
};
#pragma pack(pop)

static_assert(sizeof(TagDataReference) == 20);
} // namespace Blam