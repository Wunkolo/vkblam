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
	const Blam::Tag<Blam::TagClass::Bitmap>& Tag, Scene& TargetScene
)
{
	std::unique_ptr<Bitmap> NewBitmap(new Bitmap());

	for( const auto& CurBitmapEntry :
		 TargetScene.GetMapFile().TagHeap.GetBlock(Tag.Bitmaps) )
	{
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

		if( auto CreateResult = TargetScene.GetRenderer()
									.GetVulkanContext()
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

		NewBitmap->Bitmaps.emplace_back(std::move(CurSubBitmap));
	}

	return NewBitmap;
}
} // namespace VkBlam::Tags