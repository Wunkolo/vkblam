#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

class Bitmap final : public TagImplementation<Blam::TagClass::Bitmap>
{
private:
	struct SubBitmap
	{
		vk::UniqueImage     Image;
		vk::UniqueImageView View;
	};
	std::vector<SubBitmap> Bitmaps;

public:
	~Bitmap();

	[[nodiscard]] Blam::TagClass GetTagClass() const override
	{
		return Blam::TagClass::Bitmap;
	}

	friend class BitmapSubsystem;
};

class BitmapSubsystem final
	: public TagSubsystem<Blam::TagClass::Bitmap, Bitmap>
{
private:
	const Vulkan::Context& VulkanContext;

	std::vector<std::unique_ptr<Bitmap>> Bitmaps;

public:
	BitmapSubsystem(const Vulkan::Context& VulkanContext);
	~BitmapSubsystem();

	[[nodiscard]] Bitmap* LoadTag(
		const Blam::TagIndexEntry&               TagIndexEntry,
		const Blam::Tag<Blam::TagClass::Bitmap>& Tag, Scene& TargetScene
	) override;
};

} // namespace VkBlam::Tags