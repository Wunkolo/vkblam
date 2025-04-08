#pragma once

#include "../TagImplementation.hpp"

#include <Blam/Tags.hpp>

#include <vector>

namespace VkBlam::Tags
{

// Shader subsystem base-type
template<
	typename TypeImplementationT,
	Blam::TagClass TagClassT = Blam::TagClass::Shader>
class ShaderSubsystemBase;

template<Blam::TagClass TagClassT = Blam::TagClass::Shader>
class ShaderImplementationBase : public TagImplementation<TagClassT>
{
protected:
	vk::DescriptorSet DescriptorSet;

public:
	ShaderImplementationBase(
		const Blam::TagIndexEntry&  TagIndexEntry,
		const Blam::Tag<TagClassT>& Tag
	)
		: TagImplementation<TagClassT>(TagIndexEntry, Tag) {};

	virtual ~ShaderImplementationBase() = default;

	[[nodiscard]] vk::DescriptorSet GetDescriptorSet() const
	{
		return DescriptorSet;
	}

	friend class ShaderSubsystemBase<ShaderImplementationBase, TagClassT>;
};

template<typename TypeImplementationT, Blam::TagClass TagClassT>
class ShaderSubsystemBase : public TagSubsystem<TagClassT, TypeImplementationT>
{
protected:
	VkBlam::Rasterizer& Rasterizer;

	vk::UniquePipeline       ShaderPipeline;
	vk::UniquePipelineLayout ShaderPipelineLayout;

	vk::ShaderModule ShaderVertexShaderModule;
	vk::ShaderModule ShaderFragmentShaderModule;

	std::unique_ptr<Vulkan::DescriptorHeap> ShaderDescriptorPool;

	std::vector<std::unique_ptr<ShaderImplementationBase<TagClassT>>> Shaders;

public:
	ShaderSubsystemBase(TagPool& Pool, VkBlam::Rasterizer& Rasterizer)
		: TagSubsystem<TagClassT, TypeImplementationT>(Pool),
		  Rasterizer(Rasterizer)
	{
	}
	virtual ~ShaderSubsystemBase() = default;

	[[nodiscard]] const vk::Pipeline& GetPipeline() const
	{
		return ShaderPipeline.get();
	}

	[[nodiscard]] const vk::PipelineLayout& GetPipelineLayout() const
	{
		return ShaderPipelineLayout.get();
	}

	[[nodiscard]] std::vector<DependentTag> GetDependentTags(
		const Blam::TagIndexEntry&  TagIndexEntry,
		const Blam::Tag<TagClassT>& Tag, const Blam::MapFile& MapFile
	) const override
		= 0;

	[[nodiscard]] TypeImplementationT* LoadTag(
		const Blam::TagIndexEntry&  TagIndexEntry,
		const Blam::Tag<TagClassT>& Tag, Scene& TargetScene
	) override
		= 0;
};

using Shader          = ShaderImplementationBase<Blam::TagClass::Shader>;
using ShaderSubsystem = ShaderSubsystemBase<Shader, Blam::TagClass::Shader>;

} // namespace VkBlam::Tags