#include <Vulkan/Debug.hpp>

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

namespace Vulkan
{
void SetObjectName(
	vk::Device Device, vk::ObjectType ObjectType, const void* ObjectHandle,
	const char* Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	const auto NameLength = std::vsnprintf(nullptr, 0, Format, Args);
	va_end(Args);
	if( NameLength < 0 )
	{
		// Invalid vsnprintf
		return;
	}

	std::unique_ptr<char[]> ObjectName
		= std::make_unique<char[]>(std::size_t(NameLength) + 1u);

	// Write formatted object name
	va_start(Args, Format);
	std::vsnprintf(
		ObjectName.get(), std::size_t(NameLength) + 1u, Format, Args);
	va_end(Args);

	vk::DebugUtilsObjectNameInfoEXT NameInfo = {};
	NameInfo.objectType                      = ObjectType;
	NameInfo.objectHandle = reinterpret_cast<std::uintptr_t>(ObjectHandle);
	NameInfo.pObjectName  = ObjectName.get();

	if( Device.setDebugUtilsObjectNameEXT(NameInfo) != vk::Result::eSuccess )
	{
		// Failed to set object name
	}
}

void BeginDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	const char* Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	const auto NameLength = std::vsnprintf(nullptr, 0, Format, Args);
	va_end(Args);
	if( NameLength < 0 )
	{
		// Invalid vsnprintf
		return;
	}

	std::unique_ptr<char[]> ObjectName
		= std::make_unique<char[]>(std::size_t(NameLength) + 1u);

	// Write formatted object name
	va_start(Args, Format);
	std::vsnprintf(
		ObjectName.get(), std::size_t(NameLength) + 1u, Format, Args);
	va_end(Args);

	vk::DebugUtilsLabelEXT LabelInfo = {};
	LabelInfo.pLabelName             = ObjectName.get();
	LabelInfo.color[0]               = Color[0];
	LabelInfo.color[1]               = Color[1];
	LabelInfo.color[2]               = Color[2];
	LabelInfo.color[3]               = Color[3];

	CommandBuffer.beginDebugUtilsLabelEXT(LabelInfo);
}

void InsertDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	const char* Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	const auto NameLength = std::vsnprintf(nullptr, 0, Format, Args);
	va_end(Args);
	if( NameLength < 0 )
	{
		// Invalid vsnprintf
		return;
	}

	std::unique_ptr<char[]> ObjectName
		= std::make_unique<char[]>(std::size_t(NameLength) + 1u);

	// Write formatted object name
	va_start(Args, Format);
	std::vsnprintf(
		ObjectName.get(), std::size_t(NameLength) + 1u, Format, Args);
	va_end(Args);

	vk::DebugUtilsLabelEXT LabelInfo = {};
	LabelInfo.pLabelName             = ObjectName.get();
	LabelInfo.color[0]               = Color[0];
	LabelInfo.color[1]               = Color[1];
	LabelInfo.color[2]               = Color[2];
	LabelInfo.color[3]               = Color[3];

	CommandBuffer.insertDebugUtilsLabelEXT(LabelInfo);
}

void EndDebugLabel(vk::CommandBuffer CommandBuffer)
{
	CommandBuffer.endDebugUtilsLabelEXT();
}

} // namespace Vulkan