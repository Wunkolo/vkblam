#include "Blam/Blam.hpp"
#include "Blam/Enums.hpp"
#include "Vulkan/VulkanAPI.hpp"
#include <vulkan/vulkan_enums.hpp>

#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#pragma once

namespace vkBlam
{

constexpr vk::ImageType BlamToVk(Blam::BitmapEntryType Value)
{
	switch( Value )
	{
	case Blam::BitmapEntryType::Texture2D:
		return vk::ImageType::e2D;
	case Blam::BitmapEntryType::Texture3D:
		return vk::ImageType::e3D;
	case Blam::BitmapEntryType::CubeMap:
		return vk::ImageType::e2D;
	case Blam::BitmapEntryType::White:
		return vk::ImageType::e2D;
	}
	return vk::ImageType::e2D;
}

constexpr vk::Format BlamToVk(Blam::BitmapEntryFormat Value)
{
	switch( Value )
	{
	case Blam::BitmapEntryFormat::A8:
		return vk::Format::eR8Unorm;
	case Blam::BitmapEntryFormat::Y8:
		return vk::Format::eR8Unorm;
	case Blam::BitmapEntryFormat::AY8:
		return vk::Format::eR8Unorm;
	case Blam::BitmapEntryFormat::A8Y8:
		return vk::Format::eR8G8Unorm;
	case Blam::BitmapEntryFormat::R5G6B5:
		return vk::Format::eR5G6B5UnormPack16;
	case Blam::BitmapEntryFormat::A1R5G5B5:
		return vk::Format::eA1R5G5B5UnormPack16;
	case Blam::BitmapEntryFormat::A4R4G4B4:
		return vk::Format::eA4R4G4B4UnormPack16EXT;
	case Blam::BitmapEntryFormat::X8R8G8B8:
		return vk::Format::eA8B8G8R8UnormPack32;
	case Blam::BitmapEntryFormat::A8R8G8B8:
		return vk::Format::eA8B8G8R8UnormPack32;
	case Blam::BitmapEntryFormat::DXT1:
		return vk::Format::eBc1RgbSrgbBlock;
	case Blam::BitmapEntryFormat::DXT2AND3:
		return vk::Format::eBc2SrgbBlock;
	case Blam::BitmapEntryFormat::DXT4AND5:
		return vk::Format::eBc3SrgbBlock;
	case Blam::BitmapEntryFormat::P8:
		return vk::Format::eR8Unorm;
	}
	return vk::Format::eUndefined;
}

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

} // namespace vkBlam