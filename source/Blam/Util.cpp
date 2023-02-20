#include <Blam/Util.hpp>
#include <Common/Endian.hpp>
#include <memory>

namespace Blam
{
std::string FormatTagClass(Blam::TagClass Class)
{
	std::uint32_t TagStr = static_cast<std::uint32_t>(Class);
	if( Class == Blam::TagClass::None )
	{
		TagStr = '-' * 0x01010101;
	}
	TagStr = Common::Swap32(TagStr);
	return std::string(reinterpret_cast<const char*>(&TagStr), 4);
}

static std::array<std::size_t, 20> VertexFormatStride{{
	56, 32, 20, 8, 68, 32, 24, 36, 24, 16,
	16, 20, 32, 8, 32, 32, 36, 28, 32, 40,
}};

std::size_t GetVertexStride(VertexFormat Format)
{
	return VertexFormatStride.at(static_cast<std::size_t>(Format));
}

template<typename... ArgsT>
std::string FormatString(const std::string& Format, ArgsT... Args)
{
	const auto FormatSize
		= std::snprintf(nullptr, 0, Format.c_str(), Args...) + 1;
	if( FormatSize <= 0 )
	{
		return "(Error formatting)";
	}
	const std::size_t       Size   = static_cast<std::size_t>(FormatSize);
	std::unique_ptr<char[]> Buffer = std::make_unique<char[]>(Size);
	std::snprintf(Buffer.get(), Size, Format.c_str(), Args...);
	return std::string(Buffer.get(), Buffer.get() + Size - 1);
}

const char* ToString(const CacheVersion& Value)
{
	switch( Value )
	{
	case CacheVersion::Xbox:
		return "Xbox";
	case CacheVersion::Demo:
		return "Demo";
	case CacheVersion::Retail:
		return "Retail";
	case CacheVersion::H1A:
		return "H1A";
	case CacheVersion::CustomEdition:
		return "CustomEdition";
	default:
		return "(Unknown)";
	}
}

const char* ToString(const ScenarioType& Value)
{
	switch( Value )
	{
	case ScenarioType::SinglePlayer:
		return "SinglePlayer";
	case ScenarioType::MultiPlayer:
		return "MultiPlayer";
	case ScenarioType::UserInterface:
		return "UserInterface";
	default:
		return "(Unknown)";
	}
}

std::string ToString(const MapHeader& Value)
{
	return FormatString(
		"Version: %s\n"
		"FileSize: 0x%08x\n"
		"PaddingLength: 0x%08x\n"
		"TagIndexOffset: 0x%08x\n"
		"TagIndexSize: %u\n"
		"ScenarioName: \"%.32s\"\n"
		"BuildVersion: \"%.32s\"\n"
		"Type: %s\n"
		"Checksum: 0x%08x\n",
		ToString(Value.Version), Value.FileSize, Value.PaddingLength,
		Value.TagIndexOffset, Value.TagIndexSize, Value.ScenarioName,
		Value.BuildVersion, ToString(Value.Type), Value.Checksum);
}

std::string ToString(const TagIndexHeader& Value)
{
	return FormatString(
		"TagIndexVirtualOffset: 0x%08x\n"
		"BaseTag: 0x%08x\n"
		"ScenarioTagID: 0x%08x\n"
		"TagCount: %u\n"
		"VertexCount: %u\n"
		"VertexOffset: 0x%08x\n"
		"IndexCount: %u\n"
		"IndexOffset: 0x%08x\n"
		"ModelDataSize: %u\n",
		Value.TagIndexVirtualOffset, Value.BaseTag, Value.ScenarioTagID,
		Value.TagCount, Value.VertexCount, Value.VertexOffset, Value.IndexCount,
		Value.IndexOffset, Value.ModelDataSize);
}

std::string ToString(const TagIndexEntry& Value)
{
	return FormatString(
		"ClassPrimary: %.4s\n"
		"ClassSecondary: %.4s\n"
		"ClassTertiary: %.4s\n"
		"TagID: 0x%08x\n"
		"TagPathVirtualOffset: 0x%08x\n"
		"TagDataVirtualOffset: 0x%08x\n"
		"IsExternal: %s\n",
		FormatTagClass(Value.ClassPrimary).c_str(),
		FormatTagClass(Value.ClassSecondary).c_str(),
		FormatTagClass(Value.ClassTertiary).c_str(), Value.TagID,
		Value.TagPathVirtualOffset, Value.TagDataVirtualOffset,
		Value.IsExternal ? "true" : "false");
}
} // namespace Blam