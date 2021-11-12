#pragma once

namespace Common
{
// x86
#if defined(__i386__) || defined(__x86_64__)
#if defined(_MSC_VER)

#include <intrin.h>

inline std::uint64_t Swap64(std::uint64_t x)
{
	return _byteswap_uint64(x);
}

inline std::uint32_t Swap32(std::uint32_t x)
{
	return _byteswap_ulong(x);
}

inline std::uint16_t Swap16(std::uint16_t x)
{
	return _byteswap_ushort(x);
}

#elif defined(__GNUC__) || defined(__clang__)

#include <x86intrin.h>

inline std::uint64_t Swap64(std::uint64_t x)
{
	return __builtin_bswap64(x);
}

inline std::uint32_t Swap32(std::uint32_t x)
{
	return __builtin_bswap32(x);
}

inline std::uint16_t Swap16(std::uint16_t x)
{
	return __builtin_bswap16(x);
}

#endif

// ARM
#elif defined(__ARM_NEON)

#include <arm_neon.h>

#if defined(_MSC_VER)

inline std::uint64_t Swap64(std::uint64_t x)
{
	return _byteswap_uint64(x);
}

inline std::uint32_t Swap32(std::uint32_t x)
{
	return _byteswap_ulong(x);
}

inline std::uint16_t Swap16(std::uint16_t x)
{
	return _byteswap_ushort(x);
}

#elif defined(__GNUC__) || defined(__clang__)

inline std::uint64_t Swap64(std::uint64_t x)
{
	return __builtin_bswap64(x);
}

inline std::uint32_t Swap32(std::uint32_t x)
{
	return __builtin_bswap32(x);
}

inline std::uint16_t Swap16(std::uint16_t x)
{
	return __builtin_bswap16(x);
}

#endif

// Pure
#else

inline std::uint64_t Swap64(std::uint64_t x)
{
	return (
		((x & 0x00000000000000FF) << 56) | ((x & 0x000000000000FF00) << 40)
		| ((x & 0x0000000000FF0000) << 24) | ((x & 0x00000000FF000000) << 8)
		| ((x & 0x000000FF00000000) >> 8) | ((x & 0x0000FF0000000000) >> 24)
		| ((x & 0x00FF000000000000) >> 40) | ((x & 0xFF00000000000000) >> 56));
}

inline std::uint32_t Swap32(std::uint32_t x)
{
	return (
		((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8)
		| ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24));
}

inline std::uint16_t Swap16(std::uint16_t x)
{
	return (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8));
}

#endif
} // namespace Common