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
 * LeaderElectionService.h
 *
 *  Created on: Jun 13, 2012
 */

#ifndef LEADERELECTIONSERVICE_H_
#define LEADERELECTIONSERVICE_H_

#include "NodeID.h"

namespace spdr
{

/**
 * A name space for the leader election service API.
 */
namespace leader_election
{

/**
 * The internal key used for the leader election implementation over the attribute service.
 */
const String Election_Attribute_KEY = ".election";

/**
 * A service that allows a group of nodes to elect a leader.
 *
 * When this service is created, the node joins the leader election process as
 * either a candidate or an observer. A candidate may be elected leader, an
 * observer only observes the results of the elections.
 *
 * The service is created using the factory method SpiderCast#createLeaderElectionService().
 * Only a single instance of the service can be created per SpiderCast node.
 *
 * The results of the elections are notified via the
 * LeaderElectionListener#onLeaderChange() interface, or can be polled using the
 * getLeader() method. The results of an election are a (shared pointer to) NodeID, or NULL when there is no
 * leader.
 *
 * The leader election process is based on every candidate maintaining a view of all other candidates.
 * The candidate with the lowest NodeID proposes itself as leader in case no leader exists. If more
 * then one node proposes itself as leader, the nodes with higher NodeIDs will back-off.
 * The leader election process is eventually consistent (sa the candidate view).
 * That is, on rare occasions there may be transient periods with more than one leader proposing
 * itself, and different nodes may transiently see different leaders. However, after the view
 * stabilizes, it is guaranteed that all nodes see the same leader.
 *
 * In order to change from candidate mode to observer mode or vice-versa, the service needs
 * to be closed and reopened in the respective mode.
 *
 * @see LeaderElectionListener
 *
 */
class LeaderElectionService
{
public:
	virtual ~LeaderElectionService();

	/**
	 * Get the current leader.
	 *
	 * If there is no leader, will return NULL.
	 *
	 * @return the current leader or NULL.
	 */
	virtual NodeID_SPtr getLeader() const = 0;

	/**
	 * Is the local node the leader.
	 *
	 * This is always false if the local node is an observer (not a candidate).
	 *
	 * @return true if the local node is the leader.
	 */
	virtual bool isLeader() const = 0;

	/**
	 * Is the local node a candidate.
	 *
	 * @return true if the local node is a candidate in the elections.
	 */
	virtual bool isCandidate() const = 0;

	/**
	 * Closes the service.
	 *
	 * When a node closes the service it will no longer be a candidate leader,
	 * nor observe election results. If the node was a leader, it will relinquish
	 * its leadership and the remaining candidates will elect a new leader.
	 */
	virtual void close() = 0;

	virtual bool isClosed() const = 0;

protected:
	LeaderElectionService();
};

typedef boost::shared_ptr<LeaderElectionService> LeaderElectionService_SPtr;

}

}

#endif /* LEADERELECTIONSERVICE_H_ */
