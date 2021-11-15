#include <Common/Alignment.hpp>
#include <Vulkan/Memory.hpp>

namespace Vulkan
{

std::int32_t FindMemoryTypeIndex(
	vk::PhysicalDevice PhysicalDevice, std::uint32_t MemoryTypeMask,
	vk::MemoryPropertyFlags MemoryProperties,
	vk::MemoryPropertyFlags MemoryExcludeProperties)
{
	const vk::PhysicalDeviceMemoryProperties DeviceMemoryProperties
		= PhysicalDevice.getMemoryProperties();
	// Iterate the physical device's memory types until we find a match
	for( std::size_t i = 0; i < DeviceMemoryProperties.memoryTypeCount; i++ )
	{
		if(
			// Is within memory type mask
			(((MemoryTypeMask >> i) & 0b1) == 0b1) &&
			// Has property flags
			(DeviceMemoryProperties.memoryTypes[i].propertyFlags
			 & MemoryProperties)
				== MemoryProperties
			&&
			// None of the excluded properties are enabled
			!(DeviceMemoryProperties.memoryTypes[i].propertyFlags
			  & MemoryExcludeProperties) )
		{
			return static_cast<std::uint32_t>(i);
		}
	}

	return -1;
}

std::tuple<vk::Result, vk::UniqueDeviceMemory> CommitImageHeap(
	vk::Device Device, vk::PhysicalDevice PhysicalDevice,
	const std::span<const vk::Image> Images,
	vk::MemoryPropertyFlags          MemoryProperties,
	vk::MemoryPropertyFlags          MemoryExcludeProperties)
{
	vk::DeviceSize                       ImageHeapMemorySize = 0;
	std::uint32_t                        ImageHeapMemoryMask = 0xFFFFFFFF;
	std::vector<vk::BindImageMemoryInfo> ImageHeapBinds;

	const vk::DeviceSize BufferImageGranularity
		= PhysicalDevice.getProperties().limits.bufferImageGranularity;

	for( const vk::Image& CurImage : Images )
	{
		const vk::MemoryRequirements MemReqs
			= Device.getImageMemoryRequirements(CurImage);

		// Accumulate a mask of all the memory types that satisfies each of the
		// handles
		ImageHeapMemoryMask &= MemReqs.memoryTypeBits;

		// Pad up the memory sizes so they are not considered aliasing
		const vk::DeviceSize MemorySize
			= Common::AlignUp(MemReqs.size, BufferImageGranularity);

		// Put nullptr for the device memory for now
		ImageHeapBinds.emplace_back(
			vk::BindImageMemoryInfo{CurImage, nullptr, ImageHeapMemorySize});
		ImageHeapMemorySize += MemorySize;
	}

	vk::MemoryAllocateInfo ImageHeapAllocInfo;
	ImageHeapAllocInfo.allocationSize  = ImageHeapMemorySize;
	ImageHeapAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(
		PhysicalDevice, ImageHeapMemoryMask, MemoryProperties,
		MemoryExcludeProperties);

	vk::UniqueDeviceMemory ImageHeapMemory = {};

	if( auto AllocResult = Device.allocateMemoryUnique(ImageHeapAllocInfo);
		AllocResult.result == vk::Result::eSuccess )
	{
		ImageHeapMemory = std::move(AllocResult.value);
	}
	else
	{
		return std::make_tuple(AllocResult.result, vk::UniqueDeviceMemory());
	}

	// Now we can assign the device memory to the images
	for( vk::BindImageMemoryInfo& CurBind : ImageHeapBinds )
	{
		CurBind.memory = ImageHeapMemory.get();
	}

	// Now bind them all in one go
	if( auto BindResult = Device.bindImageMemory2(ImageHeapBinds);
		BindResult == vk::Result::eSuccess )
	{
		// Binding memory succeeded
	}
	else
	{
		return std::make_tuple(BindResult, vk::UniqueDeviceMemory());
	}

	return std::make_tuple(vk::Result::eSuccess, std::move(ImageHeapMemory));
}

std::tuple<vk::Result, vk::UniqueDeviceMemory> CommitBufferHeap(
	vk::Device Device, vk::PhysicalDevice PhysicalDevice,
	const std::span<const vk::Buffer> Buffers,
	vk::MemoryPropertyFlags           MemoryProperties,
	vk::MemoryPropertyFlags           MemoryExcludeProperties)
{
	vk::DeviceSize                        BufferHeapMemorySize = 0;
	std::uint32_t                         BufferHeapMemoryMask = 0xFFFFFFFF;
	std::vector<vk::BindBufferMemoryInfo> BufferHeapBinds;

	const vk::DeviceSize BufferImageGranularity
		= PhysicalDevice.getProperties().limits.bufferImageGranularity;

	for( const vk::Buffer& CurBuffer : Buffers )
	{
		const vk::MemoryRequirements MemReqs
			= Device.getBufferMemoryRequirements(CurBuffer);

		// Accumulate a mask of all the memory types that satisfies each of the
		// handles
		BufferHeapMemoryMask &= MemReqs.memoryTypeBits;

		// Pad up the memory sizes so they are not considered aliasing
		const vk::DeviceSize MemorySize
			= Common::AlignUp(MemReqs.size, BufferImageGranularity);

		// Put nullptr for the device memory for now
		BufferHeapBinds.emplace_back(
			vk::BindBufferMemoryInfo{CurBuffer, nullptr, BufferHeapMemorySize});
		BufferHeapMemorySize += MemorySize;
	}

	vk::MemoryAllocateInfo BufferHeapAllocInfo;
	BufferHeapAllocInfo.allocationSize  = BufferHeapMemorySize;
	BufferHeapAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(
		PhysicalDevice, BufferHeapMemoryMask, MemoryProperties,
		MemoryExcludeProperties);

	vk::UniqueDeviceMemory BufferHeapMemory = {};

	if( auto AllocResult = Device.allocateMemoryUnique(BufferHeapAllocInfo);
		AllocResult.result == vk::Result::eSuccess )
	{
		BufferHeapMemory = std::move(AllocResult.value);
	}
	else
	{
		return std::make_tuple(AllocResult.result, vk::UniqueDeviceMemory());
	}

	// Now we can assign the device memory to the Buffers
	for( vk::BindBufferMemoryInfo& CurBind : BufferHeapBinds )
	{
		CurBind.memory = BufferHeapMemory.get();
	}

	// Now bind them all in one go
	if( auto BindResult = Device.bindBufferMemory2(BufferHeapBinds);
		BindResult == vk::Result::eSuccess )
	{
		// Binding memory succeeded
	}
	else
	{
		return std::make_tuple(BindResult, vk::UniqueDeviceMemory());
	}

	return std::make_tuple(vk::Result::eSuccess, std::move(BufferHeapMemory));
}

} // namespace Vulkan