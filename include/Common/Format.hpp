#pragma once

#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>
#include <memory>

namespace Common
{
void HexDump(
	const std::span<const std::byte>& Data, std::uint8_t Columns = 16,
	std::FILE* Stream = stdout);

std::string FormatByteCount(std::size_t ByteCount);

template<typename ... ArgsT>
std::string Format( const std::string_view Format, ArgsT ... Args )
{
    int FormatSize = std::snprintf( nullptr, 0, Format.data(), Args ... ) + 1u;
    if( FormatSize <= 0 )
	{
		return "";
	}
    const std::size_t StringSize = static_cast<size_t>( FormatSize );
	std::string Result(StringSize, '\0');
    std::snprintf( Result.data(), StringSize, Format.data(), Args ... );
    return Result;
}

} // namespace Common