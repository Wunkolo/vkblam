#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <span>
#include <tuple>
namespace Vulkan
{

// Will try to find a memory type that is suitable for the given requirements.
// Returns -1 if no suitable memory type was found.
std::int32_t FindMemoryTypeIndex(
	vk::PhysicalDevice PhysicalDevice, std::uint32_t MemoryTypeMask,
	vk::MemoryPropertyFlags MemoryProperties,
	vk::MemoryPropertyFlags MemoryExcludeProperties
	= vk::MemoryPropertyFlagBits::eProtected);

// Given an array of valid Vulkan image-handles or buffer-handles, these
// functions will allocate a single block of device-memory for all of them
// and bind them consecutively.
//
// There may be a case that all the buffers or images cannot be allocated
// to the same device memory due to their required memory-type.
std::tuple<vk::Result, vk::UniqueDeviceMemory> CommitImageHeap(
	vk::Device Device, vk::PhysicalDevice PhysicalDevice,
	const std::span<const vk::Image> Images,
	vk::MemoryPropertyFlags          MemoryProperties
	= vk::MemoryPropertyFlagBits::eDeviceLocal,
	vk::MemoryPropertyFlags MemoryExcludeProperties
	= vk::MemoryPropertyFlagBits::eProtected);

std::tuple<vk::Result, vk::UniqueDeviceMemory> CommitBufferHeap(
	vk::Device Device, vk::PhysicalDevice PhysicalDevice,
	const std::span<const vk::Buffer> Buffers,
	vk::MemoryPropertyFlags           MemoryProperties
	= vk::MemoryPropertyFlagBits::eDeviceLocal,
	vk::MemoryPropertyFlags MemoryExcludeProperties
	= vk::MemoryPropertyFlagBits::eProtected);
} // namespace Vulkan