#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <optional>
#include <span>

namespace Vulkan
{

// Implements a basic heap of descriptor sets given a layout of particular
// bindings. Create a descriptor set by providing a list of bindings and it will
// automatically create both the pool, layout, and maintail a heap of descriptor
// sets.
class DescriptorHeap
{
private:
	const Vulkan::Context& VulkanContext;

	vk::UniqueDescriptorPool             DescriptorPool;
	vk::UniqueDescriptorSetLayout        DescriptorSetLayout;
	std::vector<vk::UniqueDescriptorSet> DescripterSets;

	std::vector<vk::DescriptorSetLayoutBinding> Bindings;

	std::vector<bool> AllocationMap;

	explicit DescriptorHeap(const Vulkan::Context& VulkanContext);

public:
	~DescriptorHeap() = default;

	DescriptorHeap(DescriptorHeap&&) = default;

	const vk::DescriptorPool& GetDescriptorPool() const
	{
		return DescriptorPool.get();
	};

	const vk::DescriptorSetLayout& GetDescriptorSetLayout() const
	{
		return DescriptorSetLayout.get();
	};

	const std::span<const vk::UniqueDescriptorSet> GetDescriptorSets() const
	{
		return DescripterSets;
	};

	std::span<const vk::DescriptorSetLayoutBinding> GetBindings() const
	{
		return Bindings;
	};

	std::optional<vk::DescriptorSet> AllocateDescriptorSet();
	bool                             FreeDescriptorSet(vk::DescriptorSet Set);

	static std::optional<DescriptorHeap> Create(
		Vulkan::Context                                 VulkanContext,
		std::span<const vk::DescriptorSetLayoutBinding> Bindings,
		std::uint16_t DescriptorHeapCount = 1024);
};

} // namespace Vulkan