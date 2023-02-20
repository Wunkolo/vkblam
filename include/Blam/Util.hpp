#pragma once

#include <cstdint>
#include <string>

#include "Enums.hpp"
#include "Types.hpp"

namespace Blam
{

std::string FormatTagClass(TagClass Class);

std::size_t GetVertexStride(VertexFormat Format);

// Enums
const char* ToString(const CacheVersion& Value);
const char* ToString(const ScenarioType& Value);

std::string ToString(const MapHeader& Value);
std::string ToString(const TagIndexHeader& Value);
std::string ToString(const TagIndexEntry& Value);
} // namespace Blam