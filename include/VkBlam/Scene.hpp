#pragma once

#include <optional>

#include <VkBlam/Renderer.hpp>
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
	std::unique_ptr<Vulkan::DescriptorHeap> TrivialDescriptorPool;

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

		// Some lightmap meshes don't have a lightmap!
		std::optional<std::uint32_t> LightmapTag;
		std::optional<std::uint32_t> LightmapIndex;
	};
	std::vector<LightmapMesh> LightmapMeshs;

	vk::UniqueDeviceMemory BitmapHeapMemory = {};
	BitmapHeapT            BitmapHeap       = {};

public:
	~Scene();

	Scene(Scene&&) = default;

	static std::optional<Scene>
		Create(Renderer& TargetRenderer, const World& TargetWorld);
};
} // namespace VkBlam