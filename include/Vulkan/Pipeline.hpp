#include "VulkanAPI.hpp"

#include <span>

namespace Vulkan
{
vk::UniqueShaderModule CreateShaderModule(
	vk::Device LogicalDevice, std::span<const std::uint32_t> ShaderCode);
} // namespace Vulkan