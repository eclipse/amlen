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
 * MembershipServiceImpl.h
 *
 *  Created on: 28/03/2010
 */

#ifndef MEMBERSHIPSERVICEIMPL_H_
#define MEMBERSHIPSERVICEIMPL_H_

#include <cctype>
#include <boost/thread.hpp>

#include "MembershipService.h"
#include "MembershipServiceConfig.h"
//#include "ConcurrentSharedPtr.h"
#include "SpiderCastConfigImpl.h"
#include "MembershipManager.h"
#include "HierarchyManager.h"
#include "CoreInterface.h"
#include "PropertyMap.h"
#include "MembershipListener.h"
#include "SpiderCastLogicError.h"
#include "SpiderCastRuntimeError.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class MembershipServiceImpl: public MembershipService, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;
	NodeIDImpl_SPtr myID_;
	BusName_SPtr myBusName_;
	MembershipManager_SPtr memMngr_SPtr_;
	HierarchyManager_SPtr hierMngr_SPtr_;
	const SpiderCastConfigImpl& spdrConfig_;
	MembershipServiceConfig config_;
	MembershipListener& membershipListener_;
	AttributeControl& attributeManager_;

	bool volatile closed_;
	boost::mutex mutex_;

	bool firstViewDelivered_;


public:

	MembershipServiceImpl(
			const String& instID,
			NodeIDImpl_SPtr myID,
			BusName_SPtr myBusName,
			MembershipManager_SPtr memMngr,
			HierarchyManager_SPtr hierMngr,
			const SpiderCastConfigImpl& spdrConfig,
			const PropertyMap& prop,
			MembershipListener& listener);

	~MembershipServiceImpl();

	/*
	 * @see spdr::MembershipService
	 */
	void close();

	/*
	 * @see spdr::MembershipService
	 */
	bool isClosed();

	/*
	 * @see spdr::MembershipService
	 */
	bool setHighPriorityMonitor(bool value);

	/*
	 * @see spdr::MembershipService
	 */
	bool isHighPriorityMonitor();

	/*
	 * @see spdr::MembershipService
	 */
	bool setAttribute(const String& key, Const_Buffer value);

	/*
	 * @see spdr::MembershipService
	 */
	std::pair<event::AttributeValue,bool> getAttribute(const String& key);

	/*
	 * @see spdr::MembershipService
	 */
	bool removeAttribute(const String& key);

	/*
	 * @see spdr::MembershipService
	 */
	void clearAttributeMap();

	/*
	 * @see spdr::MembershipService
	 */
	bool isEmptyAttributeMap();

	/*
	 * @see spdr::MembershipService
	 */
	size_t sizeOfAttributeMap();

	/*
	 * @see spdr::MembershipService
	 */
	StringSet getAttributeKeySet();

	/*
	 * @see spdr::MembershipService
	 */
	bool containsAttribute(const String& key);

	//=== Retained Attributes ===

	/*
	 * @see spdr::MembershipService
	 */
	bool clearRemoteNodeRetainedAttributes(NodeID_SPtr target, int64_t incarnation);

	//=== Foreign Zone Membership ===

	/*
	 * @see spdr::MembershipService
	 */
	int64_t getForeignZoneMembership(
		const String& zoneBusName, bool includeAttributes)
		throw(SpiderCastLogicError, SpiderCastRuntimeError);

	/*
	 * @see spdr::MembershipService
	 */
	int64_t getZoneCensus()
		throw(SpiderCastLogicError, SpiderCastRuntimeError);

	//===

	/**
	 * Called when closing SpiderCast
	 */
	void internalClose();


	/**
	 * Called by MemTopo-thread, puts the event in the delivery Q.
	 *
	 * TODO implement a delivery Q, currently deliver directly to app. using MemTopo-thread.
	 *
	 * Threading: MemTopo-thread.
	 */
	void queueForDelivery(event::MembershipEvent_SPtr event);

	/**
	 * Called by Delivery-thread, delivers event to listener, if service is not closed.
	 *
	 * TODO implement a delivery Q, currently deliver directly to app. using MemTopo-thread.
	 *
	 * Threading: MemTopo-thread.
	 */
	void deliverEventToListener(event::MembershipEvent_SPtr event);

	/**
	 * Called by MemTopo-thread
	 */
	bool isFirstViewDelivered()
	{
		return firstViewDelivered_;
	}

	/**
	 * A utility to get a string from a binary buffer.
	 *
	 * @param buffer
	 * @return
	 */
	static String binBufferToString(Const_Buffer buffer);
};

}

#endif /* MEMBERSHIPSERVICEIMPL_H_ */
