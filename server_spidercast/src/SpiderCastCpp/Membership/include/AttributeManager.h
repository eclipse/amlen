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
 * AttributeManager.h
 *
 *  Created on: 10/06/2010
 */

#ifndef ATTRIBUTEMANAGER_H_
#define ATTRIBUTEMANAGER_H_

#include <boost/noncopyable.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "Definitions.h"
#include "MembershipDefinitions.h"
#include "SCMessage.h"
#include "MembershipService.h"
#include "AttributeTable.h"
#include "NodeIDCache.h"
#include "MembershipEvent.h"
#include "SCMembershipEvent.h"
#include "MetaData.h"
#include "AttributeControl.h"
#include "CoreInterface.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class AttributeManager: boost::noncopyable, public AttributeControl, public ScTraceContext
{
public:
	static const char internalKeyPrefix = '.';

private:
	static ScTraceComponent* tc_;

	const String& instID_;

	const SpiderCastConfigImpl& config_;

	/*
	 * MemTopoThread only.
	 */
	const MembershipViewMap& viewMap_;

	const NodeHistoryMap& historyMap_;

	/*
	 * Thread safe.
	 */
	NodeIDImpl_SPtr myID_;

	/*
	 * MemTopoThread only.
	 */
	const NodeVersion& myVersion_;

	/*
	 * Thread safe.
	 */
	NodeIDCache& nodeIDCache_;

	CoreInterface& coreInterface_;

	boost::recursive_mutex mutex_;

	/*
	 * MemTopoThread, App.-thread
	 */
	AttributeTable myAttributeTable_;

	bool notifyTaskScheduled;

	const bool crcEnabled_;

public:
	AttributeManager(
			const String& instID,
			const SpiderCastConfigImpl& config,
			MembershipViewMap& viewMap,
			NodeHistoryMap& historyMap,
			NodeIDImpl_SPtr myID,
			NodeVersion& myVersion,
			NodeIDCache& nodeIDCache,
			CoreInterface& coreInterface);

	virtual ~AttributeManager();

	/**
	 * Test and set the notify-task-scheduled flag.
	 *
	 * Will test the notify-task-scheduled state,
	 * set it to true if it was false,
	 * and return true in case it was changed from false to true.
	 *
	 * The flag is reset in the prepareDifferentialNotificationExternal() method, which is run by the notify-task.
	 *
	 *  Threading: App-thread, MemTopo-thread
	 *
	 * @return true if a task needs to be scheduled, false if a task is pending;
	 * 				i.e. returns true for a false->true transition in the flag.
	 */
	bool testAndSetNotifyTaskScheduled();

	/**
	 * Reset the notify-task-scheduled flag.
	 */
	void resetNotifyTaskScheduled();

	/**
	 * Prepare a differential notification, mark last-notified on all tables,
	 *
	 * reset the notify-task-scheduled flag.
	 *
	 */
	std::pair<SCViewMap_SPtr, event::ViewMap_SPtr >  prepareDifferentialNotification(
			bool internal, bool external);

	/**
	 * Create at least one full view and a differential update, such that the two consumers are in synch.
	 *
	 * @param intView
	 * @param intDiff
	 * @param extView
	 * @param extDiff
	 * @return
	 */
	std::pair<SCViewMap_SPtr, event::ViewMap_SPtr >  prepareFullViewNotification(
				bool intView, bool intDiff, bool extView, bool extDiff);

//	/**
//	 * Prepare an internal differential notification, mark last-notified on all tables,
//	 *
//	 * reset the notify-task-scheduled flag.
//	 *
//	 */
//	SCViewMap_SPtr prepareDifferentialNotificationInternal();
//
//	/**
//	 * Prepare a external differential notification, mark last-notified on all tables,
//	 *
//	 * reset the notify-task-scheduled flag,
//	 */
//	event::ViewMap_SPtr prepareDifferentialNotificationExternal();

//	/**
//	 * Convert view.
//	 *
//	 * @param
//	 * @return
//	 */
//	event::ViewMap_SPtr convertView(SCViewMap_SPtr);

	/**
	 * Get a copy of my attribute table in the format ready for notification.
	 * Mark last notify (internal/external).
	 *
	 * Called during during the preparation of an external view-change-event.
	 *
	 * Reset the notify-task-scheduled flag.
	 *
	 * Thread safe.
	 *
	 * @param markInternal Mark last notify internal or external.
	 * @return a copy of the map, or null, if it is empty.
	 */
	event::AttributeMap_SPtr getMyNotifyAttributeMap(bool markInternal);


	/**
	 * Is a differential update message needed?
	 * That is, is the (current-version > sent-version) on any of the tables?
	 */
	bool isUpdateNeeded();

	/**
	 * Prepare a full update message (for a new neighbor).
	 * Does not alter the version sent.
	 * Skips tables with version 0.
	 *
	 * @return true if the message needs to be sent, i.e. some tables are with version > 0.
	 */
	bool prepareFullUpdateMsg(SCMessage_SPtr msg);

	/**
	 * Prepare a differential update message (for an old neighbor).
	 *
	 * @param msg outgoing message
	 *
	 * @return myAttributeTable version at the time of Diff calculation
	 */
	uint64_t prepareDiffUpdateMsg(SCMessage_SPtr msg);

	/**
	 * Reset the version sent on my attribute table.
	 * Induces my attribute table version to be included in the next metadata update message.
	 */
	void resetMyVersionSent();

	/**
	 * Write the rebuttal key on my attribute table.
	 * Induces my attribute table version to be included in the next metadata update message.
	 */
	void writeMyRebuttalKey();

	/**
	 * Marks the version sent on each attribute table.
	 *
	 * The version on my table can change between the time the message was prepared, and the
	 * time we mark the version sent (by the App. thread). This cannot happen with other tables.
	 *
	 * @param my_table_version_sent
	 * 		the version of my table at the time the message was prepared
	 */
	void markVersionSent(uint64_t my_table_version_sent);

	/**
	 * Process the digest items in the incoming update and produce a request, if needed.
	 *
	 * Expected update format:
	 * #digest_items	(int32_t)
	 * DigestItem		(x #digest_items)
	 * - name			(String)
	 * - node-version	(2x int64_t)
	 * - table-version	(uint64_t)
	 *
	 * @param inUpdate incoming update message
	 * @param outRequest outgoing request message to fill, if needed
	 * @return whether an outgoing request was generated
	 */
	bool processIncomingUpdateMessage(SCMessage_SPtr inUpdate,
			SCMessage_SPtr outRequest);

	/**
	 * Process the digest items in the incoming request and produce a reply, if needed.
	 *
	 * Expected request format:
	 * #digest_items	(int32_t)
	 * DigestItem		(x #digest_items)
	 * - name			(String)
	 * - node-version	(2x int64_t)
	 * - table-version	(uint64_t)
	 *
	 * If it is a push request the reply will no include request invalidations.
	 *
	 * @param inRequest incoming request message
	 * @param outReply outgoing reply message to fill, if needed
	 * @param pushRequest if it is a push request or a response to an update (normal case)
	 *
	 * @return whether an outgoing reply was generated
	 */
	bool processIncomingRequestMessage(SCMessage_SPtr inRequest,
			SCMessage_SPtr outReply, bool pushRequest = false);

	/**
	 * Process the table data items in the incoming reply.
	 *
	 * Expected reply format:
	 * #tables				(int32_t)
	 * TableDelta			(x #tables)
	 * - name				(String)
	 * - node-version		(2x int64_t)
	 * - #table_entries		(int32_t) if (0/-1) then invalidate request
	 * - TableEntry			(x #table_entries)
	 * -- key				(String)
	 * -- version			(uint64_t)
	 * -- length			(int32_t)
	 * -- value				(byte array, of size 'length', if length>0)
	 *
	 * @param reply incoming reply message
	 * @param request_invalidations the IDs nodes that invalidated their request
	 *
	 * @return first: whether a notification to the App. is needed, second: request invalidations received.
	 */
	std::pair<bool,bool> processIncomingReplyMessage(SCMessage_SPtr inReply, SCMessage_SPtr outRequest);

	/**
	 * Go over tables and retire pending requests from the disconnected neighbor,
	 * and generate an outgoing (repeated) request that goes to all the neighbors.
	 *
	 * @param id disconnected neighbor
	 * @param outRequest a repeated outgoing request
	 * @return whether a message needs to be sent
	 */
	bool undoPendingRequests(NodeIDImpl_SPtr id, SCMessage_SPtr outRequest);

	/**
	 * Prepare a full update message for a new supervisor.
	 * Does not alter the version sent.
	 * does not skip tables with version 0.
	 */
	void prepareFullUpdateMsg4Supervisor(SCMessage_SPtr msg);

	void reportStats(boost::posix_time::ptime time, bool labels);

	//=== Attribute manipulation of current node ===

	/*
	 * @see spdr::AttributeControl
	 */
	bool setAttribute(const String& key, std::pair<const int32_t,const char*> value);

	/*
	 * @see spdr::AttributeControl
	 */
	std::pair<event::AttributeValue,bool> getAttribute(const String& key);

	/*
	 * @see spdr::AttributeControl
	 */
	bool removeAttribute(const String& key);

	/*
	 * @see spdr::AttributeControl
	 */
	bool containsAttributeKey(const String& key);

	/*
	 * @see spdr::AttributeControl
	 */
	void clearAttributeMap();

	/*
	 * @see spdr::AttributeControl
	 */
	void clearExternalAttributeMap();

	/*
	 * @see spdr::AttributeControl
	 */
	void clearInternalAttributeMap();

	/*
	 * @see spdr::AttributeControl
	 */
	bool isEmptyAttributeMap();

	/*
	 * @see spdr::AttributeControl
	 */
	size_t sizeOfAttributeMap();

	/*
	 * @see spdr::AttributeControl
	 */
	StringSet getAttributeKeySet();
};

}

#endif /* ATTRIBUTEMANAGER_H_ */
