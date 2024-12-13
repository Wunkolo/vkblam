#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>
#include <Common/Literals.hpp>

#include <bit>
#include <vector>

namespace VkBlam::Tags
{
using namespace Common::Literals;
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
	Bitmap(
		const Blam::TagIndexEntry&               TagIndexEntry,
		const Blam::Tag<Blam::TagClass::Bitmap>& Tag
	);
	~Bitmap();

	[[nodiscard]] std::span<const SubBitmap> GetBitmaps() const;

	[[nodiscard]] const SubBitmap& GetBitmap(std::size_t BitmapIndex) const;

	friend class BitmapSubsystem;
};

class BitmapSubsystem final
	: public TagSubsystem<Blam::TagClass::Bitmap, Bitmap>
{
private:
	const Vulkan::Context& VulkanContext;

	// All of these images are static textures that only ever get sample from,
	// so a simple heap-allocator is utilized
	static constexpr std::uint64_t BlockSize = 32_MiB;
	static_assert(std::has_single_bit(BlockSize), "BlockSize must be pow2");

	std::vector<std::uint32_t>          BlockFreeSpace;
	std::vector<std::uint8_t>           BlockMemoryIndex;
	std::vector<vk::UniqueDeviceMemory> BlockMemory;

	vk::BindImageMemoryInfo FindFreeBlock(vk::Image);

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