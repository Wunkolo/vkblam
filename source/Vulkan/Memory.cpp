#include <Common/Alignment.hpp>
#include <Vulkan/Memory.hpp>

namespace Vulkan
{

// Given a speculative heap-allocation, defined by its current size and
// memory-type bits, appends a memory requirements structure to it, updating
// both the size and the required memory-type-bits. Returns the offset within
// the heap for the current MemoryRequirements Todo: Sun Apr 23 13:28:25 PDT
// 2023 Rather than using a running-size of the heap, look at all of the memory
// requests and optimally create a packing for all of the offset and alignment
// requirements. Such as by satisfying all of the largest alignments first, and
// then the smallest, to reduce padding
static vk::DeviceSize CommitMemoryRequestToHeap(
	const vk::MemoryRequirements& CurMemoryRequirements,
	vk::DeviceSize& CurHeapEnd, std::uint32_t& CurMemoryTypeBits,
	vk::DeviceSize SizeAlignment
)
{
	// Accumulate a mask of all the memory types that satisfies each of the
	// handles
	CurMemoryTypeBits &= CurMemoryRequirements.memoryTypeBits;

	// Pad up the memory sizes so they are not considered aliasing
	const vk::DeviceSize CurMemoryOffset
		= Common::AlignUp(CurHeapEnd, CurMemoryRequirements.alignment);
	// Pad the size by the required size-alignment.
	// Intended for BufferImageGranularity
	const vk::DeviceSize CurMemorySize
		= Common::AlignUp(CurMemoryRequirements.size, SizeAlignment);

	CurHeapEnd = (CurMemoryOffset + CurMemorySize);
	return CurMemoryOffset;
}

std::int32_t FindMemoryTypeIndex(
	vk::PhysicalDevice PhysicalDevice, std::uint32_t MemoryTypeMask,
	vk::MemoryPropertyFlags MemoryProperties,
	vk::MemoryPropertyFlags MemoryExcludeProperties
)
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

std::int32_t FindMemoryTypeIndex(
	vk::PhysicalDevice PhysicalDevice, std::uint32_t MemoryTypeMask,
	std::span<const vk::MemoryPropertyFlags> MemoryProperties,
	vk::MemoryPropertyFlags                  MemoryExcludeProperties
)
{
	std::int32_t MemoryHeapIndex = -1;

	for( const auto& MemoryProperty : MemoryProperties )
	{
		MemoryHeapIndex = FindMemoryTypeIndex(
			PhysicalDevice, MemoryTypeMask, MemoryProperty,
			MemoryExcludeProperties
		);
	}

	return MemoryHeapIndex;
}

std::tuple<vk::Result, vk::UniqueDeviceMemory> CommitImageHeap(
	vk::Device Device, vk::PhysicalDevice PhysicalDevice,
	const std::span<const vk::Image> Images,
	vk::MemoryPropertyFlags          MemoryProperties,
	vk::MemoryPropertyFlags          MemoryExcludeProperties
)
{
	vk::MemoryAllocateInfo               ImageHeapAllocInfo      = {};
	std::uint32_t                        ImageHeapMemoryTypeBits = 0xFFFFFFFF;
	std::vector<vk::BindImageMemoryInfo> ImageHeapBinds;

	const vk::DeviceSize BufferImageGranularity
		= PhysicalDevice.getProperties().limits.bufferImageGranularity;

	for( const vk::Image& CurImage : Images )
	{
		const vk::DeviceSize CurBindOffset = CommitMemoryRequestToHeap(
			Device.getImageMemoryRequirements(CurImage),
			ImageHeapAllocInfo.allocationSize, ImageHeapMemoryTypeBits,
			BufferImageGranularity
		);

		if( ImageHeapMemoryTypeBits == 0 )
		{
			// No possible memory heap for all of the images to share
			return std::make_tuple(
				vk::Result::eErrorOutOfDeviceMemory, vk::UniqueDeviceMemory()
			);
		}

		// Put nullptr for the device memory for now
		ImageHeapBinds.emplace_back(vk::BindImageMemoryInfo{
			.image        = CurImage,
			.memory       = nullptr,
			.memoryOffset = CurBindOffset,
		});
	}

	const std::int32_t MemoryTypeIndex = FindMemoryTypeIndex(
		PhysicalDevice, ImageHeapMemoryTypeBits, MemoryProperties,
		MemoryExcludeProperties
	);

	if( MemoryTypeIndex < 0 )
	{
		// Unable to find a memory heap that satisfies all the images
		return std::make_tuple(
			vk::Result::eErrorOutOfDeviceMemory, vk::UniqueDeviceMemory()
		);
	}

	ImageHeapAllocInfo.memoryTypeIndex = MemoryTypeIndex;

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

	// Assign the device memory to the bindings
	for( vk::BindImageMemoryInfo& CurBind : ImageHeapBinds )
	{
		CurBind.memory = ImageHeapMemory.get();
	}

	// Now bind them all in one call
	if( const vk::Result BindResult = Device.bindImageMemory2(ImageHeapBinds);
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
	vk::MemoryPropertyFlags           MemoryExcludeProperties
)
{
	vk::MemoryAllocateInfo                BufferHeapAllocInfo      = {};
	std::uint32_t                         BufferHeapMemoryTypeBits = 0xFFFFFFFF;
	std::vector<vk::BindBufferMemoryInfo> BufferHeapBinds;

	const vk::DeviceSize BufferImageGranularity
		= PhysicalDevice.getProperties().limits.bufferImageGranularity;

	for( const vk::Buffer& CurBuffer : Buffers )
	{
		const vk::DeviceSize CurBindOffset = CommitMemoryRequestToHeap(
			Device.getBufferMemoryRequirements(CurBuffer),
			BufferHeapAllocInfo.allocationSize, BufferHeapMemoryTypeBits,
			BufferImageGranularity
		);

		if( BufferHeapMemoryTypeBits == 0 )
		{
			// No possible memory heap for all of the buffers to share
			return std::make_tuple(
				vk::Result::eErrorOutOfDeviceMemory, vk::UniqueDeviceMemory()
			);
		}

		// Put nullptr for the device memory for now
		BufferHeapBinds.emplace_back(vk::BindBufferMemoryInfo{
			.buffer       = CurBuffer,
			.memory       = nullptr,
			.memoryOffset = CurBindOffset,
		});
	}

	const std::int32_t MemoryTypeIndex = FindMemoryTypeIndex(
		PhysicalDevice, BufferHeapMemoryTypeBits, MemoryProperties,
		MemoryExcludeProperties
	);

	if( MemoryTypeIndex < 0 )
	{
		// Unable to find a memory heap that satisfies all the buffers
		return std::make_tuple(
			vk::Result::eErrorOutOfDeviceMemory, vk::UniqueDeviceMemory()
		);
	}

	BufferHeapAllocInfo.memoryTypeIndex = MemoryTypeIndex;

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

	// Assign the device memory to the bindings
	for( vk::BindBufferMemoryInfo& CurBind : BufferHeapBinds )
	{
		CurBind.memory = BufferHeapMemory.get();
	}

	// Now bind them all in one call
	if( const vk::Result BindResult = Device.bindBufferMemory2(BufferHeapBinds);
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