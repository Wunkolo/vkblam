#pragma once

#include <Blam/Types.hpp>

#include <cstddef>
#include <span>

namespace Blam
{
// Represents a runtime chunk of memory mapped
// to the specified base address. Intended to help "devirtualize" runtime
// offsets and pointers into regular file-offsets
struct VirtualHeap
{
	const std::uint32_t              BaseAddress;
	const std::span<const std::byte> Data;

	template<typename T>
	std::span<const T> GetBlock(const TagBlock<T>& Block) const
	{
		return Block.GetSpan(Data.data(), BaseAddress);
	}

	template<typename T>
	const T& Read(std::uint32_t Offset = 0u) const
	{
		return *reinterpret_cast<const T*>(
			Data.subspan(Offset - BaseAddress).data()
		);
	}
};
} // namespace Blam