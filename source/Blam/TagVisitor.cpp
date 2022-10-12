#include <Blam/TagVisitor.hpp>

#include <algorithm>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace Blam
{

void DispatchTagVisitors(
	std::span<const TagVisitorProc> Visitors, const Blam::MapFile& Map)
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
		});

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

			const std::size_t TagsPerThread
				= std::max<std::size_t>(TagList.size() / ThreadCount, 1);

			// If there are less tags than threads, then emit a smaller amount
			// of threads
			const std::size_t CurVisitorThreads
				= std::min<std::size_t>(TagsPerThread, TagList.size());

			if( CurVisitor->Parallel )
			{
				for( auto& Thread :
					 std::span(ThreadPool).first(CurVisitorThreads) )
				{
					const std::size_t TagsThisThread
						= std::min(TagsPerThread, TagList.size());

					if( TagsThisThread == 0 )
						continue;

					std::mutex Barrier;

					auto ThreadProc =
						[&Barrier](
							const Blam::TagVisitorProc*          Visitor,
							const Blam::MapFile&                 Map,
							std::span<const Blam::TagIndexEntry> Tags) -> void {
						std::scoped_lock Lock{Barrier};
						Visitor->VisitTags(Tags, Map);
					};
					Thread = std::thread(
						ThreadProc, CurVisitor, std::ref(Map),
						TagList.first(TagsPerThread));
					TagList = TagList.subspan(TagsPerThread);
				}

				for( auto& Thread :
					 std::span(ThreadPool).first(CurVisitorThreads) )
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
