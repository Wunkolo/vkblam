#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <optional>

namespace Vulkan
{

// Given a Surface, handles the creation and recreation of a swapchain and its
// images and synchronization primitives. This generally configures and
// maintains settings relating to Vsync, Latency, HDR, and the
// format/color-space of what gets presented physically to the user.
// Users of this class are expected to be handling
// framebuffers/renderpass/imageviews/etc
// * Requires the `VK_KHR_surface` instance-extension
class Swapchain final
{
private:
	explicit Swapchain(const Vulkan::Context& VulkanContext);

	const Vulkan::Context& VulkanContext;

	vk::UniqueSwapchainKHR SwapchainInstance;

	vk::SurfaceKHR       Surface;
	vk::SurfaceFormatKHR SurfaceFormat;

	std::uint8_t SwapImageCount = 0u;
	vk::Extent2D SwapImageExtents;

	std::vector<vk::Image> SwapImages;

	// Semaphores that `vkAcquireNextImageKHR` will signal for then the
	// swapchain image is ready to be rendered into. A new frame should wait on
	// this semaphore.
	std::uint8_t                     CurImageAcquireSemaphoreIndex = 0u;
	std::vector<vk::UniqueSemaphore> SwapSemaphoreImageAcquired;

	// Current swap-image to render into. This is the result of
	// `vkAcquireNextImageKHR`
	std::uint8_t NextSwapImageIndex = 0u;

	// Semaphores that render-frames should signal to indicate that they are
	// ready to be presented. Calls to `vkPresentKHR` will wait on this
	// semaphore
	std::vector<vk::UniqueSemaphore> SwapSemaphorePresentReady;

public:
	~Swapchain()           = default;
	Swapchain(Swapchain&&) = default;

	[[nodiscard]] const vk::SurfaceFormatKHR& GetSurfaceFormat() const
	{
		return SurfaceFormat;
	}

	[[nodiscard]] const vk::Format& GetSurfaceImageFormat() const
	{
		return GetSurfaceFormat().format;
	}

	[[nodiscard]] std::uint8_t GetSwapchainCount() const
	{
		return SwapImageCount;
	}

	[[nodiscard]] const vk::Extent2D& GetSwapchainExtents() const
	{
		return SwapImageExtents;
	}

	[[nodiscard]] std::uint32_t GetWidth() const
	{
		return GetSwapchainExtents().width;
	}

	[[nodiscard]] std::uint32_t GetHeight() const
	{
		return GetSwapchainExtents().height;
	}

	[[nodiscard]] const vk::Image& GetSwapImage(std::uint8_t SwapIndex) const
	{
		return SwapImages.at(SwapIndex);
	}

	[[nodiscard]] const vk::Image& GetNextSwapImage() const
	{
		return SwapImages.at(NextSwapImageIndex);
	}

	[[nodiscard]] const vk::Semaphore&
		GetImageAcquiredSemaphore(std::uint8_t SwapIndex) const
	{
		return SwapSemaphoreImageAcquired.at(SwapIndex).get();
	}

	[[nodiscard]] const vk::Semaphore& GetCurrentImageAcquiredSemaphore() const
	{
		return GetImageAcquiredSemaphore(CurImageAcquireSemaphoreIndex);
	}

	[[nodiscard]] const vk::Semaphore&
		GetImagePresentReadySemaphore(std::uint8_t SwapIndex) const
	{
		return SwapSemaphorePresentReady.at(SwapIndex).get();
	}

	[[nodiscard]] const vk::Semaphore& GetNextImagePresentReadySemaphore() const
	{
		return GetImagePresentReadySemaphore(NextSwapImageIndex);
	}

	// Move on to the next image in the swapchain. Returns the semaphore to wait
	// on for when the image is actually ready to be rendered into. Returns a
	// null-handle if there was an error or if the swapchain needs to be
	// recreated.
	vk::Semaphore AcquireNextImage();

	// Waits on the current "Present-Ready"-semaphore and presents the current
	// swapchain image to the present-queue
	void Present();

	static std::optional<Swapchain> Create(
		const Vulkan::Context& VulkanContext, const vk::SurfaceKHR& Surface,
		vk::Extent2D SwapchainExtents, std::uint8_t SwapchainCount,
		const Swapchain* OldSwapchain = nullptr
	);
};
} // namespace Vulkan