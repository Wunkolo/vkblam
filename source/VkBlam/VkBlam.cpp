#include <VkBlam/VkBlam.hpp>

#include <array>
#include <initializer_list>

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(vkblam);
static cmrc::embedded_filesystem DataFS = cmrc::vkblam::get_filesystem();

namespace VkBlam
{

vk::SamplerCreateInfo Sampler2D(bool Filtered, bool Clamp)
{
	vk::SamplerCreateInfo SamplerInfo = {};
	SamplerInfo.magFilter             = Filtered ? vk::Filter::eLinear
												 : vk::Filter::eNearest;
	SamplerInfo.minFilter             = Filtered ? vk::Filter::eLinear
												 : vk::Filter::eNearest;

	SamplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

	SamplerInfo.addressModeU = Clamp ? vk::SamplerAddressMode::eClampToEdge
									 : vk::SamplerAddressMode::eRepeat;
	SamplerInfo.addressModeV = Clamp ? vk::SamplerAddressMode::eClampToEdge
									 : vk::SamplerAddressMode::eRepeat;
	SamplerInfo.addressModeW = Clamp ? vk::SamplerAddressMode::eClampToEdge
									 : vk::SamplerAddressMode::eRepeat;

	SamplerInfo.mipLodBias       = 0.0f;
	SamplerInfo.anisotropyEnable = VK_TRUE;
	SamplerInfo.maxAnisotropy    = 16.0f;

	SamplerInfo.compareEnable = VK_FALSE;
	SamplerInfo.compareOp     = vk::CompareOp::eAlways;

	SamplerInfo.minLod      = 0.0f;
	SamplerInfo.maxLod      = VK_LOD_CLAMP_NONE;
	SamplerInfo.borderColor = vk::BorderColor::eFloatTransparentBlack;
	SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	return SamplerInfo;
}

vk::SamplerCreateInfo SamplerCube()
{
	vk::SamplerCreateInfo SamplerInfo = {};

	SamplerInfo.magFilter = vk::Filter::eLinear;
	SamplerInfo.minFilter = vk::Filter::eLinear;

	SamplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

	SamplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	SamplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	SamplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;

	SamplerInfo.mipLodBias       = 0.0f;
	SamplerInfo.anisotropyEnable = VK_FALSE;
	SamplerInfo.maxAnisotropy    = 1.0f;

	SamplerInfo.compareEnable = VK_FALSE;
	SamplerInfo.compareOp     = vk::CompareOp::eAlways;

	SamplerInfo.minLod      = 0.0f;
	SamplerInfo.maxLod      = VK_LOD_CLAMP_NONE;
	SamplerInfo.borderColor = vk::BorderColor::eFloatTransparentBlack;
	SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	return SamplerInfo;
}

std::optional<std::span<const std::byte>> OpenResource(const std::string& Path)
{
	if( !DataFS.exists(Path) )
	{
		return {};
	}
	const cmrc::file File = DataFS.open(Path);
	return std::span<const std::byte>(
		reinterpret_cast<const std::byte*>(File.cbegin()), File.size()
	);
}

std::vector<vk::VertexInputBindingDescription>
	GetVertexInputBindings(std::span<const Blam::VertexFormat> Formats)
{
	std::vector<vk::VertexInputBindingDescription> Result;

	std::size_t BindingIndex = 0;
	for( const Blam::VertexFormat& CurFormat : Formats )
	{
		Result.push_back(vk::VertexInputBindingDescription(
			BindingIndex, Blam::GetVertexStride(CurFormat),
			vk::VertexInputRate::eVertex
		));

		++BindingIndex;
	}

	return Result;
}

static std::array<
	std::initializer_list<vk::VertexInputAttributeDescription>, 20>
	VertexFormatAttributes = {{
		// SBSPVertexUncompressed
		{
			{0, 0, vk::Format::eR32G32B32Sfloat, 0x00},
			{0, 0, vk::Format::eR32G32B32Sfloat, 0x0C},
			{0, 0, vk::Format::eR32G32B32Sfloat, 0x18},
			{0, 0, vk::Format::eR32G32B32Sfloat, 0x24},
			{0, 0, vk::Format::eR32G32Sfloat, 0x30},
		},
		// SBSPVertexCompressed
		{
			{},
			{},
			{},
		},
		// SBSPLightmapVertexUncompressed
		{
			{0, 0, vk::Format::eR32G32B32Sfloat, 0x00}, // D3DDECLUSAGE_NORMAL
			{0, 0, vk::Format::eR32G32Sfloat, 0x0C},    // D3DDECLUSAGE_TEXCOORD
		},
		// SBSPLightmapVertexCompressed
		{
			{},
			{},
			{},
		},
		// ModelUncompressed
		{
			{0, 0, vk::Format::eR32G32B32Sfloat, 0x00}, // D3DDECLUSAGE_POSITION
			{0, 0, vk::Format::eR32G32B32Sfloat, 0x0C}, // D3DDECLUSAGE_NORMAL
			{0, 0, vk::Format::eR32G32B32Sfloat, 0x18}, // D3DDECLUSAGE_BINORMAL
			{0, 0, vk::Format::eR32G32B32Sfloat, 0x24}, // D3DDECLUSAGE_TANGENT
			{0, 0, vk::Format::eR32G32Sfloat, 0x30},    // D3DDECLUSAGE_TEXCOORD

		},
		// ModelCompressed
		{
			{},
			{},
			{},
		},
		// 6
		{
			{},
			{},
			{},
		},
		// 7
		{
			{},
			{},
			{},
		},
		// 8
		{
			{},
			{},
			{},
		},
		// 9
		{
			{},
			{},
			{},
		},
		// 10
		{
			{},
			{},
			{},
		},
		// 11
		{
			{},
			{},
			{},
		},
		// 12
		{
			{},
			{},
			{},
		},
		// 13
		{
			{},
			{},
			{},
		},
		// 14
		{
			{},
			{},
			{},
		},
		// 15
		{
			{},
			{},
			{},
		},
		// 16
		{
			{},
			{},
			{},
		},
		// 17
		{
			{},
			{},
			{},
		},
		// 18
		{
			{},
			{},
			{},
		},
		// 19
		{
			{},
			{},
			{},
		},
	}};

std::vector<vk::VertexInputAttributeDescription>
	GetVertexInputAttributes(std::span<const Blam::VertexFormat> Formats)
{
	std::vector<vk::VertexInputAttributeDescription> Result;

	std::size_t BindingIndex  = 0;
	std::size_t LocationIndex = 0;
	for( const Blam::VertexFormat& CurFormat : Formats )
	{
		std::vector<vk::VertexInputAttributeDescription> CurVertexAttributes
			= VertexFormatAttributes.at(static_cast<std::size_t>(CurFormat));

		for( vk::VertexInputAttributeDescription& CurAttribute :
			 CurVertexAttributes )
		{
			CurAttribute.binding  = BindingIndex;
			CurAttribute.location = LocationIndex;
			++LocationIndex;
		}

		Result.insert(
			Result.end(), CurVertexAttributes.begin(), CurVertexAttributes.end()
		);

		++BindingIndex;
	}

	return Result;
}

} // namespace VkBlam