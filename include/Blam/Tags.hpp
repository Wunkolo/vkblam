#pragma once

#include "Enums.hpp"
#include "Types.hpp"

namespace Blam
{
#pragma pack(push, 1)
template<TagClass Class>
struct Tag
{
};

template<>
struct Tag<TagClass::Wind>
{
	float MinWindSpeed;
	float MaxWindSpeed;
	float VariationYaw;
	float VariationPitch;
	float LocalVariationWeight;
	float LocalVariationRate;
	float Damping;
};

template<>
struct Tag<TagClass::Scenario>
{
	// Depreciated fields, don't use
	TagReference                      _UnusedBSP0;
	TagReference                      _UnusedBSP1;
	TagReference                      _UnusedSky;
	TagBlock<Tag<TagClass::Sky>>      Skies;
	ScenarioType                      Type;
	std::uint16_t                     Flags;
	TagBlock<Tag<TagClass::Scenario>> ChildScenarios;
	float                             LocalNorth;

	std::byte _Padding50[0x9C];

	TagBlock<void> /*Todo*/ PredictedResources;
	TagBlock<void> /*Todo*/ Functions;
	TagDataReference        EditorData;
	TagBlock<void> /*Todo*/ Comments;

	std::byte _Padding124[0xE0];

	TagBlock<std::array<char, 32>> ObjectNames;
	TagBlock<void> /*Todo*/        Scenery;

	// A lot of the palettes are really just TagReference aligned up to a size
	// of 0x30. So this is just a utility-wrapper for TagReference that adds
	// the extra padding
	struct PaletteEntry : public TagReference
	{
	private:
		std::byte _Padding10[0x20];
	};
	static_assert(sizeof(PaletteEntry) == 0x30);

	TagBlock<PaletteEntry>  SceneryPalette;
	TagBlock<void> /*Todo*/ Bipeds;
	TagBlock<PaletteEntry>  BipedPalette;
	TagBlock<void> /*Todo*/ Vehicles;
	TagBlock<PaletteEntry>  VehiclePalette;
	TagBlock<void> /*Todo*/ Equipment;
	TagBlock<PaletteEntry>  EquipmentPalette;
	TagBlock<void> /*Todo*/ Weapons;
	TagBlock<PaletteEntry>  WeaponPalette;
	TagBlock<void> /*Todo*/ DeviceGroups;
	TagBlock<void> /*Todo*/ Machines;
	TagBlock<PaletteEntry>  MachinePalette;
	TagBlock<void> /*Todo*/ Controls;
	TagBlock<PaletteEntry>  ControlPalette;
	TagBlock<void> /*Todo*/ LightFixtures;
	TagBlock<PaletteEntry>  LightFixturePalette;
	TagBlock<void> /*Todo*/ SoundScenery;
	TagBlock<PaletteEntry>  SoundSceneryPalette;

	std::byte _Padding2F4[0x54];

	TagBlock<void> /*Todo*/ PlayerStartingProfile;

	struct PlayerStartingLocation
	{
		Vector3f      Position;
		float         Facing;
		std::uint16_t TeamIndex;
		std::uint16_t _BSPIndex;
		std::uint16_t Type0;
		std::uint16_t Type1;
		std::uint16_t Type2;
		std::uint16_t Type3;
		std::byte     _Padding[0x18];
	};
	static_assert(sizeof(PlayerStartingLocation) == 0x34);

	TagBlock<PlayerStartingLocation> PlayerStartingLocations;

	TagBlock<void> /*Todo*/ TriggerVolumes;
	TagBlock<void> /*Todo*/ RecordedAnimations;
	TagBlock<void> /*Todo*/ NetgameFlags;
	TagBlock<void> /*Todo*/ NetgameEquipment;
	TagBlock<void> /*Todo*/ StartingEquipment;
	TagBlock<void> /*Todo*/ BSPSwitchTriggerVolumes;

	struct Decal
	{
		std::uint16_t DecalIndex;
		std::int8_t   Yaw;
		std::int8_t   Pitch;
		Vector3f      Position;
	};
	static_assert(sizeof(Decal) == 0x10);
	TagBlock<Decal>        Decals;
	TagBlock<TagReference> DecalPalette;

	TagBlock<PaletteEntry> DetailObjectCollectionPalette;

	std::byte _Padding3CC[0x54];

	TagBlock<TagReference>  ActorPalette;
	TagBlock<void> /*Todo*/ Encounters;
	TagBlock<void> /*Todo*/ CommandLists;
	TagBlock<void> /*Todo*/ AIAnimationReferences;
	TagBlock<void> /*Todo*/ AIScriptReferences;
	TagBlock<void> /*Todo*/ AIRecordingReferences;
	TagBlock<void> /*Todo*/ AIConversations;
	TagDataReference        ScriptSyntaxData;
	TagDataReference        ScriptStringData;
	TagBlock<void> /*Todo*/ Scripts;
	TagBlock<void> /*Todo*/ Globals;
	TagBlock<void> /*Todo*/ References;
	TagBlock<void> /*Todo*/ SourceFiles;

	std::byte _Padding4CC[0x18];

	TagBlock<void> /*Todo*/ CutsceneFlags;
	TagBlock<void> /*Todo*/ CutsceneCameraPoints;
	TagBlock<void> /*Todo*/ CutsceneTitles;

	std::byte _Padding508[0x6C];

	TagReference CustomObjectNames;
	TagReference IngameHelpText;
	TagReference HudMessages;

	struct StructureBSP
	{
		std::uint32_t BSPStart;
		std::uint32_t BSPSize;
		std::uint32_t BSPVirtualBase;
		std::uint32_t _PaddingC;
		TagReference  BSP;

		struct BSPHeader
		{
			std::uint32_t VirtualOffset;
			// These are unused on PC
			std::uint32_t LightmapMaterialCountA;
			std::uint32_t RenderedVertexOffset;
			std::uint32_t LightmapMaterialCountB;
			std::uint32_t LightmapVerticesOffset;
			TagClass      Class; // `sbsp`
		};

		const BSPHeader& GetBSPHeader(const void* MapFile) const
		{
			return *reinterpret_cast<const BSPHeader*>(
				reinterpret_cast<const std::byte*>(MapFile) + BSPStart);
		}

		const Tag<TagClass::ScenarioStructureBsp>&
			GetBSP(const void* MapFile) const
		{
			const auto& BSPHeader = GetBSPHeader(MapFile);
			return *reinterpret_cast<
				const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>*>(
				reinterpret_cast<const std::byte*>(MapFile)
				+ ((BSPHeader.VirtualOffset - BSPVirtualBase) + BSPStart));
		}
	};
	static_assert(sizeof(StructureBSP) == 0x20);
	TagBlock<StructureBSP> StructureBSPs;
};
static_assert(offsetof(Tag<TagClass::Scenario>, _UnusedBSP0) == 0x0);
static_assert(offsetof(Tag<TagClass::Scenario>, _UnusedBSP1) == 0x10);
static_assert(offsetof(Tag<TagClass::Scenario>, _UnusedSky) == 0x20);
static_assert(offsetof(Tag<TagClass::Scenario>, Skies) == 0x30);
static_assert(offsetof(Tag<TagClass::Scenario>, Type) == 0x3C);
static_assert(offsetof(Tag<TagClass::Scenario>, Flags) == 0x3E);
static_assert(offsetof(Tag<TagClass::Scenario>, ChildScenarios) == 0x40);
static_assert(offsetof(Tag<TagClass::Scenario>, LocalNorth) == 0x4C);

static_assert(offsetof(Tag<TagClass::Scenario>, PredictedResources) == 0xEC);
static_assert(offsetof(Tag<TagClass::Scenario>, Functions) == 0xF8);

static_assert(offsetof(Tag<TagClass::Scenario>, EditorData) == 0x104);
static_assert(offsetof(Tag<TagClass::Scenario>, Comments) == 0x118);

static_assert(offsetof(Tag<TagClass::Scenario>, ObjectNames) == 0x204);
static_assert(offsetof(Tag<TagClass::Scenario>, Bipeds) == 0x228);
static_assert(offsetof(Tag<TagClass::Scenario>, Vehicles) == 0x240);
static_assert(offsetof(Tag<TagClass::Scenario>, Equipment) == 0x258);
static_assert(offsetof(Tag<TagClass::Scenario>, Weapons) == 0x270);

static_assert(offsetof(Tag<TagClass::Scenario>, SoundSceneryPalette) == 0x2E8);
static_assert(
	offsetof(Tag<TagClass::Scenario>, PlayerStartingProfile) == 0x348);

static_assert(offsetof(Tag<TagClass::Scenario>, ActorPalette) == 0x420);

static_assert(offsetof(Tag<TagClass::Scenario>, Globals) == 0x4A8);
static_assert(offsetof(Tag<TagClass::Scenario>, Scripts) == 0x49C);

static_assert(offsetof(Tag<TagClass::Scenario>, SourceFiles) == 0x4C0);
static_assert(offsetof(Tag<TagClass::Scenario>, CutsceneFlags) == 0x4E4);
static_assert(offsetof(Tag<TagClass::Scenario>, CutsceneCameraPoints) == 0x4F0);
static_assert(offsetof(Tag<TagClass::Scenario>, CutsceneTitles) == 0x4FC);

static_assert(offsetof(Tag<TagClass::Scenario>, CustomObjectNames) == 0x574);
static_assert(offsetof(Tag<TagClass::Scenario>, StructureBSPs) == 0x5A4);

static_assert(sizeof(Tag<TagClass::Scenario>) == 0x5B0);

template<>
struct Tag<TagClass::ScenarioStructureBsp>
{
	TagReference LightmapTexture;
	float        VehicleFloor;
	float        VehicleCeiling;

	std::byte _Padding18[0x14];

	Vector3f DefaultAmbientColor;

	std::byte _Padding38[0x4];

	Vector3f DefaultDistantLight0Color;
	Vector3f DefaultDistantLight0Direction;
	Vector3f DefaultDistantLight1Color;
	Vector3f DefaultDistantLight1Direction;

	std::byte _Padding6C[0xC];

	Vector4f DefaultReflectionTint;
	Vector3f DefaultShadowDirection;
	Vector3f DefaultShadowColor;

	std::byte _PaddingA0[0x4];

	TagBlock<void> /*Todo*/ CollisionMaterials;
	TagBlock<void> /*Todo*/ CollisionBSPs;
	TagBlock<void> /*Todo*/ Nodes;
	float                   WorldBoundsX[2];
	float                   WorldBoundsY[2];
	float                   WorldBoundsZ[2];
	TagBlock<void> /*Todo*/ Leaves;
	TagBlock<void> /*Todo*/ LeafSurfaces;

	using Surface = std::array<std::uint16_t, 3>;
	TagBlock<Surface> Surfaces;

	struct Lightmap
	{
		std::uint16_t LightmapIndex;
		std::uint16_t Unknown;

		std::byte _Padding4[0x10];

		struct Material
		{
			TagReference  Shader;
			std::uint16_t ShaderPermutation;
			std::uint16_t Flags;

			std::uint32_t SurfacesIndexStart;
			std::uint32_t SurfacesCount;

			Vector3f      Centroid;
			Vector3f      AmbientColor;
			std::uint16_t DistantLightCount;
			std::uint16_t Unknown36;
			Vector3f      DistantLight0Color;
			Vector3f      DistantLight0Direction;
			Vector3f      DistantLight1Color;
			Vector3f      DistantLight1Direction;

			std::byte _Padding68[0xC];

			Vector4f      ReflectionTint;
			Vector3f      ShadowDirection;
			Vector3f      ShadowColor;
			Vector4f      Plane;
			std::uint16_t BreakableSurface;
			std::uint16_t UnknownAE;

			struct VertexBufferReference
			{
				std::uint32_t Unknown0;
				std::uint32_t VertexBufferCount;
				std::uint32_t VertexBufferOffset;
				std::uint32_t UnknownC;
				std::uint32_t Unknown10;
			};

			VertexBufferReference Geometry;
			VertexBufferReference LightmapGeometry;

			TagDataReference UncompressedVertices;
			TagDataReference CompressedVertices;

			std::span<const Vertex>
				GetVertices(const void* Data, std::uint32_t VirtualBase) const
			{
				return std::span<const Vertex>(
					reinterpret_cast<const Vertex*>(
						reinterpret_cast<const std::byte*>(Data)
						+ (UncompressedVertices.VirtualOffset - VirtualBase)),
					Geometry.VertexBufferCount);
			}
		};
		static_assert(sizeof(Material) == 0x100);
		TagBlock<Material> Materials;
	};
	static_assert(sizeof(Lightmap) == 0x20);
	TagBlock<Lightmap> Lightmaps;

	std::byte _Padding110[0xC];

	TagBlock<void> /*Todo*/ LensFlares;
	TagBlock<void> /*Todo*/ LensFlareMarkers;

	struct Cluster
	{
		std::uint16_t SkyIndex;
		std::uint16_t FogIndex;
		std::uint16_t BackgroundSoundIndex;
		std::uint16_t SoundEnvironmentIndex;
		std::uint16_t WeatherIndex;
		std::uint16_t TransitionStructureBSPIndex;

		std::byte _PaddingC[0x1C];

		TagBlock<void> /*Todo*/ PredictedResources;
		TagBlock<void> /*Todo*/ SubClusters;
		std::uint16_t           LensFlareMarkersStart;
		std::uint16_t           LensFlareMarkersCount;
		TagBlock<std::uint32_t> SurfaceIndices;
		TagBlock<void> /*Todo*/ Mirrors;
		TagBlock<std::uint16_t> Portals;
	};
	static_assert(sizeof(Cluster) == 0x68);
	TagBlock<Cluster> Clusters;

	TagDataReference        ClusterData;
	TagBlock<void> /*Todo*/ ClusterPortals;

	std::byte _Padding160[0xC];

	TagBlock<void> /*Todo*/ BreakableSurfaces;
	TagBlock<void> /*Todo*/ FogPlanes;
	TagBlock<void> /*Todo*/ FogRegions;
	TagBlock<void> /*Todo*/ FogPalette;

	std::byte _Padding19C[0x18];

	TagBlock<void> /*Todo*/ WeatherPalette;
	TagBlock<void> /*Todo*/ WeatherPolyhedra;

	std::byte _Padding1CC[0x18];

	TagBlock<void> /*Todo*/ PathfindingSurfaces;
	TagBlock<void> /*Todo*/ PathfindingEdges;
	TagBlock<void> /*Todo*/ BackgroundSoundPalette;
	TagBlock<void> /*Todo*/ SoundEnvironmentPalette;
	TagDataReference        SoundPASData;

	std::byte _Padding228[0x18];

	TagBlock<void> /*Todo*/ Markers;
	TagBlock<void> /*Todo*/ DetailObjects;
	TagBlock<void> /*Todo*/ RuntimeDecals;

	std::byte _Padding264[0xC];

	TagBlock<void> /*Todo*/ LeafMapLeaves;
	TagBlock<void> /*Todo*/ LeafMapPortals;
};

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, LightmapTexture) == 0x0);

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, DefaultAmbientColor) == 0x2C);

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, DefaultDistantLight0Color)
	== 0x3C);

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, DefaultShadowDirection)
	== 0x88);

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, CollisionMaterials) == 0xA4);

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, LensFlares) == 0x11C);

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, BreakableSurfaces) == 0x16C);

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, WeatherPalette) == 0x1B4);

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, PathfindingSurfaces)
	== 0x1E4);

static_assert(offsetof(Tag<TagClass::ScenarioStructureBsp>, Markers) == 0x240);

static_assert(
	offsetof(Tag<TagClass::ScenarioStructureBsp>, LeafMapLeaves) == 0x270);

static_assert(sizeof(Tag<TagClass::ScenarioStructureBsp>) == (0x288));
#pragma pack(pop)
} // namespace Blam