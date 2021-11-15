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
} // namespace Vulkan