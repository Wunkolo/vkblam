#include "VulkanAPI.hpp"

namespace Vulkan
{
std::int32_t FindMemoryTypeIndex(
	vk::PhysicalDevice PhysicalDevice, std::uint32_t MemoryTypeMask,
	vk::MemoryPropertyFlags Properties,
	vk::MemoryPropertyFlags ExcludeProperties
	= vk::MemoryPropertyFlagBits::eProtected);
} // namespace Vulkan