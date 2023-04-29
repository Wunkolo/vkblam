#pragma once

#include <array>

namespace Blam
{
#pragma pack(push, 1)
using Vector2f = std::array<float, 2>;
using Vector3f = std::array<float, 3>;
using Vector4f = std::array<float, 4>;
#pragma pack(pop)

static_assert(sizeof(Vector2f) == sizeof(float) * 2);
static_assert(sizeof(Vector3f) == sizeof(float) * 3);
static_assert(sizeof(Vector4f) == sizeof(float) * 4);
} // namespace Blam