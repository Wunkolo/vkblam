
#include <VkBlam/Tags/TagImplementation.hpp>

namespace VkBlam
{
TagImplementationBase::~TagImplementationBase()
{
}

TagSubsystemBase::~TagSubsystemBase()
{
}

TagPool& TagSubsystemBase::GetPool() const
{
	return Pool;
}

TagSubsystemBase::TagSubsystemBase(TagPool& Pool) : Pool(Pool)
{
}
} // namespace VkBlam