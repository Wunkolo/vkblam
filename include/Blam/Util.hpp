#pragma once

#include <cstdint>
#include <string>

#include "Enums.hpp"
#include "Tags.hpp"
#include "Types.hpp"

namespace Blam
{

std::string FormatTagClass(TagClass Class);

std::size_t GetVertexStride(VertexFormat Format);

// HEK allows a max of 0x2'0000 total surfaces and uses an array of 32-bit
// integers to map each surface's visibility to a single bit
// WordIndex = SurfaceIndex / 32
// BitIndex = SurfaceIndex % 32
using SurfaceOcclusionBitArray = std::span<std::uint32_t, 0x2'0000 / 32>;

void GenerateVisibleSurfaceIndices(
	const void* BSPData, std::uint32_t VirtualBase,
	std::span<const Tag<TagClass::ScenarioStructureBsp>::Cluster::SubCluster>
					SubClusters,
	const Bounds3D& OverlapTest, SurfaceOcclusionBitArray SurfaceOcclusionArray
);

// Enums
const char* ToString(const CacheVersion& Value);
const char* ToString(const ScenarioType& Value);

std::string ToString(const MapHeader& Value);
std::string ToString(const TagIndexHeader& Value);
std::string ToString(const TagIndexEntry& Value);
} // namespace Blam