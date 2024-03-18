#include <Vulkan/Debug.hpp>

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

namespace
{
std::uint8_t SeverityColor(vk::DebugUtilsMessageSeverityFlagBitsEXT Severity)
{
	switch( Severity )
	{
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
	{
		// Dark Gray
		return 90u;
	}
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
	{
		// Light Gray
		return 90u;
	}
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
	{
		// Light Magenta
		return 95u;
	}
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
	{
		// Light red
		return 91u;
	}
	}
	// Default Foreground Color
	return 39u;
}

std::uint8_t MessageTypeColor(vk::DebugUtilsMessageTypeFlagsEXT MessageType)
{
	if( MessageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral )
	{
		// Dim
		return 2u;
	}
	if( MessageType & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance )
	{
		// Bold/Bright
		return 1u;
	}
	if( MessageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation )
	{
		// Light Gray
		return 90u;
	}
	// Default Foreground Color
	return 39u;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugMessengerCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT      Severity,
	vk::DebugUtilsMessageTypeFlagsEXT             Type,
	const vk::DebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData
)
{
	fmt::println(
		"[\033[{}m{}\033[0m]\033[{}m{}\033[0m: {}", SeverityColor(Severity),
		vk::to_string(Severity), MessageTypeColor(Type), vk::to_string(Type),
		CallbackData->pMessage
	);

	switch( Severity )
	{
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
		break;
	}

	return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT      Severity,
	VkDebugUtilsMessageTypeFlagsEXT             Type,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData
)
{
	return DebugMessengerCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT(Severity),
		vk::DebugUtilsMessageTypeFlagsEXT(Type),
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

	const vk::DebugUtilsMessengerCreateInfoEXT DebugMessengerInfo = {
		.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
						 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
						 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
						 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
		.messageType
		= vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding
		| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		.pfnUserCallback = DebugMessengerCallback,
	};

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
	const vk::DebugUtilsObjectNameInfoEXT NameInfo = {
		.objectType   = ObjectType,
		.objectHandle = reinterpret_cast<std::uintptr_t>(ObjectHandle),
		.pObjectName  = ObjectName.data(),
	};

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
	const vk::DebugUtilsLabelEXT LabelInfo = {
		.pLabelName = LabelName.data(),
		.color      = Color,
	};

	CommandBuffer.beginDebugUtilsLabelEXT(LabelInfo);
}

void InsertDebugLabel(
	vk::CommandBuffer CommandBuffer, const std::array<float, 4>& Color,
	std::string_view LabelName
)
{
	const vk::DebugUtilsLabelEXT LabelInfo = {
		.pLabelName = LabelName.data(),
		.color      = Color,
	};

	CommandBuffer.insertDebugUtilsLabelEXT(LabelInfo);
}

void EndDebugLabel(vk::CommandBuffer CommandBuffer)
{
	CommandBuffer.endDebugUtilsLabelEXT();
}

} // namespace Vulkan