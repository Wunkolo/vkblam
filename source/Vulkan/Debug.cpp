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
	std::string_view ObjectName
)
{
	vk::DebugUtilsObjectNameInfoEXT NameInfo = {};
	NameInfo.objectType                      = ObjectType;
	NameInfo.objectHandle = reinterpret_cast<std::uintptr_t>(ObjectHandle);
	NameInfo.pObjectName  = ObjectName.data();

	if( Device.setDebugUtilsObjectNameEXT(NameInfo) != vk::Result::eSuccess )
	{
		// Failed to set object name
	}
}

void BeginDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	std::string_view LabelName
)
{
	vk::DebugUtilsLabelEXT LabelInfo = {};
	LabelInfo.pLabelName             = LabelName.data();
	LabelInfo.color[0]               = Color[0];
	LabelInfo.color[1]               = Color[1];
	LabelInfo.color[2]               = Color[2];
	LabelInfo.color[3]               = Color[3];

	CommandBuffer.beginDebugUtilsLabelEXT(LabelInfo);
}

void InsertDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	std::string_view LabelName
)
{
	vk::DebugUtilsLabelEXT LabelInfo = {};
	LabelInfo.pLabelName             = LabelName.data();
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