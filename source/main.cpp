#include "Blam/Util.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <span>

#include <mio/mmap.hpp>

#include <Blam/Blam.hpp>

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

vk::UniqueDescriptorPool CreateMainDescriptorPool(vk::Device LogicalDevice);

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

	//// Create Descriptor Pool
	vk::UniqueDescriptorPool MainDescriptorPool
		= CreateMainDescriptorPool(Device.get());

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
