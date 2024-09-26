#include "Vulkan/DescriptorUpdateBatch.hpp"
#include "Vulkan/StreamBuffer.hpp"
#include <VkBlam/Rasterizer.hpp>
#include <memory>
#include <optional>

vk::UniqueRenderPass
	CreateMainRenderPass(vk::Device Device, vk::SampleCountFlagBits SampleCount)
{
	static const vk::AttachmentDescription Attachments[] = {
		// Color Attachment
		// We just care about it storing its color data, MSAA may resolve into
		// this
		vk::AttachmentDescription{
			.flags          = vk::AttachmentDescriptionFlags(),
			.format         = vk::Format::eR8G8B8A8Srgb,
			.samples        = vk::SampleCountFlagBits::e1,
			.loadOp         = vk::AttachmentLoadOp::eClear,
			.storeOp        = vk::AttachmentStoreOp::eStore,
			.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
			.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
			.initialLayout  = vk::ImageLayout::eUndefined,
			.finalLayout    = vk::ImageLayout::eTransferSrcOptimal
		},
		// Depth Attachment
		// Dont care about reading or storing it
		vk::AttachmentDescription{
			.flags          = vk::AttachmentDescriptionFlags(),
			.format         = vk::Format::eD32Sfloat,
			.samples        = SampleCount,
			.loadOp         = vk::AttachmentLoadOp::eClear,
			.storeOp        = vk::AttachmentStoreOp::eDontCare,
			.stencilLoadOp  = vk::AttachmentLoadOp::eClear,
			.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
			.initialLayout  = vk::ImageLayout::eUndefined,
			.finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal
		},
		// Color Attachment(MSAA)
		// Dont care about reading or storing it, since it gets resolved
		vk::AttachmentDescription{
			.flags          = vk::AttachmentDescriptionFlags(),
			.format         = vk::Format::eR8G8B8A8Srgb,
			.samples        = SampleCount,
			.loadOp         = vk::AttachmentLoadOp::eClear,
			.storeOp        = vk::AttachmentStoreOp::eDontCare,
			.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
			.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
			.initialLayout  = vk::ImageLayout::eUndefined,
			.finalLayout    = vk::ImageLayout::eColorAttachmentOptimal
		},
	};

	static const vk::AttachmentReference AttachmentRefs[] = {
		vk::AttachmentReference{
			.attachment = 0,
			.layout     = vk::ImageLayout::eColorAttachmentOptimal,
		},
		vk::AttachmentReference{
			.attachment = 1,
			.layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal,
		},
		vk::AttachmentReference{
			.attachment = 2,
			.layout     = vk::ImageLayout::eColorAttachmentOptimal,
		},
	};

	static const vk::SubpassDescription Subpasses[] = {
		vk::SubpassDescription{
			.colorAttachmentCount    = 1,
			.pColorAttachments       = &AttachmentRefs[2],
			.pResolveAttachments     = &AttachmentRefs[0],
			.pDepthStencilAttachment = &AttachmentRefs[1],
		},
	};

	static const vk::SubpassDependency SubpassDependencies[] = {
		// Wait for all Transfer-Writes to complete before any Vertex-Inputs
		// happen in subpass 0
		vk::SubpassDependency{
			.srcSubpass      = VK_SUBPASS_EXTERNAL,
			.dstSubpass      = 0,
			.srcStageMask    = vk::PipelineStageFlagBits::eTransfer,
			.dstStageMask    = vk::PipelineStageFlagBits::eVertexInput,
			.srcAccessMask   = vk::AccessFlagBits::eTransferWrite,
			.dstAccessMask   = vk::AccessFlagBits::eVertexAttributeRead,
			.dependencyFlags = vk::DependencyFlagBits::eByRegion
		},
	};

	static const vk::RenderPassCreateInfo RenderPassInfo = {
		.attachmentCount = std::size(Attachments),
		.pAttachments    = Attachments,
		.subpassCount    = std::size(Subpasses),
		.pSubpasses      = Subpasses,
		.dependencyCount = std::size(SubpassDependencies),
		.pDependencies   = SubpassDependencies,
	};

	if( auto CreateResult = Device.createRenderPassUnique(RenderPassInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		return std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render pass: %s\n",
			vk::to_string(CreateResult.result).c_str()
		);
		return {};
	}
}

namespace VkBlam
{
Rasterizer::Rasterizer(const Vulkan::Context& VulkanContext)
	: VulkanContext(VulkanContext)
{
}

Rasterizer::~Rasterizer()
{
}

const vk::RenderPass&
	Rasterizer::GetDefaultRenderPass(vk::SampleCountFlagBits SampleCount)
{
	if( DefaultRenderPasses.contains(SampleCount) )
	{
		return DefaultRenderPasses.at(SampleCount).get();
	}

	DefaultRenderPasses[SampleCount]
		= CreateMainRenderPass(VulkanContext.LogicalDevice, SampleCount);

	return DefaultRenderPasses[SampleCount].get();
}

std::optional<Rasterizer> Rasterizer::Create(
	const Vulkan::Context& VulkanContext, const RasterizerConfig& Config
)
{
	Rasterizer NewRasterizer(VulkanContext);

	NewRasterizer.StreamBuffer = std::make_unique<Vulkan::StreamBuffer>(
		VulkanContext, Config.StreamBufferSize
	);

	NewRasterizer.SamplerCache = std::make_unique<Vulkan::SamplerCache>(
		Vulkan::SamplerCache::Create(VulkanContext).value()
	);

	NewRasterizer.ShaderModuleCache
		= std::make_unique<Vulkan::ShaderModuleCache>(
			Vulkan::ShaderModuleCache::Create(VulkanContext).value()
		);

	NewRasterizer.DescriptorUpdateBatch = Vulkan::DescriptorUpdateBatch::Create(
		VulkanContext, Config.DescriptorWriteMax, Config.DescriptorCopyMax
	);

	return {std::move(NewRasterizer)};
}

} // namespace VkBlam