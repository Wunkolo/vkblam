#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <fmt/core.h>

#include <string_view>
#include <variant>

namespace Vulkan
{

vk::UniqueDebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance Instance);

// Command buffer markers
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

void EndDebugLabel(vk::CommandBuffer CommandBuffer);

// Queue buffer markers
void BeginDebugLabel(
	vk::Queue Queue, const std::array<float, 4>& Color,
	std::string_view LabelName
);

void InsertDebugLabel(
	vk::Queue Queue, const std::array<float, 4>& Color,
	std::string_view LabelName
);

void EndDebugLabel(vk::Queue Queue);

template<typename T>
concept VulkanHandleType = vk::isVulkanHandleType<T>::value;

// Set Vulkan-Object name (automatically deduce object-type)
template<VulkanHandleType T, typename... ArgsT>
inline void SetObjectName(
	vk::Device Device, const T ObjectHandle,
	fmt::format_string<ArgsT...> Format, ArgsT&&... Args
)
{
	SetObjectName(
		Device, T::objectType, ObjectHandle,
		fmt::format(Format, std::forward<ArgsT>(Args)...)
	);
}

// Command buffer markers (formatted)
template<typename... ArgsT>
void BeginDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	fmt::format_string<ArgsT...> Format, ArgsT&&... Args
)
{
	BeginDebugLabel(
		CommandBuffer, Color, fmt::format(Format, std::forward<ArgsT>(Args)...)
	);
}

template<typename... ArgsT>
void InsertDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	fmt::format_string<ArgsT...> Format, ArgsT&&... Args
)
{
	InsertDebugLabel(
		CommandBuffer, Color, fmt::format(Format, std::forward<ArgsT>(Args)...)
	);
}

// Command buffer markers (formatted)
template<typename... ArgsT>
void BeginDebugLabel(
	vk::Queue Queue, const std::array<float, 4>& Color,
	fmt::format_string<ArgsT...> Format, ArgsT&&... Args
)
{
	BeginDebugLabel(
		Queue, Color, fmt::format(Format, std::forward<ArgsT>(Args)...)
	);
}

template<typename... ArgsT>
void InsertDebugLabel(
	vk::Queue Queue, const std::array<float, 4>& Color,
	fmt::format_string<ArgsT...> Format, ArgsT&&... Args
)
{
	InsertDebugLabel(
		Queue, Color, fmt::format(Format, std::forward<ArgsT>(Args)...)
	);
}

// RAII-based utility-object to automatically begin and end label-scopes
// within a command-buffer
class DebugLabelScope
{
private:
	const std::variant<vk::CommandBuffer, vk::Queue> Target;

public:
	template<typename... ArgsT>
	DebugLabelScope(
		vk::CommandBuffer           TargetCommandBuffer,
		const std::array<float, 4>& Color, fmt::format_string<ArgsT...> Format,
		ArgsT&&... Args
	)
		: Target(TargetCommandBuffer)
	{
		BeginDebugLabel(
			TargetCommandBuffer, Color,
			fmt::format(Format, std::forward<ArgsT>(Args)...)
		);
	}

	template<typename... ArgsT>
	DebugLabelScope(
		vk::Queue TargetQueue, const std::array<float, 4>& Color,
		fmt::format_string<ArgsT...> Format, ArgsT&&... Args
	)
		: Target(TargetQueue)
	{
		BeginDebugLabel(
			TargetQueue, Color,
			fmt::format(Format, std::forward<ArgsT>(Args)...)
		);
	}

	template<typename... ArgsT>
	void operator()(
		const std::array<float, 4>& Color, fmt::format_string<ArgsT...> Format,
		ArgsT&&... Args
	) const
	{
		if( Target.index() == 0 )
		{
			InsertDebugLabel(
				std::get<vk::CommandBuffer>(Target), Color,
				fmt::format(Format, std::forward<ArgsT>(Args)...)
			);
		}
		else if( Target.index() == 1 )
		{
			InsertDebugLabel(
				std::get<vk::Queue>(Target), Color,
				fmt::format(Format, std::forward<ArgsT>(Args)...)
			);
		}
	}

	~DebugLabelScope()
	{
		if( Target.index() == 0 )
		{
			EndDebugLabel(std::get<vk::CommandBuffer>(Target));
		}
		else if( Target.index() == 1 )
		{
			EndDebugLabel(std::get<vk::Queue>(Target));
		}
	}
};

} // namespace Vulkan