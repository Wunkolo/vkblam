#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <type_traits>
#include <utility>

#ifdef _MSC_VER
#include <sal.h>
#define VK_PRINTF_FORMAT _Printf_format_string_
#define VK_PRINTF_FORMAT_ATTR(format_arg_index, dots_arg_index)
#else
#define VK_PRINTF_FORMAT
#define VK_PRINTF_FORMAT_ATTR(format_arg_index, dots_arg_index)                \
	__attribute__((__format__(__printf__, format_arg_index, dots_arg_index)))
#endif

namespace Vulkan
{

VK_PRINTF_FORMAT_ATTR(4, 5)
void SetObjectName(
	vk::Device Device, vk::ObjectType ObjectType, const void* ObjectHandle,
	VK_PRINTF_FORMAT const char* Format, ...
);

template<
	typename T,
	typename = std::enable_if_t<vk::isVulkanHandleType<T>::value == true>,
	typename... ArgsT>
inline void SetObjectName(
	vk::Device Device, const T ObjectHandle,
	VK_PRINTF_FORMAT const char* Format, ArgsT&&... Args
)
{
	SetObjectName(
		Device, T::objectType, ObjectHandle, Format,
		std::forward<ArgsT>(Args)...
	);
}

VK_PRINTF_FORMAT_ATTR(3, 4)
void BeginDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	VK_PRINTF_FORMAT const char* Format, ...
);

VK_PRINTF_FORMAT_ATTR(3, 4)
void InsertDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	VK_PRINTF_FORMAT const char* Format, ...
);

void EndDebugLabel(vk::CommandBuffer CommandBuffer);

class DebugLabelScope
{
private:
	const vk::CommandBuffer CommandBuffer;

public:
	template<typename... ArgsT>
	DebugLabelScope(
		vk::CommandBuffer           TargetCommandBuffer,
		const std::array<float, 4>& Color, VK_PRINTF_FORMAT const char* Format,
		ArgsT&&... Args
	)
		: CommandBuffer(TargetCommandBuffer)
	{
		BeginDebugLabel(
			CommandBuffer, Color, Format, std::forward<ArgsT>(Args)...
		);
	}

	template<typename... ArgsT>
	void operator()(
		const std::array<float, 4>& Color, VK_PRINTF_FORMAT const char* Format,
		ArgsT&&... Args
	) const
	{
		InsertDebugLabel(
			CommandBuffer, Color, Format, std::forward<ArgsT>(Args)...
		);
	}

	~DebugLabelScope()
	{
		EndDebugLabel(CommandBuffer);
	}
};

} // namespace Vulkan