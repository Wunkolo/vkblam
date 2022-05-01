#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "Enums.hpp"

namespace Blam
{

#pragma pack(push, 1)

using Vector2f = std::array<float, 2>;
using Vector3f = std::array<float, 3>;
using Vector4f = std::array<float, 4>;

struct Vertex
{
	Blam::Vector3f Position;
	Blam::Vector3f Normal;
	Blam::Vector3f Binormal;
	Blam::Vector3f Tangent;
	Blam::Vector2f UV;
};
static_assert(sizeof(Vertex) == 56);

struct LightmapVertex
{
	Blam::Vector3f Normal;
	Blam::Vector2f UV;
};
static_assert(sizeof(LightmapVertex) == 20);

struct TagReference
{
	TagClass      Class;
	std::uint32_t PathVirtualOffset;
	std::uint32_t PathLength;
	std::uint32_t TagID;
};
static_assert(sizeof(TagReference) == 0x10);

struct TagDataReference
{
	std::uint32_t Size;
	std::uint32_t IsExternal;
	std::uint32_t Offset;
	std::uint64_t VirtualOffset;
};
static_assert(sizeof(TagDataReference) == 20);

// Header for variable-sized array of data in a tag
template<typename T = void>
struct TagBlock
{
	std::uint32_t Count;
	std::uint32_t VirtualOffset;
	std::uint32_t Unknown8;

	template<
		typename U = T,
		typename = typename std::enable_if_t<std::is_same_v<U, void> == false>>
	std::span<const U>
		GetSpan(const void* Data, std::uint32_t VirtualBase) const
	{
		return std::span<const U>(
			reinterpret_cast<const U*>(
				reinterpret_cast<const std::byte*>(Data)
				+ (VirtualOffset - VirtualBase)),
			Count);
	}
};
static_assert(sizeof(TagBlock<void>) == 12);

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
static_assert(sizeof(MapHeader) == 2048);

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
static_assert(sizeof(TagIndexHeader) == 40);

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
static_assert(sizeof(TagIndexEntry) == 32);

struct ResourceMapHeader
{
	ResourceMapType Type;
	std::uint32_t   TagPathsOffset;
	std::uint32_t   ResourceOffset;
	std::uint32_t   ResourceCount;
};

#pragma pack(pop)
} // namespace Blam