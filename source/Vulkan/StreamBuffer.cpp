#include <Vulkan/StreamBuffer.hpp>

#include "Common/Alignment.hpp"
#include "Common/Format.hpp"
#include <Vulkan/Debug.hpp>
#include <Vulkan/Memory.hpp>
#include <vulkan/vulkan_enums.hpp>

namespace Vulkan
{
StreamBuffer::StreamBuffer(
	const Vulkan::Context& VulkanContext, vk::DeviceSize BufferSize
)
	: VulkanContext(VulkanContext), BufferSize(BufferSize), FlushTick(0),
	  RingOffset(0)
{
	//// Create Semaphore
	{
		const vk::StructureChain<
			vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo>
			FlushSemaphoreInfoChain = {
				vk::SemaphoreCreateInfo{},
				vk::SemaphoreTypeCreateInfo{
					.semaphoreType = vk::SemaphoreType::eTimeline,
					.initialValue  = 0,
				},
			};

		if( auto CreateResult
			= VulkanContext.LogicalDevice.createSemaphoreUnique(
				FlushSemaphoreInfoChain.get()
			);
			CreateResult.result == vk::Result::eSuccess )
		{
			FlushSemaphore = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating vertex buffer: %s\n",
				vk::to_string(CreateResult.result).c_str()
			);
			/// ??? should we exit the program
		}
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, FlushSemaphore.get(),
			"StreamBuffer: Flush Semaphore"
		);
	} // namespace Vulkan

	//// Create buffer
	{
		const vk::BufferCreateInfo RingBufferInfo = {
			.size  = BufferSize,
			.usage = vk::BufferUsageFlagBits::eTransferSrc
				   | vk::BufferUsageFlagBits::eTransferDst,
		};

		if( auto CreateResult
			= VulkanContext.LogicalDevice.createBufferUnique(RingBufferInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			RingBuffer = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating vertex buffer: %s\n",
				vk::to_string(CreateResult.result).c_str()
			);
			/// ??? should we exit the program
		}
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, RingBuffer.get(),
			"StreamBuffer: Ring Buffer( {} )",
			Common::FormatByteCount(BufferSize)
		);
	}

	//// Allocate memory for staging ring buffer
	{
		const vk::MemoryRequirements RingBufferMemoryRequirements
			= VulkanContext.LogicalDevice.getBufferMemoryRequirements(
				RingBuffer.get()
			);

		// Stream-bufer memory types ordered from best to worst
		static const vk::MemoryPropertyFlags HostMemoryTypes[] = {
			// DeviceLocal memory
			vk::MemoryPropertyFlagBits::eHostVisible
				| vk::MemoryPropertyFlagBits::eHostCoherent
				| vk::MemoryPropertyFlagBits::eDeviceLocal,
			// DeviceLocal memory
			// vk{Flush,Invalidate}MappedMemoryRanges must be used
			vk::MemoryPropertyFlagBits::eHostVisible
				| vk::MemoryPropertyFlagBits::eDeviceLocal,
			// Host memory
			vk::MemoryPropertyFlagBits::eHostCoherent
				| vk::MemoryPropertyFlagBits::eHostVisible,
			// Host memory
			// vk{Flush,Invalidate}MappedMemoryRanges must be used
			vk::MemoryPropertyFlagBits::eHostVisible,
		};

		// Try to get some shared memory
		const std::int32_t RingBufferHeapIndex = Vulkan::FindMemoryTypeIndex(
			VulkanContext.PhysicalDevice,
			RingBufferMemoryRequirements.memoryTypeBits, HostMemoryTypes
		);

		if( RingBufferHeapIndex < 0 )
		{
			std::fprintf(
				stderr, "Unable to find candidate streambuffer memory heap\n"
			);
		}

		const vk::MemoryAllocateInfo RingBufferAllocInfo = {
			.allocationSize  = RingBufferMemoryRequirements.size,
			.memoryTypeIndex = static_cast<std::uint32_t>(RingBufferHeapIndex),
		};

		if( auto AllocResult = VulkanContext.LogicalDevice.allocateMemoryUnique(
				RingBufferAllocInfo
			);
			AllocResult.result == vk::Result::eSuccess )
		{
			RingBufferMemory = std::move(AllocResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error allocating memory for staging buffer: %s\n",
				vk::to_string(AllocResult.result).c_str()
			);
			/// ??? should we exit the program
		}
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, RingBufferMemory.get(),
			"StreamBuffer: Ring Buffer Memory( {} )",
			Common::FormatByteCount(BufferSize)
		);

		if( auto BindResult = VulkanContext.LogicalDevice.bindBufferMemory(
				RingBuffer.get(), RingBufferMemory.get(), 0
			);
			BindResult == vk::Result::eSuccess )
		{
			// Successfully binded memory to buffer
		}
		else
		{
			std::fprintf(
				stderr, "Error binding memory to staging ring buffer: %s\n",
				vk::to_string(BindResult).c_str()
			);
			/// ??? should we exit the program
		}
	}

	//// Map the device memory
	if( auto MapResult = VulkanContext.LogicalDevice.mapMemory(
			RingBufferMemory.get(), 0, BufferSize
		);
		MapResult.result == vk::Result::eSuccess )
	{
		RingMemoryMapped = std::span<std::byte>(
			reinterpret_cast<std::byte*>(MapResult.value), BufferSize
		);
	}
	else
	{
		std::fprintf(
			stderr, "Error mapping staging ring buffer memory: %s\n",
			vk::to_string(MapResult.result).c_str()
		);
		/// ??? should we exit the program
	}

	//// Allocate command pool
	{
		const vk::CommandPoolCreateInfo CommandPoolInfo = {
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = VulkanContext.TransferQueueFamilyIndex,
		};

		if( auto CreateResult = VulkanContext.LogicalDevice
									.createCommandPoolUnique(CommandPoolInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			CommandPool = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating staging buffer command pool: %s\n",
				vk::to_string(CreateResult.result).c_str()
			);
			/// ??? should we exit the program
		}

		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, CommandPool.get(),
			"StreamBuffer: Command Pool"
		);
	}
}

StreamBuffer::~StreamBuffer()
{
	VulkanContext.LogicalDevice.unmapMemory(RingBufferMemory.get());
}

std::uint64_t StreamBuffer::QueueBufferUpload(
	const std::span<const std::byte> Data, vk::Buffer Buffer,
	vk::DeviceSize Offset
)
{
	std::scoped_lock StreamBufferLock{StreamBufferMutex};

	if( Data.empty() )
	{
		return FlushTick;
	}
	if( Data.size_bytes() > BufferSize )
	{
		std::fprintf(
			stderr, "Staging buffer overflow: %zu > %zu \n", Data.size_bytes(),
			BufferSize
		);
	}

	// Satisfy any alignment requirements here
	std::uint64_t CurRingOffset = RingOffset;

	if( (CurRingOffset + Data.size_bytes()) >= BufferSize )
	{
		const std::uint64_t CurFlushTick = Flush();

		// Blocking wait since we need to ensure that the staging buffer is
		// entirely free todo, attach timestamps to particular regions of the
		// ring buffer so that we can use parts of the buffer immediately when
		// it is ready
		const vk::SemaphoreWaitInfo WaitInfo = {
			.semaphoreCount = 1,
			.pSemaphores    = &GetSemaphore(),
			.pValues        = &CurFlushTick,
		};

		if( auto WaitResult
			= VulkanContext.LogicalDevice.waitSemaphores(WaitInfo, ~0ULL);
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
		RingMemoryMapped.subspan(CurRingOffset).begin()
	);

	BufferCopies[Buffer].emplace_back(vk::BufferCopy{
		.srcOffset = CurRingOffset,
		.dstOffset = Offset,
		.size      = Data.size_bytes(),
	});

	return FlushTick;
}

std::uint64_t StreamBuffer::QueueImageUpload(
	const std::span<const std::byte> Data, vk::Image Image, vk::Offset3D Offset,
	vk::Extent3D Extent, vk::ImageSubresourceLayers SubresourceLayers,
	vk::ImageLayout DstLayout
)
{
	std::scoped_lock StreamBufferLock{StreamBufferMutex};

	if( Data.empty() )
	{
		return FlushTick;
	}
	if( Data.size_bytes() > BufferSize )
	{
		std::fprintf(
			stderr, "Staging buffer overflow: %zu > %zu \n", Data.size_bytes(),
			BufferSize
		);
	}

	// Memory offset must at least match the alignment of the image format's
	// texel size. Here we just force it to handle the upper-bound alignment
	// as a temporary catch-all
	std::uint64_t CurRingOffset = Common::AlignUp(RingOffset, 16);

	if( (CurRingOffset + Data.size_bytes()) >= BufferSize )
	{
		const std::uint64_t CurFlushTick = Flush();

		// Blocking wait since we need to ensure that the staging buffer is
		// entirely free todo, attach timestamps to particular regions of the
		// ring buffer so that we can use parts of the buffer immediately when
		// it is ready
		const vk::SemaphoreWaitInfo WaitInfo = {
			.semaphoreCount = 1,
			.pSemaphores    = &GetSemaphore(),
			.pValues        = &CurFlushTick,
		};

		if( VulkanContext.LogicalDevice.waitSemaphores(WaitInfo, ~0ULL)
			!= vk::Result::eSuccess )
		{
			std::fprintf(stderr, "Error waiting on Stream buffer semaphore \n");
		}

		CurRingOffset = Common::AlignUp(RingOffset, 16);
	}

	RingOffset = CurRingOffset + Data.size_bytes();

	std::copy(
		Data.begin(), Data.end(),
		RingMemoryMapped.subspan(CurRingOffset).begin()
	);

	ImageCopies[Image].emplace_back(vk::BufferImageCopy{
		.bufferOffset      = CurRingOffset,
		.bufferRowLength   = 0,
		.bufferImageHeight = 0,
		.imageSubresource  = SubresourceLayers,
		.imageOffset       = Offset,
		.imageExtent       = Extent,
	});

	const vk::ImageSubresourceRange SubresourceRange = {
		.aspectMask     = SubresourceLayers.aspectMask,
		.baseMipLevel   = SubresourceLayers.mipLevel,
		.levelCount     = 1,
		.baseArrayLayer = SubresourceLayers.baseArrayLayer,
		.layerCount     = SubresourceLayers.layerCount,
	};

	ImagePreBarrier.emplace_back(vk::ImageMemoryBarrier{
		.srcAccessMask       = vk::AccessFlagBits::eMemoryWrite,
		.dstAccessMask       = vk::AccessFlagBits::eMemoryRead,
		.oldLayout           = vk::ImageLayout::eUndefined,
		.newLayout           = vk::ImageLayout::eTransferDstOptimal,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = Image,
		.subresourceRange    = SubresourceRange,
	});
	ImagePostBarrier.emplace_back(vk::ImageMemoryBarrier{
		.srcAccessMask       = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask       = vk::AccessFlagBits::eMemoryRead,
		.oldLayout           = vk::ImageLayout::eTransferDstOptimal,
		.newLayout           = vk::ImageLayout::eShaderReadOnlyOptimal,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = Image,
		.subresourceRange    = SubresourceRange,
	});

	return FlushTick;
}

std::uint64_t StreamBuffer::Flush()
{
	std::scoped_lock StreamBufferLock{StreamBufferMutex};

	if( RingOffset == 0 )
	{
		return FlushTick;
	}
	// Any further pushes are going to be a part of the next tick
	const std::uint64_t PrevFlushTick      = FlushTick++;
	vk::CommandBuffer   FlushCommandBuffer = {};

	// Get where the GPU is at in our submit-timeline
	std::uint64_t GpuFlushTick = 0;
	if( auto GetResult = VulkanContext.LogicalDevice.getSemaphoreCounterValue(
			FlushSemaphore.get()
		);
		GetResult.result == vk::Result::eSuccess )
	{
		GpuFlushTick = GetResult.value;
	}
	else
	{
		std::fprintf(
			stderr, "Error getting timeline semaphore value: %s\n",
			vk::to_string(GetResult.result).c_str()
		);
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

		const vk::CommandBufferAllocateInfo CommandBufferInfo = {
			.commandPool        = CommandPool.get(),
			.commandBufferCount = 1,
		};

		if( auto AllocateResult
			= VulkanContext.LogicalDevice.allocateCommandBuffersUnique(
				CommandBufferInfo
			);
			AllocateResult.result == vk::Result::eSuccess )
		{
			FlushCommandBuffer = AllocateResult.value[0].get();
			CommandBuffers.emplace_back(std::move(AllocateResult.value[0]));
		}
		else
		{
			std::fprintf(
				stderr, "Error allocating command buffer: %s\n",
				vk::to_string(AllocateResult.result).c_str()
			);
			/// ??? should we exit the program
		}

		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, FlushCommandBuffer,
			"StreamBuffer: Command Buffer {}", CommandBuffers.size()
		);
	}

	const vk::CommandBufferBeginInfo BeginInfo = {
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
	};
	if( auto BeginResult = FlushCommandBuffer.begin(BeginInfo);
		BeginResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error beginning command buffer: %s\n",
			vk::to_string(BeginResult).c_str()
		);
	}

	{
		Vulkan::DebugLabelScope DebugCopyScope(
			FlushCommandBuffer, {1.0, 1.0, 0.0, 1.0}, "Upload Buffers"
		);
		for( const auto& [CurBuffer, CurBufferCopies] : BufferCopies )
		{
			FlushCommandBuffer.copyBuffer(
				RingBuffer.get(), CurBuffer, CurBufferCopies
			);
		}
	}

	{
		Vulkan::DebugLabelScope DebugCopyScope(
			FlushCommandBuffer, {1.0, 1.0, 0.0, 1.0}, "Upload Images"
		);
		FlushCommandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags{}, {}, {},
			ImagePreBarrier
		);
		for( const auto& [CurImage, CurImageCopies] : ImageCopies )
		{
			FlushCommandBuffer.copyBufferToImage(
				RingBuffer.get(), CurImage,
				vk::ImageLayout::eTransferDstOptimal, CurImageCopies
			);
		}
		FlushCommandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags{}, {},
			{}, ImagePostBarrier
		);
	}

	if( auto EndResult = FlushCommandBuffer.end();
		EndResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error ending command buffer: %s\n",
			vk::to_string(EndResult).c_str()
		);
	}

	static const vk::PipelineStageFlags WaitStage
		= vk::PipelineStageFlagBits::eTransfer;

	const vk::StructureChain<vk::SubmitInfo, vk::TimelineSemaphoreSubmitInfo>
		SubmitInfoChain = {
			vk::SubmitInfo{
				.waitSemaphoreCount   = 1,
				.pWaitSemaphores      = &FlushSemaphore.get(),
				.pWaitDstStageMask    = &WaitStage,
				.commandBufferCount   = 1,
				.pCommandBuffers      = &FlushCommandBuffer,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores    = &FlushSemaphore.get(),
			},
			vk::TimelineSemaphoreSubmitInfo{
				.waitSemaphoreValueCount   = 1,
				.pWaitSemaphoreValues      = &PrevFlushTick,
				.signalSemaphoreValueCount = 1,
				.pSignalSemaphoreValues    = &FlushTick,
			},
		};

	// Wait until the last possible moment to ensure all writes from the host
	// are completely flushed and ready for the GPU to consume. Don't have to do
	// this if we are using VK_MEMORY_PROPERTY_HOST_COHERENT_BIT memory. Rather
	// than putting a branch for non-coherent memory, just do it in all cases
	const vk::MappedMemoryRange MappedMemoryRanges[] = {
		{
			.memory = RingBufferMemory.get(),
			.offset = 0,
			.size   = VK_WHOLE_SIZE,
		},
	};
	if( const vk::Result MapResult
		= VulkanContext.LogicalDevice.flushMappedMemoryRanges(MappedMemoryRanges
		);
		MapResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error flushing streaming buffer mapped memory: %s\n",
			vk::to_string(MapResult).c_str()
		);
	};

	if( const auto SubmitResult
		= VulkanContext.TransferQueue.submit(SubmitInfoChain.get());
		SubmitResult != vk::Result::eSuccess )
	{
		// Error submitting
		std::fprintf(
			stderr, "Error submitting streaming buffer flush: %s\n",
			vk::to_string(SubmitResult).c_str()
		);
	}

	RingOffset = 0;
	BufferCopies.clear();
	ImageCopies.clear();
	ImagePreBarrier.clear();
	ImagePostBarrier.clear();

	return FlushTick;
} // namespace Vulkan

const vk::Semaphore& StreamBuffer::GetSemaphore() const
{
	return FlushSemaphore.get();
}

} // namespace Vulkan