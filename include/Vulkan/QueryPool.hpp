#pragma once

#include <Vulkan/VulkanAPI.hpp>

#include <vector>

namespace Vulkan
{

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

	void BeginQuery(vk::CommandBuffer CommandBuffer, std::uint32_t QueryIndex);
	void EndQuery(vk::CommandBuffer CommandBuffer, std::uint32_t QueryIndex);

	void WriteTimestamp(
		vk::CommandBuffer CommandBuffer, std::uint32_t QueryIndex,
		vk::PipelineStageFlagBits PipelineStage
		= vk::PipelineStageFlagBits::eAllCommands
	);

	// Uses RAII to handle the aquiring of a query and reporting its
	// results when it is available
	struct QueryScope
	{
		std::size_t       QueryIndex;
		vk::CommandBuffer TargetCommandBuffer;
	};

	QueryScope CreateQueryScope(vk::CommandBuffer CommandBuffer);

	struct TimestampScope
	{
		std::size_t               QueryIndexStart;
		std::size_t               QueryIndexEnd;
		vk::PipelineStageFlagBits PipelineStage;
		vk::CommandBuffer         TargetCommandBuffer;
	};

	TimestampScope CreateTimestampScope(
		vk::CommandBuffer         CommandBuffer,
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