#include <Blam/Util.hpp>

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
} // namespace Blam