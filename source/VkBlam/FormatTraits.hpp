#include <Blam/Enums.hpp>
#include <Vulkan/VulkanAPI.hpp>

template<Blam::BitmapEntryFormat Format>
struct FormatTrait
{
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::A8>
{
	static constexpr vk::Format           Format  = vk::Format::eR8Unorm;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping(
		vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eR
	);
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::Y8>
{
	static constexpr vk::Format           Format  = vk::Format::eR8Unorm;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::AY8>
{
	static constexpr vk::Format           Format  = vk::Format::eR8Unorm;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::A8Y8>
{
	static constexpr vk::Format           Format  = vk::Format::eR8G8Unorm;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::R5G6B5>
{
	static constexpr vk::Format Format = vk::Format::eR5G6B5UnormPack16;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::A1R5G5B5>
{
	static constexpr vk::Format Format = vk::Format::eA1R5G5B5UnormPack16;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::A4R4G4B4>
{
	static constexpr vk::Format Format = vk::Format::eR4G4B4A4UnormPack16;
	// ARGB <-> RGBA
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping(
		vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB,
		vk::ComponentSwizzle::eA, vk::ComponentSwizzle::eR
	);
	;
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::X8R8G8B8>
{
	static constexpr vk::Format Format = vk::Format::eA8B8G8R8UnormPack32;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::A8R8G8B8>
{
	static constexpr vk::Format Format = vk::Format::eA8B8G8R8UnormPack32;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::DXT1>
{
	static constexpr vk::Format           Format = vk::Format::eBc1RgbSrgbBlock;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::DXT2AND3>
{
	static constexpr vk::Format           Format  = vk::Format::eBc2SrgbBlock;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::DXT4AND5>
{
	static constexpr vk::Format           Format  = vk::Format::eBc3SrgbBlock;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};

template<>
struct FormatTrait<Blam::BitmapEntryFormat::P8>
{
	static constexpr vk::Format           Format  = vk::Format::eR8Unorm;
	static constexpr vk::ComponentMapping Swizzle = vk::ComponentMapping();
};