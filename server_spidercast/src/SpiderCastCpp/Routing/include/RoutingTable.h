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
 * RoutingTable.h
 *
 *  Created on: Jan 31, 2012
 */

#ifndef ROUTINGTABLE_H_
#define ROUTINGTABLE_H_

#include <map>
#include <list>

#include <boost/unordered_map.hpp>

#include "RoutingTableLookup.h"
#include "SpiderCastConfigImpl.h"
#include "VirtualIDCache.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace route
{

/**
 * Thread safe.
 * Nodes added by the MemTopo thread, lookup by the Routing thread.
 */
class RoutingTable : public RoutingTableLookup, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	RoutingTable(
			const String& instID,
			const SpiderCastConfigImpl& config,
			VirtualIDCache& vidCache,
			bool failFast = true);

	virtual ~RoutingTable();

	/*
	 * Called from topology.
	 *
	 * @param id
	 * @param neighbor
	 */
	void addRoutingNeighbor(NodeIDImpl_SPtr id, Neighbor_SPtr neighbor);

	/*
	 * Called from topology.
	 *
	 * @param id
	 * @param neighbor
	 */
	void removeRoutingNeighbor(NodeIDImpl_SPtr id, Neighbor_SPtr neighbor);

	/**
	 * The number of NodeIDs in the table.
	 * @return
	 */
	std::size_t size() const;

	/**
	 * The number of neighbors in the table.
	 * @return
	 */
	std::size_t getNumNeighbors() const;

	void clear();

	/**
	 * A string representation.
	 *
	 * @return
	 */
	std::string toString() const;

	//===============================================================
	/*
	 * @see RoutingTableLookup
	 */
	Neighbor_SPtr nextHopP2P_BiDir(const util::VirtualID& targetVID);

	/*
	 * @see RoutingTableLookup
	 */
	Neighbor_SPtr nextHopP2P_CW(const util::VirtualID& targetVID);

	/*
	 * @see RoutingTableLookup
	 */
	Next2HopsBroadcast next2Hops_Broadcast(
			const util::VirtualID& upperBoundVID);

	/*
	 * @see RoutingTableLookup
	 */
	Next2HopsBroadcast next2Hops_PubSub_CSH(
				const util::VirtualID& closestSubscriberVID,
				const util::VirtualID& upperBoundVID);

private:
	const String& instID_;
	const SpiderCastConfigImpl& config_;
	VirtualIDCache& vidCache_;
	const bool failFast_;

	boost::mutex mutex_;

	typedef std::list<Neighbor_SPtr> NeighborList;
	typedef boost::unordered_map<NodeIDImpl_SPtr, NeighborList,
			spdr::SPtr_Hash<NodeIDImpl>, spdr::SPtr_Equals<NodeIDImpl> > NodeID2NeighborList_Map;
	NodeID2NeighborList_Map nodeID2NeighborList_;

	typedef std::map<util::VirtualID, Neighbor_SPtr> VID2Neighbor_OrderedMap;
	/**
	 * An  ordered map, by VID, the keys are (the actual VID - myVID),
	 * that is, the offset from my VID along the ring, CW.
	 */
	VID2Neighbor_OrderedMap vid2Neighbor_;

	util::VirtualID myVID_;

	void removeFailure(const String& description);
	void addFailure(const String& description);

};

}

}

#endif /* ROUTINGTABLE_H_ */
