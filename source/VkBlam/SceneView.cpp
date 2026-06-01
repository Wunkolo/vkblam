#include <VkBlam/SceneView.hpp>

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace VkBlam
{

void SceneView::UpdateCameraGlobals()
{
	CameraGlobalsData.View           = GetViewMatrix();
	CameraGlobalsData.ViewProjection = GetViewProjectionMatrix();
}

glm::f32mat4x4 SceneView::GetLocalToWorldMatrix() const
{
	glm::f32mat4x4 LocalToWorld(1.0f);
	LocalToWorld = glm::translate(LocalToWorld, Position);
	LocalToWorld *= glm::f32mat4x4(Rotation);
	return LocalToWorld;
}
glm::f32mat4x4 SceneView::GetViewMatrix() const
{
	return glm::lookAtLH(
		Position, Position + (Rotation * glm::f32vec3(0.0f, 0.0f, 1.0f)),
		Rotation * glm::f32vec3(0.0f, 1.0f, 1.0f)
	);
}
glm::f32mat4x4 SceneView::GetProjectionMatrix() const
{
	return glm::perspectiveLH_ZO<glm::f32>(
		FieldOfView, AspectRatio, ZNear, ZFar
	);
}
glm::f32mat4x4 SceneView::GetViewProjectionMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}
std::array<glm::f32vec4, 6> SceneView::GetFrustumPlanes() const
{
	const glm::f32mat4x4 ViewProjection = GetViewProjectionMatrix();

	std::array<glm::f32vec4, 6> Planes({
		// clang-format off
		(glm::row(ViewProjection, 3) + glm::row(ViewProjection, 0)), // left
		(glm::row(ViewProjection, 3) - glm::row(ViewProjection, 0)), // right
		(glm::row(ViewProjection, 3) + glm::row(ViewProjection, 1)), // top
		(glm::row(ViewProjection, 3) - glm::row(ViewProjection, 1)), // bottom
		(glm::row(ViewProjection, 3) + glm::row(ViewProjection, 2)), // near
		(glm::row(ViewProjection, 3) - glm::row(ViewProjection, 2)), // far
		// clang-format on
	});

	for( glm::f32vec4& Plane : Planes )
	{
		const glm::f32 Length = glm::length(glm::f32vec3(Plane));
		Plane /= Length;
	}

	return Planes;
}

static std::array<glm::f32vec4, 8> NDCCubeCorners{{
	glm::f32vec4{-1.0f, -1.0f, +0.0f, 1.0f}, // Near plane
	glm::f32vec4{-1.0f, +1.0f, +0.0f, 1.0f}, // Near plane
	glm::f32vec4{+1.0f, -1.0f, +0.0f, 1.0f}, // Near plane
	glm::f32vec4{+1.0f, +1.0f, +0.0f, 1.0f}, // Near plane
	glm::f32vec4{-1.0f, -1.0f, +1.0f, 1.0f}, // Far plane
	glm::f32vec4{-1.0f, +1.0f, +1.0f, 1.0f}, // Far plane
	glm::f32vec4{+1.0f, -1.0f, +1.0f, 1.0f}, // Far plane
	glm::f32vec4{+1.0f, +1.0f, +1.0f, 1.0f}, // Far plane
}};

std::array<glm::f32vec3, 8> SceneView::GetFrustumCorners() const
{
	glm::f32mat4x4 InverseViewProjection
		= glm::inverse(GetViewProjectionMatrix());
	std::array<glm::f32vec3, 8> FrustumCorners;
	for( std::size_t CornerIndex = 0; CornerIndex < 8; ++CornerIndex )
	{
		const glm::f32vec4 FrustumCornerNDC
			= InverseViewProjection * NDCCubeCorners[CornerIndex];
		const glm::f32vec3 FrustumCorner(FrustumCornerNDC / FrustumCornerNDC.w);

		FrustumCorners[CornerIndex] = FrustumCorner;
	}
	return FrustumCorners;
}

bool SceneView::AreBoundsVisible(
	std::span<const glm::f32vec4, 6> FrustumPlanes,
	std::span<const glm::f32vec3, 8> FrustumCorners, const glm::f32vec3 Min,
	const glm::f32vec3 Max
)
{
	// Test AABB corners against frustum planes
	for( const glm::f32vec4& Plane : FrustumPlanes )
	{
		if(
			// clang-format off
			   (glm::dot(Plane, glm::f32vec4{Min.x, Min.y, Min.z, 1.0f}) < 0.0f)
			&& (glm::dot(Plane, glm::f32vec4{Max.x, Min.y, Min.z, 1.0f}) < 0.0f)
			&& (glm::dot(Plane, glm::f32vec4{Min.x, Max.y, Min.z, 1.0f}) < 0.0f)
			&& (glm::dot(Plane, glm::f32vec4{Max.x, Max.y, Min.z, 1.0f}) < 0.0f)
			&& (glm::dot(Plane, glm::f32vec4{Min.x, Min.y, Max.z, 1.0f}) < 0.0f)
			&& (glm::dot(Plane, glm::f32vec4{Max.x, Min.y, Max.z, 1.0f}) < 0.0f)
			&& (glm::dot(Plane, glm::f32vec4{Min.x, Max.y, Max.z, 1.0f}) < 0.0f)
			&& (glm::dot(Plane, glm::f32vec4{Max.x, Max.y, Max.z, 1.0f}) < 0.0f)
			// clang-format on
		)
		{
			// All AABB corners are outside of the frustum planes
			return false;
		}
	}
	// Some AABB corners are outside of the frustum planes, further testing
	// needed

	// Test if all of the frustum corners are outside of the AABB
	if(
		// clang-format off
		   glm::all(glm::greaterThan(FrustumCorners[0], Max))
		&& glm::all(glm::greaterThan(FrustumCorners[1], Max))
		&& glm::all(glm::greaterThan(FrustumCorners[2], Max))
		&& glm::all(glm::greaterThan(FrustumCorners[3], Max))
		&& glm::all(glm::greaterThan(FrustumCorners[4], Max))
		&& glm::all(glm::greaterThan(FrustumCorners[5], Max))
		&& glm::all(glm::greaterThan(FrustumCorners[6], Max))
		&& glm::all(glm::greaterThan(FrustumCorners[7], Max))
		// clang-format on
	)
	{
		return false;
	}

	if(
		// clang-format off
		   glm::all(glm::lessThan(FrustumCorners[0], Min))
		&& glm::all(glm::lessThan(FrustumCorners[1], Min))
		&& glm::all(glm::lessThan(FrustumCorners[2], Min))
		&& glm::all(glm::lessThan(FrustumCorners[3], Min))
		&& glm::all(glm::lessThan(FrustumCorners[4], Min))
		&& glm::all(glm::lessThan(FrustumCorners[5], Min))
		&& glm::all(glm::lessThan(FrustumCorners[6], Min))
		&& glm::all(glm::lessThan(FrustumCorners[7], Min))
		// clang-format on
	)
	{
		return false;
	}

	return true;
}
} // namespace VkBlam