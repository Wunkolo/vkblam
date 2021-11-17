#include "VulkanAPI.hpp"

#include <type_traits>

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
	vk::CommandBuffer CommandBuffer;

public:
	DebugLabelScope(
		vk::CommandBuffer           TargetCommandBuffer,
		const std::array<float, 4>& Color, const char* Format, ...)
		: CommandBuffer(TargetCommandBuffer)
	{
		BeginDebugLabel(CommandBuffer, Color, Format);
	}

	void operator()(const std::array<float, 4>& Color, const char* Format, ...)
	{
		InsertDebugLabel(CommandBuffer, Color, Format);
	}

	~DebugLabelScope()
	{
		EndDebugLabel(CommandBuffer);
	}
};

} // namespace Vulkan