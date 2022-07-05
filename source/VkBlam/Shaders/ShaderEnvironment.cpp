
#include "Blam/Enums.hpp"
#include "Vulkan/Debug.hpp"
#include <VkBlam/Shaders/ShaderEnvironment.hpp>

#include <array>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

static vk::DescriptorSetLayoutBinding Bindings[] = {
	{// BaseMap
	 0, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// BumpMap
	 1, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// PrimaryDetailMap
	 2, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// SecondaryDetailMap
	 3, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// MicroDetailMap
	 4, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// GlowMap
	 5, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// ReflectionCubeMap
	 6, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
};

static vk::ImageViewType ImageTypes[] = {
	// BaseMap
	vk::ImageViewType::e2D,
	// BumpMap
	vk::ImageViewType::e2D,
	// PrimaryDetailMap
	vk::ImageViewType::e2D,
	// SecondaryDetailMap
	vk::ImageViewType::e2D,
	// MicroDetailMap
	vk::ImageViewType::e2D,
	// GlowMap
	vk::ImageViewType::e2D,
	// ReflectionCubeMap
	vk::ImageViewType::eCube,
};

namespace VkBlam
{

ShaderEnvironment::ShaderEnvironment(
	const Vulkan::Context& VulkanContext, const BitmapHeapT& BitmapHeap,
	Vulkan::DescriptorUpdateBatch& DescriptorUpdateBatch)
	: Shader(VulkanContext, BitmapHeap, DescriptorUpdateBatch)
{
	DescriptorHeap = std::make_unique<Vulkan::DescriptorHeap>(
		Vulkan::DescriptorHeap::Create(VulkanContext, Bindings).value());

	Vulkan::SetObjectName(
		VulkanContext.LogicalDevice, DescriptorHeap->GetDescriptorPool(),
		"Shader Environment Descriptor Pool");
	Vulkan::SetObjectName(
		VulkanContext.LogicalDevice, DescriptorHeap->GetDescriptorSetLayout(),
		"Shader Environment Descriptor Set Layout");
}

ShaderEnvironment::~ShaderEnvironment()
{
}

bool ShaderEnvironment::RegisterShader(
	const Blam::TagIndexEntry&               TagEntry,
	const Blam::Tag<Blam::TagClass::Shader>& Shader)
{
	if( TagEntry.ClassPrimary != Blam::TagClass::ShaderEnvironment )
	{
		return false;
	}
	const auto* ShaderEnvironment
		= reinterpret_cast<const Blam::Tag<Blam::TagClass::ShaderEnvironment>*>(
			&Shader);

	vk::DescriptorSet NewSet = {};

	if( auto AllocResult = DescriptorHeap->AllocateDescriptorSet();
		AllocResult.has_value() )
	{
		NewSet = AllocResult.value();
	}
	else
	{
		// Error allocating new descriptor set
		return false;
	}

	ShaderEnvironmentBindings[TagEntry.TagID] = NewSet;

	const auto WriteImageTag
		= [&](std::uint8_t Binding, std::uint32_t TagID,
			  Blam::DefaultTextureIndex DefaultIndex) -> void {
		if( TagID == 0xFFFFFFFF )
		{
			std::uint32_t DefaultImageTag;
			switch( ImageTypes[Binding] )
			{
			default:
			case vk::ImageViewType::e2D:
				DefaultImageTag = BitmapHeap.Default2D;
				break;
			case vk::ImageViewType::e3D:
				DefaultImageTag = BitmapHeap.Default3D;
				break;
			case vk::ImageViewType::eCube:
				DefaultImageTag = BitmapHeap.DefaultCube;
				break;
			}
			DescriptorUpdateBatch.AddImage(
				NewSet, Binding,
				BitmapHeap.Bitmaps.at(DefaultImageTag)
					.at(std::uint32_t(DefaultIndex))
					.View.get());
			return;
		}
		DescriptorUpdateBatch.AddImage(
			NewSet, Binding, BitmapHeap.Bitmaps.at(TagID).at(0).View.get());
	};

	WriteImageTag(
		0, ShaderEnvironment->BaseMap.TagID,
		Blam::DefaultTextureIndex::Additive);
	WriteImageTag(
		1, ShaderEnvironment->BumpMap.TagID, Blam::DefaultTextureIndex::Vector);
	WriteImageTag(
		2, ShaderEnvironment->PrimaryDetailMap.TagID,
		Blam::DefaultTextureIndex::SignedAdditive);
	WriteImageTag(
		3, ShaderEnvironment->SecondaryDetailMap.TagID,
		Blam::DefaultTextureIndex::SignedAdditive);
	WriteImageTag(
		4, ShaderEnvironment->MicroDetailMap.TagID,
		Blam::DefaultTextureIndex::SignedAdditive);
	WriteImageTag(
		5, ShaderEnvironment->GlowMap.TagID,
		Blam::DefaultTextureIndex::Additive);
	WriteImageTag(
		6, ShaderEnvironment->ReflectionCubeMap.TagID,
		Blam::DefaultTextureIndex::Additive);

	return true;
}

} // namespace VkBlam