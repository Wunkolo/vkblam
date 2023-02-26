#include <Vulkan/VulkanAPI.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

static const bool VulkanLoader = []() -> bool {
	static vk::DynamicLoader dl;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(
		dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")
	);
	return true;
}();

namespace Vulkan
{

} // namespace Vulkan