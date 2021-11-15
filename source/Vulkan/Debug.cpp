#include <Vulkan/Debug.hpp>

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace Vulkan
{
void SetObjectName(
	vk::Device Device, vk::ObjectType ObjectType, const void* ObjectHandle,
	const char* Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	const auto nameLength = std::vsnprintf(nullptr, 0, Format, Args);
	if( nameLength < 0 )
	{
		// Invalid vsnprintf
		va_end(Args);
		return;
	}

	std::unique_ptr<char[]> ObjectName
		= std::make_unique<char[]>(size_t(nameLength) + 1u);

	// Write formatted object name
	std::vsnprintf(ObjectName.get(), size_t(nameLength) + 1, Format, Args);

	vk::DebugUtilsObjectNameInfoEXT NameInfo = {};
	NameInfo.objectType                      = ObjectType;
	NameInfo.objectHandle = reinterpret_cast<std::uintptr_t>(ObjectHandle);
	NameInfo.pObjectName  = ObjectName.get();

	if( Device.setDebugUtilsObjectNameEXT(NameInfo) != vk::Result::eSuccess )
	{
		// Failed to set object name
	}

	va_end(Args);
}
} // namespace Vulkan