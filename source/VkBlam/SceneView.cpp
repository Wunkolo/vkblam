#include <VkBlam/SceneView.hpp>

namespace VkBlam
{
SceneView::SceneView(
	glm::f32mat4 View, glm::f32mat4 Projection, glm::uvec2 Viewport
)
	: Viewport(Viewport)
{
	CameraGlobalsData.View           = View;
	CameraGlobalsData.ViewProjection = Projection * View;
	// CameraGlobalsData.Projection = Projection;
}
} // namespace VkBlam