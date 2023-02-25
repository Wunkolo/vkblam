#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <map>
#include <span>

#include <Common/Alignment.hpp>
#include <Common/Format.hpp>
#include <Common/Literals.hpp>

#include <Vulkan/Debug.hpp>
#include <Vulkan/DescriptorHeap.hpp>
#include <Vulkan/Memory.hpp>
#include <Vulkan/Pipeline.hpp>
#include <Vulkan/VulkanAPI.hpp>

#include <VkBlam/Renderer.hpp>
#include <VkBlam/Scene.hpp>
#include <VkBlam/Shaders/ShaderEnvironment.hpp>
#include <VkBlam/VkBlam.hpp>
#include <VkBlam/World.hpp>

#include <mio/mmap.hpp>

#include <Blam/Blam.hpp>

#include "stb_image_write.h"

// Enable render-doc captures on non-windows for now
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(NDEBUG)
#define CAPTURE
#endif

#ifdef CAPTURE
#include <dlfcn.h>
#include <renderdoc_app.h>
RENDERDOC_API_1_4_1* rdoc_api = NULL;
#endif

static constexpr glm::uvec2 RenderSize = {1024, 1024};

vk::UniqueRenderPass CreateMainRenderPass(
	vk::Device              Device,
	vk::SampleCountFlagBits SampleCount = vk::SampleCountFlagBits::e1);

vk::UniqueFramebuffer CreateMainFrameBuffer(
	vk::Device Device, vk::ImageView Color, vk::ImageView DepthAA,
	vk::ImageView ColorAA, glm::uvec2 ImageSize, vk::RenderPass RenderPass);

std::string FormatDeviceCaps(vk::PhysicalDevice PhysicalDevice);

int main(int argc, char* argv[])
{
	using namespace Common::Literals;

	if( argc < 3 )
	{
		// Not enough arguments
		return EXIT_FAILURE;
	}

#ifdef CAPTURE
	void* mod = dlopen("librenderdoc.so", RTLD_NOW);
	char* msg = dlerror();
	if( msg )
		std::puts(msg);
	if( mod )
	{
		pRENDERDOC_GetAPI RENDERDOC_GetAPI
			= (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
		int ret
			= RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void**)&rdoc_api);
		rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, 1);
		rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, 1);
		rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_SaveAllInitials, 1);
		rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);
		assert(ret == 1);
	}
#endif

	std::filesystem::path MapPath(argv[1]);
	std::filesystem::path BitmapPath(argv[2]);

	auto MapFile    = mio::mmap_source(MapPath.c_str());
	auto BitmapFile = mio::mmap_source(BitmapPath.c_str());

	Blam::MapFile CurMap(
		std::span<const std::byte>(
			reinterpret_cast<const std::byte*>(MapFile.data()), MapFile.size()),
		std::span<const std::byte>(
			reinterpret_cast<const std::byte*>(BitmapFile.data()),
			BitmapFile.size()));

	VkBlam::World CurWorld = VkBlam::World::Create(CurMap).value();

	std::fputs(Blam::ToString(CurWorld.GetMapFile().MapHeader).c_str(), stdout);
	std::fputs(
		Blam::ToString(CurWorld.GetMapFile().TagIndexHeader).c_str(), stdout);

	//// Create Instance

	vk::ApplicationInfo ApplicationInfo = {};
	ApplicationInfo.apiVersion          = VK_API_VERSION_1_1;

	ApplicationInfo.pEngineName   = "VkBlam";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	ApplicationInfo.pApplicationName   = "VkBlam";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

	vk::InstanceCreateInfo InstanceInfo = {};

	InstanceInfo.pApplicationInfo = &ApplicationInfo;

	static const std::array InstanceExtensions = std::to_array({
#if defined(__APPLE__)
		VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	});

#if defined(__APPLE__)
	InstanceInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

	InstanceInfo.ppEnabledExtensionNames = InstanceExtensions.data();
	InstanceInfo.enabledExtensionCount   = InstanceExtensions.size();

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
	VULKAN_HPP_DEFAULT_DISPATCHER.init(Instance.get());

	//// Pick physical device
	vk::PhysicalDevice PhysicalDevice = {};

	if( auto EnumerateResult = Instance->enumeratePhysicalDevices();
		EnumerateResult.result == vk::Result::eSuccess )
	{
		std::vector<vk::PhysicalDevice> PhysicalDevices
			= std::move(EnumerateResult.value);

		// Prefer Discrete GPUs
		const auto IsDiscrete
			= [](const vk::PhysicalDevice& PhysicalDevice) -> bool {
			return PhysicalDevice.getProperties().deviceType
				== vk::PhysicalDeviceType::eDiscreteGpu;
		};

		std::partition(
			PhysicalDevices.begin(), PhysicalDevices.end(), IsDiscrete);

		// Pick the "best" out of all of the previous criteria
		PhysicalDevice = PhysicalDevices.front();
	}
	else
	{
		std::fprintf(
			stderr, "Error enumerating physical devices: %s\n",
			vk::to_string(EnumerateResult.result).c_str());
		return EXIT_FAILURE;
	}

	std::fprintf(
		stdout,
		"---\n"
		"%s"
		"---\n",
		FormatDeviceCaps(PhysicalDevice).c_str());

	//// Create Device
	vk::DeviceCreateInfo DeviceInfo = {};

	static const char* DeviceExtensions[]
		= {VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME};
	DeviceInfo.ppEnabledExtensionNames = DeviceExtensions;
	DeviceInfo.enabledExtensionCount   = std::size(DeviceExtensions);

	vk::StructureChain<
		vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceTimelineSemaphoreFeatures>
		DeviceFeatureChain = {};

	auto& DeviceFeatures
		= DeviceFeatureChain.get<vk::PhysicalDeviceFeatures2>().features;
	DeviceFeatures.samplerAnisotropy = true;
	DeviceFeatures.sampleRateShading = true;
	DeviceFeatures.wideLines         = true;
	DeviceFeatures.fillModeNonSolid  = true;

	auto& DeviceTimelineFeatures
		= DeviceFeatureChain.get<vk::PhysicalDeviceTimelineSemaphoreFeatures>();
	DeviceTimelineFeatures.timelineSemaphore = true;

	DeviceInfo.pNext = &DeviceFeatureChain.get();

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

	VULKAN_HPP_DEFAULT_DISPATCHER.init(Device.get());

#ifdef CAPTURE
	if( rdoc_api )
		rdoc_api->StartFrameCapture(
			*(void**)(VkInstance)(*Instance.operator->()), NULL);
#endif

	// Main Rendering queue
	vk::Queue RenderQueue = Device->getQueue(0, 0);
	// Todo: Pick the fastest transfer queue here
	vk::Queue TransferQueue = Device->getQueue(0, 0);

	const Vulkan::Context VulkanContext{
		Device.get(), PhysicalDevice, RenderQueue, 0, TransferQueue, 0};

	VkBlam::Renderer Renderer = VkBlam::Renderer::Create(VulkanContext).value();

	VkBlam::Scene CurScene = VkBlam::Scene::Create(Renderer, CurWorld).value();

	//// Main Render Pass
	vk::UniqueRenderPass MainRenderPass
		= CreateMainRenderPass(Device.get(), VkBlam::RenderSamples);

	vk::UniqueBuffer       StagingBuffer       = {};
	vk::UniqueDeviceMemory StagingBufferMemory = {};

	vk::BufferCreateInfo StagingBufferInfo = {};
	StagingBufferInfo.size
		= RenderSize.x * RenderSize.y * sizeof(std::uint32_t);
	StagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst
							| vk::BufferUsageFlagBits::eTransferSrc;

	if( auto CreateResult = Device->createBufferUnique(StagingBufferInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		StagingBuffer = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating staging buffer: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	Vulkan::SetObjectName(
		Device.get(), StagingBuffer.get(), "Staging Buffer( %s )",
		Common::FormatByteCount(StagingBufferInfo.size).c_str());

	// Allocate memory for staging buffer
	{
		const vk::MemoryRequirements StagingBufferMemoryRequirements
			= Device->getBufferMemoryRequirements(StagingBuffer.get());

		vk::MemoryAllocateInfo StagingBufferAllocInfo = {};
		StagingBufferAllocInfo.allocationSize
			= StagingBufferMemoryRequirements.size;
		StagingBufferAllocInfo.memoryTypeIndex = Vulkan::FindMemoryTypeIndex(
			PhysicalDevice, StagingBufferMemoryRequirements.memoryTypeBits,
			vk::MemoryPropertyFlagBits::eHostVisible
				| vk::MemoryPropertyFlagBits::eHostCoherent);

		if( auto AllocResult
			= Device->allocateMemoryUnique(StagingBufferAllocInfo);
			AllocResult.result == vk::Result::eSuccess )
		{
			StagingBufferMemory = std::move(AllocResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error allocating memory for staging buffer: %s\n",
				vk::to_string(AllocResult.result).c_str());
			return EXIT_FAILURE;
		}

		if( auto BindResult = Device->bindBufferMemory(
				StagingBuffer.get(), StagingBufferMemory.get(), 0);
			BindResult == vk::Result::eSuccess )
		{
			// Successfully binded memory to buffer
		}
		else
		{
			std::fprintf(
				stderr, "Error binding memory to staging buffer: %s\n",
				vk::to_string(BindResult).c_str());
			return EXIT_FAILURE;
		}
	}

	std::span<std::byte> StagingBufferData;
	if( auto MapResult = Device->mapMemory(
			StagingBufferMemory.get(), 0, StagingBufferInfo.size);
		MapResult.result == vk::Result::eSuccess )
	{
		StagingBufferData = std::span<std::byte>(
			reinterpret_cast<std::byte*>(MapResult.value),
			StagingBufferInfo.size);
	}
	else
	{
		std::fprintf(
			stderr, "Error mapping staging buffer memory: %s\n",
			vk::to_string(MapResult.result).c_str());
		return EXIT_FAILURE;
	}

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
	RenderImageAAInfo.samples             = VkBlam::RenderSamples;
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
	RenderImageDepthInfo.samples             = VkBlam::RenderSamples;
	RenderImageDepthInfo.extent = vk::Extent3D(RenderSize.x, RenderSize.y, 1);
	RenderImageDepthInfo.mipLevels   = 1;
	RenderImageDepthInfo.arrayLayers = 1;
	RenderImageDepthInfo.tiling      = vk::ImageTiling::eOptimal;
	RenderImageDepthInfo.usage
		= vk::ImageUsageFlagBits::eDepthStencilAttachment;
	RenderImageDepthInfo.sharingMode   = vk::SharingMode::eExclusive;
	RenderImageDepthInfo.initialLayout = vk::ImageLayout::eUndefined;

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
	Vulkan::SetObjectName(
		Device.get(), RenderImage.get(), "Render Image Resolve");

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
	Vulkan::SetObjectName(
		Device.get(), RenderImageAA.get(), "Render Image(AA)");

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
	Vulkan::SetObjectName(
		Device.get(), RenderImageDepth.get(), "Render Image Depth(AA)");

	std::vector<vk::Image> ImageHeapTargets = {};
	ImageHeapTargets.push_back(RenderImage.get());
	ImageHeapTargets.push_back(RenderImageAA.get());
	ImageHeapTargets.push_back(RenderImageDepth.get());

	// Allocate all the memory we need for these images up-front into a single
	// heap.
	vk::UniqueDeviceMemory ImageHeapMemory = {};

	if( auto [Result, Value] = Vulkan::CommitImageHeap(
			Device.get(), PhysicalDevice, ImageHeapTargets);
		Result == vk::Result::eSuccess )
	{
		ImageHeapMemory = std::move(Value);
	}
	else
	{
		std::fprintf(
			stderr, "Error committing image memory: %s\n",
			vk::to_string(Result).c_str());
		return EXIT_FAILURE;
	}

	//// Image Views
	// Create the image views for the render-targets
	vk::ImageViewCreateInfo ImageViewInfoTemplate = {};
	ImageViewInfoTemplate.viewType                = vk::ImageViewType::e2D;
	ImageViewInfoTemplate.components.r            = vk::ComponentSwizzle::eR;
	ImageViewInfoTemplate.components.g            = vk::ComponentSwizzle::eG;
	ImageViewInfoTemplate.components.b            = vk::ComponentSwizzle::eB;
	ImageViewInfoTemplate.components.a            = vk::ComponentSwizzle::eA;
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

	// VkBlam::ShaderEnvironment ShaderEnvironments(
	// 	VulkanContext, BitmapHeap, Renderer.GetDescriptorUpdateBatch());

	// CurWorld.GetMapFile().VisitTagClass<Blam::TagClass::ShaderEnvironment>(
	// 	[&](const Blam::TagIndexEntry& TagEntry,
	// 		const Blam::Tag<Blam::TagClass::ShaderEnvironment>&
	// 			ShaderEnvironment) -> void {
	// 		ShaderEnvironments.RegisterShader(TagEntry, ShaderEnvironment);
	// 	});

	Renderer.GetDescriptorUpdateBatch().Flush();

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
		Vulkan::DebugLabelScope FrameScope(
			CommandBuffer.get(), {1.0, 0.0, 1.0, 1.0}, "Frame");

		{
			Vulkan::DebugLabelScope RenderPassScope(
				CommandBuffer.get(), {1.0, 1.0, 0.0, 1.0}, "Main Render Pass");

			vk::RenderPassBeginInfo RenderBeginInfo   = {};
			RenderBeginInfo.renderPass                = MainRenderPass.get();
			static const vk::ClearValue ClearColors[] = {
				vk::ClearColorValue(
					std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
				vk::ClearDepthStencilValue(1.0f, 0),
				vk::ClearColorValue(
					std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
			};
			RenderBeginInfo.pClearValues             = ClearColors;
			RenderBeginInfo.clearValueCount          = std::size(ClearColors);
			RenderBeginInfo.renderArea.extent.width  = RenderSize.x;
			RenderBeginInfo.renderArea.extent.height = RenderSize.y;
			RenderBeginInfo.framebuffer              = RenderFramebuffer.get();
			CommandBuffer->beginRenderPass(
				RenderBeginInfo, vk::SubpassContents::eInline);

			// Draw

			const auto WorldBounds = CurWorld.GetWorldBounds();

			const glm::vec3 WorldCenter
				= glm::mix(WorldBounds[0], WorldBounds[1], 0.5);

			const glm::f32 MaxExtent
				= glm::compMax(glm::xyz(WorldBounds[1] - WorldBounds[0]))
				/ 2.0f;

			const auto View = glm::lookAt<glm::f32>(
				glm::vec3(WorldBounds[1].x, WorldBounds[1].y, MaxExtent) * 1.5f,
				// glm::vec3(WorldCenter.x, WorldCenter.y, WorldBoundMax.z),
				glm::vec3(WorldCenter.x, WorldCenter.y, WorldBounds[0].z),
				glm::vec3(0, 0, 1));

			const auto Projection
				// = glm::ortho<glm::f32>(
				// 	-MaxExtent, MaxExtent, -MaxExtent, MaxExtent, 0.0f,
				// 	WorldBoundMax.z - WorldBoundMin.z);
				= glm::perspective<glm::f32>(
					glm::radians(60.0f),
					static_cast<float>(RenderSize.x) / RenderSize.y, 1.0f,
					1000.0f);

			VkBlam::SceneView SceneView(View, Projection, RenderSize);

			CurScene.Render(SceneView, CommandBuffer.get());

			CommandBuffer->endRenderPass();
		}

		// Wait for image data to be ready
		{
			Vulkan::DebugLabelScope DebugCopyScope(
				CommandBuffer.get(), {1.0, 1.0, 0.0, 1.0},
				"Upload framebuffer to staging buffer");
			CommandBuffer->pipelineBarrier(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), {},
				{},
				{// Source Image
				 vk::ImageMemoryBarrier(
					 vk::AccessFlagBits::eColorAttachmentWrite,
					 vk::AccessFlagBits::eTransferRead,
					 vk::ImageLayout::eTransferSrcOptimal,
					 vk::ImageLayout::eTransferSrcOptimal,
					 VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
					 RenderImage.get(),
					 vk::ImageSubresourceRange(
						 vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))});
			CommandBuffer->copyImageToBuffer(
				RenderImage.get(), vk::ImageLayout::eTransferSrcOptimal,
				StagingBuffer.get(),
				{vk::BufferImageCopy(
					0, RenderSize.x, RenderSize.y,
					vk::ImageSubresourceLayers(
						vk::ImageAspectFlagBits::eColor, 0, 0, 1),
					vk::Offset3D(),
					vk::Extent3D(RenderSize.x, RenderSize.y, 1))});
		}
	}

	if( auto EndResult = CommandBuffer->end();
		EndResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error ending command buffer: %s\n",
			vk::to_string(EndResult).c_str());
		return EXIT_FAILURE;
	}

	const std::uint64_t UploadTick = Renderer.GetStreamBuffer().Flush();

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

	vk::StructureChain<vk::SubmitInfo, vk::TimelineSemaphoreSubmitInfo>
		SubmitInfoChain;

	auto& SubmitInfo = SubmitInfoChain.get<vk::SubmitInfo>();

	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers    = &CommandBuffer.get();

	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores    = &Renderer.GetStreamBuffer().GetSemaphore();

	static const vk::PipelineStageFlags WaitStage
		= vk::PipelineStageFlagBits::eTransfer;
	SubmitInfo.pWaitDstStageMask = &WaitStage;

	auto& SubmitTimelineInfo
		= SubmitInfoChain.get<vk::TimelineSemaphoreSubmitInfo>();

	SubmitTimelineInfo.waitSemaphoreValueCount = 1;
	SubmitTimelineInfo.pWaitSemaphoreValues    = &UploadTick;

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

#ifdef CAPTURE
	if( rdoc_api )
		rdoc_api->EndFrameCapture(
			*(void**)(VkInstance)(*Instance.operator->()), NULL);
#endif

	stbi_write_png_compression_level = 0;
	stbi_write_png(
		("./" + MapPath.stem().string() + ".png").c_str(), RenderSize.x,
		RenderSize.y, 4, StagingBufferData.data(), 0);

	return EXIT_SUCCESS;
}

vk::UniqueFramebuffer CreateMainFrameBuffer(
	vk::Device Device, vk::ImageView Color, vk::ImageView DepthAA,
	vk::ImageView ColorAA, glm::uvec2 ImageSize, vk::RenderPass RenderPass)
{
	vk::FramebufferCreateInfo FramebufferInfo = {};

	FramebufferInfo.width      = ImageSize.x;
	FramebufferInfo.height     = ImageSize.y;
	FramebufferInfo.layers     = 1;
	FramebufferInfo.renderPass = RenderPass;

	const vk::ImageView Attachments[] = {Color, DepthAA, ColorAA};
	FramebufferInfo.attachmentCount   = std::size(Attachments);
	FramebufferInfo.pAttachments      = Attachments;

	if( auto CreateResult = Device.createFramebufferUnique(FramebufferInfo);
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

std::string FormatDeviceCaps(vk::PhysicalDevice PhysicalDevice)
{
	std::string Result;

	const vk::PhysicalDeviceProperties Properties
		= PhysicalDevice.getProperties();

	Result += Common::Format(
		"Device Name: %.256s\n", Properties.deviceName.data());
	Result += Common::Format(
		"Device Type: %s\n", vk::to_string(Properties.deviceType).c_str());
	Result += Common::Format(
		"DeviceID/VendorID: %8x:%8x\n", Properties.deviceID,
		Properties.vendorID);

	const vk::PhysicalDeviceMemoryProperties MemoryProperties
		= PhysicalDevice.getMemoryProperties();
	for( std::uint8_t HeapIdx = 0; HeapIdx < MemoryProperties.memoryHeapCount;
		 ++HeapIdx )
	{
		const auto& CurHeap = MemoryProperties.memoryHeaps[HeapIdx];
		Result += Common::Format(
			"Heap %2u: %12s %s\n", HeapIdx,
			Common::FormatByteCount(CurHeap.size).c_str(),
			vk::to_string(CurHeap.flags).c_str());
	}

	return Result;
}