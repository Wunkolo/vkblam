#include "Vulkan/DescriptorUpdateBatch.hpp"
#include "Vulkan/StreamBuffer.hpp"
#include <VkBlam/Renderer.hpp>
#include <memory>
#include <optional>

namespace VkBlam
{
Renderer::Renderer(const Vulkan::Context& VulkanContext)
	: VulkanContext(VulkanContext)
{
}

Renderer::~Renderer()
{
}

std::optional<Renderer> Renderer::Create(
	const Vulkan::Context& VulkanContext, const RendererConfig& Config)
{
	Renderer NewRenderer(VulkanContext);

	//// Create Default Sampler
	{
		vk::SamplerCreateInfo SamplerInfo{};
		SamplerInfo.magFilter        = vk::Filter::eLinear;
		SamplerInfo.minFilter        = vk::Filter::eLinear;
		SamplerInfo.mipmapMode       = vk::SamplerMipmapMode::eLinear;
		SamplerInfo.addressModeU     = vk::SamplerAddressMode::eRepeat;
		SamplerInfo.addressModeV     = vk::SamplerAddressMode::eRepeat;
		SamplerInfo.addressModeW     = vk::SamplerAddressMode::eRepeat;
		SamplerInfo.mipLodBias       = 0.0f;
		SamplerInfo.anisotropyEnable = VK_FALSE;
		SamplerInfo.maxAnisotropy    = 1.0f;
		SamplerInfo.compareEnable    = VK_FALSE;
		SamplerInfo.compareOp        = vk::CompareOp::eAlways;
		SamplerInfo.minLod           = 0.0f;
		SamplerInfo.maxLod           = VK_LOD_CLAMP_NONE;
		SamplerInfo.borderColor      = vk::BorderColor::eFloatOpaqueWhite;
		SamplerInfo.unnormalizedCoordinates = VK_FALSE;

		if( auto CreateResult
			= VulkanContext.LogicalDevice.createSamplerUnique(SamplerInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewRenderer.DefaultSampler = std::move(CreateResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error creating default sampler: %s\n",
				vk::to_string(CreateResult.result).c_str());
			return {};
		}
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewRenderer.DefaultSampler.get(),
			"VkBlam::Renderer: Default Sampler");
	}

	NewRenderer.StreamBuffer = std::make_unique<Vulkan::StreamBuffer>(
		VulkanContext, Config.StreamBufferSize);

	NewRenderer.DescriptorUpdateBatch
		= std::make_unique<Vulkan::DescriptorUpdateBatch>(
			Vulkan::DescriptorUpdateBatch::Create(
				VulkanContext, Config.DescriptorWriteMax,
				Config.DescriptorCopyMax)
				.value());

	return {std::move(NewRenderer)};
}

} // namespace VkBlam