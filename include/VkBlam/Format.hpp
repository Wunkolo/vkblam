#pragma once

#include <VkBlam/VkBlam.hpp>

namespace VkBlam
{
vk::ImageType BlamToVk(Blam::BitmapEntryType Value);
vk::Format    BlamToVk(Blam::BitmapEntryFormat Value);

vk::ComponentMapping GetFormatSwizzle(Blam::BitmapEntryFormat Value);
} // namespace VkBlam