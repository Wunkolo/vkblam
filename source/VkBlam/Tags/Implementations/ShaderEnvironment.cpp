#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/ShaderEnvironment.hpp>

#include <Vulkan/Memory.hpp>

namespace
{
const auto [VertexBindingDescriptions, VertexAttributeDescriptions]
	= VkBlam::GetVertexInputDescriptions({{
		Blam::VertexFormat::SBSPVertexUncompressed,
		Blam::VertexFormat::SBSPLightmapVertexUncompressed,
	}});

vk::DescriptorSetLayoutBinding ShaderEnvironmentBindings[] = {
	{// Basemap
	 0, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// PrimaryDetailMap
	 1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// SecondaryDetailMap
	 2, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// MicroDetailMap
	 3, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// BumpMap
	 4, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// GlowMap
	 5, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
	{// ReflectionCubeMap
	 6, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment
	},
};
} // namespace

namespace VkBlam::Tags
{

ShaderEnvironment::~ShaderEnvironment()
{
}

ShaderEnvironmentSubsystem::ShaderEnvironmentSubsystem(
	TagPool& Pool, VkBlam::Renderer& Renderer
)
	: TagSubsystem<Blam::TagClass::ShaderEnvironment, ShaderEnvironment>(Pool),
	  Renderer(Renderer)
{
	const auto& VulkanContext = Renderer.GetVulkanContext();

	ShaderEnvironmentDescriptorPool = std::make_unique<Vulkan::DescriptorHeap>(
		Vulkan::DescriptorHeap::Create(
			Renderer.GetVulkanContext(), ShaderEnvironmentBindings
		)
			.value()
	);
	const auto ShaderEnvironmentVertShaderData
		= VkBlam::OpenResource("shaders/Default.vert.spv").value();
	const auto ShaderEnvironmentFragShaderData
		= VkBlam::OpenResource("shaders/Default.frag.spv").value();

	std::hash<std::string> StringHasher = {};

	ShaderEnvironmentVertexShaderModule
		= Renderer.GetShaderModuleCache()
			  .GetShaderModule(
				  StringHasher("shaders/Default.vert.spv"),
				  ShaderEnvironmentVertShaderData
			  )
			  .value();
	ShaderEnvironmentFragmentShaderModule
		= Renderer.GetShaderModuleCache()
			  .GetShaderModule(
				  StringHasher("shaders/Default.frag.spv"),
				  ShaderEnvironmentFragShaderData
			  )
			  .value();

	const vk::RenderPass RenderPass
		= Renderer.GetDefaultRenderPass(RenderSamples);

	// std::tie(ShaderEnvironmentPipeline, ShaderEnvironmentPipelineLayout)
	// 	= CreateGraphicsPipeline(
	// 		VulkanContext.LogicalDevice,
	// 		{{vk::PushConstantRange{
	// 			.stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
	// 			.offset     = 0,
	// 			.size       = sizeof(VkBlam::CameraGlobals),
	// 		}}},
	// 		{{SceneDescriptorPool->GetDescriptorSetLayout(),
	// 		  ShaderEnvironmentDescriptorPool->GetDescriptorSetLayout(),
	// 		  DebugDrawDescriptorPool->GetDescriptorSetLayout()}},
	// 		ShaderEnvironmentVertexShaderModule,
	// 		ShaderEnvironmentFragmentShaderModule, VertexBindingDescriptions,
	// 		VertexAttributeDescriptions, RenderPass, RenderSamples,
	// 		vk::PolygonMode::eFill
	// 	);
}

ShaderEnvironmentSubsystem::~ShaderEnvironmentSubsystem()
{
}

std::vector<DependentTag> ShaderEnvironmentSubsystem::GetDependentTags(
	const Blam::TagIndexEntry&                          TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ShaderEnvironment>& Tag,
	const Blam::MapFile&                                MapFile
) const
{
	std::vector<DependentTag> DependentTags;

	// Default2D/3D/Cube textures
	if( const Blam::TagIndexEntry* GlobalsTagEntry
		= MapFile.FindTagIndexEntry("globals\\globals");
		GlobalsTagEntry )
	{
		DependentTags.push_back({GlobalsTagEntry->TagID});
	}

	if( Tag.BaseMap.Valid() )
	{
		DependentTags.push_back({Tag.BaseMap.TagID});
	}
	if( Tag.PrimaryDetailMap.Valid() )
	{
		DependentTags.push_back({Tag.PrimaryDetailMap.TagID});
	}
	if( Tag.SecondaryDetailMap.Valid() )
	{
		DependentTags.push_back({Tag.SecondaryDetailMap.TagID});
	}
	if( Tag.MicroDetailMap.Valid() )
	{
		DependentTags.push_back({Tag.MicroDetailMap.TagID});
	}
	if( Tag.BumpMap.Valid() )
	{
		DependentTags.push_back({Tag.BumpMap.TagID});
	}
	if( Tag.GlowMap.Valid() )
	{
		DependentTags.push_back({Tag.GlowMap.TagID});
	}
	if( Tag.ReflectionCubeMap.Valid() )
	{
		DependentTags.push_back({Tag.ReflectionCubeMap.TagID});
	}

	return DependentTags;
}

ShaderEnvironment* ShaderEnvironmentSubsystem::LoadTag(
	const Blam::TagIndexEntry&                          TagIndexEntry,
	const Blam::Tag<Blam::TagClass::ShaderEnvironment>& Tag, Scene& TargetScene
)
{
	std::unique_ptr<ShaderEnvironment> NewShaderEnvironment(
		new ShaderEnvironment()
	);

	return ShaderEnvironments.emplace_back(std::move(NewShaderEnvironment))
		.get();
}
} // namespace VkBlam::Tags