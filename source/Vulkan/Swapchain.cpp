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
	// std::ranges::stable_partition(
	//	SurfaceFormats,
	//	[](const vk::SurfaceFormatKHR& SurfaceFormat) -> bool {
	//		return std::string_view("SRGB")
	//			== vk::componentNumericFormat(SurfaceFormat.format, 0);
	//	}
	//);

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

bool Swapchain::RecreateSwapchain(
	std::optional<vk::Extent2D>     NewExtent,
	std::optional<vk::SwapchainKHR> OldSwapchain
)
{
	// Unfortunately this is the best way to ensure that any currently in-flight
	// frames are done.
	// TODO: VK_{KHR,EXT}_swapchain_maintenance1 has better swapchain
	// waiting/cleanup mechanisms that should be used here - 11/7/2025
	if( const auto WaitResult = VulkanContext.LogicalDevice.waitIdle();
		WaitResult != vk::Result::eSuccess )
	{
		return false;
	}

	/// Swapchain surface format
	SurfaceFormat = FindSurfaceFormat(VulkanContext.PhysicalDevice, Surface);

	// Get present mode
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
		return false;
	}

	/// Swapchain image count

	// Clamp the requested swapchain size between the supported min/max
	// `maxImageCount` may be `0`, indicating there is no limit
	if( SurfaceCapabilities.maxImageCount != 0u )
	{
		SwapImageCount = std::clamp<std::uint32_t>(
			SwapImageCount, SurfaceCapabilities.minImageCount,
			SurfaceCapabilities.maxImageCount
		);
	}
	else
	{
		SwapImageCount = std::max<std::uint32_t>(
			SwapImageCount, SurfaceCapabilities.minImageCount
		);
	}

	/// Swapchain image extents
	if( SurfaceCapabilities.currentExtent.width == 0
		|| SurfaceCapabilities.currentExtent.height == 0 )
	{
		// Window is likely minimized
		SwapImages.clear();
		SwapchainInstance.reset();
		return true;
	}

	// Set new size, or preserve the older one
	SwapImageExtents = NewExtent.value_or(SurfaceCapabilities.currentExtent);

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

	// Use old(current) swapchain if available, or the user-provided one
	// This seems to help allow previous image handles to be recycled, such as
	// how resizing a window to be smaller means you can just use a smaller
	// subset of the larger image. Or when the window is minified.
	const vk::SwapchainKHR OldSwapchainInstance
		= OldSwapchain.value_or(SwapchainInstance.get());

	const vk::SwapchainCreateInfoKHR SwapchainInfo{
		.flags                 = {},
		.surface               = Surface,
		.minImageCount         = SwapImageCount,
		.imageFormat           = SurfaceFormat.format,
		.imageColorSpace       = SurfaceFormat.colorSpace,
		.imageExtent           = SwapImageExtents,
		.imageArrayLayers      = 1u,
		.imageUsage            = SwapchainImageFlags,
		.imageSharingMode      = vk::SharingMode::eExclusive,
		.queueFamilyIndexCount = 1u,
		.pQueueFamilyIndices   = &QueueFamily,
		.preTransform          = SurfaceTransform,
		.compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode           = PresetMode,
		.clipped               = vk::True,
		.oldSwapchain          = OldSwapchainInstance,
	};

	if( auto CreateResult
		= VulkanContext.LogicalDevice.createSwapchainKHRUnique(SwapchainInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		Vulkan::SetObjectName(
			VulkanContext.LogicalDevice, CreateResult.value.get(), "Swapchain"
		);

		SwapchainInstance = std::move(CreateResult.value);
	}
	else
	{
		// Error creating swapchain
		return false;
	}

	/// Get swapchain images
	if( auto GetResult = VulkanContext.LogicalDevice.getSwapchainImagesKHR(
			SwapchainInstance.get()
		);
		GetResult.result == vk::Result::eSuccess )
	{
		SwapImages = std::move(GetResult.value);
	}
	else
	{
		// Error getting swapchain images;
		return false;
	}

	return true;
}

vk::Semaphore Swapchain::AcquireNextImage()
{
	if( !SwapchainInstance )
	{
		return {};
	}

	// Semaphore to signal when the image has been acquired, generally all image
	// operations that use the swapchain image should wait on this semaphore
	const vk::Semaphore SemaphoreImageAcquired
		= SwapSemaphoreImageAcquired[CurImageAcquireSemaphoreIndex].get();

	// Get the next swapchain image to render into
	constexpr std::uint64_t Timeout = std::numeric_limits<std::uint64_t>::max();

	// Bypass the default vulkan-hpp implementation which asserts upon results
	// such as `eErrorSurfaceLostKHR` and `eErrorOutOfDateKHR`
	const auto UnwrappedAcquireNextImageKHR
		= [](vk::Device m_device, vk::SwapchainKHR swapchain, uint64_t timeout,
			 vk::Semaphore semaphore, vk::Fence fence,
			 VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const& d
			 = VULKAN_HPP_DEFAULT_DISPATCHER) {
			  VULKAN_HPP_ASSERT(d.getVkHeaderVersion() == vk::HeaderVersion);
#if ( VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1 )
			  VULKAN_HPP_ASSERT(
			d.vkAcquireNextImageKHR
			&& "Function <vkAcquireNextImageKHR> requires <VK_KHR_swapchain>"
		);
#endif

			  std::uint32_t imageIndex;
			  vk::Result    result
				  = static_cast<vk::Result>(d.vkAcquireNextImageKHR(
					  m_device, static_cast<VkSwapchainKHR>(swapchain), timeout,
					  static_cast<VkSemaphore>(semaphore),
					  static_cast<VkFence>(fence), &imageIndex
				  ));

			  return vk::ResultValue<uint32_t>(result, imageIndex);
		  };

	// const vk::ResultValue<std::uint32_t> AcquireResult
	//	= VulkanContext.LogicalDevice.acquireNextImageKHR(
	//		SwapchainInstance.get(), Timeout, SemaphoreImageAcquired,
	//		vk::Fence{}
	//	);
	const vk::ResultValue<std::uint32_t> AcquireResult
		= UnwrappedAcquireNextImageKHR(
			VulkanContext.LogicalDevice, SwapchainInstance.get(), Timeout,
			SemaphoreImageAcquired, vk::Fence{}
		);

	switch( AcquireResult.result )
	{
	case vk::Result::eSuccess:
	{
		assert(
			AcquireResult.value
			<= std::numeric_limits<decltype(NextSwapImageIndex)>::max()
		);

		// Got the next swapchain image to render into
		NextSwapImageIndex = AcquireResult.value;
		break;
	}
	case vk::Result::eSuboptimalKHR:
	case vk::Result::eErrorSurfaceLostKHR:
	case vk::Result::eErrorOutOfDateKHR:
	{
		// TODO: Swapchain needs to be recreated
		if( RecreateSwapchain({}, SwapchainInstance.get()) )
		{
			return AcquireNextImage();
		}
		else
		{
			return {};
		}
		break;
	}
	default:
		return {};
	}

	return SemaphoreImageAcquired;
}

bool Swapchain::Present()
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

	// Bypass the default vulkan-hpp implementation which asserts upon results
	// such as `eErrorSurfaceLostKHR` and `eErrorOutOfDateKHR`
	const auto UnwrappedPresentKHR =
		[](vk::Queue m_queue, const vk::PresentInfoKHR& presentInfo,
		   VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const& d
		   = VULKAN_HPP_DEFAULT_DISPATCHER) {
			VULKAN_HPP_ASSERT(d.getVkHeaderVersion() == vk::HeaderVersion);
#if ( VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1 )
			VULKAN_HPP_ASSERT(
				d.vkQueuePresentKHR
				&& "Function <vkQueuePresentKHR> requires <VK_KHR_swapchain>"
			);
#endif

			return static_cast<vk::Result>(d.vkQueuePresentKHR(
				m_queue, reinterpret_cast<const VkPresentInfoKHR*>(&presentInfo)
			));
		};

	// const vk::Result PresentResult
	//	= VulkanContext.PresentQueue.presentKHR(PresentInfo);
	const vk::Result PresentResult
		= UnwrappedPresentKHR(VulkanContext.PresentQueue, PresentInfo);

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
		return false;
	}
	default:
	{
		// Unhandled result
		return false;
	}
	}

	// Move on to the next semaphore
	CurImageAcquireSemaphoreIndex
		= (CurImageAcquireSemaphoreIndex + 1) % GetSwapchainCount();

	return true;
}

std::expected<Swapchain, vk::Result> Swapchain::Create(
	const Vulkan::Context& VulkanContext, const vk::SurfaceKHR& Surface,
	std::uint8_t SwapchainCount, bool Vsync, const Swapchain* OldSwapchain
)
{
	Swapchain NewSwapchain(VulkanContext);

	NewSwapchain.Surface = Surface;
	NewSwapchain.Vsync   = Vsync;

	// Let RecreateSwapchain "fix" these assignments
	NewSwapchain.SwapImageCount = SwapchainCount;

	const vk::SwapchainKHR OldSwapchainHandle
		= OldSwapchain != nullptr ? OldSwapchain->SwapchainInstance.get()
								  : vk::SwapchainKHR();

	NewSwapchain.RecreateSwapchain({}, OldSwapchainHandle);

	/// Swapchain synchronization primitives
	const vk::SemaphoreCreateInfo SemaphoreInfo{};
	for( std::uint8_t SwapIndex = 0; SwapIndex < NewSwapchain.SwapImageCount;
		 ++SwapIndex )
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
			return std::unexpected(CreateResult.result);
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
			return std::unexpected(CreateResult.result);
		}
	}

	return NewSwapchain;
}
} // namespace Vulkan