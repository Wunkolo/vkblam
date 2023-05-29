#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_hash.hpp>

#include <cstdint>
namespace Vulkan
{
// Lightweight object to encapsulate everything minimally needed to interface
// with a fully initialized vulkan context
struct Context
{
	vk::Device         LogicalDevice;
	vk::PhysicalDevice PhysicalDevice;

	vk::Queue    RenderQueue;
	std::uint8_t RenderQueueFamilyIndex;

	vk::Queue    TransferQueue;
	std::uint8_t TransferQueueFamilyIndex;
};
} // namespace Vulkan
