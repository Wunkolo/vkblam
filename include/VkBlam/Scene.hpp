#pragma once

#include <memory>

#include <VkBlam/Rasterizer.hpp>
#include <VkBlam/SceneView.hpp>
#include <VkBlam/World.hpp>

#include <Vulkan/DescriptorHeap.hpp>

namespace VkBlam
{

class TagPool;

// All rendering state associated with a world.
class Scene
{
private:
	const World&             TargetWorld;
	Rasterizer&              TargetRasterizer;
	std::unique_ptr<TagPool> Pool;

	Scene(Rasterizer& TargetRasterizer, const World& TargetWorld);

	// std::unique_ptr<Vulkan::DescriptorHeap> DebugDrawDescriptorPool;

	// std::unique_ptr<Vulkan::DescriptorHeap> UnlitDescriptorPool;

	// vk::UniquePipeline       UnlitDrawPipeline       = {};
	// vk::UniquePipelineLayout UnlitDrawPipelineLayout = {};

	// vk::ShaderModule UnlitFragmentShaderModule;

	std::unique_ptr<Vulkan::DescriptorHeap> SceneDescriptorPool;

	vk::DescriptorSet CurSceneDescriptor = {};

public:
	~Scene();

	Scene(Scene&&) = default;

	const World& GetWorld() const
	{
		return TargetWorld;
	};

	const Blam::MapFile& GetMapFile() const
	{
		return TargetWorld.GetMapFile();
	};

	Rasterizer& GetRasterizer() const
	{
		return TargetRasterizer;
	};

	const Vulkan::Context& GetVulkanContext() const
	{
		return TargetRasterizer.GetVulkanContext();
	};

	void Render(const SceneView& View, vk::CommandBuffer CommandBuffer);

	static std::unique_ptr<Scene>
		Create(Rasterizer& TargetRasterizer, const World& TargetWorld);
};
} // namespace VkBlam