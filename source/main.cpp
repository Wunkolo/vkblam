#include "Blam/Util.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <span>

#include <Common/Alignment.hpp>

#include <mio/mmap.hpp>

#include <Blam/Blam.hpp>

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

static constexpr glm::uvec2              RenderSize = {2048, 2048};
static constexpr vk::SampleCountFlagBits RenderSamples
	= vk::SampleCountFlagBits::e4;

std::int32_t FindMemoryTypeIndex(
	vk::PhysicalDevice PhysicalDevice, std::uint32_t MemoryTypeMask,
	vk::MemoryPropertyFlags Properties,
	vk::MemoryPropertyFlags ExcludeProperties
	= vk::MemoryPropertyFlagBits::eProtected)
{
	const vk::PhysicalDeviceMemoryProperties DeviceMemoryProperties
		= PhysicalDevice.getMemoryProperties();
	// Iterate the physical device's memory types until we find a match
	for( std::size_t i = 0; i < DeviceMemoryProperties.memoryTypeCount; i++ )
	{
		if(
			// Is within memory type mask
			(((MemoryTypeMask >> i) & 0b1) == 0b1) &&
			// Has property flags
			(DeviceMemoryProperties.memoryTypes[i].propertyFlags & Properties)
				== Properties
			&&
			// None of the excluded properties are enabled
			!(DeviceMemoryProperties.memoryTypes[i].propertyFlags
			  & ExcludeProperties) )
		{
			return static_cast<std::uint32_t>(i);
		}
	}

	return -1;
}

vk::UniqueDescriptorPool CreateMainDescriptorPool(vk::Device LogicalDevice);

vk::UniqueRenderPass CreateMainRenderPass(
	vk::Device              LogicalDevice,
	vk::SampleCountFlagBits SampleCount = vk::SampleCountFlagBits::e1);

vk::UniqueFramebuffer CreateMainFrameBuffer(
	vk::Device LogicalDevice, vk::ImageView Color, vk::ImageView DepthAA,
	vk::ImageView ColorAA, glm::uvec2 ImageSize, vk::RenderPass RenderPass);

int main(int argc, char* argv[])
{
	if( argc < 2 )
	{
		// Not enough arguments
		return EXIT_FAILURE;
	}
	auto MapFile = mio::mmap_source(argv[1]);

	Blam::MapFile CurMap(std::span<const std::byte>(
		reinterpret_cast<const std::byte*>(MapFile.data()), MapFile.size()));

	std::fputs(Blam::ToString(CurMap.MapHeader).c_str(), stdout);
	std::fputs(Blam::ToString(CurMap.TagIndexHeader).c_str(), stdout);

	//// Create Instance
	vk::InstanceCreateInfo InstanceInfo = {};

	vk::ApplicationInfo ApplicationInfo = {};
	ApplicationInfo.apiVersion          = VK_API_VERSION_1_1;

	ApplicationInfo.pEngineName   = "vkblam";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	ApplicationInfo.pApplicationName   = "vkblam";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

	InstanceInfo.pApplicationInfo = &ApplicationInfo;

	vk::UniqueInstance Instance = {};

	if( auto CreateResult = vk::createInstanceUnique(InstanceInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		Instance = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating Vulkan instance: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	//// Pick physical device
	vk::PhysicalDevice PhysicalDevice = {};

	if( auto EnumerateResult = Instance->enumeratePhysicalDevices();
		EnumerateResult.result == vk::Result::eSuccess )
	{
		for( const auto& CurPhysicalDevice : EnumerateResult.value )
		{
			// Just pick the first physical device
			if( CurPhysicalDevice.getProperties().deviceType
				== vk::PhysicalDeviceType::eDiscreteGpu )
			{
				PhysicalDevice = CurPhysicalDevice;
				break;
			}
		}
	}
	else
	{
		std::fprintf(
			stderr, "Error enumerating physical devices: %s\n",
			vk::to_string(EnumerateResult.result).c_str());
		return EXIT_FAILURE;
	}

	//// Create Device
	vk::DeviceCreateInfo DeviceInfo = {};

	static const float QueuePriority = 1.0f;

	vk::DeviceQueueCreateInfo QueueInfo = {};
	QueueInfo.queueFamilyIndex          = 0;
	QueueInfo.queueCount                = 1;
	QueueInfo.pQueuePriorities          = &QueuePriority;

	DeviceInfo.queueCreateInfoCount = 1;
	DeviceInfo.pQueueCreateInfos    = &QueueInfo;

	vk::UniqueDevice Device = {};
	if( auto CreateResult = PhysicalDevice.createDeviceUnique(DeviceInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		Device = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating logical device: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	// Main Rendering queue
	vk::Queue RenderQueue = Device->getQueue(0, 0);

	//// Main Render Pass
	vk::UniqueRenderPass MainRenderPass
		= CreateMainRenderPass(Device.get(), RenderSamples);

	// Render Target images
	vk::UniqueImage RenderImage;

	vk::UniqueImage RenderImageAA;

	vk::UniqueImage RenderImageDepth;

	// Render-image, R8G8B8A8_SRGB
	vk::ImageCreateInfo RenderImageInfo = {};
	RenderImageInfo.imageType           = vk::ImageType::e2D;
	RenderImageInfo.format              = vk::Format::eR8G8B8A8Srgb;
	RenderImageInfo.extent      = vk::Extent3D(RenderSize.x, RenderSize.y, 1);
	RenderImageInfo.mipLevels   = 1;
	RenderImageInfo.arrayLayers = 1;
	RenderImageInfo.samples     = vk::SampleCountFlagBits::e1;
	RenderImageInfo.tiling      = vk::ImageTiling::eOptimal;
	RenderImageInfo.usage       = vk::ImageUsageFlagBits::eColorAttachment
						  | vk::ImageUsageFlagBits::eTransferSrc;
	RenderImageInfo.sharingMode   = vk::SharingMode::eExclusive;
	RenderImageInfo.initialLayout = vk::ImageLayout::eUndefined;

	// Render-image(MSAA), R8G8B8A8_SRGB
	vk::ImageCreateInfo RenderImageAAInfo = {};
	RenderImageAAInfo.imageType           = vk::ImageType::e2D;
	RenderImageAAInfo.format              = vk::Format::eR8G8B8A8Srgb;
	RenderImageAAInfo.samples             = RenderSamples;
	RenderImageAAInfo.extent      = vk::Extent3D(RenderSize.x, RenderSize.y, 1);
	RenderImageAAInfo.mipLevels   = 1;
	RenderImageAAInfo.arrayLayers = 1;
	RenderImageAAInfo.tiling      = vk::ImageTiling::eOptimal;
	RenderImageAAInfo.usage       = vk::ImageUsageFlagBits::eColorAttachment;
	RenderImageAAInfo.sharingMode = vk::SharingMode::eExclusive;
	RenderImageAAInfo.initialLayout = vk::ImageLayout::eUndefined;

	// Render-image-depth(MSAA), D32_sfloat
	vk::ImageCreateInfo RenderImageDepthInfo = {};
	RenderImageDepthInfo.imageType           = vk::ImageType::e2D;
	RenderImageDepthInfo.format              = vk::Format::eD32Sfloat;
	RenderImageDepthInfo.samples             = RenderSamples;
	RenderImageDepthInfo.extent = vk::Extent3D(RenderSize.x, RenderSize.y, 1);
	RenderImageDepthInfo.mipLevels   = 1;
	RenderImageDepthInfo.arrayLayers = 1;
	RenderImageDepthInfo.tiling      = vk::ImageTiling::eOptimal;
	RenderImageDepthInfo.usage
		= vk::ImageUsageFlagBits::eDepthStencilAttachment;
	RenderImageInfo.sharingMode   = vk::SharingMode::eExclusive;
	RenderImageInfo.initialLayout = vk::ImageLayout::eUndefined;

	if( auto CreateResult = Device->createImageUnique(RenderImageInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImage = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	if( auto CreateResult = Device->createImageUnique(RenderImageAAInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageAA = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	if( auto CreateResult = Device->createImageUnique(RenderImageDepthInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageDepth = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	// Allocate all the memory we need for these images up-front into a single
	// heap.
	vk::UniqueDeviceMemory ImageHeapMemory;
	{
		const static vk::Image Images[]
			= {RenderImage.get(), RenderImageAA.get(), RenderImageDepth.get()};
		std::size_t                          ImageHeapMemorySize = 0;
		std::uint32_t                        ImageHeapMemoryMask = 0xFFFFFFFF;
		std::vector<vk::BindImageMemoryInfo> ImageHeapBinds;

		// We're putting these images right after each other, so we need to
		// ensure they are far apart enough to not be considered aliasing
		const std::size_t BufferImageGranularity
			= PhysicalDevice.getProperties().limits.bufferImageGranularity;

		for( const vk::Image& CurImage : Images )
		{
			const vk::MemoryRequirements MemReqs
				= Device->getImageMemoryRequirements(CurImage);

			// Accumulate a mask of all the memory types we can use for these
			// images
			ImageHeapMemoryMask &= MemReqs.memoryTypeBits;

			// Padd up the image-size so they are not padding
			const std::size_t ImageSize
				= Common::AlignUp(MemReqs.size, BufferImageGranularity);

			// Put nullptr for the device memory for now
			ImageHeapBinds.emplace_back(vk::BindImageMemoryInfo{
				CurImage, nullptr, ImageHeapMemorySize});
			ImageHeapMemorySize += ImageSize;
		}

		vk::MemoryAllocateInfo ImageHeapAllocInfo;
		ImageHeapAllocInfo.allocationSize  = ImageHeapMemorySize;
		ImageHeapAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(
			PhysicalDevice, ImageHeapMemoryMask,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		if( auto AllocResult = Device->allocateMemoryUnique(ImageHeapAllocInfo);
			AllocResult.result == vk::Result::eSuccess )
		{
			ImageHeapMemory = std::move(AllocResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error allocating image memory: %s\n",
				vk::to_string(AllocResult.result).c_str());
			return EXIT_FAILURE;
		}

		// Now we can assign the device memory to the images
		for( vk::BindImageMemoryInfo& CurBind : ImageHeapBinds )
		{
			CurBind.memory = ImageHeapMemory.get();
		}

		// Now bind them all in one go
		if( auto BindResult = Device->bindImageMemory2(ImageHeapBinds);
			BindResult == vk::Result::eSuccess )
		{
			// Binding memory succeeded
		}
		else
		{
			std::fprintf(
				stderr, "Error allocating binding image memory: %s\n",
				vk::to_string(BindResult).c_str());
			return EXIT_FAILURE;
		}
	}

	//// Image Views
	// Create the image views for the render-targets
	vk::ImageViewCreateInfo ImageViewInfoTemplate{};
	ImageViewInfoTemplate.viewType     = vk::ImageViewType::e2D;
	ImageViewInfoTemplate.components.r = vk::ComponentSwizzle::eR;
	ImageViewInfoTemplate.components.g = vk::ComponentSwizzle::eG;
	ImageViewInfoTemplate.components.b = vk::ComponentSwizzle::eB;
	ImageViewInfoTemplate.components.a = vk::ComponentSwizzle::eA;
	ImageViewInfoTemplate.subresourceRange.aspectMask
		= vk::ImageAspectFlagBits::eColor;
	ImageViewInfoTemplate.subresourceRange.baseMipLevel   = 0;
	ImageViewInfoTemplate.subresourceRange.levelCount     = 1;
	ImageViewInfoTemplate.subresourceRange.baseArrayLayer = 0;
	ImageViewInfoTemplate.subresourceRange.layerCount     = 1;

	ImageViewInfoTemplate.image  = RenderImage.get();
	ImageViewInfoTemplate.format = RenderImageInfo.format;

	vk::UniqueImageView RenderImageView = {};
	if( auto CreateResult
		= Device->createImageViewUnique(ImageViewInfoTemplate);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageView = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target view: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	ImageViewInfoTemplate.image           = RenderImageAA.get();
	ImageViewInfoTemplate.format          = RenderImageAAInfo.format;
	vk::UniqueImageView RenderImageAAView = {};
	if( auto CreateResult
		= Device->createImageViewUnique(ImageViewInfoTemplate);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageAAView = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target view: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	ImageViewInfoTemplate.image  = RenderImageDepth.get();
	ImageViewInfoTemplate.format = RenderImageDepthInfo.format;
	ImageViewInfoTemplate.subresourceRange.aspectMask
		= vk::ImageAspectFlagBits::eDepth;
	vk::UniqueImageView RenderImageDepthView = {};
	if( auto CreateResult
		= Device->createImageViewUnique(ImageViewInfoTemplate);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageDepthView = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target view: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	//// MainFrameBuffer
	vk::UniqueFramebuffer RenderFramebuffer = CreateMainFrameBuffer(
		Device.get(), RenderImageView.get(), RenderImageDepthView.get(),
		RenderImageAAView.get(), RenderSize, MainRenderPass.get());

	//// Create Descriptor Pool
	vk::UniqueDescriptorPool MainDescriptorPool
		= CreateMainDescriptorPool(Device.get());

	//// Create Command Pool
	vk::CommandPoolCreateInfo CommandPoolInfo = {};
	CommandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	CommandPoolInfo.queueFamilyIndex = 0;

	vk::UniqueCommandPool CommandPool = {};
	if( auto CreateResult = Device->createCommandPoolUnique(CommandPoolInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		CommandPool = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating command pool: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	//// Create Command Buffer
	vk::CommandBufferAllocateInfo CommandBufferInfo = {};
	CommandBufferInfo.commandPool                   = CommandPool.get();
	CommandBufferInfo.level              = vk::CommandBufferLevel::ePrimary;
	CommandBufferInfo.commandBufferCount = 1;

	vk::UniqueCommandBuffer CommandBuffer = {};

	if( auto AllocateResult
		= Device->allocateCommandBuffersUnique(CommandBufferInfo);
		AllocateResult.result == vk::Result::eSuccess )
	{
		CommandBuffer = std::move(AllocateResult.value[0]);
	}
	else
	{
		std::fprintf(
			stderr, "Error allocating command buffer: %s\n",
			vk::to_string(AllocateResult.result).c_str());
		return EXIT_FAILURE;
	}

	if( auto BeginResult = CommandBuffer->begin(vk::CommandBufferBeginInfo{});
		BeginResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error beginning command buffer: %s\n",
			vk::to_string(BeginResult).c_str());
		return EXIT_FAILURE;
	}

	{
		vk::RenderPassBeginInfo RenderBeginInfo   = {};
		RenderBeginInfo.renderPass                = MainRenderPass.get();
		static const vk::ClearValue ClearColors[] = {
			vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
			vk::ClearDepthStencilValue(1.0f, 0),
			vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
		};
		RenderBeginInfo.pClearValues    = ClearColors;
		RenderBeginInfo.clearValueCount = std::extent_v<decltype(ClearColors)>;
		RenderBeginInfo.renderArea.extent.width  = RenderSize.x;
		RenderBeginInfo.renderArea.extent.height = RenderSize.y;
		RenderBeginInfo.framebuffer              = RenderFramebuffer.get();
		CommandBuffer->beginRenderPass(
			RenderBeginInfo, vk::SubpassContents::eInline);

		CommandBuffer->endRenderPass();
	}

	if( auto EndResult = CommandBuffer->end();
		EndResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error ending command buffer: %s\n",
			vk::to_string(EndResult).c_str());
		return EXIT_FAILURE;
	}

	// Submit work
	vk::UniqueFence Fence = {};
	if( auto CreateResult = Device->createFenceUnique({});
		CreateResult.result == vk::Result::eSuccess )
	{
		Fence = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating fence: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	vk::SubmitInfo SubmitInfo     = {};
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers    = &CommandBuffer.get();

	if( auto SubmitResult = RenderQueue.submit(SubmitInfo, Fence.get());
		SubmitResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error submitting command buffer: %s\n",
			vk::to_string(SubmitResult).c_str());
		return EXIT_FAILURE;
	}

	// Wait for it
	if( auto WaitResult = Device->waitForFences(Fence.get(), true, ~0ULL);
		WaitResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error waiting for fence: %s\n",
			vk::to_string(WaitResult).c_str());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

vk::UniqueDescriptorPool CreateMainDescriptorPool(vk::Device LogicalDevice)
{
	const vk::DescriptorPoolSize PoolSizes[] = {
		{vk::DescriptorType::eStorageBuffer, 32},
		{vk::DescriptorType::eSampler, 32},
		{vk::DescriptorType::eSampledImage, 32},
	};

	vk::DescriptorPoolCreateInfo DescriptorPoolInfo{};
	DescriptorPoolInfo.flags
		= vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	DescriptorPoolInfo.poolSizeCount = std::extent<decltype(PoolSizes)>();
	DescriptorPoolInfo.pPoolSizes    = PoolSizes;
	DescriptorPoolInfo.maxSets       = 0xFF;

	if( auto CreateResult
		= LogicalDevice.createDescriptorPoolUnique(DescriptorPoolInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		return std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating descriptor pool: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return {};
	}
}

vk::UniqueRenderPass CreateMainRenderPass(
	vk::Device LogicalDevice, vk::SampleCountFlagBits SampleCount)
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
			vk::ImageLayout::eTransferSrcOptimal),
		// Depth Attachment
		// Dont care about reading or storing it
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(), vk::Format::eD32Sfloat,
			SampleCount, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal),
		// Color Attachment(MSAA)
		// We just care about it storing its color data
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(), vk::Format::eR8G8B8A8Srgb,
			SampleCount, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal)};

	const vk::AttachmentReference AttachmentRefs[] = {
		vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(
			1, vk::ImageLayout::eDepthStencilAttachmentOptimal),
		vk::AttachmentReference(2, vk::ImageLayout::eColorAttachmentOptimal),
	};

	RenderPassInfo.attachmentCount = std::extent_v<decltype(Attachments)>;
	RenderPassInfo.pAttachments    = Attachments;

	vk::SubpassDescription Subpasses[1] = {{}};

	// First subpass
	Subpasses[0].colorAttachmentCount    = 1;
	Subpasses[0].pColorAttachments       = &AttachmentRefs[2];
	Subpasses[0].pDepthStencilAttachment = &AttachmentRefs[1];
	Subpasses[0].pResolveAttachments     = &AttachmentRefs[0];

	RenderPassInfo.subpassCount = std::extent_v<decltype(Subpasses)>;
	RenderPassInfo.pSubpasses   = Subpasses;

	const vk::SubpassDependency SubpassDependencies[] = {vk::SubpassDependency(
		VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eVertexInput,
		vk::AccessFlagBits::eShaderWrite,
		vk::AccessFlagBits::eVertexAttributeRead, vk::DependencyFlags())};

	RenderPassInfo.dependencyCount
		= std::extent_v<decltype(SubpassDependencies)>;
	RenderPassInfo.pDependencies = SubpassDependencies;

	if( auto CreateResult
		= LogicalDevice.createRenderPassUnique(RenderPassInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		return std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render pass: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return {};
	}
}

vk::UniqueFramebuffer CreateMainFrameBuffer(
	vk::Device LogicalDevice, vk::ImageView Color, vk::ImageView DepthAA,
	vk::ImageView ColorAA, glm::uvec2 ImageSize, vk::RenderPass RenderPass)
{
	vk::FramebufferCreateInfo FramebufferInfo = {};

	FramebufferInfo.width      = ImageSize.x;
	FramebufferInfo.height     = ImageSize.y;
	FramebufferInfo.layers     = 1;
	FramebufferInfo.renderPass = RenderPass;

	const vk::ImageView Attachments[] = {Color, DepthAA, ColorAA};
	FramebufferInfo.attachmentCount   = std::extent_v<decltype(Attachments)>;
	FramebufferInfo.pAttachments      = Attachments;

	if( auto CreateResult
		= LogicalDevice.createFramebufferUnique(FramebufferInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		return std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating framebuffer: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return {};
	}
}