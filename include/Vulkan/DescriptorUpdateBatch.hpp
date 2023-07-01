#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <memory>
#include <mutex>
#include <variant>

namespace Vulkan
{
// Implements a re-usable structure for batching up descriptor writes with a
// finite amount of space for both convenience and to reduce the overall amount
// of API calls to `vkUpdateDescriptorSets`
class DescriptorUpdateBatch
{
private:
	const Vulkan::Context& VulkanContext;

	const std::size_t DescriptorWriteMax;
	const std::size_t DescriptorCopyMax;

	using DescriptorInfoUnion = std::variant<
		vk::DescriptorImageInfo, vk::DescriptorBufferInfo, vk::BufferView>;

	std::recursive_mutex UpdateBatchMutex;

	// Todo: Maybe some kind of hash so that these structures can be re-used
	// among descriptor writes.
	std::unique_ptr<DescriptorInfoUnion[]>    DescriptorInfos;
	std::unique_ptr<vk::WriteDescriptorSet[]> DescriptorWrites;
	std::unique_ptr<vk::CopyDescriptorSet[]>  DescriptorCopies;

	std::size_t DescriptorWriteEnd = 0;
	std::size_t DescriptorCopyEnd  = 0;

	DescriptorUpdateBatch(
		const Vulkan::Context& VulkanContext, std::size_t DescriptorWriteMax,
		std::size_t DescriptorCopyMax
	)
		: VulkanContext(VulkanContext), DescriptorWriteMax(DescriptorWriteMax),
		  DescriptorCopyMax(DescriptorCopyMax)
	{
	}

public:
	void Flush();

	void AddImage(
		vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
		vk::ImageView   ImageView,
		vk::ImageLayout ImageLayout = vk::ImageLayout::eGeneral
	);
	void AddSampler(
		vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
		vk::Sampler Sampler
	);

	void AddImageSampler(
		vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
		vk::ImageView ImageView, vk::Sampler Sampler,
		vk::ImageLayout ImageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	);
	void AddBuffer(
		vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
		vk::Buffer Buffer, vk::DeviceSize Offset,
		vk::DeviceSize Size = VK_WHOLE_SIZE
	);

	void CopyBinding(
		vk::DescriptorSet SourceDescriptor, vk::DescriptorSet TargetDescriptor,
		std::uint8_t SourceBinding, std::uint8_t TargetBinding,
		std::uint8_t SourceArrayElement = 0,
		std::uint8_t TargetArrayElement = 0, std::uint8_t DescriptorCount = 1
	);

	static std::unique_ptr<DescriptorUpdateBatch> Create(
		const Vulkan::Context& VulkanContext,
		std::size_t            DescriptorWriteMax = 256,
		std::size_t            DescriptorCopyMax  = 256
	);
};
} // namespace Vulkan