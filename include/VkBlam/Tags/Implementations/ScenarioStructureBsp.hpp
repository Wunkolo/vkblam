#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

struct LightmapMesh
{
	std::uint32_t VertexIndexOffset = 0;
	std::uint32_t IndexCount        = 0;

	std::span<const Blam::Vertex>         VertexData;
	std::span<const Blam::LightmapVertex> LightmapVertexData;

	std::uint32_t ShaderTag;

	// Some lightmap meshes don't have a lightmap!
	std::optional<std::uint32_t> LightmapTag;
	std::optional<std::uint32_t> LightmapIndex;
};
class ScenarioStructureBsp final
	: public TagImplementation<Blam::TagClass::ScenarioStructureBsp>
{
private:
	const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP& SBSPData;

	// Contains _both_ the vertex buffers and the index buffer
	vk::UniqueDeviceMemory BSPGeometryMemory = {};

	vk::UniqueBuffer BSPVertexBuffer         = {};
	vk::UniqueBuffer BSPLightmapVertexBuffer = {};
	vk::UniqueBuffer BSPIndexBuffer          = {};

	std::vector<LightmapMesh> LightmapMeshs;

public:
	ScenarioStructureBsp(
		const Blam::TagIndexEntry&                               TagIndexEntry,
		const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>&   Tag,
		const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP& SBSPData
	);
	~ScenarioStructureBsp();

	friend class ScenarioStructureBspSubsystem;
};

class ScenarioStructureBspSubsystem final
	: public TagSubsystem<
		  Blam::TagClass::ScenarioStructureBsp, ScenarioStructureBsp>
{
private:
	Rasterizer& TargetRasterizer;

	std::vector<std::unique_ptr<ScenarioStructureBsp>> ScenarioStructureBsps;

	vk::UniquePipeline       DebugDrawPipeline       = {};
	vk::UniquePipelineLayout DebugDrawPipelineLayout = {};

public:
	ScenarioStructureBspSubsystem(TagPool& Pool, Rasterizer& TargetRasterizer);
	~ScenarioStructureBspSubsystem();

	[[nodiscard]] std::vector<DependentTag> GetDependentTags(
		const Blam::TagIndexEntry&                             TagIndexEntry,
		const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& Tag,
		const Blam::MapFile&                                   MapFile
	) const override;

	[[nodiscard]] ScenarioStructureBsp* LoadTag(
		const Blam::TagIndexEntry&                             TagIndexEntry,
		const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& Tag,
		Scene&                                                 TargetScene
	) override;

	[[deprecated("To be removed for a render-architecture refactor")]]
	void Draw(
		ScenarioStructureBsp& ScenarioStructureBsp, const SceneView& View,
		vk::CommandBuffer CommandBuffer
	);
};

} // namespace VkBlam::Tags