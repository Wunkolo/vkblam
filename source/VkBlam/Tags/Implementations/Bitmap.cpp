#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/Bitmap.hpp>

#include <Vulkan/Memory.hpp>

namespace VkBlam::Tags
{

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

Bitmap* BitmapSubsystem::LoadTag(
	const Blam::TagIndexEntry&               TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Bitmap>& Tag, Scene& TargetScene
)
{
	std::unique_ptr<Bitmap> NewBitmap(new Bitmap());

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
		ImageInfo.mipLevels
			= std::max<std::uint16_t>(CurBitmapEntry.MipmapCount, 1);
		ImageInfo.arrayLayers
			= CurBitmapEntry.Type == Blam::BitmapEntryType::CubeMap ? 6 : 1;
		ImageInfo.samples = vk::SampleCountFlagBits::e1;
		ImageInfo.tiling  = vk::ImageTiling::eOptimal;
		ImageInfo.usage   = vk::ImageUsageFlagBits::eSampled
						| vk::ImageUsageFlagBits::eTransferDst
						| vk::ImageUsageFlagBits::eTransferSrc;
		ImageInfo.sharingMode   = vk::SharingMode::eExclusive;
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

	// Create a single heap of device memory for all the subimages
	{
		std::vector<vk::Image> Images;
		for( const auto& Bitmap : NewBitmap->Bitmaps )
		{
			Images.push_back(Bitmap.Image.get());
		}

		if( auto [Result, Value] = Vulkan::CommitImageHeap(
				VulkanContext.LogicalDevice, VulkanContext.PhysicalDevice,
				Images, vk::MemoryPropertyFlagBits::eDeviceLocal
			);
			Result == vk::Result::eSuccess )
		{
			NewBitmap->Memory = std::move(Value);
		}
		else
		{
			std::fprintf(
				stderr, "Error committing image memory: %s\n",
				vk::to_string(Result).c_str()
			);
			return nullptr;
		}
	}

	Vulkan::SetObjectName(
		VulkanContext.LogicalDevice, NewBitmap->Memory.get(),
		"Bitmap[{:08X}]: DeviceMemory | {}", TagIndexEntry.TagID,
		TargetScene.GetMapFile().GetTagPath(TagIndexEntry.TagID)
	);

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

		const std::size_t BlockSize
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
					* BlockSize;

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