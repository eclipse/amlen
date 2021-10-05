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
 * OutgoingStructuredNeighborTable.h
 *
 *  Created on: Feb 1, 2012
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: OutgoingStructuredNeighborTable.h,v $
 * Revision 1.3  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:16 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:18 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2012/02/19 09:07:11 
 * Add a "routbale" field to the neighbor tables
 *
 * Revision 1.1  2012/02/14 12:45:18 
 * Add structured topology
 *
 */

#ifndef OUTGOINGSTRUCTUREDNEIGHBORTABLE_H_
#define OUTGOINGSTRUCTUREDNEIGHBORTABLE_H_

#include <boost/unordered_map.hpp>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/noncopyable.hpp>

#include "Neighbor.h"
//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"
#include "SpiderCastLogicError.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"
#include "ScTraceContext.h"

namespace spdr
{

class OutgoingStructuredNeighborTable: boost::noncopyable, public ScTraceContext
{

public:
	boost::recursive_mutex _mutex;

	typedef boost::tuple<Neighbor_SPtr, int, bool> curViewSize;

	typedef boost::unordered_map<NodeIDImpl_SPtr, curViewSize,
			NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals > OutgoingStructuredNeighborTableMap;
	OutgoingStructuredNeighborTableMap _table;

	OutgoingStructuredNeighborTable(String myName, String tableName, const String& instID);
	virtual ~OutgoingStructuredNeighborTable();

	bool addEntry(NodeIDImpl_SPtr targetName, Neighbor_SPtr neighbor, int viewSize);
	Neighbor_SPtr getNeighbor(NodeIDImpl_SPtr targetName);
	int getViewSize(NodeIDImpl_SPtr targetName);
	bool removeEntry(NodeIDImpl_SPtr targetName);
	bool contains(NodeIDImpl_SPtr targetName);
	int size();

	bool sendToNeighbor(NodeIDImpl_SPtr target, SCMessage_SPtr msg);
	bool refreshNeeded(int viewSize);
	std::vector<NodeIDImpl_SPtr> structuredLinksToRefresh(int viewSize);

	void setRoutable(NodeIDImpl_SPtr targetName);
	bool getRoutable(NodeIDImpl_SPtr targetName);

	virtual String toString();

private:
	static ScTraceComponent* _tc;

	String _myNodeName;
	String _tableName;

	const String _instID;

};

}

#endif /* OUTGOINGSTRUCTUREDNEIGHBORTABLE_H_ */
