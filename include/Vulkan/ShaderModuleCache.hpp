#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <optional>
#include <span>
#include <unordered_map>

namespace Vulkan
{

// Implements a simple pool of reusable shader module objects
class ShaderModuleCache
{
private:
	const Vulkan::Context& VulkanContext;

	std::unordered_map<std::size_t, vk::UniqueShaderModule> ShaderModuleMap;

	explicit ShaderModuleCache(const Vulkan::Context& VulkanContext);

public:
	~ShaderModuleCache() = default;

	ShaderModuleCache(ShaderModuleCache&&) = default;

	std::optional<const vk::ShaderModule> GetShaderModule(
		std::size_t Hash, std::span<const std::byte> ShaderCode
	);

	static std::optional<ShaderModuleCache>
		Create(const Vulkan::Context& VulkanContext);
};
} // namespace Vulkan