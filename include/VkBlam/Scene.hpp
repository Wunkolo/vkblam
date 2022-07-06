#pragma once

#include <optional>

#include <VkBlam/Renderer.hpp>
#include <VkBlam/World.hpp>

namespace VkBlam
{

// All rendering state associated with a world.
class Scene
{
private:
	const World& TargetWorld;
	Renderer&    TargetRenderer;

	Scene(Renderer& TargetRenderer, const World& TargetWorld);

	// Contains _both_ the vertex buffers and the index buffer
	vk::UniqueDeviceMemory BSPGeometryMemory = {};

	vk::UniqueBuffer BSPVertexBuffer;
	vk::UniqueBuffer BSPLightmapVertexBuffer;
	vk::UniqueBuffer BSPIndexBuffer;

	struct LightmapMesh
	{
		std::uint32_t VertexIndexOffset;
		std::uint32_t IndexCount;
		std::uint32_t IndexOffset;

		std::span<const Blam::Vertex>         VertexData;
		std::span<const Blam::LightmapVertex> LightmapVertexData;
		std::span<const std::byte>            IndexData;

		// Some lightmap meshes don't have a lightmap!
		std::optional<std::uint32_t> LightmapTag;
		std::optional<std::uint32_t> LightmapIndex;
	};
	std::vector<LightmapMesh> LightmapMeshs;

	BitmapHeapT BitmapHeap;

public:
	~Scene();

	Scene(Scene&&) = default;

	static std::optional<Scene>
		Create(Renderer& TargetRenderer, const World& TargetWorld);
};
} // namespace VkBlam