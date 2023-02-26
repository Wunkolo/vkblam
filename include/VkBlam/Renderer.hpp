#pragma once

#include <Common/Literals.hpp>

#include <VkBlam/VkBlam.hpp>
#include <VkBlam/World.hpp>

#include <Vulkan/Debug.hpp>
#include <Vulkan/DescriptorUpdateBatch.hpp>
#include <Vulkan/SamplerCache.hpp>
#include <Vulkan/ShaderModuleCache.hpp>
#include <Vulkan/StreamBuffer.hpp>

#include <memory>
#include <optional>

namespace VkBlam
{

using namespace Common::Literals;

struct RendererConfig
{
	vk::DeviceSize StreamBufferSize = 128_MiB;

	std::size_t DescriptorWriteMax = 256;
	std::size_t DescriptorCopyMax  = 256;
};

// Encapsulates the top-level global state of the renderer.
class Renderer
{
private:
	const Vulkan::Context& VulkanContext;

	vk::UniqueSampler DefaultSampler = {};

	// Temporary
	std::unordered_map<vk::SampleCountFlagBits, vk::UniqueRenderPass>
		DefaultRenderPasses = {};

	std::unique_ptr<Vulkan::StreamBuffer>          StreamBuffer;
	std::unique_ptr<Vulkan::SamplerCache>          SamplerCache;
	std::unique_ptr<Vulkan::ShaderModuleCache>     ShaderModuleCache;
	std::unique_ptr<Vulkan::DescriptorUpdateBatch> DescriptorUpdateBatch;

	Renderer(const Vulkan::Context& VulkanContext);

public:
	~Renderer();

	Renderer(Renderer&&) = default;

	const Vulkan::Context& GetVulkanContext() const
	{
		return VulkanContext;
	}

	Vulkan::StreamBuffer& GetStreamBuffer() const
	{
		return *StreamBuffer.get();
	}

	Vulkan::SamplerCache& GetSamplerCache() const
	{
		return *SamplerCache.get();
	}

	Vulkan::ShaderModuleCache& GetShaderModuleCache() const
	{
		return *ShaderModuleCache.get();
	}

	Vulkan::DescriptorUpdateBatch& GetDescriptorUpdateBatch() const
	{
		return *DescriptorUpdateBatch.get();
	}

	const vk::RenderPass&
		GetDefaultRenderPass(vk::SampleCountFlagBits SampleCount);

	static std::optional<Renderer> Create(
		const Vulkan::Context& VulkanContext, const RendererConfig& Config = {}
	);
};
} // namespace VkBlam