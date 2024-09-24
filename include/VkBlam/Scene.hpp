#pragma once

#include <optional>

#include <VkBlam/Renderer.hpp>
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
	Renderer&                TargetRenderer;
	std::unique_ptr<TagPool> Pool;

	Scene(Renderer& TargetRenderer, const World& TargetWorld);

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

	Renderer& GetRenderer() const
	{
		return TargetRenderer;
	};

	const Vulkan::Context& GetVulkanContext() const
	{
		return TargetRenderer.GetVulkanContext();
	};

	void Render(const SceneView& View, vk::CommandBuffer CommandBuffer);

	static std::optional<Scene>
		Create(Renderer& TargetRenderer, const World& TargetWorld);
};
} // namespace VkBlam