#include <Vulkan/StreamBuffer.hpp>

#include "Common/Alignment.hpp"
#include "Common/Format.hpp"
#include <Vulkan/Debug.hpp>
#include <Vulkan/Memory.hpp>
#include <vulkan/vulkan_enums.hpp>

namespace Vulkan
{
StreamBuffer::StreamBuffer(
	vk::Device Device, vk::PhysicalDevice PhysicalDevice,
	vk::Queue TransferQueue, std::uint8_t TransferQueueFamilyIndex,
	vk::DeviceSize BufferSize)
	: Device(Device), PhysicalDevice(PhysicalDevice),
	  TransferQueue(TransferQueue),
	  TransferQueueFamilyIndex(TransferQueueFamilyIndex),
	  BufferSize(BufferSize), FlushTick(0), RingOffset(0)
{
	//// Create Semaphore
	{
		vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo>
			FlushSemaphoreInfoChain = {};

		auto& FlushSemaphoreInfo
			= FlushSemaphoreInfoChain.get<vk::SemaphoreCreateInfo>();

		FlushSemaphoreInfo.flags = {};

		auto& FlushSemaphoreTypeInfo
			= FlushSemaphoreInfoChain.get<vk::SemaphoreTypeCreateInfo>();

		FlushSemaphoreTypeInfo.initialValue  = 0;
		FlushSemaphoreTypeInfo.semaphoreType = vk::SemaphoreType::eTimeline;

		if( auto CreateResult
			= Device.createSemaphoreUnique(FlushSemaphoreInfoChain.get());
			CreateResult.result == vk::Result::eSuccess )
		{
			FlushSemaphore = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating vertex buffer: %s\n",
				vk::to_string(CreateResult.result).c_str());
			/// ??? should we exit the program
		}
		Vulkan::SetObjectName(
			Device, FlushSemaphore.get(), "Staging Ring Buffer Semaphore");
	}

	//// Create buffer
	{
		vk::BufferCreateInfo RingBufferInfo;
		RingBufferInfo.size  = BufferSize;
		RingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc
							 | vk::BufferUsageFlagBits::eTransferDst;

		if( auto CreateResult = Device.createBufferUnique(RingBufferInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			RingBuffer = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating vertex buffer: %s\n",
				vk::to_string(CreateResult.result).c_str());
			/// ??? should we exit the program
		}
		Vulkan::SetObjectName(
			Device, RingBuffer.get(), "Staging Ring Buffer( %s )",
			Common::FormatByteCount(BufferSize).c_str());
	}

	//// Allocate memory for staging ring buffer
	{
		const vk::MemoryRequirements RingBufferMemoryRequirements
			= Device.getBufferMemoryRequirements(RingBuffer.get());

		vk::MemoryAllocateInfo RingBufferAllocInfo = {};
		RingBufferAllocInfo.allocationSize = RingBufferMemoryRequirements.size;

		// Try to get some shared memory
		std::int32_t RingBufferHeapIndex = Vulkan::FindMemoryTypeIndex(
			PhysicalDevice, RingBufferMemoryRequirements.memoryTypeBits,
			vk::MemoryPropertyFlagBits::eHostVisible
				| vk::MemoryPropertyFlagBits::eHostCoherent
				| vk::MemoryPropertyFlagBits::eDeviceLocal);

		// If that failed, then just get some host memory
		if( RingBufferHeapIndex < 0 )
		{
			RingBufferHeapIndex = Vulkan::FindMemoryTypeIndex(
				PhysicalDevice, RingBufferMemoryRequirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eHostVisible
					| vk::MemoryPropertyFlagBits::eHostCoherent);
		}

		RingBufferAllocInfo.memoryTypeIndex = RingBufferHeapIndex;

		if( auto AllocResult = Device.allocateMemoryUnique(RingBufferAllocInfo);
			AllocResult.result == vk::Result::eSuccess )
		{
			RingBufferMemory = std::move(AllocResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error allocating memory for staging buffer: %s\n",
				vk::to_string(AllocResult.result).c_str());
			/// ??? should we exit the program
		}
		Vulkan::SetObjectName(
			Device, RingBufferMemory.get(), "Staging Ring Buffer Memory( %s )",
			Common::FormatByteCount(BufferSize).c_str());

		if( auto BindResult = Device.bindBufferMemory(
				RingBuffer.get(), RingBufferMemory.get(), 0);
			BindResult == vk::Result::eSuccess )
		{
			// Successfully binded memory to buffer
		}
		else
		{
			std::fprintf(
				stderr, "Error binding memory to staging ring buffer: %s\n",
				vk::to_string(BindResult).c_str());
			/// ??? should we exit the program
		}
	}

	//// Map the device memory
	if( auto MapResult
		= Device.mapMemory(RingBufferMemory.get(), 0, BufferSize);
		MapResult.result == vk::Result::eSuccess )
	{
		RingMemoryMapped = std::span<std::byte>(
			reinterpret_cast<std::byte*>(MapResult.value), BufferSize);
	}
	else
	{
		std::fprintf(
			stderr, "Error mapping staging ring buffer memory: %s\n",
			vk::to_string(MapResult.result).c_str());
		/// ??? should we exit the program
	}

	//// Allocate command pool
	{
		vk::CommandPoolCreateInfo CommandPoolInfo;
		CommandPoolInfo.flags
			= vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		CommandPoolInfo.queueFamilyIndex = TransferQueueFamilyIndex;

		if( auto CreateResult = Device.createCommandPoolUnique(CommandPoolInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			CommandPool = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating staging buffer command pool: %s\n",
				vk::to_string(CreateResult.result).c_str());
			/// ??? should we exit the program
		}
	}
}

StreamBuffer::~StreamBuffer()
{
	Device.unmapMemory(RingBufferMemory.get());
}

std::uint64_t StreamBuffer::QueueBufferUpload(
	const std::span<const std::byte> Data, vk::Buffer Buffer,
	vk::DeviceSize Offset)
{
	if( Data.empty() )
	{
		return FlushTick;
	}
	if( Data.size_bytes() > BufferSize )
	{
		std::fprintf(
			stderr, "Staging buffer overflow: %zu > %zu \n", Data.size_bytes(),
			BufferSize);
	}

	// Satisfy any alignment requirements here
	std::uint64_t CurRingOffset = RingOffset;

	if( (CurRingOffset + Data.size_bytes()) >= BufferSize )
	{
		const std::uint64_t FlushTick = Flush();

		// Blocking wait since we need to ensure that the staging buffer is
		// entirely free todo, attach timestamps to particular regions of the
		// ring buffer so that we can use parts of the buffer immediately when
		// it is ready
		vk::SemaphoreWaitInfo WaitInfo;
		WaitInfo.semaphoreCount = 1;
		WaitInfo.pSemaphores    = &GetSemaphore();
		WaitInfo.pValues        = &FlushTick;
		if( auto WaitResult = Device.waitSemaphores(WaitInfo, ~0ULL);
			WaitResult != vk::Result::eSuccess )
		{
			std::fprintf(stderr, "Error waiting on Stream buffer semaphore \n");
		}

		// Satisfy any alignment requirements here
		CurRingOffset = RingOffset;
	}

	RingOffset = CurRingOffset + Data.size_bytes();

	std::copy(
		Data.begin(), Data.end(),
		RingMemoryMapped.subspan(CurRingOffset).begin());

	BufferCopies[Buffer].emplace_back(
		vk::BufferCopy{CurRingOffset, Offset, Data.size_bytes()});

	return FlushTick;
}

std::uint64_t StreamBuffer::QueueImageUpload(
	const std::span<const std::byte> Data, vk::Image Image, vk::Offset3D Offset,
	vk::Extent3D Extent, vk::ImageSubresourceLayers SubresourceLayers,
	vk::ImageLayout DstLayout)
{
	if( Data.empty() )
	{
		return FlushTick;
	}
	if( Data.size_bytes() > BufferSize )
	{
		std::fprintf(
			stderr, "Staging buffer overflow: %zu > %zu \n", Data.size_bytes(),
			BufferSize);
	}

	// Memory offset must at least match the alignment of the image format's
	// texel size. Here we just force it to handle the upper-bound alignment
	// as a temporary catch-all
	std::uint64_t CurRingOffset = Common::AlignUp(RingOffset, 16);

	if( (CurRingOffset + Data.size_bytes()) >= BufferSize )
	{
		const std::uint64_t FlushTick = Flush();

		// Blocking wait since we need to ensure that the staging buffer is
		// entirely free todo, attach timestamps to particular regions of the
		// ring buffer so that we can use parts of the buffer immediately when
		// it is ready
		vk::SemaphoreWaitInfo WaitInfo;
		WaitInfo.semaphoreCount = 1;
		WaitInfo.pSemaphores    = &GetSemaphore();
		WaitInfo.pValues        = &FlushTick;
		if( Device.waitSemaphores(WaitInfo, ~0ULL) != vk::Result::eSuccess )
		{
			std::fprintf(stderr, "Error waiting on Stream buffer semaphore \n");
		}

		CurRingOffset = Common::AlignUp(RingOffset, 16);
	}

	RingOffset = CurRingOffset + Data.size_bytes();

	std::copy(
		Data.begin(), Data.end(),
		RingMemoryMapped.subspan(CurRingOffset).begin());

	ImageCopies[Image].emplace_back(vk::BufferImageCopy{
		CurRingOffset, 0, 0, SubresourceLayers, Offset, Extent});

	ImagePreBarrier.emplace_back(vk::ImageMemoryBarrier(
		vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eMemoryRead,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, Image,
		vk::ImageSubresourceRange(
			SubresourceLayers.aspectMask, SubresourceLayers.mipLevel, 1, 0,
			SubresourceLayers.layerCount)));
	ImagePostBarrier.emplace_back(vk::ImageMemoryBarrier(
		vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal, VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED, Image,
		vk::ImageSubresourceRange(
			SubresourceLayers.aspectMask, SubresourceLayers.mipLevel, 1, 0,
			SubresourceLayers.layerCount)));

	return FlushTick;
}

std::uint64_t StreamBuffer::Flush()
{
	if( RingOffset == 0 )
	{
		return FlushTick;
	}
	// Any further pushes are going to be a part of the next tick
	const std::uint64_t PrevFlushTick      = FlushTick++;
	vk::CommandBuffer   FlushCommandBuffer = {};

	// Get where the GPU is at in our submit-timeline
	std::uint64_t GpuFlushTick = 0;
	if( auto GetResult = Device.getSemaphoreCounterValue(FlushSemaphore.get());
		GetResult.result == vk::Result::eSuccess )
	{
		GpuFlushTick = GetResult.value;
	}
	else
	{
		std::fprintf(
			stderr, "Error getting timeline semaphore value: %s\n",
			vk::to_string(GetResult.result).c_str());
		/// ??? should we exit the program
	}

	// Find a free command buffer
	for( std::size_t i = 0; i < CommandBuffers.size(); ++i )
	{
		// This command context is free! recycle it
		if( CommandBufferTimeStamps[i] < GpuFlushTick )
		{
			CommandBufferTimeStamps[i] = FlushTick;
			FlushCommandBuffer         = CommandBuffers[i].get();
			break;
		}
	}

	if( !FlushCommandBuffer )
	{
		// No command buffer was free, we need to push a new one
		CommandBufferTimeStamps.push_back(FlushTick);

		vk::CommandBufferAllocateInfo CommandBufferInfo;
		CommandBufferInfo.commandPool        = CommandPool.get();
		CommandBufferInfo.commandBufferCount = 1;

		if( auto AllocateResult
			= Device.allocateCommandBuffersUnique(CommandBufferInfo);
			AllocateResult.result == vk::Result::eSuccess )
		{
			FlushCommandBuffer = AllocateResult.value[0].get();
			CommandBuffers.emplace_back(std::move(AllocateResult.value[0]));
		}
		else
		{
			std::fprintf(
				stderr, "Error allocating command buffer: %s\n",
				vk::to_string(AllocateResult.result).c_str());
			/// ??? should we exit the program
		}
	}

	vk::CommandBufferBeginInfo BeginInfo;
	BeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	if( auto BeginResult
		= FlushCommandBuffer.begin(vk::CommandBufferBeginInfo{});
		BeginResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error beginning command buffer: %s\n",
			vk::to_string(BeginResult).c_str());
	}

	{
		Vulkan::DebugLabelScope DebugCopyScope(
			FlushCommandBuffer, {1.0, 1.0, 0.0, 1.0}, "Upload Buffers");
		for( const auto& [CurBuffer, CurBufferCopies] : BufferCopies )
		{
			FlushCommandBuffer.copyBuffer(
				RingBuffer.get(), CurBuffer, CurBufferCopies);
		}
	}

	{
		Vulkan::DebugLabelScope DebugCopyScope(
			FlushCommandBuffer, {1.0, 1.0, 0.0, 1.0}, "Upload Images");
		FlushCommandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags{}, {}, {},
			ImagePreBarrier);
		for( const auto& [CurImage, CurImageCopies] : ImageCopies )
		{
			FlushCommandBuffer.copyBufferToImage(
				RingBuffer.get(), CurImage,
				vk::ImageLayout::eTransferDstOptimal, CurImageCopies);
		}
		FlushCommandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags{}, {},
			{}, ImagePostBarrier);
	}

	if( auto EndResult = FlushCommandBuffer.end();
		EndResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error ending command buffer: %s\n",
			vk::to_string(EndResult).c_str());
	}

	vk::StructureChain<vk::SubmitInfo, vk::TimelineSemaphoreSubmitInfo>
		SubmitInfoChain = {};

	auto& SubmitInfo              = SubmitInfoChain.get<vk::SubmitInfo>();
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers    = &FlushCommandBuffer;

	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores    = &FlushSemaphore.get();

	static const vk::PipelineStageFlags WaitStage
		= vk::PipelineStageFlagBits::eTransfer;
	SubmitInfo.pWaitDstStageMask = &WaitStage;

	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.pSignalSemaphores    = &FlushSemaphore.get();

	auto& SubmitTimelineInfo
		= SubmitInfoChain.get<vk::TimelineSemaphoreSubmitInfo>();

	SubmitTimelineInfo.waitSemaphoreValueCount = 1;
	SubmitTimelineInfo.pWaitSemaphoreValues    = &PrevFlushTick;

	SubmitTimelineInfo.signalSemaphoreValueCount = 1;
	SubmitTimelineInfo.pSignalSemaphoreValues    = &FlushTick;

	if( auto SubmitResult = TransferQueue.submit(SubmitInfoChain.get());
		SubmitResult != vk::Result::eSuccess )
	{
		// Error submitting
		std::fprintf(
			stderr, "Error submitting streaming buffer flush: %s\n",
			vk::to_string(SubmitResult).c_str());
	}

	RingOffset = 0;
	BufferCopies.clear();
	ImageCopies.clear();
	ImagePreBarrier.clear();
	ImagePostBarrier.clear();

	return FlushTick;
}

const vk::Semaphore& StreamBuffer::GetSemaphore() const
{
	return FlushSemaphore.get();
}

} // namespace Vulkan