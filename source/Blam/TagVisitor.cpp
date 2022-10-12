#include <Blam/TagVisitor.hpp>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace Blam
{

void DispatchTagVisitors(
	std::span<const TagVisiterProc> Visitors, const Blam::MapFile& Map)
{
	// Set of all tag-classes that are ever going to be visited
	std::unordered_set<TagClass> VisitClasses;

	std::unordered_map<TagClass, std::vector<Blam::TagIndexEntry>> Tags;

	// Created a new list of each visitor such that they are sorted by their
	// dependencies
	std::vector<const TagVisiterProc*> VisitorDAG;
	VisitorDAG.reserve(Visitors.size());

	for( const auto& CurVisitor : Visitors )
	{
		VisitClasses.insert(CurVisitor.VisitClass);

		VisitorDAG.push_back(&CurVisitor);
	}

	std::stable_sort(
		VisitorDAG.begin(), VisitorDAG.end(),
		[](const TagVisiterProc* A, const TagVisiterProc* B) -> bool {
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

	for( const auto& CurVisitor : VisitorDAG )
	{
		if( CurVisitor->BeginVisits )
		{
			CurVisitor->BeginVisits(Map);
		}

		// Todo: Parallel Visits with a pool of threads
		if( CurVisitor->VisitTags )
		{
			CurVisitor->VisitTags(Tags.at(CurVisitor->VisitClass), Map);
		}

		if( CurVisitor->EndVisits )
		{
			CurVisitor->EndVisits(Map);
		}
	}
}

} // namespace Blam
