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
 * RoutingTableLookup.h
 *
 *  Created on: Jan 31, 2012
 */

#ifndef ROUTINGTABLELOOKUP_H_
#define ROUTINGTABLELOOKUP_H_

#include <boost/tuple/tuple.hpp>

#include "NodeIDImpl.h"
#include "Neighbor.h"
#include "VirtualID.h"
#include "Next2HopsBroadcast.h"

namespace spdr
{

namespace route
{

/**
 * The interface for routing table lookup.
 */
class RoutingTableLookup
{
public:
	RoutingTableLookup();
	virtual ~RoutingTableLookup();

	/**
	 * The next two hops of a range based broadcast.
	 *
	 * Route to the range  [my-VID,upperBound).
	 *
	 * Successor-neighbor, Mid-range-VID, Mid-range-neighbor
	 *
	 * @param upperBoundVID
	 * @return
	 */
	virtual Next2HopsBroadcast next2Hops_Broadcast(const util::VirtualID& upperBoundVID) = 0;

	/**
	 * The next two pub-sub hops with closest-subscriber-heuristc (CSH).
	 *
	 *  @param closestSubscriberVID
	 * @param upperBoundVID
	 * @return
	 */
	virtual Next2HopsBroadcast next2Hops_PubSub_CSH(
			const util::VirtualID& closestSubscriberVID,
			const util::VirtualID& upperBoundVID) = 0;


	virtual Neighbor_SPtr nextHopP2P_BiDir(const util::VirtualID& targetVID) = 0;
	virtual Neighbor_SPtr nextHopP2P_CW(const util::VirtualID& targetVID) = 0;
};

}

}

#endif /* ROUTINGTABLELOOKUP_H_ */
