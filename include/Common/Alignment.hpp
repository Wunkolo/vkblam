#pragma once

#include <cstddef>

namespace Common
{

template<typename T>
constexpr T AlignUp(T value, std::size_t size)
{
	const T mod = static_cast<T>(value % size);
	value -= mod;
	return static_cast<T>(mod == T{0} ? value : value + size);
}

template<typename T>
constexpr T AlignDown(T value, std::size_t size)
{
	return static_cast<T>(value - value % size);
}

} // namespace Common