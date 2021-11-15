#pragma once

#include <cstdint>
#include <cstdio>
#include <span>
#include <string>

namespace Common
{
void HexDump(
	const std::span<const std::byte>& Data, std::uint8_t Columns = 16,
	std::FILE* Stream = stdout);

std::string FormatByteCount(std::size_t ByteCount);

} // namespace Common