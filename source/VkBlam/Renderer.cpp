#include "Vulkan/DescriptorUpdateBatch.hpp"
#include "Vulkan/StreamBuffer.hpp"
#include <VkBlam/Renderer.hpp>
#include <memory>
#include <optional>

vk::UniqueRenderPass
	CreateMainRenderPass(vk::Device Device, vk::SampleCountFlagBits SampleCount)
{
	const vk::AttachmentDescription Attachments[] = {
		// Color Attachment
		// We just care about it storing its color data
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
		// We just care about it storing its color data
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

	const vk::AttachmentReference AttachmentRefs[] = {
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

	const vk::SubpassDescription Subpasses[1] = {{
		.colorAttachmentCount    = 1,
		.pColorAttachments       = &AttachmentRefs[2],
		.pResolveAttachments     = &AttachmentRefs[0],
		.pDepthStencilAttachment = &AttachmentRefs[1],
	}};

	const vk::SubpassDependency SubpassDependencies[] = {vk::SubpassDependency{
		.srcSubpass      = VK_SUBPASS_EXTERNAL,
		.dstSubpass      = 0,
		.srcStageMask    = vk::PipelineStageFlagBits::eTransfer,
		.dstStageMask    = vk::PipelineStageFlagBits::eVertexInput,
		.srcAccessMask   = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask   = vk::AccessFlagBits::eVertexAttributeRead,
		.dependencyFlags = vk::DependencyFlagBits::eByRegion
	}};

	const vk::RenderPassCreateInfo RenderPassInfo = {
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
Renderer::Renderer(const Vulkan::Context& VulkanContext)
	: VulkanContext(VulkanContext)
{
}

Renderer::~Renderer()
{
}

const vk::RenderPass&
	Renderer::GetDefaultRenderPass(vk::SampleCountFlagBits SampleCount)
{
	if( DefaultRenderPasses.contains(SampleCount) )
	{
		return DefaultRenderPasses.at(SampleCount).get();
	}

	DefaultRenderPasses[SampleCount]
		= CreateMainRenderPass(VulkanContext.LogicalDevice, SampleCount);

	return DefaultRenderPasses[SampleCount].get();
}

std::optional<Renderer> Renderer::Create(
	const Vulkan::Context& VulkanContext, const RendererConfig& Config
)
{
	Renderer NewRenderer(VulkanContext);

	NewRenderer.StreamBuffer = std::make_unique<Vulkan::StreamBuffer>(
		VulkanContext, Config.StreamBufferSize
	);

	NewRenderer.SamplerCache = std::make_unique<Vulkan::SamplerCache>(
		Vulkan::SamplerCache::Create(VulkanContext).value()
	);

	NewRenderer.ShaderModuleCache = std::make_unique<Vulkan::ShaderModuleCache>(
		Vulkan::ShaderModuleCache::Create(VulkanContext).value()
	);

	NewRenderer.DescriptorUpdateBatch = Vulkan::DescriptorUpdateBatch::Create(
		VulkanContext, Config.DescriptorWriteMax, Config.DescriptorCopyMax
	);

	return {std::move(NewRenderer)};
}

} // namespace VkBlam