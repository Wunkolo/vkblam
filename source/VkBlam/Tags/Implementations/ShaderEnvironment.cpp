#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/ShaderEnvironment.hpp>

#include <VkBlam/Tags/Implementations/Bitmap.hpp>
#include <VkBlam/Tags/Implementations/Globals.hpp>
#include <VkBlam/Tags/TagPool.hpp>

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

static vk::ImageViewType ImageTypes[] = {
	// BaseMap
	vk::ImageViewType::e2D,
	// BumpMap
	vk::ImageViewType::e2D,
	// PrimaryDetailMap
	vk::ImageViewType::e2D,
	// SecondaryDetailMap
	vk::ImageViewType::e2D,
	// MicroDetailMap
	vk::ImageViewType::e2D,
	// GlowMap
	vk::ImageViewType::e2D,
	// ReflectionCubeMap
	vk::ImageViewType::eCube,
};

} // namespace

namespace VkBlam::Tags
{

ShaderEnvironment::~ShaderEnvironment()
{
}

vk::DescriptorSet ShaderEnvironment::GetDescriptorSet() const
{
	return DescriptorSet;
}

ShaderEnvironmentSubsystem::ShaderEnvironmentSubsystem(
	TagPool& Pool, VkBlam::Rasterizer& Rasterizer
)
	: TagSubsystem<Blam::TagClass::ShaderEnvironment, ShaderEnvironment>(Pool),
	  Rasterizer(Rasterizer)
{
	const auto& VulkanContext = Rasterizer.GetVulkanContext();

	ShaderEnvironmentDescriptorPool = std::make_unique<Vulkan::DescriptorHeap>(
		Vulkan::DescriptorHeap::Create(
			Rasterizer.GetVulkanContext(), ShaderEnvironmentBindings
		)
			.value()
	);
	const auto ShaderEnvironmentVertShaderData
		= VkBlam::OpenResource("shaders/Default.vert.spv").value();
	const auto ShaderEnvironmentFragShaderData
		= VkBlam::OpenResource("shaders/Default.frag.spv").value();

	std::hash<std::string> StringHasher = {};

	ShaderEnvironmentVertexShaderModule
		= Rasterizer.GetShaderModuleCache()
			  .GetShaderModule(
				  StringHasher("shaders/Default.vert.spv"),
				  ShaderEnvironmentVertShaderData
			  )
			  .value();
	ShaderEnvironmentFragmentShaderModule
		= Rasterizer.GetShaderModuleCache()
			  .GetShaderModule(
				  StringHasher("shaders/Default.frag.spv"),
				  ShaderEnvironmentFragShaderData
			  )
			  .value();

	const vk::RenderPass RenderPass
		= Rasterizer.GetDefaultRenderPass(RenderSamples);

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

	// Create descriptor set

	if( auto AllocResult
		= ShaderEnvironmentDescriptorPool->AllocateDescriptorSet();
		AllocResult.has_value() )
	{
		NewShaderEnvironment->DescriptorSet = AllocResult.value();
	}
	else
	{
		// Error allocating new descriptor set
		return nullptr;
	}

	Vulkan::SetObjectName(
		Rasterizer.GetVulkanContext().LogicalDevice,
		NewShaderEnvironment->DescriptorSet, "ShaderEnvironment[{:08X}]: {}",
		TagIndexEntry.TagID,
		TargetScene.GetMapFile().GetTagPath(TagIndexEntry.TagID)
	);

	// Get default rasterizer images from globals;
	const Blam::TagIndexEntry* GlobalsTagEntry
		= TargetScene.GetMapFile().FindTagIndexEntry("globals\\globals");

	if( GlobalsTagEntry == nullptr )
	{
		// Error getting globals tag
		return nullptr;
	}

	const Blam::Tag<Blam::TagClass::Globals>* Globals
		= TargetScene.GetMapFile().GetTag<Blam::TagClass::Globals>(
			GlobalsTagEntry->TagID
		);

	if( Globals == nullptr )
	{
		// Error loading globals tag
		return nullptr;
	}

	const auto RasterizerData
		= TargetScene.GetMapFile().TagHeap.GetBlock(Globals->RasterizerData)[0];

	const auto WriteImageTag
		= [&](std::uint8_t Binding, std::uint32_t TagID,
			  Blam::DefaultTextureIndex DefaultIndex) -> void {
		if( TagID == 0xFFFFFFFF )
		{
			std::uint32_t DefaultImageTag;
			switch( ImageTypes[Binding] )
			{
			default:
			case vk::ImageViewType::e2D:
				DefaultImageTag = RasterizerData.Default2D.TagID;
				break;
			case vk::ImageViewType::e3D:
				DefaultImageTag = RasterizerData.Default3D.TagID;
				break;
			case vk::ImageViewType::eCube:
				DefaultImageTag = RasterizerData.DefaultCube.TagID;
				break;
			}
			Rasterizer.GetDescriptorUpdateBatch().AddImage(
				NewShaderEnvironment->DescriptorSet, Binding,
				GetPool()
					.LoadTag<Tags::Bitmap>(DefaultImageTag)
					->GetBitmap(std::uint16_t(DefaultIndex))
					.View.get()
			);
			return;
		}
		Rasterizer.GetDescriptorUpdateBatch().AddImage(
			NewShaderEnvironment->DescriptorSet, Binding,
			GetPool().LoadTag<Tags::Bitmap>(TagID)->GetBitmap(0).View.get()
		);
	};

	WriteImageTag(0, Tag.BaseMap.TagID, Blam::DefaultTextureIndex::Additive);
	WriteImageTag(1, Tag.BumpMap.TagID, Blam::DefaultTextureIndex::Vector);
	WriteImageTag(
		2, Tag.PrimaryDetailMap.TagID, Blam::DefaultTextureIndex::SignedAdditive
	);
	WriteImageTag(
		3, Tag.SecondaryDetailMap.TagID,
		Blam::DefaultTextureIndex::SignedAdditive
	);
	WriteImageTag(
		4, Tag.MicroDetailMap.TagID, Blam::DefaultTextureIndex::SignedAdditive
	);
	WriteImageTag(5, Tag.GlowMap.TagID, Blam::DefaultTextureIndex::Additive);
	WriteImageTag(
		6, Tag.ReflectionCubeMap.TagID, Blam::DefaultTextureIndex::Additive
	);

	Rasterizer.GetDescriptorUpdateBatch().Flush();

	return ShaderEnvironments.emplace_back(std::move(NewShaderEnvironment))
		.get();
}
} // namespace VkBlam::Tags