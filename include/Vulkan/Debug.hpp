#pragma once

#include <Vulkan/VulkanAPI.hpp>
#include <format>
#include <string_view>

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

void SetObjectName(
	vk::Device Device, vk::ObjectType ObjectType, const void* ObjectHandle,
	std::string_view ObjectName
);

void BeginDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	std::string_view LabelName
);

void InsertDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	std::string_view LabelName
);

template<typename T>
concept VulkanHandleType = vk::isVulkanHandleType<T>::value;

template<VulkanHandleType T, typename... ArgsT>
inline void SetObjectName(
	vk::Device Device, const T ObjectHandle,
	std::format_string<ArgsT...> Format, ArgsT&&... Args
)
{
	SetObjectName(
		Device, T::objectType, ObjectHandle,
		std::format(Format, std::forward<ArgsT>(Args)...)
	);
}

template<typename... ArgsT>
void BeginDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	std::format_string<ArgsT...> Format, ArgsT&&... Args
)
{
	BeginDebugLabel(
		CommandBuffer, Color, std::format(Format, std::forward<ArgsT>(Args)...)
	);
}

template<typename... ArgsT>
void InsertDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	std::format_string<ArgsT...> Format, ArgsT&&... Args
)
{
	InsertDebugLabel(
		CommandBuffer, Color, std::format(Format, std::forward<ArgsT>(Args)...)
	);
}

void EndDebugLabel(vk::CommandBuffer CommandBuffer);

class DebugLabelScope
{
private:
	const vk::CommandBuffer CommandBuffer;

public:
	template<typename... ArgsT>
	DebugLabelScope(
		vk::CommandBuffer           TargetCommandBuffer,
		const std::array<float, 4>& Color, std::format_string<ArgsT...> Format,
		ArgsT&&... Args
	)
		: CommandBuffer(TargetCommandBuffer)
	{
		BeginDebugLabel(
			CommandBuffer, Color,
			std::format(Format, std::forward<ArgsT>(Args)...)
		);
	}

	template<typename... ArgsT>
	void operator()(
		const std::array<float, 4>& Color, std::format_string<ArgsT...> Format,
		ArgsT&&... Args
	) const
	{
		InsertDebugLabel(
			CommandBuffer, Color,
			std::format(Format, std::forward<ArgsT>(Args)...)
		);
	}

	~DebugLabelScope()
	{
		EndDebugLabel(CommandBuffer);
	}
};

} // namespace Vulkan