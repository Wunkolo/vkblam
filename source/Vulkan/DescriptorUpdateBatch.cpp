#include <Vulkan/DescriptorUpdateBatch.hpp>

#include <memory>
#include <span>

namespace Vulkan
{

void DescriptorUpdateBatch::Flush()
{
	std::scoped_lock UpdateBatchLock{UpdateBatchMutex};

	VulkanContext.LogicalDevice.updateDescriptorSets(
		{std::span(DescriptorWrites.get(), DescriptorWriteEnd)},
		{std::span(DescriptorCopies.get(), DescriptorCopyEnd)}
	);

	DescriptorWriteEnd = 0;
	DescriptorCopyEnd  = 0;
}

void DescriptorUpdateBatch::AddImage(
	vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
	vk::ImageView ImageView, vk::ImageLayout ImageLayout
)
{
	std::scoped_lock UpdateBatchLock{UpdateBatchMutex};

	if( DescriptorWriteEnd >= DescriptorWriteMax )
	{
		Flush();
	}

	const auto& ImageInfo
		= DescriptorInfos[DescriptorWriteEnd].emplace<vk::DescriptorImageInfo>(
			vk::Sampler(), ImageView, ImageLayout
		);

	DescriptorWrites[DescriptorWriteEnd] = vk::WriteDescriptorSet{
		.dstSet          = TargetDescriptor,
		.dstBinding      = TargetBinding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType  = vk::DescriptorType::eSampledImage,
		.pImageInfo      = &ImageInfo,
	};

	++DescriptorWriteEnd;
}

void DescriptorUpdateBatch::AddSampler(
	vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
	vk::Sampler Sampler
)
{
	std::scoped_lock UpdateBatchLock{UpdateBatchMutex};
	if( DescriptorWriteEnd >= DescriptorWriteMax )
	{
		Flush();
	}

	const auto& ImageInfo
		= DescriptorInfos[DescriptorWriteEnd].emplace<vk::DescriptorImageInfo>(
			Sampler, vk::ImageView(), vk::ImageLayout()
		);

	DescriptorWrites[DescriptorWriteEnd] = vk::WriteDescriptorSet{
		.dstSet          = TargetDescriptor,
		.dstBinding      = TargetBinding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType  = vk::DescriptorType::eSampler,
		.pImageInfo      = &ImageInfo,
	};

	++DescriptorWriteEnd;
}

void DescriptorUpdateBatch::AddImageSampler(
	vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
	vk::ImageView ImageView, vk::Sampler Sampler, vk::ImageLayout ImageLayout
)
{
	std::scoped_lock UpdateBatchLock{UpdateBatchMutex};
	if( DescriptorWriteEnd >= DescriptorWriteMax )
	{
		Flush();
	}

	const auto& ImageInfo
		= DescriptorInfos[DescriptorWriteEnd].emplace<vk::DescriptorImageInfo>(
			Sampler, ImageView, ImageLayout
		);

	DescriptorWrites[DescriptorWriteEnd] = vk::WriteDescriptorSet{
		.dstSet          = TargetDescriptor,
		.dstBinding      = TargetBinding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType  = vk::DescriptorType::eCombinedImageSampler,
		.pImageInfo      = &ImageInfo,
	};

	++DescriptorWriteEnd;
}

void DescriptorUpdateBatch::AddBuffer(
	vk::DescriptorSet TargetDescriptor, std::uint8_t TargetBinding,
	vk::Buffer Buffer, vk::DeviceSize Offset, vk::DeviceSize Size
)
{
	std::scoped_lock UpdateBatchLock{UpdateBatchMutex};
	if( DescriptorWriteEnd >= DescriptorWriteMax )
	{
		Flush();
	}

	const auto& BufferInfo
		= DescriptorInfos[DescriptorWriteEnd].emplace<vk::DescriptorBufferInfo>(
			Buffer, Offset, Size
		);

	DescriptorWrites[DescriptorWriteEnd] = vk::WriteDescriptorSet{
		.dstSet          = TargetDescriptor,
		.dstBinding      = TargetBinding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType  = vk::DescriptorType::eStorageImage,
		.pBufferInfo     = &BufferInfo,
	};

	++DescriptorWriteEnd;
}

void DescriptorUpdateBatch::CopyBinding(
	vk::DescriptorSet SourceDescriptor, vk::DescriptorSet TargetDescriptor,
	std::uint8_t SourceBinding, std::uint8_t TargetBinding,
	std::uint8_t SourceArrayElement, std::uint8_t TargetArrayElement,
	std::uint8_t DescriptorCount
)
{
	std::scoped_lock UpdateBatchLock{UpdateBatchMutex};
	if( DescriptorCopyEnd >= DescriptorCopyMax )
	{
		Flush();
	}

	DescriptorCopies[DescriptorCopyEnd] = vk::CopyDescriptorSet{
		.srcSet          = SourceDescriptor,
		.srcBinding      = SourceBinding,
		.srcArrayElement = SourceArrayElement,
		.dstSet          = TargetDescriptor,
		.dstBinding      = TargetBinding,
		.dstArrayElement = TargetArrayElement,
		.descriptorCount = DescriptorCount
	};

	++DescriptorCopyEnd;
}

std::unique_ptr<DescriptorUpdateBatch> DescriptorUpdateBatch::Create(
	const Vulkan::Context& VulkanContext, std::size_t DescriptorWriteMax,
	std::size_t DescriptorCopyMax
)

{
	std::unique_ptr<DescriptorUpdateBatch> NewDescriptorUpdateBatch(
		new DescriptorUpdateBatch(
			VulkanContext, DescriptorWriteMax, DescriptorCopyMax
		)
	);

	NewDescriptorUpdateBatch->DescriptorInfos
		= std::make_unique<DescriptorInfoUnion[]>(DescriptorWriteMax);
	NewDescriptorUpdateBatch->DescriptorWrites
		= std::make_unique<vk::WriteDescriptorSet[]>(DescriptorWriteMax);
	NewDescriptorUpdateBatch->DescriptorCopies
		= std::make_unique<vk::CopyDescriptorSet[]>(DescriptorCopyMax);

	return NewDescriptorUpdateBatch;
}

} // namespace Vulkan