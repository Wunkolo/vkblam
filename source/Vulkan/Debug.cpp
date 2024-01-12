#include <Vulkan/Debug.hpp>

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

namespace
{
static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugMessengerCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT      Severity,
	vk::DebugUtilsMessageTypeFlagsEXT             Type,
	const vk::DebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData
)
{
	std::printf(
		"[%s]%s: %s\n", vk::to_string(Severity).c_str(),
		vk::to_string(Type).c_str(), CallbackData->pMessage
	);

	return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT      Severity,
	VkDebugUtilsMessageTypeFlagsEXT             Type,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData
)
{
	return DebugMessengerCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT{Severity},
		vk::DebugUtilsMessageTypeFlagsEXT{Type},
		reinterpret_cast<const vk::DebugUtilsMessengerCallbackDataEXT*>(
			CallbackData
		),
		UserData
	);
}
} // namespace

namespace Vulkan
{

vk::UniqueDebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance Instance)
{
	std::vector<vk::ExtensionProperties> InstanceExtensionProperties;

	if( const auto EnumerateResult = vk::enumerateInstanceExtensionProperties();
		EnumerateResult.result == vk::Result::eSuccess )
	{
		InstanceExtensionProperties = EnumerateResult.value;
	}
	else
	{
		// Error enumerating instance extensions
		return {};
	}

	const auto it = std::find_if(
		InstanceExtensionProperties.begin(), InstanceExtensionProperties.end(),
		[](const vk::ExtensionProperties& ExtensionProperties) {
			return std::strcmp(
					   VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
					   ExtensionProperties.extensionName
				   )
				== 0;
		}
	);

	if( it == InstanceExtensionProperties.end() )
	{
		// Does not support VK_EXT_DEBUG_UTILS
		return {};
	}
	// VK_EXT_DEBUG_UTILS supported

	// Create debug messenger

	vk::DebugUtilsMessengerCreateInfoEXT DebugMessengerInfo = {};
	DebugMessengerInfo.messageSeverity
		= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
	DebugMessengerInfo.messageType
		= vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding
		| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

	DebugMessengerInfo.pfnUserCallback = DebugMessengerCallback;

	if( auto CreateResult
		= Instance.createDebugUtilsMessengerEXTUnique(DebugMessengerInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		return std::move(CreateResult.value);
	}
	// Error creating debug utils messenger
	return {};
}

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