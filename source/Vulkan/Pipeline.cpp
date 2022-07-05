
#include <Vulkan/Pipeline.hpp>

namespace Vulkan
{
vk::UniqueShaderModule CreateShaderModule(
	vk::Device LogicalDevice, std::span<const std::uint32_t> ShaderCode)
{
	vk::ShaderModuleCreateInfo ShaderInfo{};
	ShaderInfo.pCode    = ShaderCode.data();
	ShaderInfo.codeSize = ShaderCode.size_bytes();

	vk::UniqueShaderModule ShaderModule{};
	if( auto CreateResult = LogicalDevice.createShaderModuleUnique(ShaderInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		ShaderModule = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Failed to create shader module: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return {};
	}
	return ShaderModule;
}
} // namespace Vulkan