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
 * TopologyManager.h
 *
 *  Created on: 15/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: TopologyManager.h,v $
 * Revision 1.4  2015/11/18 08:37:00 
 * Update copyright headers
 *
 * Revision 1.3  2015/08/06 06:59:16 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.2  2015/08/04 12:19:11 
 * split brain bug fix
 *
 * Revision 1.1  2015/03/22 12:29:18 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.24  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.23  2012/09/30 12:47:57 
 * Delete commented code; no functional changes
 *
 * Revision 1.22  2012/02/14 12:45:18 
 * Add structured topology
 *
 * Revision 1.21  2011/05/01 08:38:07 
 * Move connection context from TopologyManager to Definitions.h
 *
 * Revision 1.20  2011/03/03 11:24:56 
 * Add a predecessor to setSuccessor()
 *
 * Revision 1.19  2011/02/20 12:01:01 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.18  2011/01/09 14:03:15 
 * Added HierarchyCtx to connection-context
 *
 * Revision 1.17  2010/12/29 13:20:42 
 * Add connect() context types
 *
 * Revision 1.16  2010/12/14 09:56:49 
 * Remove periodic task
 *
 * Revision 1.15  2010/12/01 18:53:08 
 * Add discovery reply send task
 *
 * Revision 1.14  2010/10/12 18:13:49 
 * Support for hierarchy
 *
 * Revision 1.13  2010/08/26 18:59:35 
 * Make "random disconnect" and "update degree" seperate and independent tasks. Eliminat etopology periodic task
 *
 * Revision 1.12  2010/08/26 11:15:57 
 * Make "random connect" a seperate and independent task
 *
 * Revision 1.11  2010/08/26 08:34:03 
 * Make "change successor" a seperate and independent task
 *
 * Revision 1.10  2010/08/18 10:14:37 
 * Discovery -  runs as independent task, runs in two modes - normal and frequent discovery.
 *
 * Revision 1.9  2010/06/20 14:42:42 
 * Handle termination better. Add grace parameter
 *
 * Revision 1.8  2010/06/14 15:54:43 
 * Add stopFrequentDiscovery task
 *
 * Revision 1.7  2010/05/27 09:35:00 
 * Topology initial implementation
 *
 * */

#ifndef TOPOLOGYMANAGER_H_
#define TOPOLOGYMANAGER_H_

//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"
#include "NeighborTable.h"

namespace spdr
{

class TopologyManager;
typedef boost::shared_ptr<TopologyManager> TopologyManager_SPtr;

class TopologyManager
{
public:
	TopologyManager();
	virtual ~TopologyManager();

	virtual void start() = 0;

	virtual void terminate(bool soft) = 0;

	/**
	 * Called by the MembershipManager when the successor/predecessor changes.
	 *
	 * TODO make successor list
	 *
	 * @param successor
	 * @param predecessor
	 */
	virtual void setSuccessor(NodeIDImpl_SPtr successor,
			NodeIDImpl_SPtr predecessor) = 0;

	/**
	 * Called by the MembershipManager when it starts, to gain a pointer to the neighbor table.
	 * This table contains the random neighbors, the successor, but not discovery peers.
	 * The table is read/write by the TopologyManager, and read by the MembershipManager.
	 *
	 * @return
	 */
	virtual NeighborTable_SPtr getNeighborTable() = 0;

	virtual void discoveryTask() = 0;

	virtual void stopFrequentDiscoveryTask() = 0;

	virtual void terminationTask() = 0;

	virtual void changeSuccessorTask() = 0;

	virtual void randomTopologyConnectTask() = 0;

	virtual void randomTopologyDisconnectTask() = 0;

	virtual void structuredTopologyConnectTask() = 0;

	virtual void structuredTopologyRefreshTask() = 0;

	virtual void changedDegreeTask() = 0;

	virtual void discoveryReplySendTask() = 0;

	virtual void multicastDiscoveryReplySendTask() = 0;

	/** Processes an incoming topology message.
	 *
	 * Topology messages and hierarchy messages go through here.
	 */
	virtual void processIncomingTopologyMessage(SCMessage_SPtr topologyMsg) = 0;

	enum State
	{
		/**
		 * After creation, before start.
		 */
		Init,

		/**
		 * After start, discovery mode.
		 */
		Discovery,

		/**
		 * Normal operation.
		 */
		Normal,

		/**
		 * Closed.
		 */
		Closed
	};
	static const String nodeStateName[];

};

}

#endif /* TOPOLOGYMANAGER_H_ */
