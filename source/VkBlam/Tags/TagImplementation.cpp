
#include <VkBlam/Tags/TagImplementation.hpp>

namespace VkBlam
{
TagImplementationBase::~TagImplementationBase()
{
}

TagSubsystemBase::~TagSubsystemBase()
{
}

TagSubsystemBase::TagSubsystemBase(TagPool& Pool) : Pool(Pool)
{
}
} // namespace VkBlam