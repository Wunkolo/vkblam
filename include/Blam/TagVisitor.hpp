#pragma once

#include <functional>
#include <vector>

#include <Blam/Blam.hpp>

namespace Blam
{

struct TagVisiterProc
{
	// Do not begin this TagVisitor until previous visitors have ran of this
	// type
	std::vector<Blam::TagClass> DependClasses;

	// Visits tags of this designated primary class
	Blam::TagClass VisitClass;

	// Ran before all tags within a map are about to be visited
	std::function<void(const Blam::MapFile&)> BeginVisits;

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

	virtual std::vector<TagVisiterProc> GetTagVisiterProcs() = 0;
};

// Dispatch a sequence of TagVisitorProc structures against all the tags within
// a particular map file. Will automatically handle dispatching dependent Tag
// Visitors in order
void DispatchTagVisitors(
	std::span<const TagVisiterProc> Visitors, const Blam::MapFile& Map);

} // namespace Blam