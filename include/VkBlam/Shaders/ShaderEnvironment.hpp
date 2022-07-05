#pragma once

#include <memory>
#include <unordered_map>

#include <VkBlam/Shader.hpp>
#include <VkBlam/VkBlam.hpp>

#include <Vulkan/DescriptorHeap.hpp>

namespace VkBlam
{
class ShaderEnvironment final : public Shader
{
private:
	std::unique_ptr<Vulkan::DescriptorHeap> DescriptorHeap;

	std::unordered_map<std::uint32_t, vk::DescriptorSet>
		ShaderEnvironmentBindings;

public:
	ShaderEnvironment(
		const Vulkan::Context&         VulkanContext,
		const VkBlam::BitmapHeapT&     BitmapHeap,
		Vulkan::DescriptorUpdateBatch& DescriptorUpdateBatch);
	virtual ~ShaderEnvironment();

	bool RegisterShader(
		const Blam::TagIndexEntry&               TagEntry,
		const Blam::Tag<Blam::TagClass::Shader>& Shader) override;
};
} // namespace VkBlam