#include <Common/Format.hpp>

#include <algorithm>
#include <cinttypes>
#include <cmath>

namespace Common
{
void HexDump(
	const std::span<const std::byte>& Data, std::uint8_t Columns,
	std::FILE* Stream
)
{

	for( std::size_t CurOffset = 0; CurOffset < Data.size();
		 CurOffset += Columns )
	{
		std::printf("0x%08" PRIX64 ":", CurOffset);
		for( const auto& Byte : Data.subspan(
				 CurOffset,
				 std::min<std::size_t>(Data.size() - CurOffset, Columns)
			 ) )
		{
			std::fprintf(Stream, " %02" SCNx8, std::uint8_t(Byte));
		}
		std::fprintf(Stream, "\n");
	}
}

std::string FormatByteCount(std::size_t ByteCount)
{
	static const char* SizeUnits[]
		= {"Bytes", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};
	std::size_t   Index;
	std::double_t ByteSize = ByteCount;
	for( Index = 0; Index < std::extent_v<decltype(SizeUnits)>; Index++ )
	{
		if( ByteSize < 1024 )
			break;
		ByteSize /= 1024;
	}
	return std::to_string(ByteSize) + " " + SizeUnits[Index];
}
} // namespace Common