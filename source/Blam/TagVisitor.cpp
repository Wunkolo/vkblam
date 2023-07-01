#include <Blam/TagVisitor.hpp>

#include <algorithm>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Blam
{

void DispatchTagVisitors(
	std::span<const TagVisitorProc> Visitors, const Blam::MapFile& Map
)
{
	// Set of all tag-classes that are ever going to be visited
	std::unordered_set<TagClass> VisitClasses;

	std::unordered_map<TagClass, std::vector<Blam::TagIndexEntry>> Tags;

	// Created a new list of each visitor such that they are sorted by their
	// dependencies
	std::vector<const TagVisitorProc*> VisitorDAG;
	VisitorDAG.reserve(Visitors.size());

	for( const auto& CurVisitor : Visitors )
	{
		VisitClasses.insert(CurVisitor.VisitClass);

		VisitorDAG.push_back(&CurVisitor);
	}

	std::stable_sort(
		VisitorDAG.begin(), VisitorDAG.end(),
		[](const TagVisitorProc* A, const TagVisitorProc* B) -> bool {
			// A should go before B if B depends on A, otherwise just keep the
			// order as it is
			return B->DependClasses.contains(A->VisitClass);
		}
	);

	// Collect all the tags to be visited
	for( const auto& CurTagEntry : Map.GetTagIndexArray() )
	{
		if( VisitClasses.contains(CurTagEntry.ClassPrimary) )
		{
			Tags[CurTagEntry.ClassPrimary].push_back(CurTagEntry);
		}
	}

	const std::size_t ThreadCount = std::thread::hardware_concurrency();

	std::vector<std::thread> ThreadPool(ThreadCount);

	for( const auto& CurVisitor : VisitorDAG )
	{
		if( CurVisitor->BeginVisits )
		{
			CurVisitor->BeginVisits(Map);
		}

		if( CurVisitor->VisitTags )
		{
			auto TagList = std::span(Tags.at(CurVisitor->VisitClass));

			if( CurVisitor->Parallel )
			{
				// If there are less tags than threads, then emit a smaller
				// amount of threads
				const std::size_t CurThreadCount
					= std::min<std::size_t>(ThreadCount, TagList.size());

				// Number of tags per thread, rounded up
				// In the case of an uneven amount of divisions, there will be
				// at least one thread doing a bit less work.
				const std::size_t TagsPerThread
					= (TagList.size() + (CurThreadCount - 1)) / CurThreadCount;

				const auto VisitorThreads
					= std::span(ThreadPool).first(CurThreadCount);

				for( auto& Thread : VisitorThreads )
				{
					const std::size_t TagsThisThread
						= std::min(TagsPerThread, TagList.size());

					if( TagsThisThread == 0 )
						continue;

					auto ThreadProc
						= [](const Blam::TagVisitorProc*          Visitor,
							 const Blam::MapFile&                 Map,
							 std::span<const Blam::TagIndexEntry> Tags
						  ) -> void { Visitor->VisitTags(Tags, Map); };

					Thread = std::thread(
						ThreadProc, CurVisitor, std::ref(Map),
						TagList.first(TagsThisThread)
					);

					TagList = TagList.subspan(TagsThisThread);
				}

				for( auto& Thread : VisitorThreads )
				{
					Thread.join();
				}
			}
			else
			{
				CurVisitor->VisitTags(TagList, Map);
			}
		}

		if( CurVisitor->EndVisits )
		{
			CurVisitor->EndVisits(Map);
		}
	}
}

} // namespace Blam
