#pragma once

#include <cstddef>
#include <cstdint>

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

struct TagReference
{
	TagClass      Class;
	std::uint32_t PathOffset;
	std::uint32_t PathLength;
	std::uint32_t TagID;
};

struct Reflexive
{
	std::uint32_t Count;
	std::uint32_t Offset;
	std::uint32_t UnknownC;
};

// Header for variable-sized array of data in a tag
struct Block
{
	std::uint32_t Count;
	std::uint64_t Offset;
};
static_assert(sizeof(Block) == 12);

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
	std::uint32_t TagArrayOffset;
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
	std::uint32_t TagPathOffset;
	std::uint32_t TagDataOffset;
	std::uint32_t IsExternal; // For bitmaps
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

#pragma pack(pop)
} // namespace Blam