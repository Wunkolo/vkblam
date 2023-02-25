#pragma once

#include <map>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_map>

#include <Blam/Blam.hpp>
#include <Blam/Enums.hpp>
#include <Vulkan/VulkanAPI.hpp>

#define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/vec_swizzle.hpp>

namespace VkBlam
{

// Remove me
constexpr vk::SampleCountFlagBits RenderSamples = vk::SampleCountFlagBits::e4;

// Temporary structure so that the image heap can be passed around
struct BitmapHeapT
{
	struct Bitmap
	{
		vk::UniqueImage     Image;
		vk::UniqueImageView View;
	};
	std::unordered_map<std::uint32_t, std::map<std::uint16_t, Bitmap>> Bitmaps;

	// From the globals tag
	std::uint32_t Default2D;
	std::uint32_t Default3D;
	std::uint32_t DefaultCube;

	// Remove me
	std::unordered_map<
		std::uint32_t, std::map<std::uint16_t, vk::DescriptorSet>>
		Sets;
};

std::optional<std::span<const std::byte>> OpenResource(const std::string& Path);

std::vector<vk::VertexInputBindingDescription>
	GetVertexInputBindings(std::span<const Blam::VertexFormat> Formats);
std::vector<vk::VertexInputAttributeDescription>
	GetVertexInputAttributes(std::span<const Blam::VertexFormat> Formats);

inline std::tuple<
	std::vector<vk::VertexInputBindingDescription>,
	std::vector<vk::VertexInputAttributeDescription>>
	GetVertexInputDescriptions(std::span<const Blam::VertexFormat> Formats)
{
	return std::make_tuple(
		GetVertexInputBindings(Formats), GetVertexInputAttributes(Formats));
}

// Abstracts the way that halo utilizes its samplers
vk::SamplerCreateInfo Sampler2D(bool Filtered = true, bool Clamp = false);
vk::SamplerCreateInfo SamplerCube();

//// Must match vkBlam.glsl structures

struct CameraGlobals
{
	alignas(16) glm::f32mat4x4 View;
	alignas(16) glm::f32mat4x4 Projection;
	alignas(16) glm::f32mat4x4 ViewProjection;
};

struct SimulationGlobals
{
	alignas(16) glm::float32_t Time;
};
struct PassGlobals
{
	alignas(16) glm::f32vec4 ScreenSize; // {width, height, 1/width, 1/height}
};

} // namespace VkBlam