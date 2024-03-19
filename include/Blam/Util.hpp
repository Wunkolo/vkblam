#pragma once

#include <cstdint>
#include <string>

#include "Enums.hpp"
#include "Tags.hpp"
#include "Types.hpp"

#include "Util/VirtualHeap.hpp"

namespace Blam
{

std::string FormatTagClass(TagClass Class);

std::uint8_t GetVertexStride(VertexFormat Format);

// HEK allows a max of 0x2'0000 total surfaces and uses an array of 32-bit
// integers to map each surface's visibility to a single bit
// WordIndex = SurfaceIndex / 32
// BitIndex = SurfaceIndex % 32
using SurfaceOcclusionBitArray = std::span<std::uint32_t, 0x2'0000 / 32>;

void GenerateVisibleSurfaceIndices(
	const VirtualHeap& Heap,
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