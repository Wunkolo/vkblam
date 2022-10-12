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

		auto InsertPoint = VisitorDAG.begin();

		// Place this visitor strictly AFTER any other visitors it depends on
		for( const TagClass& DependClass : CurVisitor.DependClasses )
		{
			InsertPoint
				= std::find_if(
					  VisitorDAG.rbegin(), std::reverse_iterator(InsertPoint),
					  [&DependClass](const TagVisiterProc* Visitor) -> bool {
						  return Visitor->VisitClass == DependClass;
					  })
					  .base();
		}

		VisitorDAG.insert(InsertPoint, &CurVisitor);
	}

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
		CurVisitor->BeginVisits(Map);

		// Todo: Parallel Visits with a pool of threads
		CurVisitor->VisitTags(Tags.at(CurVisitor->VisitClass), Map);

		CurVisitor->EndVisits(Map);
	}
}

} // namespace Blam
