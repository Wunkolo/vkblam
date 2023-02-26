#include <VkBlam/Shader.hpp>

namespace VkBlam
{
Shader::Shader(
	const Vulkan::Context& VulkanContext, const BitmapHeapT& BitmapHeap,
	Vulkan::DescriptorUpdateBatch& DescriptorUpdateBatch
)
	: VulkanContext(VulkanContext), BitmapHeap(BitmapHeap),
	  DescriptorUpdateBatch(DescriptorUpdateBatch)
{
}
} // namespace VkBlam