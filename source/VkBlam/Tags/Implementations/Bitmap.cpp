#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/Bitmap.hpp>

#include <Common/Alignment.hpp>
#include <Common/Format.hpp>

#include <Vulkan/Memory.hpp>

namespace VkBlam::Tags
{

Bitmap::Bitmap(
	const Blam::TagIndexEntry&               TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Bitmap>& Tag
)
	: TagImplementation<Blam::TagClass::Bitmap>(TagIndexEntry, Tag)
{
}

Bitmap::~Bitmap()
{
}

std::span<const Bitmap::SubBitmap> Bitmap::GetBitmaps() const
{
	return Bitmaps;
}
const Bitmap::SubBitmap& Bitmap::GetBitmap(std::size_t BitmapIndex) const
{
	return Bitmaps.at(BitmapIndex);
}

BitmapSubsystem::BitmapSubsystem(
	TagPool& Pool, const Vulkan::Context& VulkanContext
)
	: TagSubsystem<Blam::TagClass::Bitmap, Bitmap>(Pool),
	  VulkanContext(VulkanContext)
{
}

BitmapSubsystem::~BitmapSubsystem()
{
}

std::optional<vk::BindImageMemoryInfo>
	BitmapSubsystem::FindFreeBlock(vk::Image Image)
{
	const vk::PhysicalDevice& PhysicalDevice = VulkanContext.PhysicalDevice;
	// Align the incoming memory-size to bufferImageGranularity
	const vk::DeviceSize BufferImageGranularity
		= PhysicalDevice.getProperties().limits.bufferImageGranularity;

	vk::MemoryRequirements ImageRequirements
		= VulkanContext.LogicalDevice.getImageMemoryRequirements(Image);

	ImageRequirements.size
		= Common::AlignUp(ImageRequirements.size, BufferImageGranularity);

	// Find free space in a block
	for( std::size_t i = 0; i < BlockFreeSpace.size(); ++i )
	{
		std::uint32_t&      CurBlockFreeSpace   = BlockFreeSpace[i];
		const std::uint8_t& CurBlockMemoryIndex = BlockMemoryIndex[i];

		const std::uint32_t CurBlockOffStart = (BlockSize - CurBlockFreeSpace);

		const std::uint32_t CurBlockOffStartAlign
			= Common::AlignUp(CurBlockOffStart, ImageRequirements.alignment);

		const std::uint32_t CurBlockOffNewEnd
			= CurBlockOffStartAlign + ImageRequirements.size;

		// Found a space
		const bool ValidMemoryIndex
			= (ImageRequirements.memoryTypeBits & 1 << CurBlockMemoryIndex)
		   != 0u;

		if( ValidMemoryIndex && CurBlockOffNewEnd < BlockSize )
		{
			CurBlockFreeSpace = BlockSize - CurBlockOffNewEnd;
			return vk::BindImageMemoryInfo{
				.image        = Image,
				.memory       = BlockMemory[i].get(),
				.memoryOffset = CurBlockOffStartAlign,
			};
		}
	}

	// No free block found, create a new one
	// If the image is larger than the block size(?!) then just give it a whole
	// block large enough to satisfy the allocation
	const std::size_t NewBlockIndex = BlockFreeSpace.size();
	const std::size_t NewBlockSize
		= std::max(ImageRequirements.size, BlockSize);
	BlockFreeSpace.emplace_back(NewBlockSize) -= ImageRequirements.size;

	// Allocate a new block
	const vk::Device& Device = VulkanContext.LogicalDevice;

	const std::int32_t MemoryTypeIndex = Vulkan::FindMemoryTypeIndex(
		PhysicalDevice, ImageRequirements.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	if( MemoryTypeIndex < 0 )
	{
		// Unable to find any qualifying memory
		return std::nullopt;
	}

	const vk::MemoryAllocateInfo BlockAllocInfo = {
		.allocationSize  = NewBlockSize,
		.memoryTypeIndex = std::uint32_t(MemoryTypeIndex),
	};

	if( auto AllocResult = Device.allocateMemoryUnique(BlockAllocInfo);
		AllocResult.result == vk::Result::eSuccess )
	{
		BlockMemoryIndex.emplace_back(MemoryTypeIndex);
		BlockMemory.emplace_back(std::move(AllocResult.value));
	}
	else
	{
		// Error allocating new block
		return std::nullopt;
	}

	Vulkan::SetObjectName(
		VulkanContext.LogicalDevice, BlockMemory[NewBlockIndex].get(),
		"Bitmap Memory Block #{:02}", NewBlockIndex
	);

	return vk::BindImageMemoryInfo{
		.image        = Image,
		.memory       = BlockMemory[NewBlockIndex].get(),
		.memoryOffset = 0,
	};
}

Bitmap* BitmapSubsystem::LoadTag(
	const Blam::TagIndexEntry&               TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Bitmap>& Tag, Scene& TargetScene
)
{
	std::unique_ptr<Bitmap> NewBitmap(new Bitmap(TagIndexEntry, Tag));

	const auto SubBitmaps
		= TargetScene.GetMapFile().TagHeap.GetBlock(Tag.Bitmaps);

	NewBitmap->Bitmaps.resize(SubBitmaps.size());

	for( std::size_t CurSubTextureIdx = 0; CurSubTextureIdx < SubBitmaps.size();
		 ++CurSubTextureIdx )
	{
		const auto& CurBitmapEntry = SubBitmaps[CurSubTextureIdx];

		const std::size_t MipCount
			= std::max<std::uint16_t>(CurBitmapEntry.MipmapCount, 1);
		const std::size_t LayerCount
			= CurBitmapEntry.Type == Blam::BitmapEntryType::CubeMap ? 6 : 1;

		// Create Image handles
		vk::ImageCreateInfo ImageInfo = {};
		ImageInfo.imageType           = VkBlam::BlamToVk(CurBitmapEntry.Type);
		ImageInfo.format              = VkBlam::BlamToVk(CurBitmapEntry.Format);
		ImageInfo.extent              = vk::Extent3D{
						 .width  = CurBitmapEntry.Width,
						 .height = CurBitmapEntry.Height,
						 .depth  = CurBitmapEntry.Depth,
        };
		ImageInfo.mipLevels   = MipCount;
		ImageInfo.arrayLayers = LayerCount;
		ImageInfo.samples     = vk::SampleCountFlagBits::e1;
		ImageInfo.tiling      = vk::ImageTiling::eOptimal;
		ImageInfo.usage       = vk::ImageUsageFlagBits::eSampled
						| vk::ImageUsageFlagBits::eTransferDst
						| vk::ImageUsageFlagBits::eTransferSrc;

		// This image should only ever be touched by the render-queue and the
		// transfer-queue(for streaming). If the two queues are not the same,
		// then it must be designated with concurrent sharing.
		const std::uint32_t SharedQueues[2] = {
			VulkanContext.RenderQueueFamilyIndex,
			VulkanContext.TransferQueueFamilyIndex,
		};
		if( VulkanContext.RenderQueueFamilyIndex
			== VulkanContext.TransferQueueFamilyIndex )
		{
			// Queues are the same exclusively owned by this queue
			ImageInfo.sharingMode = vk::SharingMode::eExclusive;
		}
		else
		{
			// Queues are different, concurrently shared by two queues
			ImageInfo.sharingMode = vk::SharingMode::eConcurrent;
			ImageInfo.setQueueFamilyIndices(SharedQueues);
		}

		ImageInfo.initialLayout = vk::ImageLayout::eUndefined;

		if( CurBitmapEntry.Type == Blam::BitmapEntryType::CubeMap )
		{
			ImageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
		}

		Bitmap::SubBitmap CurSubBitmap = {};

		if( auto CreateResult = TargetScene.GetVulkanContext()
									.LogicalDevice.createImageUnique(ImageInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			CurSubBitmap.Image = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating image: %s\n",
				vk::to_string(CreateResult.result).c_str()
			);
		}

		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, CurSubBitmap.Image.get(),
			"Bitmap[{:08X}][{:2}]: Image | {}", TagIndexEntry.TagID,
			CurSubTextureIdx,
			TargetScene.GetMapFile().GetTagPath(TagIndexEntry.TagID)
		);
		NewBitmap->Bitmaps[CurSubTextureIdx] = std::move(CurSubBitmap);
	}

	// Bind bitmaps to memory
	std::vector<vk::BindImageMemoryInfo> ImageHeapBinds;
	for( const auto& Bitmap : NewBitmap->Bitmaps )
	{
		if( const auto& AllocateInfo = FindFreeBlock(Bitmap.Image.get());
			AllocateInfo.has_value() )
		{
			ImageHeapBinds.emplace_back(AllocateInfo.value());
		}
		else
		{
			// Error allocating space for image
			return nullptr;
		}
	}

	// Now bind them all in one call
	if( const vk::Result BindResult
		= VulkanContext.LogicalDevice.bindImageMemory2(ImageHeapBinds);
		BindResult == vk::Result::eSuccess )
	{
		// Binding memory succeeded
	}
	else
	{
		// Error binding memory
		return nullptr;
	}

	// Image is binded to memory now
	// Stream image, create image view, etc
	for( std::size_t CurSubTextureIdx = 0; CurSubTextureIdx < SubBitmaps.size();
		 ++CurSubTextureIdx )
	{
		const auto& CurBitmapEntry = SubBitmaps[CurSubTextureIdx];
		const auto  PixelData      = std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(
                TargetScene.GetMapFile().GetBitmapData().data()
            ) + CurBitmapEntry.PixelDataOffset,
            CurBitmapEntry.PixelDataSize
        );

		const auto& CurSubBitmap = NewBitmap->Bitmaps[CurSubTextureIdx];

		const std::size_t MipCount
			= std::max<std::uint16_t>(CurBitmapEntry.MipmapCount, 1);
		const std::size_t LayerCount
			= CurBitmapEntry.Type == Blam::BitmapEntryType::CubeMap ? 6 : 1;

		const std::size_t FormatBlockSize
			= vk::blockSize(VkBlam::BlamToVk(CurBitmapEntry.Format));
		const std::array<std::uint8_t, 3> BlockExtent
			= vk::blockExtent(VkBlam::BlamToVk(CurBitmapEntry.Format));

		std::size_t PixelDataOff = 0;

		auto CurExtent = vk::Extent3D{
			.width  = CurBitmapEntry.Width,
			.height = CurBitmapEntry.Height,
			.depth  = CurBitmapEntry.Depth
		};
		for( std::uint16_t CurMip = 0; CurMip < MipCount; ++CurMip )
		{
			for( std::uint16_t CurLayer = 0; CurLayer < LayerCount; ++CurLayer )
			{
				const std::array<std::uint32_t, 3> CurBlockCount
					= {std::max(1u, CurExtent.width / BlockExtent[0]),
					   std::max(1u, CurExtent.height / BlockExtent[1]),
					   std::max(1u, CurExtent.depth / BlockExtent[2])};

				const std::size_t CurPixelDataSize
					= CurBlockCount[0] * CurBlockCount[1] * CurBlockCount[2]
					* FormatBlockSize;

				TargetScene.GetRasterizer().GetStreamBuffer().QueueImageUpload(
					PixelData.subspan(PixelDataOff, CurPixelDataSize),
					CurSubBitmap.Image.get(), vk::Offset3D{0, 0, 0}, CurExtent,
					vk::ImageSubresourceLayers{
						.aspectMask     = vk::ImageAspectFlagBits::eColor,
						.mipLevel       = CurMip,
						.baseArrayLayer = CurLayer,
						.layerCount     = 1,
					}
				);

				PixelDataOff += CurPixelDataSize;
			}

			CurExtent.width  = std::max(1u, CurExtent.width / 2);
			CurExtent.height = std::max(1u, CurExtent.height / 2);
			CurExtent.depth  = std::max(1u, CurExtent.depth / 2);
		}

		// Create Image View
		vk::ImageViewCreateInfo BitmapImageViewInfo = {};
		BitmapImageViewInfo.image                   = CurSubBitmap.Image.get();
		switch( CurBitmapEntry.Type )
		{
		default:
		case Blam::BitmapEntryType::Texture2D:
		{
			BitmapImageViewInfo.viewType = vk::ImageViewType::e2D;
			break;
		}
		case Blam::BitmapEntryType::Texture3D:
		{
			BitmapImageViewInfo.viewType = vk::ImageViewType::e3D;
			break;
		}
		case Blam::BitmapEntryType::CubeMap:
		{
			BitmapImageViewInfo.viewType = vk::ImageViewType::eCube;
			break;
		}
		}
		BitmapImageViewInfo.format = VkBlam::BlamToVk(CurBitmapEntry.Format);
		;
		BitmapImageViewInfo.components.r = vk::ComponentSwizzle::eR;
		BitmapImageViewInfo.components.g = vk::ComponentSwizzle::eG;
		BitmapImageViewInfo.components.b = vk::ComponentSwizzle::eB;
		BitmapImageViewInfo.components.a = vk::ComponentSwizzle::eA;
		BitmapImageViewInfo.subresourceRange.aspectMask
			= vk::ImageAspectFlagBits::eColor;
		BitmapImageViewInfo.subresourceRange.baseMipLevel   = 0;
		BitmapImageViewInfo.subresourceRange.levelCount     = MipCount;
		BitmapImageViewInfo.subresourceRange.baseArrayLayer = 0;
		BitmapImageViewInfo.subresourceRange.layerCount     = LayerCount;

		if( auto CreateResult = VulkanContext.LogicalDevice
									.createImageViewUnique(BitmapImageViewInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewBitmap->Bitmaps[CurSubTextureIdx].View
				= std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating bitmap view: %s\n",
				vk::to_string(CreateResult.result).c_str()
			);
		}

		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, CurSubBitmap.View.get(),
			"Bitmap View {:08X}[{:2}] | {}", TagIndexEntry.TagID,
			CurSubTextureIdx,
			TargetScene.GetMapFile().GetTagPath(TagIndexEntry.TagID)
		);
	}

	return Bitmaps.emplace_back(std::move(NewBitmap)).get();
}
} // namespace VkBlam::Tags