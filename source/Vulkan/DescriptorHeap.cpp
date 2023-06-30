#include <Vulkan/DescriptorHeap.hpp>

#include <algorithm>
#include <optional>
#include <unordered_map>

namespace Vulkan
{

DescriptorHeap::DescriptorHeap(const Vulkan::Context& VulkanContext)
	: VulkanContext(VulkanContext)
{
}

std::optional<vk::DescriptorSet> DescriptorHeap::AllocateDescriptorSet()
{
	std::scoped_lock DescriptorHeapLock{*DescriptorHeapMutex};

	// Find a free slot
	const auto FreeSlot
		= std::find(AllocationMap.begin(), AllocationMap.end(), false);

	// If there is no free slot, return
	if( FreeSlot == AllocationMap.end() )
	{
		return std::nullopt;
	}

	// Mark the slot as allocated
	*FreeSlot = true;

	const std::uint16_t Index = static_cast<std::uint16_t>(
		std::distance(AllocationMap.begin(), FreeSlot)
	);

	vk::UniqueDescriptorSet& NewDescriptorSet = DescriptorSets[Index];

	if( !NewDescriptorSet )
	{
		// Descriptor set doesn't exist yet. Allocate a new one
		vk::DescriptorSetAllocateInfo AllocateInfo = {};

		AllocateInfo.descriptorPool     = DescriptorPool.get();
		AllocateInfo.pSetLayouts        = &DescriptorSetLayout.get();
		AllocateInfo.descriptorSetCount = 1;

		if( auto AllocateResult
			= VulkanContext.LogicalDevice.allocateDescriptorSetsUnique(
				AllocateInfo
			);
			AllocateResult.result == vk::Result::eSuccess )
		{
			NewDescriptorSet = std::move(AllocateResult.value[0]);
		}
		else
		{
			// Error allocating descriptor set
			return std::nullopt;
		}
	}

	return NewDescriptorSet.get();
}

bool DescriptorHeap::FreeDescriptorSet(vk::DescriptorSet Set)
{
	std::scoped_lock DescriptorHeapLock{*DescriptorHeapMutex};
	// Find the descriptor set
	const auto Found = std::find_if(
		DescriptorSets.begin(), DescriptorSets.end(),
		[&Set](const auto& CurSet) -> bool { return CurSet.get() == Set; }
	);

	// If the descriptor set is not found, return
	if( Found == DescriptorSets.end() )
	{
		return false;
	}

	// Mark the slot as free
	const std::uint16_t Index = static_cast<std::uint16_t>(
		std::distance(DescriptorSets.begin(), Found)
	);

	AllocationMap[Index] = false;

	return true;
}

std::optional<DescriptorHeap> DescriptorHeap::Create(
	const Vulkan::Context&                          VulkanContext,
	std::span<const vk::DescriptorSetLayoutBinding> Bindings,
	std::uint16_t                                   DescriptorHeapCount
)
{
	DescriptorHeap NewDescriptorHeap(VulkanContext);

	// Create a histogram of each of the descriptor types and how many of each
	// the pool should have
	// Todo: maybe keep this around as a hash table to do more dynamic
	// allocations of descriptor sets rather than allocating them all up-front
	// - Sun May 15 07:52:14 AM PDT 2022
	std::vector<vk::DescriptorPoolSize> PoolSizes;
	{
		std::unordered_map<vk::DescriptorType, std::uint16_t>
			DescriptorTypeCounts;

		for( const auto& CurBinding : Bindings )
		{
			DescriptorTypeCounts[CurBinding.descriptorType]
				+= CurBinding.descriptorCount;
		}
		for( const auto& CurDescriptorTypeCount : DescriptorTypeCounts )
		{
			PoolSizes.push_back(vk::DescriptorPoolSize(
				CurDescriptorTypeCount.first,
				CurDescriptorTypeCount.second * DescriptorHeapCount
			));
		}
	}

	// Create descriptor pool
	{
		vk::DescriptorPoolCreateInfo PoolInfo;
		PoolInfo.flags   = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
		PoolInfo.maxSets = DescriptorHeapCount;
		PoolInfo.pPoolSizes    = PoolSizes.data();
		PoolInfo.poolSizeCount = PoolSizes.size();
		if( auto CreateResult
			= VulkanContext.LogicalDevice.createDescriptorPoolUnique(PoolInfo);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewDescriptorHeap.DescriptorPool = std::move(CreateResult.value);
		}
		else
		{
			return std::nullopt;
		}
	}

	// Create descriptor set layout
	{
		vk::DescriptorSetLayoutCreateInfo LayoutInfo;
		LayoutInfo.pBindings    = Bindings.data();
		LayoutInfo.bindingCount = Bindings.size();

		if( auto CreateResult
			= VulkanContext.LogicalDevice.createDescriptorSetLayoutUnique(
				LayoutInfo
			);
			CreateResult.result == vk::Result::eSuccess )
		{
			NewDescriptorHeap.DescriptorSetLayout
				= std::move(CreateResult.value);
		}
		else
		{
			return std::nullopt;
		}
	}

	NewDescriptorHeap.DescriptorSets.resize(DescriptorHeapCount);
	NewDescriptorHeap.AllocationMap.resize(DescriptorHeapCount);

	NewDescriptorHeap.Bindings.assign(Bindings.begin(), Bindings.end());

	return {std::move(NewDescriptorHeap)};
}
} // namespace Vulkan