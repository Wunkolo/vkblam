#pragma once

#include <VkBlam/VkBlam.hpp>

namespace VkBlam
{
class SceneView
{
private:
public:
	SceneView(glm::f32mat4 View, glm::f32mat4 Projection, glm::uvec2 Viewport);

	VkBlam::CameraGlobals CameraGlobalsData;
	glm::uvec2            Viewport;
};
} // namespace VkBlam