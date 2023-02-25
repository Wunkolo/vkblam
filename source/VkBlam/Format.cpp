#include <VkBlam/Format.hpp>

#include "FormatTraits.hpp"

namespace VkBlam
{

vk::ImageType BlamToVk(Blam::BitmapEntryType Value)
{
	switch( Value )
	{
	case Blam::BitmapEntryType::Texture2D:
		return vk::ImageType::e2D;
	case Blam::BitmapEntryType::Texture3D:
		return vk::ImageType::e3D;
	case Blam::BitmapEntryType::CubeMap:
		return vk::ImageType::e2D;
	case Blam::BitmapEntryType::White:
		return vk::ImageType::e2D;
	}
	return vk::ImageType::e2D;
}

vk::Format BlamToVk(Blam::BitmapEntryFormat Value)
{
	switch( Value )
	{
	case Blam::BitmapEntryFormat::A8:
		return FormatTrait<Blam::BitmapEntryFormat::A8>::Format;
	case Blam::BitmapEntryFormat::Y8:
		return FormatTrait<Blam::BitmapEntryFormat::Y8>::Format;
	case Blam::BitmapEntryFormat::AY8:
		return FormatTrait<Blam::BitmapEntryFormat::AY8>::Format;
	case Blam::BitmapEntryFormat::A8Y8:
		return FormatTrait<Blam::BitmapEntryFormat::A8Y8>::Format;
	case Blam::BitmapEntryFormat::R5G6B5:
		return FormatTrait<Blam::BitmapEntryFormat::R5G6B5>::Format;
	case Blam::BitmapEntryFormat::A1R5G5B5:
		return FormatTrait<Blam::BitmapEntryFormat::A1R5G5B5>::Format;
	case Blam::BitmapEntryFormat::A4R4G4B4:
		return FormatTrait<Blam::BitmapEntryFormat::A4R4G4B4>::Format;
	case Blam::BitmapEntryFormat::X8R8G8B8:
		return FormatTrait<Blam::BitmapEntryFormat::X8R8G8B8>::Format;
	case Blam::BitmapEntryFormat::A8R8G8B8:
		return FormatTrait<Blam::BitmapEntryFormat::A8R8G8B8>::Format;
	case Blam::BitmapEntryFormat::DXT1:
		return FormatTrait<Blam::BitmapEntryFormat::DXT1>::Format;
	case Blam::BitmapEntryFormat::DXT2AND3:
		return FormatTrait<Blam::BitmapEntryFormat::DXT2AND3>::Format;
	case Blam::BitmapEntryFormat::DXT4AND5:
		return FormatTrait<Blam::BitmapEntryFormat::DXT4AND5>::Format;
	case Blam::BitmapEntryFormat::P8:
		return FormatTrait<Blam::BitmapEntryFormat::P8>::Format;
	}
	return vk::Format::eUndefined;
}

vk::ComponentMapping GetFormatSwizzle(Blam::BitmapEntryFormat Value)
{
	switch( Value )
	{
	case Blam::BitmapEntryFormat::A8:
		return FormatTrait<Blam::BitmapEntryFormat::A8>::Swizzle;
	case Blam::BitmapEntryFormat::Y8:
		return FormatTrait<Blam::BitmapEntryFormat::Y8>::Swizzle;
	case Blam::BitmapEntryFormat::AY8:
		return FormatTrait<Blam::BitmapEntryFormat::AY8>::Swizzle;
	case Blam::BitmapEntryFormat::A8Y8:
		return FormatTrait<Blam::BitmapEntryFormat::A8Y8>::Swizzle;
	case Blam::BitmapEntryFormat::R5G6B5:
		return FormatTrait<Blam::BitmapEntryFormat::R5G6B5>::Swizzle;
	case Blam::BitmapEntryFormat::A1R5G5B5:
		return FormatTrait<Blam::BitmapEntryFormat::A1R5G5B5>::Swizzle;
	case Blam::BitmapEntryFormat::A4R4G4B4:
		return FormatTrait<Blam::BitmapEntryFormat::A4R4G4B4>::Swizzle;
	case Blam::BitmapEntryFormat::X8R8G8B8:
		return FormatTrait<Blam::BitmapEntryFormat::X8R8G8B8>::Swizzle;
	case Blam::BitmapEntryFormat::A8R8G8B8:
		return FormatTrait<Blam::BitmapEntryFormat::A8R8G8B8>::Swizzle;
	case Blam::BitmapEntryFormat::DXT1:
		return FormatTrait<Blam::BitmapEntryFormat::DXT1>::Swizzle;
	case Blam::BitmapEntryFormat::DXT2AND3:
		return FormatTrait<Blam::BitmapEntryFormat::DXT2AND3>::Swizzle;
	case Blam::BitmapEntryFormat::DXT4AND5:
		return FormatTrait<Blam::BitmapEntryFormat::DXT4AND5>::Swizzle;
	case Blam::BitmapEntryFormat::P8:
		return FormatTrait<Blam::BitmapEntryFormat::P8>::Swizzle;
	}
	return vk::ComponentMapping();
}
} // namespace VkBlam