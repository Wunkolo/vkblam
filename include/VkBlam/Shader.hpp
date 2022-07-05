#pragma once

#include <VkBlam/VkBlam.hpp>

#include <Vulkan/DescriptorUpdateBatch.hpp>

#include <Blam/Tags.hpp>

namespace VkBlam
{

// Each shader creates a derived graphics pipeline as well as a
// descriptor set+buffer that flattens out all of the material
// parameters that need to be passed onto the fragment shader
// itself into a uniform buffer
// There might possibly be some runtime overhead needed for the
// animated U/V functions and other animation phases but they might
// just be able to be derived at runtime from the current time
// without having to maintain a per-shader animation-state
class Shader
{
protected:
	const vk::Device   LogicalDevice;
	const BitmapHeapT& BitmapHeap;

	Vulkan::DescriptorUpdateBatch& DescriptorUpdateBatch;

	Shader(
		vk::Device LogicalDevice, const BitmapHeapT& BitmapHeap,
		Vulkan::DescriptorUpdateBatch& DescriptorUpdateBatch);

public:
	virtual ~Shader() = default;

	virtual bool RegisterShader(
		const Blam::TagIndexEntry&               TagEntry,
		const Blam::Tag<Blam::TagClass::Shader>& Shader)
		= 0;
};
} // namespace VkBlam