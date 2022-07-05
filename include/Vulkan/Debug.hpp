#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <type_traits>
#include <utility>

namespace Vulkan
{
void SetObjectName(
	vk::Device Device, vk::ObjectType ObjectType, const void* ObjectHandle,
	const char* Format, ...);

template<
	typename T,
	typename = std::enable_if_t<vk::isVulkanHandleType<T>::value == true>,
	typename... ArgsT>
inline void SetObjectName(
	vk::Device Device, const T ObjectHandle, const char* Format,
	ArgsT&&... Args)
{
	SetObjectName(
		Device, T::objectType, ObjectHandle, Format,
		std::forward<ArgsT>(Args)...);
}

void BeginDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	const char* Format, ...);

void InsertDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	const char* Format, ...);

void EndDebugLabel(vk::CommandBuffer CommandBuffer);

class DebugLabelScope
{
private:
	const vk::CommandBuffer CommandBuffer;

public:
	template<typename... ArgsT>
	DebugLabelScope(
		vk::CommandBuffer           TargetCommandBuffer,
		const std::array<float, 4>& Color, const char* Format, ArgsT&&... Args)
		: CommandBuffer(TargetCommandBuffer)
	{
		BeginDebugLabel(
			CommandBuffer, Color, Format, std::forward<ArgsT>(Args)...);
	}

	template<typename... ArgsT>
	void operator()(
		const std::array<float, 4>& Color, const char* Format,
		ArgsT&&... Args) const
	{
		InsertDebugLabel(
			CommandBuffer, Color, Format, std::forward<ArgsT>(Args)...);
	}

	~DebugLabelScope()
	{
		EndDebugLabel(CommandBuffer);
	}
};

} // namespace Vulkan