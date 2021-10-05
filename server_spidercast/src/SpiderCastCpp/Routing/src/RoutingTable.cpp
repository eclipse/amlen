// Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
/*
 * RoutingTable.cpp
 *
 *  Created on: Jan 31, 2012
 */

#include "RoutingTable.h"

namespace spdr
{

namespace route
{

ScTraceComponent* RoutingTable::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Route,
		trace::ScTrConstants::Layer_ID_Route,
		"RoutingTable",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

RoutingTable::RoutingTable(
		const String& instID,
		const SpiderCastConfigImpl& config,
		VirtualIDCache& vidCache,
		bool failFast) :
	RoutingTableLookup(),
	ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
	instID_(instID),
	config_(config),
	vidCache_(vidCache),
	failFast_(failFast),
	mutex_(),
	nodeID2NeighborList_(),
	vid2Neighbor_(),
	myVID_(*(vidCache.get(config.getMyNodeID()->getNodeName())))
{
	if (ScTraceBuffer::isEntryEnabled(tc_)) {
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "RoutingTable()");
		buffer->addProperty<bool>("fail-fast", failFast);
		buffer->addProperty("my-VID", myVID_.toString());
		buffer->invoke();
	}
}

RoutingTable::~RoutingTable()
{
	Trace_Entry(this, "~RoutingTable()");
}

void RoutingTable::addRoutingNeighbor(NodeIDImpl_SPtr id, Neighbor_SPtr neighbor)
{
	Trace_Entry(this, "addRoutingNeighbor()",
				"id", spdr::toString<NodeIDImpl>(id),
				"neighbor", neighbor->toString());

	{
		boost::mutex::scoped_lock lock(mutex_);

		NodeID2NeighborList_Map::iterator pos = nodeID2NeighborList_.find(id);
		if (pos != nodeID2NeighborList_.end())
		{
			//there is a neighbor on this id already
			if (!pos->second.empty())
			{
				for (NeighborList::const_iterator it = pos->second.begin(); it != pos->second.end(); ++it)
				{
					if (*(*it) == *neighbor)
					{
						String desc("Error: Neighbor already exists in NeighborList");
						ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "addRoutingNeighbor()", desc);
						buffer->invoke();
						addFailure(desc); //FIXME - this can happen!
					}
				}
				pos->second.push_back(neighbor);
				Trace_Event<int>(this, "addRoutingNeighbor()", "added to neighbor list",
						"list-size",pos->second.size());
			}
			else
			{
				//should never happen
				String desc("Error: Empty NeighborList");

				ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "addRoutingNeighbor()", desc);
				buffer->invoke();

				String what("RoutingTable::addRoutingNeighbor ");
				what.append(desc);
				throw SpiderCastRuntimeError(what);
			}
		}
		else
		{
			//new neighbor on this id
			NeighborList nl;
			nl.push_back(neighbor);
			nodeID2NeighborList_[id] = nl;
			Trace_Event(this, "addRoutingNeighbor()", "added new neighbor, new list");

			util::VirtualID_SPtr vid = vidCache_.get(id->getNodeName());
			util::VirtualID vid_diff(*vid);
			vid_diff.sub(myVID_);
			std::pair<VID2Neighbor_OrderedMap::iterator,bool> res = vid2Neighbor_.insert(
					std::make_pair(vid_diff,neighbor));
			if (res.second)
			{
				Trace_Event(this, "addRoutingNeighbor()", "added VID to neighbor mapping",
						"VID", vid->toString(),
						"VID-diff", vid_diff.toString());
			}
			else
			{
				//should never happen
				String desc("Failed to insert VID and new NeighborList");

				ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "addRoutingNeighbor()", desc);
				buffer->addProperty("VID", res.first->first.toString());
				buffer->addProperty("Neighbor", res.first->second->toString());
				buffer->invoke();

				String what("RoutingTable::addRoutingNeighbor ");
				what.append(desc);
				throw SpiderCastRuntimeError(what);
			}
		}
	}

	Trace_Exit(this, "addRoutingNeighbor()");
}

void RoutingTable::removeRoutingNeighbor(NodeIDImpl_SPtr id, Neighbor_SPtr neighbor)
{
	Trace_Entry(this, "removeRoutingNeighbor()",
			"id", spdr::toString<NodeIDImpl>(id),
			"neighbor", neighbor->toString());

	{
		boost::mutex::scoped_lock lock(mutex_);

		NodeID2NeighborList_Map::iterator id2nl_pos = nodeID2NeighborList_.find(id);
		if (id2nl_pos != nodeID2NeighborList_.end())
		{
			//there is a neighbor on this id already
			if (!id2nl_pos->second.empty())
			{
				bool found = false;
				bool last_neighbor = false;

				for (NeighborList::iterator it = id2nl_pos->second.begin();
						it != id2nl_pos->second.end(); ++it)
				{
					if (*(*it) == *neighbor)
					{

						found = true;
						id2nl_pos->second.erase(it);
						if (id2nl_pos->second.empty())
						{
							nodeID2NeighborList_.erase(id2nl_pos);
							last_neighbor = true;
						}

						Trace_Event<bool>(this,"removeRoutingNeighbor()","found ID","last",last_neighbor);

						break;
					}
				}

				if (found)
				{
					util::VirtualID_SPtr vid = vidCache_.get(id->getNodeName());
					util::VirtualID vid_diff(*vid);
					vid_diff.sub(myVID_);
					VID2Neighbor_OrderedMap::iterator vid2n_pos = vid2Neighbor_.find(vid_diff);
					if (vid2n_pos != vid2Neighbor_.end())
					{
						if (last_neighbor)
						{
							vid2Neighbor_.erase(vid2n_pos);
							Trace_Event(this,"removeRoutingNeighbor()","erased VID",
									"VID",vid->toString(),
									"VID-diff", vid_diff.toString());
						}
						else if (*(vid2n_pos->second) == *neighbor) //same neighbor
						{
							vid2n_pos->second = id2nl_pos->second.front(); //replace neighbor
							Trace_Event(this,"removeRoutingNeighbor()","replaced neighbor",
									"new neighbor",id2nl_pos->second.front()->toString());
						}
					}
					else
					{
						String desc("Error: Inconsistent VID2Neighbor map, cannot find VID");

						ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "removeRoutingNeighbor()", desc);
						buffer->addProperty("VID", vid->toString());
						buffer->addProperty("VID-diff", vid_diff.toString());
						buffer->invoke();

						removeFailure(desc);
					}
				}
				else
				{
					String desc("Error: Inconsistent ID to NeighborList map, cannot find neighbor in NeighborList");

					ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "removeRoutingNeighbor()", desc);
					buffer->invoke();

					removeFailure(desc);
				}
			}
			else
			{
				String desc("Error: Inconsistent ID to NeighborList map, NeighborList empty");

				ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "removeRoutingNeighbor()", desc);
				buffer->invoke();

				removeFailure(desc);
			}
		}
		else
		{
			String desc("Error: Inconsistent ID to NeighborList map, cannot find ID");

			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "removeRoutingNeighbor()", desc);
			buffer->invoke();

			removeFailure(desc);
		}
	}

	Trace_Exit(this, "removeRoutingNeighbor()");
}

void RoutingTable::removeFailure(const String& desc)
{

	if (failFast_)
	{
		String what("Error: RoutingTable::removeRoutingNeighbor ");
		what.append(desc);
		Trace_Error(this, "removeFailure()", desc);
		throw SpiderCastRuntimeError(what);
	}
	else
	{
		String what("Warning: RoutingTable::removeRoutingNeighbor ");
		what.append(desc);
		Trace_Event(this, "removeFailure()", desc);
	}
}

void RoutingTable::addFailure(const String& desc)
{
	if (failFast_)
	{
		String what("Error: RoutingTable::addRoutingNeighbor ");
		what.append(desc);
		Trace_Error(this, "addFailure()", what);
		throw SpiderCastRuntimeError(what);
	}
	else
	{
		String what("Warning: RoutingTable::addRoutingNeighbor ");
		what.append(desc);
		Trace_Event(this, "addFailure()", what);
	}
}


std::size_t RoutingTable::size() const
{
	if (nodeID2NeighborList_.size() != vid2Neighbor_.size())
	{
		throw SpiderCastRuntimeError("RoutingTable::size NodeID2NeighborList != VID2Neighbor");
	}

	return nodeID2NeighborList_.size();
}

	/**
	 * The number of neighbors in the table.
	 * @return
	 */
std::size_t RoutingTable::getNumNeighbors() const
{
	if (nodeID2NeighborList_.size() != vid2Neighbor_.size())
	{
		throw SpiderCastRuntimeError("RoutingTable::getNumNeighbors NodeID2NeighborList != VID2Neighbor");
	}

	std::size_t num=0;
	for (NodeID2NeighborList_Map::const_iterator it = nodeID2NeighborList_.begin();
			it!=nodeID2NeighborList_.end(); ++it)
	{
		num += it->second.size();
	}

	return num;
}

void RoutingTable::clear()
{
	vid2Neighbor_.clear();
	nodeID2NeighborList_.clear();
}

std::string RoutingTable::toString() const
{
	using namespace util;
	std::ostringstream oss;
	oss << "RoutingTable size=" << size()
			<< " #neig=" << getNumNeighbors()
			<< " my-VID=" << myVID_.toString() << std::endl;
	for (VID2Neighbor_OrderedMap::const_iterator it = vid2Neighbor_.begin();
			it != vid2Neighbor_.end(); ++it)
	{
		oss << it->second->getName() << "\t"
				<<  it->first.toString() << "\t"
				<< (add(it->first,myVID_)).toString() << std::endl;
	}

	return oss.str();
}


/*
 * @see RoutingTableLookup
 */
Neighbor_SPtr RoutingTable::nextHopP2P_BiDir(
		const util::VirtualID& targetVID)
{
	//TODO bidirectional unicast routing
	return Neighbor_SPtr();
}

/*
 * @see RoutingTableLookup
 */
Neighbor_SPtr RoutingTable::nextHopP2P_CW(
		const util::VirtualID& targetVID)
{
	// TODO CW unicast routing
	return Neighbor_SPtr();
}

/*
 * @see RoutingTableLookup
 */
Next2HopsBroadcast RoutingTable::next2Hops_Broadcast(
		const util::VirtualID& upperBoundVID)
{
	using namespace spdr::util;

	Trace_Entry(this, "next2Hops_Broadcast()", "upperBound",
			upperBoundVID.toString());

	Next2HopsBroadcast next2Hops;
	util::VirtualID ub_diff = sub(upperBoundVID,myVID_);
	util::VirtualID ub_diff_m1 = sub(ub_diff, util::VirtualID::OneValue);

	Trace_Entry(this, "next2Hops_Broadcast()",
			"upperBound", upperBoundVID.toString(),
			"upperBound-diff", ub_diff.toString());

	{
		boost::mutex::scoped_lock lock(mutex_);

		if (!vid2Neighbor_.empty())
		{
			VID2Neighbor_OrderedMap::const_iterator pos_succ = vid2Neighbor_.begin(); //Successor
			if (pos_succ->first <= ub_diff_m1)
			{
				next2Hops.firstHop = pos_succ->second; //Successor in range
				Trace_Debug(this, "next2Hops_Broadcast()", "found 1st hop",
						"1st-VID", pos_succ->first.toString(),
						"successor", pos_succ->second->getName());

				//find mid-range link
				util::VirtualID mid_ub_diff(ub_diff);
				if (ub_diff == util::VirtualID::MinValue)
				{
					mid_ub_diff = util::VirtualID::MiddleValue;
				}
				else
				{
					mid_ub_diff.shiftRight(1); //divide by 2
				}

				Trace_Debug(this, "next2Hops_Broadcast()", "",
						"mid-UB-diff", mid_ub_diff.toString());

				VID2Neighbor_OrderedMap::const_iterator pos_mid = vid2Neighbor_.lower_bound(
						mid_ub_diff); //first that is greater or equal

				if (pos_mid == vid2Neighbor_.end()) //the last link is not far enough
				{
					--pos_mid; //the last link
					Trace_Debug(this, "next2Hops_Broadcast()", "past last");
				}

				Trace_Debug(this, "next2Hops_Broadcast()", "found 2nd hop",
						"mid-VID", pos_mid->first.toString(),
						"mid", pos_mid->second->getName());

				if ( (pos_mid->first <= ub_diff_m1) && (pos_mid->first > pos_succ->first) )
				{
					//In range and does not equal successor
					next2Hops.firstHopUpperBound = add(pos_mid->first,myVID_); //Mid-range link VID
					next2Hops.secondHop = pos_mid->second; //Mid-range link
				}
				else
				{
					Trace_Debug(this, "next2Hops_Broadcast()", "mid-range lower then successor or not in range");
					next2Hops.firstHopUpperBound = upperBoundVID;
				}
			}
			else
			{
				Trace_Debug(this, "next2Hops_Broadcast()", "upper-bound lower then successor");
			}
		}
		else
		{
			Trace_Debug(this, "next2Hops_Broadcast()", "table empty");
		}
	}

	 if (ScTraceBuffer::isExitEnabled(tc_))
	 {
	        ScTraceBufferAPtr buffer = ScTraceBuffer::exit(this, "next2Hops_Broadcast()");
	        buffer->addProperty("next2hops", next2Hops.toString());
	        buffer->invoke();
	 }

	return next2Hops;
}

Next2HopsBroadcast RoutingTable::next2Hops_PubSub_CSH(
				const util::VirtualID& closestSubscriberVID,
				const util::VirtualID& upperBoundVID)
{
	using namespace spdr::util;

	Trace_Entry(this, "next2Hops_PubSub_CSH()",
			"closestSub", closestSubscriberVID.toString(),
			"upperBound", upperBoundVID.toString());

	if (closestSubscriberVID == myVID_)
	{
		throw SpiderCastRuntimeError("RoutingTable::next2Hops_PubSub_CSH closest subscriber cannot be me");
	}

	Next2HopsBroadcast next2Hops;

	const util::VirtualID cs_diff = sub(closestSubscriberVID,myVID_);

	const util::VirtualID ub_diff = sub(upperBoundVID,myVID_);
	const util::VirtualID ub_diff_m1 = sub(ub_diff, util::VirtualID::OneValue);

	if (cs_diff <= ub_diff_m1)
	{
		Trace_Debug(this, "next2Hops_PubSub_CSH()","closest subscriber in range");

		{
			boost::mutex::scoped_lock lock(mutex_);

			if (!vid2Neighbor_.empty())
			{
				//we seek the last that is lower or equal to the closest subscriber
				VID2Neighbor_OrderedMap::const_iterator pos1 = vid2Neighbor_.upper_bound(
						cs_diff); //first that is greater from closest-subscriber

				if (pos1 != vid2Neighbor_.end())
				{
					if (pos1 != vid2Neighbor_.begin())
					{
						--pos1;
					}
					else
					{
						Trace_Debug(this, "next2Hops_PubSub_CSH()",
								"closest subscriber between me and my successor, sending to successor, may mean message loss");
					}
				}
				else
				{
					--pos1;
				}

				next2Hops.firstHop = pos1->second;
				Trace_Debug(this, "next2Hops_PubSub_CSH()","found 1st-hop",
						"vid", pos1->first.toString(),
						"name", pos1->second->getName());

				//find mid-range link = 1st + (ub - 1st)/2 = 1st/2 +ub/2
				util::VirtualID mid_1st_ub_diff(pos1->first);
				mid_1st_ub_diff.shiftRight(1);
				util::VirtualID ub_div2_diff(ub_diff);
				if (ub_diff == util::VirtualID::MinValue)
				{
					ub_div2_diff = util::VirtualID::MiddleValue;
				}
				else
				{
					ub_div2_diff.shiftRight(1); //divide by 2
				}
				mid_1st_ub_diff.add(ub_div2_diff);

				Trace_Debug(this, "next2Hops_PubSub_CSH()","calculated mid-range",
						"vid", mid_1st_ub_diff.toString());

				VID2Neighbor_OrderedMap::const_iterator pos2 = vid2Neighbor_.lower_bound(
						mid_1st_ub_diff); //first that is greater or equal to mid-range

				if (pos2 == vid2Neighbor_.end()) //the last link is not far enough
				{
					--pos2; //the last link
					Trace_Debug(this, "next2Hops_Broadcast()", "past last");
				}

				Trace_Debug(this, "next2Hops_Broadcast()", "found 2nd hop candidate",
						"mid-VID", pos2->first.toString(),
						"mid", pos2->second->getName());

				if ( (pos2->first <= ub_diff_m1) && (pos2->first > pos1->first) )
				{
					//In range and is greater than 1st-hop
					next2Hops.firstHopUpperBound = add(pos2->first,myVID_); //Mid-range link VID
					next2Hops.secondHop = pos2->second; //Mid-range link
				}
				else
				{
					Trace_Debug(this, "next2Hops_Broadcast()", "mid-range lower/equal than 1st-hop or not in range");
					next2Hops.firstHopUpperBound = upperBoundVID;
				}
			}
			else
			{
				Trace_Debug(this, "next2Hops_PubSub_CSH()","empty");
			}
		} //lock
	}
	else
	{
		Trace_Debug(this, "next2Hops_PubSub_CSH()","closest subscriber beyond upper-bound, no forwarding");
	}

	if (ScTraceBuffer::isExitEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::exit(this, "next2Hops_PubSub_CSH()");
		buffer->addProperty("next2hops", next2Hops.toString());
		buffer->addProperty("upperBound", upperBoundVID.toString());
		buffer->invoke();
	}

	return next2Hops;

}

}

}
