#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

class Bitmap final : public TagImplementation<Blam::TagClass::Bitmap>
{
private:
	vk::UniqueDeviceMemory Memory;
	struct SubBitmap
	{
		vk::UniqueImage     Image;
		vk::UniqueImageView View;
	};
	std::vector<SubBitmap> Bitmaps;

public:
	~Bitmap();

	friend class BitmapSubsystem;
};

class BitmapSubsystem final
	: public TagSubsystem<Blam::TagClass::Bitmap, Bitmap>
{
private:
	const Vulkan::Context& VulkanContext;

	std::vector<std::unique_ptr<Bitmap>> Bitmaps;

public:
	BitmapSubsystem(TagPool& Pool, const Vulkan::Context& VulkanContext);
	~BitmapSubsystem();

	[[nodiscard]] Bitmap* LoadTag(
		const Blam::TagIndexEntry&               TagIndexEntry,
		const Blam::Tag<Blam::TagClass::Bitmap>& Tag, Scene& TargetScene
	) override;
};

} // namespace VkBlam::Tags