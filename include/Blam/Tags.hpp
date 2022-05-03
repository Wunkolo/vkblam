#pragma once

#include "Enums.hpp"
#include "Types.hpp"
#include <cstddef>

namespace Blam
{
#pragma pack(push, 1)
template<TagClass Class>
struct Tag
{
};

template<>
struct Tag<TagClass::Bitmap>
{
	BitmapType   Type;
	BitmapFormat Format;
	BitmapUsage  Usage;
	BitmapFlags  Flags;

	float DetailFadeFactor;
	float SharpenAmount;
	float BumpHeight;

	BitmapSpriteBudgetSize SpriteBudgetSize;
	std::uint16_t          SpriteBudgetCount;

	std::uint16_t    SpritePlateWidth;
	std::uint16_t    SpritePlateHeight;
	TagDataReference CompressedColorPlateData;

	TagDataReference ProcessedPixelData;

	float BlurFilterSize;
	float AlphaBias;

	// 0 means all levels
	std::uint16_t MipmapCount;

	BitmapSpriteUsage SpriteUsage;
	std::uint16_t     SpriteSpacing;
	std::uint16_t     Unknown52;

	TagBlock<void> Sequences;

	struct BitmapEntry
	{
		TagClass      Class;
		std::uint16_t Width;
		std::uint16_t Height;
		std::uint16_t Depth;

		BitmapEntryType   Type;
		BitmapEntryFormat Format;

		BitmapEntryBitFlags Flags;

		std::uint16_t RegPointX, RegPointY;

		std::uint16_t MipmapCount;
		std::uint16_t Unknown16;
		// This is an actual file-offset. No base-address fixup needed at all.
		// Depending on the map version, this may be an offset either within the
		// file itself or within bitmaps.map
		std::uint32_t PixelDataOffset;
		std::uint32_t PixelDataSize;

		// This seems to be the ID for the owning bitmap tag
		std::uint32_t BitmapTagID;

		std::int32_t  Unknown24;
		std::uint32_t Unknown28;
		std::uint8_t  Unknown2C;
		std::uint8_t  Unknown2D;
		std::uint8_t  Unknown2E;
		std::uint8_t  Unknown2F;
	};
	TagBlock<BitmapEntry> Bitmaps;
};
static_assert(sizeof(Tag<TagClass::Bitmap>) == 0x6C);

template<>
struct Tag<TagClass::Shader>
{
	// Radiocity properties
	enum class RadiosityBitFlags : std::uint16_t
	{
		SimpleParameterization = 1 << 0,
		IgnoreNormals          = 1 << 1,
		TransparentLit         = 1 << 2,
	} RadiosityFlags;

	enum class RadiosityDetailLevel : std::uint16_t
	{
		High,
		Medium,
		Low,
		Turd
	} RadiosityDetailLevel;
	float    Power;
	Vector3f EmissionColor;
	Vector3f TintColor;

	// Physics properties
	std::uint16_t       PhysicsFlags;
	PhysicsMaterialType PhysicsMaterial;
	std::int16_t        Unknown24;
	std::int16_t        Unknown26;
};
static_assert(sizeof(Tag<TagClass::Shader>) == 0x28);

template<>
struct Tag<TagClass::ShaderTransparentChicago> : public Tag<TagClass::Shader>
{
	// Chicago Shader
	std::int8_t NumericCounterLimit;
	enum class ChicagoFlags : std::uint8_t
	{
		AlphaTested               = 1 << 0,
		Decal                     = 1 << 1,
		TwoSided                  = 1 << 2,
		FirstMapIsScreenSpace     = 1 << 3,
		DrawBeforeWater           = 1 << 4,
		IgnoreEffectColor         = 1 << 5,
		ScaleFirstMapWithDistance = 1 << 6,
		Numeric                   = 1 << 7,
	} ChicagoFlags;

	enum class FirstMapType : std::uint16_t
	{
		MapIs2DMap,
		MapIsObjectCenteredCubeMap,
		MapIsViewCenteredCubeMap
	} FirstMapType;

	enum class FramebufferBlendFunction : std::uint16_t
	{
		AlphaBlend,
		Multiply,
		DoubleMultiply,
		Add,
		Subtract,
		ComponentMin,
		ComponentMax,
		AlphaMultiplyAdd
	} FramebufferBlend;

	enum class FramebufferFadeMode : std::uint16_t
	{
		None,
		FadeWhenPerpendicular,
		FadeWhenParallel,
	} FramebufferFade;

	AnimationSource FramebufferFadeSource;

	std::uint16_t Unknown32;

	// Lens flare
	float        LensFlareSpacing;
	TagReference LensFlare;

	TagBlock<TagReference> ExtraLayers;

	struct MapEntry
	{
		enum MapEntryBitFlags : std::uint16_t
		{
			Unfiltered     = 1 << 0,
			AlphaReplicate = 1 << 1,
			UClamped       = 1 << 2,
			VClamped       = 1 << 3,
		} Flags;
		std::int16_t Unknown2;

		std::uint32_t Unknown4;
		std::uint32_t Unknown8;
		std::uint32_t UnknownC;
		std::uint32_t Unknown10;
		std::uint32_t Unknown14;
		std::uint32_t Unknown18;
		std::uint32_t Unknown1C;
		std::uint32_t Unknown20;
		std::uint32_t Unknown24;
		std::uint32_t Unknown28;

		enum class BlendFunction : std::uint16_t
		{
			Current,
			NextMap,
			Multiply,
			DoubleMultiply,
			Add,
			AddSignedCurrent,
			AddSignedNextMap,
			SubtractCurrent,
			SubtractNextMap,
			BlendCurrentAlpha,
			BlendCurrentAlphaInverse,
			BlendNextMapAlpha,
			BlendNextMapAlphaInverse,
		};

		BlendFunction ColorBlendFunction;
		BlendFunction AlphaBlendFunction;

		std::uint32_t Unknown30;
		std::uint32_t Unknown34;
		std::uint32_t Unknown38;
		std::uint32_t Unknown3C;
		std::uint32_t Unknown40;
		std::uint32_t Unknown44;
		std::uint32_t Unknown48;
		std::uint32_t Unknown4C;
		std::uint32_t Unknown50;

		float MapUScale;
		float MapVScale;
		float MapUOffset;
		float MapVOffset;
		float MapRotation;
		float MipmapBias;

		TagReference Map;

		std::uint32_t Unknown7C;
		std::uint32_t Unknown80;
		std::uint32_t Unknown84;
		std::uint32_t Unknown88;
		std::uint32_t Unknown8C;
		std::uint32_t Unknown90;
		std::uint32_t Unknown94;
		std::uint32_t Unknown98;
		std::uint32_t Unknown9C;
		std::uint32_t UnknownA0;

		AnimationSource   UAnimationSource;
		AnimationFunction UAnimationFunction;
		float             UAnimationPeriod;
		float             UAnimationPhase;
		float             UAnimationScale;

		AnimationSource   VAnimationSource;
		AnimationFunction VAnimationFunction;
		float             VAnimationPeriod;
		float             VAnimationPhase;
		float             VAnimationScale;

		AnimationSource   RotationAnimationSource;
		AnimationFunction RotationAnimationFunction;
		float             RotationAnimationPeriod;
		float             RotationAnimationPhase;
		float             RotationAnimationScale;
		Vector2f          RotationPointCenter;
	};
	static_assert(sizeof(MapEntry) == 0xDC);

	TagBlock<MapEntry> Maps;

	enum class ExtraFlagBits : std::uint32_t
	{
		DontFadeActiveCamoflage = 1 << 0,
		NumericCountdownTimer   = 1 << 1,
	} ExtraFlags;

	std::uint32_t Unknown64;
	std::uint32_t Unknown68;
};
static_assert(sizeof(Tag<TagClass::ShaderTransparentChicago>) == 0x6C);

template<>
struct Tag<TagClass::ShaderTransparentWater> : public Tag<TagClass::Shader>
{
	enum class ShaderTransparentWaterBitFlags : std::uint16_t
	{
		BaseMapAlphaModulatesReflection = 1 << 0,
		BaseMapColorModulatesReflection = 1 << 1,
		AtmosphericFog                  = 1 << 2,
		DrawBeforeFog                   = 1 << 3,
	} Flags;
	std::uint16_t Unknown2A;

	std::uint32_t Unknown2C;
	std::uint32_t Unknown30;
	std::uint32_t Unknown34;
	std::uint32_t Unknown38;
	std::uint32_t Unknown3C;
	std::uint32_t Unknown40;
	std::uint32_t Unknown44;
	std::uint32_t Unknown48;

	TagReference BaseMap;

	std::uint32_t Unknown5C;
	std::uint32_t Unknown60;
	std::uint32_t Unknown64;
	std::uint32_t Unknown68;

	float    ViewPerpendicularBrightness;
	Vector3f ViewPerpendicularTintColor;

	float    ViewParallelBrightness;
	Vector3f ViewParallelTintColor;

	std::uint32_t Unknown8C;
	std::uint32_t Unknown90;
	std::uint32_t Unknown94;
	std::uint32_t Unknown98;

	TagReference ReflectionMap;

	std::uint32_t UnknownAC;
	std::uint32_t UnknownB0;
	std::uint32_t UnknownB4;
	std::uint32_t UnknownB8;

	float        RippleAnimationAngle;
	float        RippleAnimationVelocity;
	float        RippleAnimationScale;
	TagReference RippleMaps;

	std::int16_t RippleMipmapLevels;
	std::int16_t UnknownDA;

	float RippleMipmapFadeFactor;
	float RippleMipmapDetailBias;

	std::uint32_t UnknownE4;
	std::uint32_t UnknownE8;
	std::uint32_t UnknownEC;
	std::uint32_t UnknownF0;
	std::uint32_t UnknownF4;
	std::uint32_t UnknownF8;
	std::uint32_t UnknownFC;
	std::uint32_t Unknown100;
	std::uint32_t Unknown104;
	std::uint32_t Unknown108;
	std::uint32_t Unknown10C;
	std::uint32_t Unknown110;
	std::uint32_t Unknown114;
	std::uint32_t Unknown118;
	std::uint32_t Unknown11C;
	std::uint32_t Unknown120;

	struct RippleEntry
	{
		std::uint16_t Unknown0;
		std::uint16_t Unknown2;
		float         ContributionFactor;
		std::uint32_t Unknown8;
		std::uint32_t UnknownC;
		std::uint32_t Unknown10;
		std::uint32_t Unknown14;
		std::uint32_t Unknown18;
		std::uint32_t Unknown1C;
		std::uint32_t Unknown20;
		std::uint32_t Unknown24;
		float         AnimationAngle;
		float         AnimationVelocity;
		Vector2f      MapOffset;
		std::int16_t  MapRepeats;
		std::int16_t  MapIndex;

		std::uint32_t Unknown3C;
		std::uint32_t Unknown40;
		std::uint32_t Unknown44;
		std::uint32_t Unknown48;
	};
	static_assert(sizeof(RippleEntry) == 0x4C);

	TagBlock<RippleEntry> Ripples;

	std::uint32_t Unknown130;
	std::uint32_t Unknown134;
	std::uint32_t Unknown138;
	std::uint32_t Unknown13C;
};
static_assert(sizeof(Tag<TagClass::ShaderTransparentWater>) == 0x140);

template<>
struct Tag<TagClass::ShaderEnvironment> : public Tag<TagClass::Shader>
{
	// Environment Shader properties
	enum class ShaderBitFlags : std::uint16_t
	{
		AlphaTested           = 1 << 0,
		BumpMapIsSpecularMask = 1 << 1,
		TrueAtmosphericFog    = 1 << 2,
	} ShaderFlags;

	enum class ShaderEnvironmentType : std::uint16_t
	{
		Normal,
		Blended,
		BlendedBaseSpecular
	} ShaderType;

	// Lens flare
	float        LensFlareSpacing;
	TagReference LensFlare;

	std::uint32_t Unknown40;
	std::uint32_t Unknown44;
	std::uint32_t Unknown48;
	std::uint32_t Unknown4C;
	std::uint32_t Unknown50;
	std::uint32_t Unknown54;
	std::uint32_t Unknown58;
	std::uint32_t Unknown5C;
	std::uint32_t Unknown60;
	std::uint32_t Unknown64;
	std::uint32_t Unknown68;

	// Diffuse properties
	enum class DiffuseBitFlags : std::uint16_t
	{
		RescaleDetailMaps = 1 << 0,
		RescaleBumpMap    = 1 << 1,
	} DiffuseFlags;
	std::uint16_t Unknown6E;

	std::uint32_t Unknown70;
	std::uint32_t Unknown74;
	std::uint32_t Unknown78;
	std::uint32_t Unknown7C;
	std::uint32_t Unknown80;
	std::uint32_t Unknown84;

	TagReference BaseMap;

	std::uint32_t Unknown98;
	std::uint32_t Unknown9C;
	std::uint32_t UnknownA0;
	std::uint32_t UnknownA4;
	std::uint32_t UnknownA8;
	std::uint32_t UnknownAC;

	DetailMapFunction BaseDetailMapFunction;

	std::int16_t UnknownB2;

	float        PrimaryDetailMapScale;
	TagReference PrimaryDetailMap;

	float        SecondaryDetailMapScale;
	TagReference SecondaryDetailMap;

	std::uint32_t UnknownDC;
	std::uint32_t UnknownE0;
	std::uint32_t UnknownE4;
	std::uint32_t UnknownE8;
	std::uint32_t UnknownEC;
	std::uint32_t UnknownF0;

	DetailMapFunction MicroDetailMapFunction;
	std::int16_t      UnknownF6;

	float        MicroDetailMapScale;
	TagReference MicroDetailMap;

	Vector3f MaterialColor;

	std::uint32_t Unknown118;
	std::uint32_t Unknown11C;
	std::uint32_t Unknown120;

	// Bump Map
	float        BumpMapScale;
	TagReference BumpMap;

	std::uint32_t Unknown138;
	std::uint32_t Unknown13C;
	std::uint32_t Unknown140;
	std::uint32_t Unknown144;
	std::uint32_t Unknown148;
	std::uint32_t Unknown14C;

	AnimationFunction UAnimationFunction;
	std::int16_t      Unknown152;
	float             UAnimationPeriod;
	float             UAnimationScale;

	AnimationFunction VAnimationFunction;
	std::int16_t      Unknown15E;
	float             VAnimationPeriod;
	float             VAnimationScale;

	std::uint32_t Unknown168;
	std::uint32_t Unknown16C;
	std::uint32_t Unknown170;
	std::uint32_t Unknown174;
	std::uint32_t Unknown178;
	std::uint32_t Unknown17C;

	// Self-illumination properties
	enum class SelfIlluminationBitFlags : std::uint16_t
	{
		Unfiltered = 1 << 0,
	} SelfIlluminationFlags;
	std::int16_t Unknown182;

	std::uint32_t Unknown184;
	std::uint32_t Unknown188;
	std::uint32_t Unknown18C;
	std::uint32_t Unknown190;
	std::uint32_t Unknown194;
	std::uint32_t Unknown198;

	Vector3f PrimaryOnColor;
	Vector3f PrimaryOffColor;

	AnimationFunction PrimaryAnimationFunction;
	std::int16_t      Unknown1B6;

	float PrimaryAnimationPeriod;
	float PrimaryAnimationPhase;

	std::uint32_t Unknown1C0;
	std::uint32_t Unknown1C4;
	std::uint32_t Unknown1C8;
	std::uint32_t Unknown1CC;
	std::uint32_t Unknown1D0;
	std::uint32_t Unknown1D4;

	Vector3f SecondaryOnColor;
	Vector3f SecondaryOffColor;

	AnimationFunction SecondaryAnimationFunction;
	std::int16_t      Unknown1F2;

	float SecondaryAnimationPeriod;
	float SecondaryAnimationPhase;

	std::uint32_t Unknown1FC;
	std::uint32_t Unknown200;
	std::uint32_t Unknown204;
	std::uint32_t Unknown208;
	std::uint32_t Unknown20C;
	std::uint32_t Unknown210;

	Vector3f PlasmaOnColor;
	Vector3f PlasmaOffColor;

	AnimationFunction PlasmaAnimationFunction;
	std::int16_t      Unknown22E;

	float PlasmaAnimationPeriod;
	float PlasmaAnimationPhase;

	std::uint32_t Unknown238;
	std::uint32_t Unknown23C;
	std::uint32_t Unknown240;
	std::uint32_t Unknown244;
	std::uint32_t Unknown248;
	std::uint32_t Unknown24C;

	float        MapScale;
	TagReference Map;

	std::uint32_t Unknown264;
	std::uint32_t Unknown268;
	std::uint32_t Unknown26C;
	std::uint32_t Unknown270;
	std::uint32_t Unknown274;
	std::uint32_t Unknown278;

	// Specular properties
	enum class SpecularBitFlags : std::uint16_t
	{
		Overbright         = 1 << 0,
		ExtraShiny         = 1 << 1,
		LightmapIsSpecular = 1 << 2,
	} SpecularFlags;
	std::int16_t Unknown27E;

	std::uint32_t Unknown280;
	std::uint32_t Unknown284;
	std::uint32_t Unknown288;
	std::uint32_t Unknown28C;

	float SpecularBrightness;

	std::uint32_t Unknown294;
	std::uint32_t Unknown298;
	std::uint32_t Unknown29C;
	std::uint32_t Unknown2A0;
	std::uint32_t Unknown2A4;

	Vector3f PerpendicularColor;
	Vector3f ParallelColor;

	std::uint32_t Unknown2C0;
	std::uint32_t Unknown2C4;
	std::uint32_t Unknown2C8;
	std::uint32_t Unknown2CC;

	// Reflection properties
	enum class ReflectionBitFlags : std::uint16_t
	{
		DynamicMirror = 1 << 0
	} ReflectionFlags;
	enum class ReflectionType : std::uint16_t
	{
		BumpedCubeMap,
		FlatCubeMap,
		BumpedRadiosity
	} ReflectionType;

	float LightmapBrightnessScale;

	std::uint32_t Unknown2D8;
	std::uint32_t Unknown2DC;
	std::uint32_t Unknown2E0;
	std::uint32_t Unknown2E4;
	std::uint32_t Unknown2E8;
	std::uint32_t Unknown2EC;
	std::uint32_t Unknown2F0;

	float PerpendicularBrightness;
	float ParallelBrightness;

	std::uint32_t Unknown2FC;
	std::uint32_t Unknown300;
	std::uint32_t Unknown304;
	std::uint32_t Unknown308;
	std::uint32_t Unknown30C;
	std::uint32_t Unknown310;
	std::uint32_t Unknown314;
	std::uint32_t Unknown318;
	std::uint32_t Unknown31C;
	std::uint32_t Unknown320;

	TagReference ReflectionCubeMap;

	std::uint32_t Unknown334;
	std::uint32_t Unknown338;
	std::uint32_t Unknown33C;
	std::uint32_t Unknown340;
};

static_assert(sizeof(Tag<TagClass::ShaderEnvironment>) == 0x344);

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

	// A lot of the palettes are really just TagReference aligned up to a
	// size of 0x30. So this is just a utility-wrapper for TagReference that
	// adds the extra padding
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

		// Gets the entire SBSP data
		std::span<const std::byte> GetSBSPData(const void* MapFile) const
		{
			return std::span<const std::byte>(
				reinterpret_cast<const std::byte*>(MapFile) + BSPStart,
				BSPSize);
		}

		struct SBSPHeader
		{
			std::uint32_t VirtualOffset;
			// These are unused on PC
			std::uint32_t LightmapMaterialCountA;
			std::uint32_t RenderedVertexOffset;
			std::uint32_t LightmapMaterialCountB;
			std::uint32_t LightmapVerticesOffset;
			TagClass      Class; // `sbsp`
		};

		const SBSPHeader& GetSBSPHeader(const void* MapFile) const
		{
			return *reinterpret_cast<const SBSPHeader*>(
				reinterpret_cast<const std::byte*>(MapFile) + BSPStart);
		}

		const Tag<TagClass::ScenarioStructureBsp>&
			GetSBSP(const void* MapFile) const
		{
			const auto& SBSPHeader = GetSBSPHeader(MapFile);
			return *reinterpret_cast<
				const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>*>(
				reinterpret_cast<const std::byte*>(MapFile)
				+ ((SBSPHeader.VirtualOffset - BSPVirtualBase) + BSPStart));
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
		std::int16_t  LightmapIndex;
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
			std::int16_t  BreakableSurface;
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

			std::span<const std::byte> GetVertexBuffer(
				const void* Data, std::uint32_t VirtualBase) const
			{
				return std::span<const std::byte>(
					reinterpret_cast<const std::byte*>(Data)
						+ (UncompressedVertices.VirtualOffset - VirtualBase),
					UncompressedVertices.Size);
			}

			std::span<const Vertex>
				GetVertices(const void* Data, std::uint32_t VirtualBase) const
			{
				return std::span<const Vertex>(
					reinterpret_cast<const Vertex*>(
						reinterpret_cast<const std::byte*>(Data)
						+ (UncompressedVertices.VirtualOffset - VirtualBase)),
					Geometry.VertexBufferCount);
			}
			std::span<const LightmapVertex> GetLightmapVertices(
				const void* Data, std::uint32_t VirtualBase) const
			{
				return std::span<const LightmapVertex>(
					reinterpret_cast<const LightmapVertex*>(
						reinterpret_cast<const std::byte*>(Data)
						+ (UncompressedVertices.VirtualOffset - VirtualBase)
						+ (Geometry.VertexBufferCount * sizeof(Vertex))),
					LightmapGeometry.VertexBufferCount);
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
