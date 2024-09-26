#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

class ShaderEnvironment final
	: public TagImplementation<Blam::TagClass::ShaderEnvironment>
{
private:
	vk::DescriptorSet DescriptorSet;

public:
	~ShaderEnvironment();

	friend class ShaderEnvironmentSubsystem;
};

class ShaderEnvironmentSubsystem final
	: public TagSubsystem<Blam::TagClass::ShaderEnvironment, ShaderEnvironment>
{
private:
	VkBlam::Renderer& Renderer;

	vk::UniquePipeline       ShaderEnvironmentPipeline       = {};
	vk::UniquePipelineLayout ShaderEnvironmentPipelineLayout = {};

	vk::ShaderModule ShaderEnvironmentVertexShaderModule;
	vk::ShaderModule ShaderEnvironmentFragmentShaderModule;

	std::unique_ptr<Vulkan::DescriptorHeap> ShaderEnvironmentDescriptorPool;

	std::vector<std::unique_ptr<ShaderEnvironment>> ShaderEnvironments;

public:
	ShaderEnvironmentSubsystem(TagPool& Pool, VkBlam::Renderer& Renderer);
	~ShaderEnvironmentSubsystem();

	[[nodiscard]] std::vector<DependentTag> GetDependentTags(
		const Blam::TagIndexEntry&                          TagIndexEntry,
		const Blam::Tag<Blam::TagClass::ShaderEnvironment>& Tag,
		const Blam::MapFile&                                MapFile
	) const override;

	[[nodiscard]] ShaderEnvironment* LoadTag(
		const Blam::TagIndexEntry&                          TagIndexEntry,
		const Blam::Tag<Blam::TagClass::ShaderEnvironment>& Tag,
		Scene&                                              TargetScene
	) override;
};

} // namespace VkBlam::Tags