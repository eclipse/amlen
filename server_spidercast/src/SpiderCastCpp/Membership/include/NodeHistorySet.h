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
 * NodeHistorySet.h
 *
 *  Created on: 18/04/2010
 */

#ifndef NODEHISTORYSET_H_
#define NODEHISTORYSET_H_

#include <map>
//#include <boost/unordered_map.hpp>
#include <cstdlib>
#include <ctime>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/tuple/tuple.hpp>

//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"
#include "NodeVersion.h"
#include "NodeInfo.h"
#include "MembershipDefinitions.h"

namespace spdr
{

class NodeHistorySet
{
public:
	NodeHistorySet();
	virtual ~NodeHistorySet();

	/**
	 * Get a (shallow) copy of the NodeInfo.
	 *
	 * @param node
	 * @return
	 */
	std::pair<NodeInfo,bool> getNodeInfo(NodeIDImpl_SPtr node) const;

	/**
	 * Remove a node from history set.
	 *
	 * @param node
	 * @return whether the node was in the set (true if action changed the set).
	 */
	bool remove(NodeIDImpl_SPtr node);

	/**
	 * Add a node to the set, with NodeInfo.
	 *
	 * If the node was already in the set, the old entry is over written only if the version is higher.
	 *
	 * @param node
	 * @param ver
	 * @param time
	 * @return whether the node was not already in the set
	 */
	bool add(NodeIDImpl_SPtr node, const NodeInfo& info);

	/**
	 * Update the version, time and status.
	 *
	 * Update the time and version only if the node exists and supplied version is higher.
	 *
	 * If the supplied version is equal or higher and the status is STATUS_REMOVED, the retained attributes (if any) are removed.
	 *
	 * Invoked following a Leave message to make sure we have the most updated info on the node.
	 *
	 * @param node
	 * @param ver
	 * @param status
	 * @param time
	 * @return true if updated (either version or status).
	 */
	std::pair<bool,bool> updateVer(NodeIDImpl_SPtr node, NodeVersion ver, spdr::event::NodeStatus status, boost::posix_time::ptime time);

	/**
	 * Get the next node from the set.
	 * A cyclic iteration of the set.
	 *
	 * @return a node, or null if set is empty.
	 */
	NodeIDImpl_SPtr getNextNode();

	int size() const;

	void clear();

	/**
	 * Removes nodes that if do NOT retain attributes, if their entry time is older than retention threshold.
	 *
	 * @param retention_threshold
	 * @return number of nodes removed.
	 */
	int prune(boost::posix_time::ptime retention_threshold);

	/**
	 * Whether history contains a node with version greater or equal of the parameter.
	 *
	 * If true, this means that the parameter node is dead.
	 *
	 * @param node
	 * @param ver
	 * @return
	 */
	bool containsVerGreaterEqual(NodeIDImpl_SPtr node, NodeVersion ver) const;

	bool contains(NodeIDImpl_SPtr node) const;

	bool forceRemoveRetained(NodeIDImpl_SPtr node, int64_t incarnation);

	NodeHistoryMap& getNodeHistoryMap()
	{
		return historyMap_;
	}

private:
	typedef NodeIDImpl_SPtr NodeHistoryMap_Key;
	//typedef boost::tuples::tuple<NodeVersion, boost::posix_time::ptime >  NodeHistoryMap_Value;
	typedef NodeInfo  NodeHistoryMap_Value;

	//typedef std::map< NodeIDImpl_SPtr, NodeInfo, NodeIDImpl::SPtr_Less > NodeHistoryMap;
	//typedef boost::unordered_map< NodeIDImpl_SPtr, NodeInfo, NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals > NodeHistoryMap;
	NodeHistoryMap historyMap_;

	bool validIterator_;
	NodeHistoryMap::const_iterator iterator_;

};

}

#endif /* NODEHISTORYSET_H_ */
