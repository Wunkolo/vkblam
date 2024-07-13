#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <optional>
#include <vector>

namespace Vulkan
{

// Simple management class for a query pool of a specific query-type.
// To be utilized by other types for profiling
class QueryPool
{
private:
	const Vulkan::Context& VulkanContext;

	const vk::QueryType        QueryType;
	const std::size_t          QueryCount;
	std::vector<std::uint64_t> QueryData;
	std::vector<bool>          QueryActive;

	vk::UniqueQueryPool Pool;

public:
	QueryPool(
		const Vulkan::Context& VulkanContext, vk::QueryType QueryType,
		std::size_t QueryCount = 256
	);
	~QueryPool();

	[[nodiscard]] std::optional<std::size_t> FindFreeQuery() const;
	[[nodiscard]] std::optional<std::size_t> AllocateFreeQuery();

	void FlushActiveQuerys();

	// Depending on the type of query, will return a span of either 1 value or
	// multiple values
	[[nodiscard]] std::optional<std::span<std::uint64_t>>
		GetQuery(std::size_t QueryIndex);

	void BeginQuery(vk::CommandBuffer CommandBuffer, std::uint32_t QueryIndex);
	void EndQuery(vk::CommandBuffer CommandBuffer, std::uint32_t QueryIndex);

	void WriteTimestamp(
		vk::CommandBuffer CommandBuffer, std::uint32_t QueryIndex,
		vk::PipelineStageFlagBits PipelineStage
		= vk::PipelineStageFlagBits::eAllCommands
	);
};

/*
extern template class QueryPool<vk::QueryType::eOcclusion>;
extern template class QueryPool<vk::QueryType::ePipelineStatistics>;
extern template class QueryPool<vk::QueryType::eTimestamp>;
*/

} // namespace Vulkan