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
 * NeighborTable.h
 *
 *  Created on: 15/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.5 $
 *-----------------------------------------------------------------
 * $Log: NeighborTable.h,v $
 * Revision 1.5  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.4.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.4  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.3  2015/10/09 06:56:11 
 * bug fix - when neighbor in the table but newNeighbor not invoked yet, don't send to all neighbors but only to routable neighbors. this way the node does not get a diff mem/metadata before the full view is received.
 *
 * Revision 1.2  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:18 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.21  2012/10/25 10:11:09 
 * Added copyright and proprietary notice
 *
 * Revision 1.20  2012/02/19 09:07:11 
 * Add a "routbale" field to the neighbor tables
 *
 * Revision 1.19  2011/08/07 12:13:48 
 * Obtain neighbors names
 *
 * Revision 1.18  2011/02/20 12:01:01 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.17  2011/01/09 09:12:58 
 * Hierarchy branch merged in to HEAD
 *
 * Revision 1.16  2010/11/29 14:10:07 
 * change std::map to boost::unordered_map
 *
 * Revision 1.15  2010/10/06 10:29:30 
 * Bug fix - (1) send to all when 0 neighbors undetected, (2) increase version when discovered
 *
 * Revision 1.14  2010/08/18 18:54:24 
 * renamed method sent -> send
 *
 * Revision 1.13  2010/08/16 13:21:04 
 * *** empty log message ***
 *
 * Revision 1.12  2010/08/16 11:24:34 
 * *** empty log message ***
 *
 * Revision 1.11  2010/08/10 09:52:30 
 * Add toString()
 *
 * Revision 1.10  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.9  2010/07/01 11:55:47 
 * Rename NeighborTable::exists() to contains()
 *
 * Revision 1.8  2010/06/08 10:31:17 
 * Add a size() method
 *
 * Revision 1.7  2010/06/06 12:45:10 
 * Add table name; no functional chage
 *
 * Revision 1.6  2010/06/03 14:43:22 
 * Arrange print statmenets
 *
 * Revision 1.5  2010/05/30 15:33:06 
 * Add NodeIdImpl::Less to map declaration
 *
 * Revision 1.4  2010/05/27 09:35:01 
 * Topology initial implementation
 *
 * Revision 1.2  2010/05/20 09:35:16 
 * Initial version
 *
 */

#ifndef NEIGHBORTABLE_H_
#define NEIGHBORTABLE_H_

//#include <map>
#include <boost/unordered_map.hpp>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/noncopyable.hpp>

#include "Neighbor.h"
//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"
#include "SpiderCastLogicError.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class NeighborTable;
typedef boost::shared_ptr<NeighborTable> NeighborTable_SPtr;

class NeighborTable: boost::noncopyable, public ScTraceContext
{
public:
	mutable boost::recursive_mutex _mutex;

	struct Value
	{
		Value();
		Value(Neighbor_SPtr neighbor, bool routable);

		Neighbor_SPtr neighbor;
		bool routable;
	};
	// <Neighbor, isRoutable>
	typedef boost::tuple<Neighbor_SPtr, bool> entryState; //TODO a pair or a struct

//	typedef boost::unordered_map<NodeIDImpl_SPtr, entryState,
//			NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals > NeighborTableMap;
	typedef boost::unordered_map<NodeIDImpl_SPtr, Value,
			NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals > NeighborTableMap;
	NeighborTableMap _table;

	NeighborTable(String myName, String tableName, const String& instID);
	virtual ~NeighborTable();

	bool addEntry(NodeIDImpl_SPtr targetName, Neighbor_SPtr neighbor);
	Neighbor_SPtr getNeighbor(NodeIDImpl_SPtr targetName);
	bool removeEntry(NodeIDImpl_SPtr targetName);
	bool contains(NodeIDImpl_SPtr targetName);
	int size();

	bool sendToNeighbor(NodeIDImpl_SPtr target, SCMessage_SPtr msg);

	/**
	 * Send a message to all neighbors.
	 * @param msg
	 * @return a pair of integers where: first=#successes, second=#neighbors.
	 */
	std::pair<int,int> sendToAllNeighbors(SCMessage_SPtr msg);

	/**
	 * Send a message to all routable neighbors.
	 * @param msg
	 * @return a pair of integers where: first=#successes, second=#routable-neighbors.
	 */
	std::pair<int,int> sendToAllRoutableNeighbors(SCMessage_SPtr msg);


	virtual String toString() const;
	String toStringDump() const;

	void setRoutable(NodeIDImpl_SPtr targetName);
	bool getRoutable(NodeIDImpl_SPtr targetName);

	String getNeighborsStr();

private:
	static ScTraceComponent* _tc;

	String _myNodeName;
	String _tableName;

	const String _instID;
};

}

#endif /* NEIGHBORTABLE_H_ */
