#include "Vulkan/DescriptorUpdateBatch.hpp"
#include "Vulkan/StreamBuffer.hpp"
#include <VkBlam/Renderer.hpp>
#include <memory>
#include <optional>

vk::UniqueRenderPass
	CreateMainRenderPass(vk::Device Device, vk::SampleCountFlagBits SampleCount)
{
	vk::RenderPassCreateInfo RenderPassInfo = {};

	const vk::AttachmentDescription Attachments[] = {
		// Color Attachment
		// We just care about it storing its color data
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(), vk::Format::eR8G8B8A8Srgb,
			vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferSrcOptimal
		),
		// Depth Attachment
		// Dont care about reading or storing it
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(), vk::Format::eD32Sfloat,
			SampleCount, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		),
		// Color Attachment(MSAA)
		// We just care about it storing its color data
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(), vk::Format::eR8G8B8A8Srgb,
			SampleCount, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal
		)};

	const vk::AttachmentReference AttachmentRefs[] = {
		vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(
			1, vk::ImageLayout::eDepthStencilAttachmentOptimal
		),
		vk::AttachmentReference(2, vk::ImageLayout::eColorAttachmentOptimal),
	};

	RenderPassInfo.attachmentCount = std::size(Attachments);
	RenderPassInfo.pAttachments    = Attachments;

	vk::SubpassDescription Subpasses[1] = {{}};

	// First subpass
	Subpasses[0].colorAttachmentCount    = 1;
	Subpasses[0].pColorAttachments       = &AttachmentRefs[2];
	Subpasses[0].pDepthStencilAttachment = &AttachmentRefs[1];
	Subpasses[0].pResolveAttachments     = &AttachmentRefs[0];

	RenderPassInfo.subpassCount = std::size(Subpasses);
	RenderPassInfo.pSubpasses   = Subpasses;

	const vk::SubpassDependency SubpassDependencies[] = {vk::SubpassDependency(
		VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eVertexInput,
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eVertexAttributeRead,
		vk::DependencyFlagBits::eByRegion
	)};

	RenderPassInfo.dependencyCount = std::size(SubpassDependencies);
	RenderPassInfo.pDependencies   = SubpassDependencies;

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

	NewRenderer.DescriptorUpdateBatch
		= std::make_unique<Vulkan::DescriptorUpdateBatch>(
			Vulkan::DescriptorUpdateBatch::Create(
				VulkanContext, Config.DescriptorWriteMax,
				Config.DescriptorCopyMax
			)
				.value()
		);

	return {std::move(NewRenderer)};
}

} // namespace VkBlam