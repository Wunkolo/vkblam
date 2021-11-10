#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "Enums.hpp"
#include "Tags.hpp"
#include "Types.hpp"

#include "Util.hpp"

namespace Blam
{

class MapFile
{
private:
	std::span<const std::byte> MapData;

public:
	MapFile(std::span<const std::byte> MapFileData);

	const Blam::MapHeader&      MapHeader;
	const Blam::TagIndexHeader& TagIndexHeader;
	const std::uint32_t         TagHeapVirtualBase;

	std::span<const Blam::TagIndexEntry> GetTagArray() const;
};

} // namespace Blam
