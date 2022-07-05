#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <span>

namespace Vulkan
{

template<typename T>
inline vk::VertexInputBindingDescription CreateVertexInputBinding(
	std::uint8_t        BindingIndex,
	vk::VertexInputRate InputRate = vk::VertexInputRate::eVertex)
{
	return vk::VertexInputBindingDescription(
		BindingIndex, sizeof(T), InputRate);
}

vk::UniqueShaderModule CreateShaderModule(
	vk::Device LogicalDevice, std::span<const std::uint32_t> ShaderCode);
} // namespace Vulkan