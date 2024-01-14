#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/Bitmap.hpp>

namespace VkBlam::Tags
{

Bitmap::Bitmap()
{
}

Bitmap::~Bitmap()
{
}

std::unique_ptr<Bitmap> Bitmap::LoadTag(
	const Blam::TagIndexEntry&               TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Bitmap>& Tag, Scene& TargetScene
)
{
	std::unique_ptr<Bitmap> NewBitmap(new Bitmap());
	const Vulkan::Context&  VulkanContext = TargetScene.GetVulkanContext();

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
		ImageInfo.extent              = vk::Extent3D(
            CurBitmapEntry.Width, CurBitmapEntry.Height, CurBitmapEntry.Depth
        );
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

		SubBitmap CurSubBitmap = {};

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
			"Bitmap {:08X}[{:2}] | {}", TagIndexEntry.TagID, CurSubTextureIdx,
			TargetScene.GetMapFile().GetTagName(TagIndexEntry.TagID)
		);

		//// Image must be fully bound before creating an ImageView

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
			CurSubBitmap.View = std::move(CreateResult.value);
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
			TargetScene.GetMapFile().GetTagName(TagIndexEntry.TagID)
		);

		NewBitmap->Bitmaps[CurSubTextureIdx] = std::move(CurSubBitmap);
	}

	return NewBitmap;
}
} // namespace VkBlam::Tags