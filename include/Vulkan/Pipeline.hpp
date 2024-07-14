#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <span>

namespace Vulkan
{

template<typename T>
inline vk::VertexInputBindingDescription CreateVertexInputBinding(
	std::uint8_t        BindingIndex,
	vk::VertexInputRate InputRate = vk::VertexInputRate::eVertex
)
{
	return vk::VertexInputBindingDescription{
		.binding   = BindingIndex,
		.stride    = sizeof(T),
		.inputRate = InputRate,
	};
}
} // namespace Vulkan