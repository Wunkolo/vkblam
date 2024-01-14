
#include <VkBlam/VkBlam.hpp>

#include <VkBlam/Tags/TagPool.hpp>

#include <concepts>
#include <map>
#include <memory>

namespace VkBlam
{

TagPool::TagPool(Scene& TargetScene) : TargetScene(TargetScene)
{
}

TagPool::~TagPool()
{
}

} // namespace VkBlam