#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Blam
{

enum class CacheVersion : std::uint32_t
{
	Xbox          = 0x5,
	Demo          = 0x6,
	Retail        = 0x7,
	H1A           = 0xD,
	CustomEdition = 0x261
};

enum class ScenarioType : std::uint16_t
{
	SinglePlayer  = 0x0,
	MultiPlayer   = 0x1,
	UserInterface = 0x2,
};

enum class ResourceMapType : std::uint32_t
{
	Bitmaps = 0x0,
	Sounds  = 0x1,
	Loc     = 0x2,
};

enum class TagClass : std::uint32_t
{
	None                             = 4294967295,
	Null                             = 0000000000,
	Actor                            = 1633907826,
	ActorVariant                     = 1633907830,
	Antenna                          = 1634628641,
	Biped                            = 1651077220,
	Bitmap                           = 1651078253,
	ContinuousDamageEffect           = 1667525991,
	ModelCollisionGeometry           = 1668246636,
	ColorTable                       = 1668246639,
	Contrail                         = 1668247156,
	DeviceControl                    = 1668575852,
	Decal                            = 1684366177,
	UiWidgetDefinition               = 1147489377,
	InputDeviceDefaults              = 1684371043,
	Device                           = 1684371049,
	DetailObjectCollection           = 1685021283,
	Effect                           = 1701209701,
	Equipment                        = 1701931376,
	Flag                             = 1718378855,
	Fog                              = 1718576928,
	Font                             = 1718578804,
	MaterialEffects                  = 1718579060,
	Garbage                          = 1734439522,
	Glow                             = 1735161633,
	GrenadeHudInterface              = 1735551081,
	HudMessageText                   = 1752003616,
	HudNumber                        = 1752523811,
	HudGlobals                       = 1752523879,
	Item                             = 1769235821,
	ItemCollection                   = 1769237859,
	DamageEffect                     = 1785754657,
	LensFlare                        = 1818586739,
	Lightning                        = 1701602659,
	DeviceLightFixture               = 1818846825,
	Light                            = 1818847080,
	SoundLooping                     = 1819504228,
	DeviceMachine                    = 1835098984,
	Globals                          = 1835103335,
	Meter                            = 1835365490,
	LightVolume                      = 1835496242,
	Gbxmodel                         = 1836016690,
	Model                            = 1836016741,
	MultiplayerScenarioDescription   = 1836084345,
	PreferencesNetworkGame           = 1852272754,
	Object                           = 1868720741,
	Particle                         = 1885434484,
	ParticleSystem                   = 1885566060,
	Physics                          = 1885895027,
	Placeholder                      = 1886151011,
	PointPhysics                     = 1886414969,
	Projectile                       = 1886547818,
	WeatherParticleSystem            = 1918986606,
	ScenarioStructureBsp             = 1935831920,
	Scenery                          = 1935893870,
	ShaderTransparentChicagoExtended = 1935893880,
	ShaderTransparentChicago         = 1935894633,
	Scenario                         = 1935896178,
	ShaderEnvironment                = 1936027254,
	ShaderTransparentGlass           = 1936157793,
	Shader                           = 1936221298,
	Sky                              = 1936423200,
	ShaderTransparentMeter           = 1936549236,
	Sound                            = 1936614433,
	SoundEnvironment                 = 1936614501,
	ShaderModel                      = 1936683887,
	ShaderTransparentGeneric         = 1936684146,
	UiWidgetCollection               = 1399813484,
	ShaderTransparentPlasma          = 1936747617,
	SoundScenery                     = 1936941925,
	StringList                       = 1937011235,
	ShaderTransparentWater           = 1937203572,
	TagCollection                    = 1952540515,
	CameraTrack                      = 1953653099,
	Dialogue                         = 1969515623,
	UnitHudInterface                 = 1970169961,
	Unit                             = 1970170228,
	UnicodeStringList                = 1970500722,
	VirtualKeyboard                  = 1986227065,
	Vehicle                          = 1986357353,
	Weapon                           = 2003132784,
	Wind                             = 2003398244,
	WeaponHudInterface               = 2003855465,
};

#pragma pack(push, 1)

using Vector2f = std::array<float, 2>;
using Vector3f = std::array<float, 3>;
using Vector4f = std::array<float, 4>;

struct TagReference
{
	TagClass      Class;
	std::uint32_t PathVirtualOffset;
	std::uint32_t PathLength;
	std::uint32_t TagID;
};
static_assert(sizeof(TagReference) == 0x10);

struct TagDataReference
{
	std::uint32_t Size;
	std::uint32_t IsExternal;
	std::uint32_t Offset;
	std::uint64_t VirtualOffset;
};
static_assert(sizeof(TagDataReference) == 20);

struct Reflexive
{
	std::uint32_t Count;
	std::uint32_t Offset;
	std::uint32_t UnknownC;
};

// Header for variable-sized array of data in a tag
template<typename T = void>
struct TagBlock
{
	std::uint32_t Count;
	std::uint32_t VirtualOffset;
	std::uint32_t Unknown8;

	template<
		typename U = T,
		typename = typename std::enable_if_t<std::is_same_v<U, void> == false>>
	std::span<const U> GetSpan(const void* Data, std::uint32_t VitualBase) const
	{
		return std::span<const U>(
			reinterpret_cast<const U*>(
				reinterpret_cast<const std::byte*>(Data)
				+ (VirtualOffset - VitualBase)),
			Count);
	}
};
static_assert(sizeof(TagBlock<void>) == 12);

struct MapHeader
{
	std::uint32_t MagicHead; // 'head'
	CacheVersion  Version;
	std::uint32_t FileSize;
	std::uint32_t PaddingLength; // Xbox Only
	std::uint32_t TagIndexOffset;
	std::uint32_t TagIndexSize;
	std::byte     Pad18[8];
	char          ScenarioName[32];
	char          BuildVersion[32];
	ScenarioType  Type;
	std::byte     Pad64[2];
	std::uint32_t Checksum;
	std::uint32_t H1AFlags; // Todo

	std::byte     Pad6C[1936];
	std::uint32_t MagicFoot; // 'foot'
};
static_assert(sizeof(MapHeader) == 2048);

struct TagIndexHeader
{
	std::uint32_t TagIndexVirtualOffset;
	std::uint32_t BaseTag;
	std::uint32_t ScenarioTagID;
	std::uint32_t TagCount;
	std::uint32_t VertexCount;
	std::uint32_t VertexOffset;
	std::uint32_t IndexCount;
	std::uint32_t IndexOffset;
	std::uint32_t ModelDataSize;
	std::uint32_t MagicTags; // 'tags'
};
static_assert(sizeof(TagIndexHeader) == 40);

struct TagIndexEntry
{
	TagClass      ClassPrimary;
	TagClass      ClassSecondary;
	TagClass      ClassTertiary;
	std::uint32_t TagID;
	std::uint32_t TagPathVirtualOffset;
	std::uint32_t TagDataVirtualOffset;
	std::uint32_t IsExternal;
	std::uint32_t Unused;
};
static_assert(sizeof(TagIndexEntry) == 32);

struct ResourceMapHeader
{
	ResourceMapType Type;
	std::uint32_t   TagPathsOffset;
	std::uint32_t   ResourceOffset;
	std::uint32_t   ResourceCount;
};

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
