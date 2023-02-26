#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace Vulkan
{

// A basic stream buffer implementation to handle uploading and download to
// buffers and images This implementation does not handle image layout
// transitions or barriers of any sort and must be handled externally
class StreamBuffer
{
private:
	const Vulkan::Context& VulkanContext;
	const vk::DeviceSize   BufferSize;

	// This is a timeline semaphore with a value that increases with each flush.
	vk::UniqueSemaphore FlushSemaphore;
	std::uint64_t       FlushTick;

	vk::UniqueBuffer       RingBuffer;
	vk::UniqueDeviceMemory RingBufferMemory;

	// The host-mapped vulkan memory for the ring buffer
	std::span<std::byte> RingMemoryMapped;
	// Current write-point for the ring buffer.
	std::size_t RingOffset;

	vk::UniqueCommandPool                CommandPool;
	std::vector<vk::UniqueCommandBuffer> CommandBuffers;
	std::vector<std::uint64_t>           CommandBufferTimeStamps;

	std::unordered_map<vk::Buffer, std::vector<vk::BufferCopy>> BufferCopies;
	std::unordered_map<vk::Image, std::vector<vk::BufferImageCopy>> ImageCopies;

	std::vector<vk::ImageMemoryBarrier> ImagePreBarrier;
	std::vector<vk::ImageMemoryBarrier> ImagePostBarrier;

public:
	StreamBuffer(
		const Vulkan::Context& VulkanContext, vk::DeviceSize BufferSize
	);

	~StreamBuffer();

	// Queue an Upload of the passed in span of bytes to the target buffer at
	// the designated offset
	// Upon success, returns the timeline semaphore value to listen
	// for to know when this transfer is complete
	std::uint64_t QueueBufferUpload(
		const std::span<const std::byte> Data, vk::Buffer Buffer,
		vk::DeviceSize Offset = 0
	);

	// Queue an Upload of the passed in span of bytes to the target image at the
	// specified offset, extent, and subresource
	// Upon success, returns the timeline semaphore value to listen
	// for to know when this transfer is complete
	std::uint64_t QueueImageUpload(
		const std::span<const std::byte> Data, vk::Image Image,
		vk::Offset3D Offset, vk::Extent3D Extent,
		vk::ImageSubresourceLayers SubresourceLayers
		= vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
		vk::ImageLayout DstLayout = vk::ImageLayout::eTransferDstOptimal
	);

	// Flush all pending uploads and downloads to the specified queue
	// and returns the semaphore value to wait for to know when completion is
	// done.
	std::uint64_t Flush();

	// The timeline-semaphore used to synchronize uploads
	const vk::Semaphore& GetSemaphore() const;
};

} // namespace Vulkan