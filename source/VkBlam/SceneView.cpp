#include <VkBlam/SceneView.hpp>

namespace VkBlam
{
SceneView::SceneView(
	glm::f32mat4 View, glm::f32mat4 Projection, glm::uvec2 Viewport
)
	: Viewport(Viewport)
{
	CameraGlobalsData.View       = View;
	CameraGlobalsData.Projection = Projection;

	CameraGlobalsData.ViewProjection = Projection * View;
}
} // namespace VkBlam