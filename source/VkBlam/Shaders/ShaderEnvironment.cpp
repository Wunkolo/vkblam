
#include "Blam/Enums.hpp"
#include <VkBlam/Shaders/ShaderEnvironment.hpp>

#include <array>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

static vk::DescriptorSetLayoutBinding Bindings[] = {
	{// BaseMap
	 0, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// PrimaryDetailMap
	 1, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// SecondaryDetailMap
	 2, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// MicroDetailMap
	 3, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// GlowMap
	 4, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
	{// ReflectionCubeMap
	 5, vk::DescriptorType::eSampledImage, 1,
	 vk::ShaderStageFlagBits::eFragment},
};

namespace VkBlam
{

ShaderEnvironment::ShaderEnvironment(
	vk::Device LogicalDevice, const BitmapHeapT& BitmapHeap,
	Vulkan::DescriptorUpdateBatch& DescriptorUpdateBatch)
	: Shader(LogicalDevice, BitmapHeap, DescriptorUpdateBatch)
{
	DescriptorHeap = std::make_unique<Vulkan::DescriptorHeap>(
		Vulkan::DescriptorHeap::Create(LogicalDevice, Bindings).value());
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

	vk::DescriptorSet NewSet = DescriptorHeap->AllocateDescriptorSet().value();

	const auto WriteImageTag
		= [&](std::uint8_t Binding, std::uint32_t TagID) -> void {
		if( TagID == 0xFFFFFFFF )
		{
			// Todo: Handle default texture input for different texture types
			// Globals has a rasterization block for this.
			return;
		}
		DescriptorUpdateBatch.AddImage(
			NewSet, Binding, BitmapHeap.Views.at(TagID).at(0).get());
	};

	WriteImageTag(0, ShaderEnvironment->BaseMap.TagID);
	WriteImageTag(1, ShaderEnvironment->PrimaryDetailMap.TagID);
	WriteImageTag(2, ShaderEnvironment->SecondaryDetailMap.TagID);
	WriteImageTag(3, ShaderEnvironment->MicroDetailMap.TagID);
	WriteImageTag(4, ShaderEnvironment->GlowMap.TagID);
	WriteImageTag(5, ShaderEnvironment->ReflectionCubeMap.TagID);

	return true;
}

} // namespace VkBlam