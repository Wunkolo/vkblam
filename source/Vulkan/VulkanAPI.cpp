#include <Vulkan/VulkanAPI.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

static const bool VulkanLoader = []() -> bool {

// Remove this once the Lunarg SDK releases beyond this version number.
// Currently(12/19/2024) Lunarg is at 296.
// https://github.com/KhronosGroup/Vulkan-Hpp/pull/1983
#if VK_HEADER_VERSION >= 303
	static vk::detail::DynamicLoader DynamicLoader;
#else
	static vk::DynamicLoader DynamicLoader;
#endif

	VULKAN_HPP_DEFAULT_DISPATCHER.init(
		DynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>(
			"vkGetInstanceProcAddr"
		)
	);
	return true;
}();

namespace Vulkan
{

} // namespace Vulkan