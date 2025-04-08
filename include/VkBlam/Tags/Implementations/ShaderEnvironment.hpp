#pragma once

#include "Shader.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

class ShaderEnvironment final
	: public ShaderImplementationBase<Blam::TagClass::ShaderEnvironment>
{
public:
	ShaderEnvironment(
		const Blam::TagIndexEntry&                          TagIndexEntry,
		const Blam::Tag<Blam::TagClass::ShaderEnvironment>& Tag
	);
	~ShaderEnvironment();

	friend class ShaderEnvironmentSubsystem;
};

class ShaderEnvironmentSubsystem
	: public ShaderSubsystemBase<
		  ShaderEnvironment, Blam::TagClass::ShaderEnvironment>
{
private:
	VkBlam::Rasterizer& Rasterizer;

	vk::ShaderModule ShaderEnvironmentVertexShaderModule;
	vk::ShaderModule ShaderEnvironmentFragmentShaderModule;

	std::unique_ptr<Vulkan::DescriptorHeap> ShaderEnvironmentDescriptorPool;

	std::vector<std::unique_ptr<ShaderEnvironment>> ShaderEnvironments;

public:
	ShaderEnvironmentSubsystem(TagPool& Pool, VkBlam::Rasterizer& Rasterizer);
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