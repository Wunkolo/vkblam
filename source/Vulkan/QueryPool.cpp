#include <Vulkan/QueryPool.hpp>

#include <cmath>
#include <fmt/core.h>
namespace
{
constexpr vk::QueryPipelineStatisticFlags PIPELINE_STATISTICS_ALL_GRAPHICS
	= vk::QueryPipelineStatisticFlagBits::eInputAssemblyVertices
	| vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives
	| vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations
	| vk::QueryPipelineStatisticFlagBits::eGeometryShaderInvocations
	| vk::QueryPipelineStatisticFlagBits::eGeometryShaderPrimitives
	| vk::QueryPipelineStatisticFlagBits::eClippingInvocations
	| vk::QueryPipelineStatisticFlagBits::eClippingPrimitives
	| vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations;

constexpr std::size_t PIPELINE_STATISTICS_ALL_GRAPHICS_COUNT
	= std::popcount(static_cast<std::uint32_t>(PIPELINE_STATISTICS_ALL_GRAPHICS)
	);

// Number of 32-bit or 64-bit elements that an individual query occupies
constexpr std::size_t GetQueryElementCount(
	vk::QueryType                   QueryType,
	vk::QueryPipelineStatisticFlags PipelineStatisticFlags
)
{
	std::size_t QueryItemCount = 0;
	switch( QueryType )
	{
	case vk::QueryType::eOcclusion:
		QueryItemCount = 1;
		break;
	case vk::QueryType::ePipelineStatistics:
		QueryItemCount
			= std::popcount(static_cast<std::uint32_t>(PipelineStatisticFlags));
		break;
	case vk::QueryType::eTimestamp:
		QueryItemCount = 1;
		break;
	default:
		break;
	}

	return QueryItemCount;
}

// Size, in bytes, that an individual 64-bit query of this type will need
constexpr std::size_t GetQueryElementSize(
	vk::QueryType                   QueryType,
	vk::QueryPipelineStatisticFlags PipelineStatisticFlags = {}
)
{
	return sizeof(std::uint64_t)
		 * GetQueryElementCount(QueryType, PipelineStatisticFlags);
}

void PrintPipelineStats(std::span<const std::uint64_t> QueryData)
{
	std::uint32_t CurStatMask
		= static_cast<std::uint32_t>(PIPELINE_STATISTICS_ALL_GRAPHICS);

	for( std::size_t i = 0; i < PIPELINE_STATISTICS_ALL_GRAPHICS_COUNT; ++i )
	{
		// Get lowest set bit
		const std::uint32_t CurBit = (-CurStatMask) & CurStatMask;
		fmt::println(
			"\t{:32}:{}",
			vk::to_string(vk::QueryPipelineStatisticFlagBits(CurBit)),
			QueryData[i]
		);

		// Clear lowest bit
		CurStatMask &= ~CurBit;
	}
}

void PrintTimelineStats(
	std::span<const std::uint64_t> TimeStamps, std::double_t TimestampPeriod
)
{
	for( const std::uint64_t& TimeStamp : TimeStamps )
	{
		fmt::println("\t{} ns", TimeStamp * TimestampPeriod);
	}
}

// Detect runs of "true" values in a std::vector<bool> and call a function
// with the starting-index and count of each run
void IterateRuns(const std::vector<bool>& BitMask, auto&& Proc)
{
	for( std::size_t i = 0; i < BitMask.size(); i++ )
	{
		if( BitMask[i] == true )
		{
			const std::size_t RunStart = i;
			std::size_t       RunCount = 1;
			while( i < BitMask.size() - 1 && BitMask[i] == BitMask[i + 1] )
			{
				RunCount++;
				i++;
			}
			Proc(RunStart, RunCount);
		}
	}
}

} // namespace

namespace Vulkan
{

QueryPool::QueryPool(
	const Vulkan::Context& VulkanContext, vk::QueryType QueryType,
	std::size_t QueryCount
)
	: VulkanContext(VulkanContext), QueryType(QueryType),
	  QueryCount(QueryCount),
	  QueryData(
		  GetQueryElementCount(QueryType, PIPELINE_STATISTICS_ALL_GRAPHICS)
		  * QueryCount
	  ),
	  QueryActive(QueryCount)
{

	const vk::QueryPoolCreateInfo PoolInfo = {
		.queryType          = QueryType,
		.queryCount         = QueryCount,
		.pipelineStatistics = PIPELINE_STATISTICS_ALL_GRAPHICS,
	};

	if( auto CreateResult
		= VulkanContext.LogicalDevice.createQueryPoolUnique(PoolInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		Pool = std::move(CreateResult.value);
	}
}

QueryPool::~QueryPool()
{
	// Testing
	IterateRuns(
		QueryActive,
		[this](std::size_t CurQueryIndex, std::size_t CurQueryCount) {
			const std::size_t QueryDataStride = GetQueryElementSize(
				QueryType, PIPELINE_STATISTICS_ALL_GRAPHICS
			);
			const std::size_t QueryDataSize = QueryDataStride * CurQueryCount;

			if( const auto GetResult
				= VulkanContext.LogicalDevice.getQueryPoolResults(
					Pool.get(), CurQueryIndex, CurQueryCount, QueryDataSize,
					QueryData.data() + CurQueryIndex, QueryDataStride,
					vk::QueryResultFlagBits::e64
						| vk::QueryResultFlagBits::eWait
				);
				GetResult == vk::Result::eSuccess )
			{
				switch( QueryType )
				{
				case vk::QueryType::eOcclusion:
					break;
				case vk::QueryType::ePipelineStatistics:
					fmt::println("{}", CurQueryIndex);
					PrintPipelineStats(std::span(QueryData).subspan(
						CurQueryIndex * PIPELINE_STATISTICS_ALL_GRAPHICS_COUNT,
						PIPELINE_STATISTICS_ALL_GRAPHICS_COUNT
					));
					break;
				case vk::QueryType::eTimestamp:
					PrintTimelineStats(
						std::span(QueryData).subspan(
							CurQueryIndex, CurQueryCount
						),
						VulkanContext.PhysicalDevice.getProperties()
							.limits.timestampPeriod
					);
					break;
				default:
					break;
				}
			}
		}
	);
}

void QueryPool::BeginQuery(
	vk::CommandBuffer CommandBuffer, std::uint32_t QueryIndex
)
{
	CommandBuffer.resetQueryPool(Pool.get(), QueryIndex, 1);
	CommandBuffer.beginQuery(Pool.get(), QueryIndex, {});
	QueryActive[QueryIndex] = true;
}

void QueryPool::EndQuery(
	vk::CommandBuffer CommandBuffer, std::uint32_t QueryIndex
)
{
	CommandBuffer.endQuery(Pool.get(), QueryIndex);
}

void QueryPool::WriteTimestamp(
	vk::CommandBuffer CommandBuffer, std::uint32_t QueryIndex,
	vk::PipelineStageFlagBits PipelineStage
)
{
	CommandBuffer.resetQueryPool(Pool.get(), QueryIndex, 1);
	CommandBuffer.writeTimestamp(PipelineStage, Pool.get(), QueryIndex);
	QueryActive[QueryIndex] = true;
}

} // namespace Vulkan