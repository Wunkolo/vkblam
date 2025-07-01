#include <Vulkan/Swapchain.hpp>

#include <Vulkan/Debug.hpp>

#include <algorithm>
#include <vulkan/vulkan_format_traits.hpp>

namespace
{

vk::SurfaceFormatKHR FindSurfaceFormat(
	const vk::PhysicalDevice& PhysicalDevice, const vk::SurfaceKHR& Surface
)
{
	// Determine surface format and color-space
	std::vector<vk::SurfaceFormatKHR> SurfaceFormats;
	if( auto EnumerateResult = PhysicalDevice.getSurfaceFormatsKHR(Surface);
		EnumerateResult.result == vk::Result::eSuccess )
	{
		SurfaceFormats = std::move(EnumerateResult.value);
	}

	// Prefer an sRGB image format
	std::ranges::stable_partition(
		SurfaceFormats, [](const vk::SurfaceFormatKHR& SurfaceFormat) -> bool {
			return std::string_view("SRGB")
				== vk::componentNumericFormat(SurfaceFormat.format, 0);
		}
	);

	// Prefer an sRGB presentation color-space
	std::ranges::stable_partition(
		SurfaceFormats, [](const vk::SurfaceFormatKHR& SurfaceFormat) -> bool {
			return SurfaceFormat.colorSpace
				== vk::ColorSpaceKHR::eSrgbNonlinear;
		}
	);

	// After the stable partitions, the top of the list is the best candidate
	// surface format/color-space
	return SurfaceFormats[0];
}

vk::PresentModeKHR FindPresentMode(
	const vk::PhysicalDevice& PhysicalDevice, const vk::SurfaceKHR& Surface,
	bool Vsync
)
{
	std::vector<vk::PresentModeKHR> PresentModes;
	if( auto EnumerateResult
		= PhysicalDevice.getSurfacePresentModesKHR(Surface);
		EnumerateResult.result == vk::Result::eSuccess )
	{
		PresentModes = std::move(EnumerateResult.value);
	}
	else
	{
		return vk::PresentModeKHR::eFifo;
	}

	const auto SupportsPresentMode
		= [&PresentModes](vk::PresentModeKHR RequestedPresentMode) -> bool {
		return std::ranges::find_if(
				   PresentModes,
				   [&RequestedPresentMode](
					   const vk::PresentModeKHR& CurPresentMode
				   ) -> bool { return CurPresentMode == RequestedPresentMode; }
			   )
			!= PresentModes.cend();
	};

	const bool HasImmediate
		= SupportsPresentMode(vk::PresentModeKHR::eImmediate);

	const bool HasMailbox = SupportsPresentMode(vk::PresentModeKHR::eMailbox);

	// Vulkan mandates support for FIFO present mode as a baseline
	// Hard-sync with the monitor's refresh rate(vsync) with a FIFO queue
	// No tearing, most latency
	vk::PresentModeKHR Result = vk::PresentModeKHR::eFifo;

	// Double/Triple/etc-Buffering with adaptive sync, similar to FIFO but
	// new images are allowed to bypass the queue when full and be presented
	// immediately as it comes in.
	// No tearing, less latency
	Result = HasMailbox ? vk::PresentModeKHR::eMailbox : Result;

	// No VSync requested
	if( !Vsync )
	{
		// Immediately present frame, no synchronization
		// Tearing, minimal latency
		Result = HasImmediate ? vk::PresentModeKHR::eImmediate : Result;
	}

	return Result;
}
} // namespace

namespace Vulkan
{

Swapchain::Swapchain(const Vulkan::Context& VulkanContext)
	: VulkanContext(VulkanContext)
{
}
vk::Semaphore Swapchain::AcquireNextImage()
{

	// Semaphore to signal when the image has been acquired, generally all image
	// operations that use the swapchain image should wait on this semaphore
	const vk::Semaphore ImageAcquired
		= SwapSemaphoreImageAcquired[CurImageAcquireSemaphoreIndex].get();

	// Get the next swapchain image to render into
	constexpr std::uint64_t Timeout = std::numeric_limits<std::uint64_t>::max();
	const vk::ResultValue<std::uint32_t> AcquireResult
		= VulkanContext.LogicalDevice.acquireNextImageKHR(
			SwapchainInstance.get(), Timeout, ImageAcquired, vk::Fence{}
		);

	switch( AcquireResult.result )
	{
	case vk::Result::eSuccess:
	{
		assert(AcquireResult.value <= 0xFF);

		// Got the next swapchain image to render into
		NextSwapImageIndex = AcquireResult.value;
		break;
	}
	case vk::Result::eSuboptimalKHR:
	case vk::Result::eErrorSurfaceLostKHR:
	case vk::Result::eErrorOutOfDateKHR:
	{
		// TODO: Swapchain needs to be recreated
		return {};
		break;
	}
	default:
		return {};
	}

	return ImageAcquired;
}

void Swapchain::Present()
{
	vk::PresentInfoKHR PresentInfo{};

	const vk::SwapchainKHR& Swapchain = SwapchainInstance.get();
	PresentInfo.setSwapchains(Swapchain);

	const std::uint32_t NextImageIndex = NextSwapImageIndex;
	PresentInfo.setImageIndices(NextImageIndex);

	// Wait for the image to be ready to be presented into
	std::vector<vk::Semaphore> WaitSemaphores;
	WaitSemaphores.emplace_back(GetNextImagePresentReadySemaphore());
	PresentInfo.setWaitSemaphores(WaitSemaphores);

	const vk::Result PresentResult
		= VulkanContext.PresentQueue.presentKHR(PresentInfo);

	switch( PresentResult )
	{
	case vk::Result::eSuccess:
	{
		break;
	}
	case vk::Result::eSuboptimalKHR:
	case vk::Result::eErrorSurfaceLostKHR:
	case vk::Result::eErrorOutOfDateKHR:
	{
		// TODO: Swapchain needs to be recreated
		return;
	}
	default:
	{

		// Unhandled result
		return;
	}
	}

	// Move on to the next semaphore
	CurImageAcquireSemaphoreIndex
		= (CurImageAcquireSemaphoreIndex + 1) % GetSwapchainCount();
}

std::optional<Swapchain> Swapchain::Create(
	const Vulkan::Context& VulkanContext, const vk::SurfaceKHR& Surface,
	vk::Extent2D SwapchainExtents, std::uint8_t SwapchainCount,
	const Swapchain* OldSwapchain
)
{
	Swapchain NewSwapchain(VulkanContext);

	NewSwapchain.Surface = Surface;

	/// Swapchain surface format
	const vk::SurfaceFormatKHR SurfaceFormat
		= FindSurfaceFormat(VulkanContext.PhysicalDevice, Surface);

	NewSwapchain.SurfaceFormat = SurfaceFormat;

	// Get present mode
	constexpr bool           Vsync = false;
	const vk::PresentModeKHR PresetMode
		= FindPresentMode(VulkanContext.PhysicalDevice, Surface, Vsync);

	vk::SurfaceCapabilitiesKHR SurfaceCapabilities{};
	if( auto GetResult
		= VulkanContext.PhysicalDevice.getSurfaceCapabilitiesKHR(Surface);
		GetResult.result == vk::Result::eSuccess )
	{
		SurfaceCapabilities = GetResult.value;
	}
	else
	{
		// Error getting surface capabilities
		return std::nullopt;
	}

	/// Swapchain image count

	// Clamp the requested swapchain size between the supported min/max
	// `maxImageCount` may be `0`, indicating there is no limit
	if( SurfaceCapabilities.maxImageCount != 0u )
	{
		SwapchainCount = std::clamp<std::uint32_t>(
			SwapchainCount, SurfaceCapabilities.minImageCount,
			SurfaceCapabilities.maxImageCount
		);
	}
	else
	{
		SwapchainCount = std::max<std::uint32_t>(
			SwapchainCount, SurfaceCapabilities.minImageCount
		);
	}
	NewSwapchain.SwapImageCount = SwapchainCount;

	/// Swapchain image extents
	// Clamp the requested swapchain image size between the supported min/max
	SwapchainExtents.width = std::clamp<std::uint32_t>(
		SwapchainExtents.width, SurfaceCapabilities.minImageExtent.width,
		SurfaceCapabilities.maxImageExtent.width
	);
	SwapchainExtents.height = std::clamp<std::uint32_t>(
		SwapchainExtents.height, SurfaceCapabilities.minImageExtent.height,
		SurfaceCapabilities.maxImageExtent.height
	);
	NewSwapchain.SwapImageExtents = SwapchainExtents;

	/// Swapchain image usage
	// Color-attachment is mandated by the vulkan spec
	vk::ImageUsageFlags SwapchainImageFlags
		= vk::ImageUsageFlagBits::eColorAttachment;

	// Transfer Src, possibly for screenshots
	if( SurfaceCapabilities.supportedUsageFlags
		& vk::ImageUsageFlagBits::eTransferSrc )
	{
		SwapchainImageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
	}

	// Transfer Dst, for blits, resolves, writes, etc
	if( SurfaceCapabilities.supportedUsageFlags
		& vk::ImageUsageFlagBits::eTransferDst )
	{
		SwapchainImageFlags |= vk::ImageUsageFlagBits::eTransferDst;
	}

	/// Swapchain queue family
	const std::uint32_t QueueFamily = VulkanContext.PresentQueueFamilyIndex;

	/// Swapchain transform
	vk::SurfaceTransformFlagBitsKHR SurfaceTransform;
	SurfaceTransform = SurfaceCapabilities.currentTransform;

	// Prefer Identity, if supported
	if( SurfaceCapabilities.supportedTransforms
		& vk::SurfaceTransformFlagBitsKHR::eIdentity )
	{
		SurfaceTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	}

	// Use old swapchain if available
	// This seems to help allow previous image handles to be recycled, such as
	// how resizing a window to be smaller means you can just use a smaller
	// subset of the larger image. Or when the window is minified.
	const vk::SwapchainKHR OldSwapchainInstance
		= (OldSwapchain != nullptr) ? OldSwapchain->SwapchainInstance.get()
									: vk::SwapchainKHR{};

	const vk::SwapchainCreateInfoKHR SwapchainInfo{
		.flags                 = {},
		.surface               = Surface,
		.minImageCount         = SwapchainCount,
		.imageFormat           = SurfaceFormat.format,
		.imageColorSpace       = SurfaceFormat.colorSpace,
		.imageExtent           = SwapchainExtents,
		.imageArrayLayers      = 1u,
		.imageUsage            = SwapchainImageFlags,
		.imageSharingMode      = vk::SharingMode::eExclusive,
		.queueFamilyIndexCount = 1u,
		.pQueueFamilyIndices   = &QueueFamily,
		.preTransform          = SurfaceTransform,
		.compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode           = PresetMode,
		.clipped               = VK_TRUE,
		.oldSwapchain          = OldSwapchainInstance,
	};

	if( auto CreateResult
		= VulkanContext.LogicalDevice.createSwapchainKHRUnique(SwapchainInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, CreateResult.value.get(), "Swapchain"
		);

		NewSwapchain.SwapchainInstance = std::move(CreateResult.value);
	}
	else
	{
		// Error creating swapchain
		return std::nullopt;
	}

	/// Get swapchain images
	if( auto GetResult = VulkanContext.LogicalDevice.getSwapchainImagesKHR(
			NewSwapchain.SwapchainInstance.get()
		);
		GetResult.result == vk::Result::eSuccess )
	{
		NewSwapchain.SwapImages = std::move(GetResult.value);
	}
	else
	{
		// Error getting swapchain images;
		return std::nullopt;
	}

	/// Swapchain synchronization primitives
	const vk::SemaphoreCreateInfo SemaphoreInfo{};
	for( std::uint8_t SwapIndex = 0; SwapIndex < SwapchainCount; ++SwapIndex )
	{
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, NewSwapchain.SwapImages[SwapIndex],
			"Swapchain: Image #{}", SwapIndex
		);

		if( auto CreateResult
			= VulkanContext.LogicalDevice.createSemaphoreUnique(SemaphoreInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			Vulkan::SetObjectName(
				VulkanContext.LogicalDevice, CreateResult.value.get(),
				"Swapchain: Image-Acquired Semaphore #{}", SwapIndex
			);

			NewSwapchain.SwapSemaphoreImageAcquired.emplace_back(
				std::move(CreateResult.value)
			);
		}
		else
		{
			// Error creating swapchain acquire-semaphore
			return std::nullopt;
		}

		if( auto CreateResult
			= VulkanContext.LogicalDevice.createSemaphoreUnique(SemaphoreInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			Vulkan::SetObjectName(
				VulkanContext.LogicalDevice, CreateResult.value.get(),
				"Swapchain: Present-Ready Semaphore #{}", SwapIndex
			);

			NewSwapchain.SwapSemaphorePresentReady.emplace_back(
				std::move(CreateResult.value)
			);
		}
		else
		{
			// Error creating swapchain present-ready-semaphore
			return std::nullopt;
		}
	}

	return {std::move(NewSwapchain)};
}
} // namespace Vulkan