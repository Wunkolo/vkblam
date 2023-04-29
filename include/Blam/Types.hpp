#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include <Blam/Math.hpp>

#include <Blam/Types/MapHeader.hpp>
#include <Blam/Types/TagBlock.hpp>
#include <Blam/Types/TagDataReferece.hpp>
#include <Blam/Types/TagIndex.hpp>
#include <Blam/Types/TagReference.hpp>
#include <Blam/Types/Vertex.hpp>

namespace Blam
{

#pragma pack(push, 1)
union DatumIndex
{
	std::int32_t Handle;
	struct
	{
		std::int16_t Index;
		std::int16_t Salt;
	};

	static constexpr DatumIndex Invalid()
	{
		return DatumIndex{-1};
	}
};

struct ResourceMapHeader
{
	ResourceMapType Type;
	std::uint32_t   TagPathsOffset;
	std::uint32_t   ResourceOffset;
	std::uint32_t   ResourceCount;
};
#pragma pack(pop)

static_assert(sizeof(DatumIndex) == sizeof(std::int32_t));
} // namespace Blam