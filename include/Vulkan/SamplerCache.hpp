#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <optional>
#include <unordered_map>

namespace Vulkan
{

// Implements a simple pool of reusable sampler objects
class SamplerCache
{
private:
	const Vulkan::Context& VulkanContext;

	std::unordered_map<std::size_t, vk::UniqueSampler> SamplerMap;

	explicit SamplerCache(const Vulkan::Context& VulkanContext);

public:
	~SamplerCache() = default;

	SamplerCache(SamplerCache&&) = default;

	const vk::Sampler& GetSampler(const vk::SamplerCreateInfo& SamplerInfo);

	static std::optional<SamplerCache> Create(Vulkan::Context VulkanContext);
};
} // namespace Vulkan