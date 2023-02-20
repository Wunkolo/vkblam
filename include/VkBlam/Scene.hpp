#pragma once

#include <optional>

#include <VkBlam/Renderer.hpp>
#include <VkBlam/SceneView.hpp>
#include <VkBlam/World.hpp>

#include <Vulkan/DescriptorHeap.hpp>

namespace VkBlam
{

// All rendering state associated with a world.
class Scene
{
private:
	const World& TargetWorld;
	Renderer&    TargetRenderer;

	Scene(Renderer& TargetRenderer, const World& TargetWorld);

	// Temporary
	std::unordered_map<std::uint32_t, vk::DescriptorSet>
											ShaderEnvironmentDescriptors;
	std::unique_ptr<Vulkan::DescriptorHeap> ShaderEnvironmentDescriptorPool;

	std::unique_ptr<Vulkan::DescriptorHeap> DebugDrawDescriptorPool;

	vk::UniquePipeline       DebugDrawPipeline       = {};
	vk::UniquePipelineLayout DebugDrawPipelineLayout = {};

	std::unique_ptr<Vulkan::DescriptorHeap> UnlitDescriptorPool;

	vk::UniquePipeline       UnlitDrawPipeline       = {};
	vk::UniquePipelineLayout UnlitDrawPipelineLayout = {};

	vk::ShaderModule DefaultVertexShaderModule;
	vk::ShaderModule DefaultFragmentShaderModule;
	vk::ShaderModule UnlitFragmentShaderModule;

	// Contains _both_ the vertex buffers and the index buffer
	vk::UniqueDeviceMemory BSPGeometryMemory = {};

	vk::UniqueBuffer BSPVertexBuffer         = {};
	vk::UniqueBuffer BSPLightmapVertexBuffer = {};
	vk::UniqueBuffer BSPIndexBuffer          = {};

	struct LightmapMesh
	{
		std::uint32_t VertexIndexOffset = 0;
		std::uint32_t IndexCount        = 0;
		std::uint32_t IndexOffset       = 0;

		std::span<const Blam::Vertex>         VertexData;
		std::span<const Blam::LightmapVertex> LightmapVertexData;
		std::span<const std::byte>            IndexData;

		std::uint32_t ShaderTag;

		// Some lightmap meshes don't have a lightmap!
		std::optional<std::uint32_t> LightmapTag;
		std::optional<std::uint32_t> LightmapIndex;
	};
	std::vector<LightmapMesh> LightmapMeshs;

	vk::UniqueDeviceMemory BitmapHeapMemory = {};
	BitmapHeapT            BitmapHeap       = {};

	std::unique_ptr<Vulkan::DescriptorHeap> SceneDescriptorPool;

	vk::DescriptorSet CurSceneDescriptor = {};

public:
	~Scene();

	Scene(Scene&&) = default;

	void Render(const SceneView& View, vk::CommandBuffer CommandBuffer);

	static std::optional<Scene>
		Create(Renderer& TargetRenderer, const World& TargetWorld);
};
} // namespace VkBlam