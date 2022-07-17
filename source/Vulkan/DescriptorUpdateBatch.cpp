#include <Vulkan/DescriptorUpdateBatch.hpp>

#include <memory>
#include <span>

namespace Vulkan
{

void DescriptorUpdateBatch::Flush()
{
	VulkanContext.LogicalDevice.updateDescriptorSets(
		{std::span(DescriptorWrites.get(), DescriptorWriteEnd)},
		{std::span(DescriptorCopies.get(), DescriptorCopyEnd)});

	DescriptorWriteEnd = 0;
	DescriptorCopyEnd  = 0;
}

void DescriptorUpdateBatch::AddImage(
	vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
	vk::ImageView ImageView, vk::ImageLayout ImageLayout)
{
	if( DescriptorWriteEnd >= DescriptorWriteMax )
	{
		Flush();
	}

	const auto& ImageInfo
		= DescriptorInfos[DescriptorWriteEnd].emplace<vk::DescriptorImageInfo>(
			vk::Sampler(), ImageView, ImageLayout);

	DescriptorWrites[DescriptorWriteEnd] = vk::WriteDescriptorSet(
		TargetDescriptor, TargetBinding, 0, 1,
		vk::DescriptorType::eSampledImage, &ImageInfo, nullptr, nullptr);

	++DescriptorWriteEnd;
}

void DescriptorUpdateBatch::AddSampler(
	vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
	vk::Sampler Sampler)
{
	if( DescriptorWriteEnd >= DescriptorWriteMax )
	{
		Flush();
	}

	const auto& ImageInfo
		= DescriptorInfos[DescriptorWriteEnd].emplace<vk::DescriptorImageInfo>(
			Sampler, vk::ImageView(), vk::ImageLayout());

	DescriptorWrites[DescriptorWriteEnd] = vk::WriteDescriptorSet(
		TargetDescriptor, TargetBinding, 0, 1, vk::DescriptorType::eSampler,
		&ImageInfo, nullptr, nullptr);

	++DescriptorWriteEnd;
}

void DescriptorUpdateBatch::AddImageSampler(
	vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
	vk::ImageView ImageView, vk::Sampler Sampler, vk::ImageLayout ImageLayout)
{
	if( DescriptorWriteEnd >= DescriptorWriteMax )
	{
		Flush();
	}

	const auto& ImageInfo
		= DescriptorInfos[DescriptorWriteEnd].emplace<vk::DescriptorImageInfo>(
			Sampler, ImageView, ImageLayout);

	DescriptorWrites[DescriptorWriteEnd] = vk::WriteDescriptorSet(
		TargetDescriptor, TargetBinding, 0, 1,
		vk::DescriptorType::eCombinedImageSampler, &ImageInfo, nullptr,
		nullptr);

	++DescriptorWriteEnd;
}

void DescriptorUpdateBatch::AddBuffer(
	vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
	vk::Buffer Buffer, vk::DeviceSize Offset, vk::DeviceSize Size)
{
	if( DescriptorWriteEnd >= DescriptorWriteMax )
	{
		Flush();
	}

	const auto& BufferInfo
		= DescriptorInfos[DescriptorWriteEnd].emplace<vk::DescriptorBufferInfo>(
			Buffer, Offset, Size);

	DescriptorWrites[DescriptorWriteEnd] = vk::WriteDescriptorSet(
		TargetDescriptor, TargetBinding, 0, 1,
		vk::DescriptorType::eStorageImage, nullptr, &BufferInfo, nullptr);

	++DescriptorWriteEnd;
}

void DescriptorUpdateBatch::CopyBinding(
	vk::DescriptorSet SourceDescriptor, vk::DescriptorSet TargetDescriptor,
	std::uint8_t SourceBinding, std::uint8_t TargetBinding,
	std::uint8_t SourceArrayElement, std::uint8_t TargetArrayElement,
	std::uint8_t DescriptorCount)
{
	if( DescriptorCopyEnd >= DescriptorCopyMax )
	{
		Flush();
	}

	DescriptorCopies[DescriptorCopyEnd] = vk::CopyDescriptorSet(
		SourceDescriptor, SourceBinding, SourceArrayElement, TargetDescriptor,
		TargetBinding, TargetArrayElement, DescriptorCount);

	++DescriptorCopyEnd;
}

std::optional<DescriptorUpdateBatch> DescriptorUpdateBatch::Create(
	const Vulkan::Context& VulkanContext, std::size_t DescriptorWriteMax,
	std::size_t DescriptorCopyMax)

{
	DescriptorUpdateBatch NewDescriptorUpdateBatch(
		VulkanContext, DescriptorWriteMax, DescriptorCopyMax);

	NewDescriptorUpdateBatch.DescriptorInfos
		= std::make_unique<DescriptorInfoUnion[]>(DescriptorWriteMax);
	NewDescriptorUpdateBatch.DescriptorWrites
		= std::make_unique<vk::WriteDescriptorSet[]>(DescriptorWriteMax);
	NewDescriptorUpdateBatch.DescriptorCopies
		= std::make_unique<vk::CopyDescriptorSet[]>(DescriptorCopyMax);

	return {std::move(NewDescriptorUpdateBatch)};
}

} // namespace Vulkan