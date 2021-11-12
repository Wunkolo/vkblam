#pragma once

#include <cstdint>
#include <cstdio>
#include <span>

namespace Common
{
void HexDump(
	const std::span<const std::byte>& Data, std::uint8_t Columns = 16,
	std::FILE* Stream = stdout);

} // namespace Common