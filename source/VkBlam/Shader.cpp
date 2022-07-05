#include <VkBlam/Shader.hpp>

namespace VkBlam
{
Shader::Shader(
	vk::Device LogicalDevice, const BitmapHeapT& BitmapHeap,
	Vulkan::DescriptorUpdateBatch& DescriptorUpdateBatch)
	: LogicalDevice(LogicalDevice), BitmapHeap(BitmapHeap),
	  DescriptorUpdateBatch(DescriptorUpdateBatch)
{
}
} // namespace VkBlam