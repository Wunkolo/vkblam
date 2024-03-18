#include <Vulkan/ShaderModuleCache.hpp>

#include <vulkan/vulkan_hash.hpp>

namespace Vulkan
{

ShaderModuleCache::ShaderModuleCache(const Vulkan::Context& VulkanContext)
	: VulkanContext(VulkanContext)
{
}

std::optional<const vk::ShaderModule> ShaderModuleCache::GetShaderModule(
	std::size_t Hash, std::span<const std::byte> ShaderCode
)
{
	// Cache hit
	if( ShaderModuleMap.contains(Hash) )
	{
		return {ShaderModuleMap.at(Hash).get()};
	}

	const vk::ShaderModuleCreateInfo ShaderModuleInfo = {
		.codeSize = ShaderCode.size_bytes(),
		.pCode    = reinterpret_cast<const std::uint32_t*>(ShaderCode.data()),
	};

	if( auto CreateResult
		= VulkanContext.LogicalDevice.createShaderModuleUnique(ShaderModuleInfo
		);
		CreateResult.result == vk::Result::eSuccess )
	{
		const auto Iterator
			= ShaderModuleMap.insert({Hash, std::move(CreateResult.value)});

		return {Iterator.first->second.get()};
	}
	else
	{
		// Todo: std::expected
		return {std::nullopt};
	}
}

std::optional<ShaderModuleCache>
	ShaderModuleCache::Create(const Vulkan::Context& VulkanContext)
{
	ShaderModuleCache NewShaderModuleCache(VulkanContext);

	return {std::move(NewShaderModuleCache)};
}
} // namespace Vulkan