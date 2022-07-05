#pragma once

#include <memory>

#include <VkBlam/Shader.hpp>
#include <VkBlam/VkBlam.hpp>

#include <Vulkan/DescriptorHeap.hpp>

namespace VkBlam
{
class ShaderEnvironment final : public Shader
{
private:
	std::unique_ptr<Vulkan::DescriptorHeap> DescriptorHeap;

public:
	ShaderEnvironment(
		vk::Device LogicalDevice, const VkBlam::BitmapHeapT& BitmapHeap,
		Vulkan::DescriptorUpdateBatch& DescriptorUpdateBatch);
	virtual ~ShaderEnvironment() = default;

	bool RegisterShader(
		const Blam::TagIndexEntry&               TagEntry,
		const Blam::Tag<Blam::TagClass::Shader>& Shader) override;
};
} // namespace VkBlam