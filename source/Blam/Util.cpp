#include <Blam/Util.hpp>
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
	TagStr = __builtin_bswap32(TagStr);
	return std::string(reinterpret_cast<const char*>(&TagStr), 4);
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
		"FileSize: %d\n"
		"PaddingLength: %d\n"
		"TagIndexOffset: %08x\n"
		"TagIndexSize: %d\n"
		"ScenarioName: %.32s\n"
		"BuildVersion: %.32s\n"
		"Type: %s\n"
		"Checksum: %08x\n",
		ToString(Value.Version), Value.FileSize, Value.PaddingLength,
		Value.TagIndexOffset, Value.TagIndexSize, Value.ScenarioName,
		Value.BuildVersion, ToString(Value.Type), Value.Checksum);
}
} // namespace Blam