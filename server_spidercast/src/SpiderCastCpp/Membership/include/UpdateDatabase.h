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
 * UpdateDatabase.h
 *
 *  Created on: 16/05/2010
 */

#ifndef UPDATEDATABASE_H_
#define UPDATEDATABASE_H_

//#include <map>
#include <set>

#include <boost/unordered_map.hpp>
//#include <boost/unordered_set.hpp>

//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"
#include "NodeVersion.h"
#include "Suspicion.h"
#include "SCMessage.h"
#include "SpiderCastConfigImpl.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

class SpiderCastConfigImpl;

namespace spdr
{

class UpdateDatabase : public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	UpdateDatabase(const String& instID, const String& nodeName, const SpiderCastConfigImpl& config);
	virtual ~UpdateDatabase();

	void clear();

	/**
	 * Add to the left map. If exist, update if the version parameter is higher.
	 *
	 * @param nodeName
	 * @param ver
	 * @return whether the map was modified, in size or value.
	 */
	bool addToLeft(const String& nodeName, NodeVersion ver, spdr::event::NodeStatus status);

	/**
	 * Add to the alive map. If exist, update if the version parameter is higher.
	 *
	 * @param id
	 * @param ver
	 * @return whether the map was modified, in size or value.
	 */
	bool addToAlive(NodeIDImpl_SPtr id, NodeVersion ver);

	/**
	 * Add to the retained map. If exist, update if the version parameter is higher.
	 *
	 * @param id
	 * @param ver
	 * @return whether the map was modified, in size or value.
	 */
	bool addToRetained(NodeIDImpl_SPtr id, NodeVersion ver, spdr::event::NodeStatus status);

	/**
	 * Add to the suspicion set. If and entry with the same reporter and suspect exist,
	 * update if the version parameter is higher.
	 *
	 * @param reporter
	 * @param suspect
	 * @param suspectVer
	 * @return
	 */
	bool addToSuspect(StringSPtr reporter, StringSPtr suspect,
			NodeVersion suspectVer);

	/**
	 * Write the update database to a message.
	 * updates total length.
	 *
	 * @param msg
	 */
	void writeToMessage(SCMessage_SPtr msg);

	bool empty();

private:

	const bool crcEnabled_;

	typedef boost::unordered_map<String, std::pair<NodeVersion,spdr::event::NodeStatus> >	NodeName2VersionStatusMap;

	/*
	 * The set of leave messages the current node heard about in the last gossip round,
	 * together with their NodeVersion.
	 * Keep the leave message with the highest version number on each node.
	 *
	 * Threading: MemTopo-thread only
	 */
	NodeName2VersionStatusMap leftMap;

	typedef boost::unordered_map<NodeIDImpl_SPtr, NodeVersion,
			NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals >	NodeID2VersionMap;
	/*
	 * The set of alive nodes the current node heard about in the last gossip round,
	 * together with their NodeVersion.
	 * Keep the alive message with the highest version number on each node.
	 *
	 * Threading: MemTopo-thread only
	 */
	NodeID2VersionMap aliveMap;

	typedef boost::unordered_map<NodeIDImpl_SPtr, std::pair<NodeVersion,spdr::event::NodeStatus>,
				NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals >	NodeID2VersionStatusMap;
	/*
	 * The set of retained nodes the current node heard about in the last gossip round,
	 * together with their NodeVersion and NodeStatus.
	 * Keep the retained message with the highest version number on each node.
	 *
	 * Threading: MemTopo-thread only
	 */
	NodeID2VersionStatusMap retainedMap;

	typedef std::set<Suspicion> SuspicionSet; //TODO convert to unordered_set
	/**
	 * The set of suspicions the current node heard about in the last gossip round.
	 *
	 * Threading: MemTopo-thread only
	 */
	SuspicionSet suspicionSet;



};

}

#endif /* UPDATEDATABASE_H_ */
