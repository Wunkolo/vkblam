#include <Vulkan/SamplerCache.hpp>

#include <vulkan/vulkan_hash.hpp>

namespace Vulkan
{

SamplerCache::SamplerCache(const Vulkan::Context& VulkanContext)
	: VulkanContext(VulkanContext)
{
}

const vk::Sampler&
	SamplerCache::GetSampler(const vk::SamplerCreateInfo& SamplerInfo)
{
	const std::size_t SamplerHash
		= std::hash<vk::SamplerCreateInfo>()(SamplerInfo);

	// Cache hit
	if( SamplerMap.contains(SamplerHash) )
	{
		return SamplerMap.at(SamplerHash).get();
	}

	auto CreateResult
		= VulkanContext.LogicalDevice.createSamplerUnique(SamplerInfo);

	// TODO: Handle creation issues. std::optional?
	// CreateResult.result == vk::Result::eSuccess;

	return (SamplerMap[SamplerHash] = std::move(CreateResult.value)).get();
}

std::optional<SamplerCache>
	SamplerCache::Create(const Vulkan::Context& VulkanContext)
{
	SamplerCache NewSamplerCache(VulkanContext);

	return {std::move(NewSamplerCache)};
}
} // namespace Vulkan