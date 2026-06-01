#pragma once

#include <VkBlam/VkBlam.hpp>
#include <glm/gtx/quaternion.hpp>

namespace VkBlam
{
class SceneView
{
private:
public:
	glm::uvec2 Viewport;

	// View
	glm::f32vec3 Position = glm::f32vec3();
	glm::f32quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	// Projection
	glm::f32 FieldOfView = glm::radians(60.0f);
	glm::f32 AspectRatio = 16.0f / 9.0f;
	glm::f32 ZNear       = 0.01f;
	glm::f32 ZFar        = 1'000.0f;

	VkBlam::CameraGlobals CameraGlobalsData;

	void UpdateCameraGlobals();

	glm::f32mat4x4 GetLocalToWorldMatrix() const;
	glm::f32mat4x4 GetViewMatrix() const;
	glm::f32mat4x4 GetProjectionMatrix() const;
	glm::f32mat4x4 GetViewProjectionMatrix() const;

	std::array<glm::f32vec4, 6> GetFrustumPlanes() const;
	std::array<glm::f32vec3, 8> GetFrustumCorners() const;

	static bool AreBoundsVisible(
		std::span<const glm::f32vec4, 6> FrustumPlanes,
		std::span<const glm::f32vec3, 8> FrustumCorners, const glm::f32vec3 Min,
		const glm::f32vec3 Max
	);
};
} // namespace VkBlam