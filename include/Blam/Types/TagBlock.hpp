#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace Blam
{
#pragma pack(push, 1)
template<typename T = void>
struct TagBlock
{
	std::uint32_t Count;
	std::uint32_t VirtualOffset;
	std::uint32_t Unknown8;

	template<
		typename U = T,
		typename = typename std::enable_if_t<std::is_same_v<U, void> == false>>
	std::span<const U>
		GetSpan(const std::byte Data[], std::uint32_t VirtualBase) const
	{
		return std::span<const U>(
			reinterpret_cast<const U*>(Data + (VirtualOffset - VirtualBase)),
			Count
		);
	}
};
#pragma pack(pop)

static_assert(sizeof(TagBlock<void>) == 12);
} // namespace Blam