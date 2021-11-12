#include <Common/Format.hpp>

#include <algorithm>

namespace Common
{
void HexDump(
	const std::span<const std::byte>& Data, std::uint8_t Columns,
	std::FILE* Stream)
{

	for( std::size_t CurOffset = 0; CurOffset < Data.size();
		 CurOffset += Columns )
	{
		std::printf("0x%08lX:", CurOffset);
		for( const auto& Byte : Data.subspan(
				 CurOffset,
				 std::min<std::size_t>(Data.size() - CurOffset, Columns)) )
		{
			std::fprintf(Stream, " %02hhX", Byte);
		}
		std::fprintf(Stream, "\n");
	}
}
} // namespace Common