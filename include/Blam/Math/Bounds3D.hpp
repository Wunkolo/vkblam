#pragma once

#include "Vector.hpp"

namespace Blam
{
#pragma pack(push, 1)
struct Bounds3D
{
	Vector2f BoundsX;
	Vector2f BoundsY;
	Vector2f BoundsZ;

	inline bool Intersects(const Bounds3D& Other) const
	{
		return (BoundsX[0] <= Other.BoundsX[1] && BoundsX[1] >= Other.BoundsX[0]
			   )
			&& (BoundsY[0] <= Other.BoundsY[1] && BoundsY[1] >= Other.BoundsY[0]
			)
			&& (BoundsZ[0] <= Other.BoundsZ[1] && BoundsZ[1] >= Other.BoundsZ[0]
			);
	}
};
#pragma pack(pop)

static_assert(sizeof(Bounds3D) == sizeof(float) * 6);
} // namespace Blam