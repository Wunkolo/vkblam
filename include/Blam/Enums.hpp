#pragma once

#include <cstdint>
#include <cstddef>

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

enum class BitmapType : std::uint16_t
{
	Texture2D = 0x0,
	Texture3D = 0x1,
	CubeMap   = 0x2,
	Sprite    = 0x3,
	Interface = 0x4,
};

enum class BitmapFormat : std::uint16_t
{
	CompressedColorKeyTransparency = 0x00,
	CompressedExplicitAlpha        = 0x01,
	CompressedInterpolatedAlpha    = 0x02,
	Color16Bit                     = 0x03,
	Color32Bit                     = 0x04,
	Monochrome                     = 0x05,
};

enum class BitmapUsage : std::uint16_t
{
	AlphaBlend = 0x00,
	Default    = 0x01,
	HeightMap  = 0x02,
	DetailMap  = 0x03,
	LightMap   = 0x04,
	VectorMap  = 0x05,
};

enum class BitmapSpriteBudgetSize : std::uint16_t
{
	_32  = 0x00,
	_64  = 0x01,
	_128 = 0x02,
	_256 = 0x03,
	_512 = 0x04,
};

enum class BitmapSpriteUsage : std::uint16_t
{
	Blend_Add_Subtract_Max = 0x00,
	Multiply_Min           = 0x01,
	Double_Multiply        = 0x02,
};

enum class BitmapFlags : std::uint16_t
{
	EnableDiffusionDithering    = 0,
	DisableHeightMapCompression = 1,
	UniformSpriteSequences      = 2,
	FilthySpriteBugFix          = 3,
};

enum class DefaultTextureIndex : std::uint16_t
{
	Additive       = 0,
	Multiplicative = 1,
	SignedAdditive = 2,
	Vector         = 3,
};

enum class BitmapEntryType : std::uint16_t
{
	Texture2D = 0x00,
	Texture3D = 0x01,
	CubeMap   = 0x02,
	White     = 0x03,
};

enum class BitmapEntryBitFlags : std::uint16_t
{
	PowerOfTwoDimensions = 1u << 0,
	Compressed           = 1u << 1,
	Palettized           = 1u << 2,
	Swizzled             = 1u << 3,
	Linear               = 1u << 4,
	V16U16               = 1u << 5,
};

enum class BitmapEntryFormat : std::uint16_t
{
	A8       = 0x00,
	Y8       = 0x01,
	AY8      = 0x02,
	A8Y8     = 0x03,
	R5G6B5   = 0x06,
	A1R5G5B5 = 0x08,
	A4R4G4B4 = 0x09,
	X8R8G8B8 = 0x0A,
	A8R8G8B8 = 0x0B,
	DXT1     = 0x0E,
	DXT2AND3 = 0x0F,
	DXT4AND5 = 0x10,
	P8       = 0x11,
};

enum class PhysicsMaterialType : std::uint16_t
{
	Dirt,
	Sand,
	Stone,
	Snow,
	Wood,
	MetalHollow,
	MetalThin,
	MetalThick,
	Rubber,
	Glass,
	ForceField,
	Grunt,
	HunterArmor,
	HunterSkin,
	Elite,
	Jackal,
	JackalEnergyShield,
	EngineerSkin,
	EngineerForceField,
	FloodCombatForm,
	FloodCarrierForm,
	CyborgArmor,
	CyborgEnergyShield,
	HumanArmor,
	HumanSkin,
	Sentinel,
	Monitor,
	Plastic,
	Water,
	Leaves,
	EliteEnergyShield,
	Ice,
	HunterShield,
};

enum class DetailMapFunction : std::uint16_t
{
	DoubleBiasedMultiply,
	Multiply,
	DoubleBiasedAdd,
};

enum class AnimationFunction : std::uint16_t
{
	One,
	Zero,
	Cosine,
	CosineVariablePeriod,
	DiagonalWave,
	DiagonalWaveVariablePeriod,
	Slide,
	SlideVariablePeriod,
	Noise,
	Jitter,
	Wander,
	Spark,
};

enum class AnimationSource : std::uint16_t
{
	None,
	AOut,
	BOut,
	COut,
	DOut
};

template<std::size_t N>
struct FourCC
{
	std::uint32_t Value;

	constexpr FourCC(const char (&Identifier)[N]) : Value(0)
	{
		static_assert(N == 5, "Tag must be 4 characters");
		Value
			= ((Identifier[3] << 0) | (Identifier[2] << 8)
			   | (Identifier[1] << 16) | (Identifier[0] << 24));
	}
};

template<FourCC Code>
constexpr std::uint32_t operator""_u32()
{
	return Code.Value;
}

enum class TagClass : std::uint32_t
{
	None                             = 0xFFFFFFFF, // 4294967295
	Null                             = 0x00000000, // 0000000000
	Actor                            = "actr"_u32, // 1633907826
	ActorVariant                     = "actv"_u32, // 1633907830
	Antenna                          = "ant!"_u32, // 1634628641
	Biped                            = "bipd"_u32, // 1651077220
	Bitmap                           = "bitm"_u32, // 1651078253
	ContinuousDamageEffect           = "cdmg"_u32, // 1667525991
	ModelCollisionGeometry           = "coll"_u32, // 1668246636
	ColorTable                       = "colo"_u32, // 1668246639
	Contrail                         = "cont"_u32, // 1668247156
	DeviceControl                    = "ctrl"_u32, // 1668575852
	Decal                            = "deca"_u32, // 1684366177
	UiWidgetDefinition               = "DeLa"_u32, // 1147489377
	InputDeviceDefaults              = "devc"_u32, // 1684371043
	Device                           = "devi"_u32, // 1684371049
	DetailObjectCollection           = "dobc"_u32, // 1685021283
	Effect                           = "effe"_u32, // 1701209701
	Equipment                        = "eqip"_u32, // 1701931376
	Flag                             = "flag"_u32, // 1718378855
	Fog                              = "fog "_u32, // 1718576928
	Font                             = "font"_u32, // 1718578804
	MaterialEffects                  = "foot"_u32, // 1718579060
	Garbage                          = "garb"_u32, // 1734439522
	Glow                             = "glw!"_u32, // 1735161633
	GrenadeHudInterface              = "grhi"_u32, // 1735551081
	HudMessageText                   = "hmt "_u32, // 1752003616
	HudNumber                        = "hud#"_u32, // 1752523811
	HudGlobals                       = "hudg"_u32, // 1752523879
	Item                             = "item"_u32, // 1769235821
	ItemCollection                   = "itmc"_u32, // 1769237859
	DamageEffect                     = "jpt!"_u32, // 1785754657
	LensFlare                        = "lens"_u32, // 1818586739
	Lightning                        = "elec"_u32, // 1701602659
	DeviceLightFixture               = "lifi"_u32, // 1818846825
	Light                            = "ligh"_u32, // 1818847080
	SoundLooping                     = "lsnd"_u32, // 1819504228
	DeviceMachine                    = "mach"_u32, // 1835098984
	Globals                          = "matg"_u32, // 1835103335
	Meter                            = "metr"_u32, // 1835365490
	LightVolume                      = "mgs2"_u32, // 1835496242
	Gbxmodel                         = "mod2"_u32, // 1836016690
	Model                            = "mode"_u32, // 1836016741
	MultiplayerScenarioDescription   = "mply"_u32, // 1836084345
	PreferencesNetworkGame           = "ngpr"_u32, // 1852272754
	Object                           = "obje"_u32, // 1868720741
	Particle                         = "part"_u32, // 1885434484
	ParticleSystem                   = "pctl"_u32, // 1885566060
	Physics                          = "phys"_u32, // 1885895027
	Placeholder                      = "plac"_u32, // 1886151011
	PointPhysics                     = "pphy"_u32, // 1886414969
	Projectile                       = "proj"_u32, // 1886547818
	WeatherParticleSystem            = "rain"_u32, // 1918986606
	ScenarioStructureBsp             = "sbsp"_u32, // 1935831920
	Scenery                          = "scen"_u32, // 1935893870
	ShaderTransparentChicagoExtended = "scex"_u32, // 1935893880
	ShaderTransparentChicago         = "schi"_u32, // 1935894633
	Scenario                         = "scnr"_u32, // 1935896178
	ShaderEnvironment                = "senv"_u32, // 1936027254
	ShaderTransparentGlass           = "sgla"_u32, // 1936157793
	Shader                           = "shdr"_u32, // 1936221298
	Sky                              = "sky "_u32, // 1936423200
	ShaderTransparentMeter           = "smet"_u32, // 1936549236
	Sound                            = "snd!"_u32, // 1936614433
	SoundEnvironment                 = "snde"_u32, // 1936614501
	ShaderModel                      = "soso"_u32, // 1936683887
	ShaderTransparentGeneric         = "sotr"_u32, // 1936684146
	UiWidgetCollection               = "Soul"_u32, // 1399813484
	ShaderTransparentPlasma          = "spla"_u32, // 1936747617
	SoundScenery                     = "ssce"_u32, // 1936941925
	StringList                       = "str#"_u32, // 1937011235
	ShaderTransparentWater           = "swat"_u32, // 1937203572
	TagCollection                    = "tagc"_u32, // 1952540515
	CameraTrack                      = "trak"_u32, // 1953653099
	Dialogue                         = "udlg"_u32, // 1969515623
	UnitHudInterface                 = "unhi"_u32, // 1970169961
	Unit                             = "unit"_u32, // 1970170228
	UnicodeStringList                = "ustr"_u32, // 1970500722
	VirtualKeyboard                  = "vcky"_u32, // 1986227065
	Vehicle                          = "vehi"_u32, // 1986357353
	Weapon                           = "weap"_u32, // 2003132784
	Wind                             = "wind"_u32, // 2003398244
	WeaponHudInterface               = "wphi"_u32, // 2003855465
};
} // namespace Blam