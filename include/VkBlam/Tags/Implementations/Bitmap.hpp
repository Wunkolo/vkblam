#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

class Bitmap : public TagImplementation
{
private:
	Bitmap();

	struct SubBitmap
	{
		vk::UniqueImage     Image;
		vk::UniqueImageView View;
	};
	std::vector<SubBitmap> Bitmaps;

public:
	virtual ~Bitmap();

	static constexpr Blam::TagClass ClassT = Blam::TagClass::Bitmap;

	static std::unique_ptr<Bitmap> LoadTag(
		const Blam::TagIndexEntry&               TagIndexEntry,
		const Blam::Tag<Blam::TagClass::Bitmap>& Tag, Scene& TargetScene
	);
};

} // namespace VkBlam::Tags