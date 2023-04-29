#pragma once

#include <cstdint>

#include <Blam/Enums.hpp>

namespace Blam
{
#pragma pack(push, 1)
struct TagReference
{
	TagClass      Class;
	std::uint32_t PathVirtualOffset;
	std::uint32_t PathLength;
	std::uint32_t TagID;

	bool Valid() const
	{
		return TagID != 0xFFFFFFFFu;
	}
};
#pragma pack(pop)

static_assert(sizeof(TagReference) == 0x10);
} // namespace Blam