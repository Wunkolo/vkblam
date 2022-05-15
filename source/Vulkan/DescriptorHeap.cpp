#include <Vulkan/DescriptorHeap.hpp>

#include <optional>
#include <unordered_map>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace Vulkan
{
std::optional<DescriptorHeap> DescriptorHeap::Create(
	vk::Device                                      LogicalDevice,
	std::span<const vk::DescriptorSetLayoutBinding> Bindings,
	std::uint16_t                                   DescriptorHeapCount)
{
	DescriptorHeap NewDescriptorHeap;

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
				CurDescriptorTypeCount.second * DescriptorHeapCount));
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
			= LogicalDevice.createDescriptorPoolUnique(PoolInfo);
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
			= LogicalDevice.createDescriptorSetLayoutUnique(LayoutInfo);
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

	// Create descriptor sets
	{
		vk::DescriptorSetAllocateInfo AllocateInfo;
		AllocateInfo.descriptorPool = NewDescriptorHeap.DescriptorPool.get();
		AllocateInfo.pSetLayouts = &NewDescriptorHeap.DescriptorSetLayout.get();
		AllocateInfo.descriptorSetCount = 1;

		NewDescriptorHeap.DescripterSets.resize(DescriptorHeapCount);
		for( auto& CurDescriptorSet : NewDescriptorHeap.DescripterSets )
		{
			if( auto AllocateResult
				= LogicalDevice.allocateDescriptorSetsUnique(AllocateInfo);
				AllocateResult.result == vk::Result::eSuccess )
			{
				CurDescriptorSet = std::move(AllocateResult.value[0]);
			}
			else
			{
				return std::nullopt;
			}
		}
	}

	NewDescriptorHeap.Bindings.assign(Bindings.begin(), Bindings.end());

	return {std::move(NewDescriptorHeap)};
}
} // namespace Vulkan