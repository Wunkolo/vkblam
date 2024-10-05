#include <VkBlam/Format.hpp>
#include <VkBlam/Tags/Implementations/Globals.hpp>

namespace VkBlam::Tags
{

Globals::Globals(
	const Blam::TagIndexEntry&                TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Globals>& Tag
)
	: TagImplementation<Blam::TagClass::Globals>(TagIndexEntry, Tag)
{
}

Globals::~Globals()
{
}

GlobalsSubsystem::GlobalsSubsystem(TagPool& Pool)
	: TagSubsystem<Blam::TagClass::Globals, Globals>(Pool)
{
}

GlobalsSubsystem::~GlobalsSubsystem()
{
}

std::vector<DependentTag> GlobalsSubsystem::GetDependentTags(
	const Blam::TagIndexEntry&                TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Globals>& Tag, const Blam::MapFile& MapFile
) const
{
	std::vector<DependentTag> DependentTags;

	// Load all rasterizer texture data
	// There _should_ only be one of these
	for( const Blam::Tag<Blam::TagClass::Globals>::RasterizerDataEntry&
			 RasterData : MapFile.TagHeap.GetBlock(Tag.RasterizerData) )
	{
		DependentTags.push_back({RasterData.DistanceAttenuation.TagID});
		DependentTags.push_back({RasterData.VectorNormalization.TagID});
		DependentTags.push_back({RasterData.AtmosphericFogDensity.TagID});
		DependentTags.push_back({RasterData.PlanarFogDensity.TagID});
		DependentTags.push_back({RasterData.LinearCornerFade.TagID});
		DependentTags.push_back({RasterData.ActiveCamouflageDistortion.TagID});
		DependentTags.push_back({RasterData.Glow.TagID});
		DependentTags.push_back({RasterData.Default2D.TagID});
		DependentTags.push_back({RasterData.Default3D.TagID});
		DependentTags.push_back({RasterData.DefaultCube.TagID});
		DependentTags.push_back({RasterData.Test0.TagID});
		DependentTags.push_back({RasterData.Test1.TagID});
		DependentTags.push_back({RasterData.VideoScanlineMap.TagID});
		DependentTags.push_back({RasterData.VideoNoiseMap.TagID});
	}

	return DependentTags;
}

Globals* GlobalsSubsystem::LoadTag(
	const Blam::TagIndexEntry&                TagIndexEntry,
	const Blam::Tag<Blam::TagClass::Globals>& Tag, Scene& TargetScene
)
{
	std::unique_ptr<Globals> NewGlobals(new Globals(TagIndexEntry, Tag));

	return Globalss.emplace_back(std::move(NewGlobals)).get();
}
} // namespace VkBlam::Tags