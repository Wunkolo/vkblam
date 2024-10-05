#include <VkBlam/Tags/TagImplementation.hpp>
#include <VkBlam/Tags/TagPool.hpp>

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

const Blam::MapFile& TagSubsystemBase::GetMapFile() const
{
	return GetPool().GetMapFile();
}

TagSubsystemBase::TagSubsystemBase(TagPool& Pool) : Pool(Pool)
{
}
} // namespace VkBlam