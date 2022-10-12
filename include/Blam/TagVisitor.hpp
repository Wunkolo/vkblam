#pragma once

#include <functional>
#include <unordered_set>

#include <Blam/Blam.hpp>

namespace Blam
{

struct TagVisitorProc
{
	// Do not begin this TagVisitor until previous visitors have ran of this
	// type
	std::unordered_set<Blam::TagClass> DependClasses;

	// Visits tags of this designated primary class
	Blam::TagClass VisitClass = Blam::TagClass::None;

	// Ran before all tags within a map are about to be visited
	std::function<void(const Blam::MapFile&)> BeginVisits;

	// Allow Tag visits to happen in parallel
	bool Parallel = false;

	// Visits a particular tag from a particular map file
	std::function<void(
		std::span<const Blam::TagIndexEntry>, const Blam::MapFile&)>
		VisitTags;

	// Ran after all tags within a map have been visited
	std::function<void(const Blam::MapFile&)> EndVisits;
};

class TagVisiter
{
	virtual ~TagVisiter() = 0;

	virtual std::vector<TagVisitorProc> GetTagVisitorProcs() = 0;
};

// Dispatch a sequence of TagVisitorProc structures against all the tags within
// a particular map file. Will automatically handle dispatching dependent Tag
// Visitors in order
void DispatchTagVisitors(
	std::span<const TagVisitorProc> Visitors, const Blam::MapFile& Map);

} // namespace Blam