#pragma once

#include <Blam/Math/Vector.hpp>

namespace Blam
{
#pragma pack(push, 1)
struct Vertex
{
	Vector3f Position;
	Vector3f Normal;
	Vector3f Binormal;
	Vector3f Tangent;
	Vector2f UV;
};

struct LightmapVertex
{
	Vector3f Normal;
	Vector2f UV;
};
#pragma pack(pop)

static_assert(sizeof(Vertex) == 56);
static_assert(sizeof(LightmapVertex) == 20);
} // namespace Blam