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
 * HierarchyManager.h
 *
 *  Created on: Oct 4, 2010
 */

#ifndef HIERARCHYMANAGER_H_
#define HIERARCHYMANAGER_H_

#include "SCMessage.h"
#include "HierarchyUtils.h"
#include "HierarchyViewListener.h"
#include "MembershipService.h"

namespace spdr
{

class HierarchyManager : public HierarchyViewListener
{
public:
	HierarchyManager();
	virtual ~HierarchyManager();

	virtual void start() = 0;

	virtual void terminate(bool soft) = 0;

	virtual void processIncomingHierarchyMessage(
			SCMessage_SPtr inHierarchyMsg) = 0;

	virtual void processIncomingCommEventMsg(
			SCMessage_SPtr incomingCommEvent) = 0;

	virtual void verifyIncomingMessageAddressing(
			const String& msgSenderName, const String& connSenderName, const String& msgTargetName)
			throw (SpiderCastRuntimeError)= 0;

	virtual bool isManagementZone() const = 0;

	/**
	 *
	 * Threading: App. thread, internal threads.
	 *
	 * @param zoneBusName
	 * @param includeAttributes
	 * @param timeoutMillis
	 * @return
	 *
	 * @throw IllegalStateException This operation is not supported on a base-zone
	 * @throw IllegalArgumentException zoneBusName must be of a managed base-zone
	 *
	 */
	virtual int64_t queueForeignZoneMembershipRequest(
		BusName_SPtr zoneBusName, bool includeAttributes, int timeoutMillis)
		throw(SpiderCastLogicError, SpiderCastRuntimeError) = 0;

	/**
	 *
	 * Threading: App. thread, internal threads.
	 *
	 * @return
	 *
	 * @throw IllegalStateException This operation is not supported on a base-zone
	 *
	 */
	virtual int64_t queueZoneCensusRequest()
	throw(SpiderCastLogicError, SpiderCastRuntimeError) = 0;

	//=== tasks ===

	virtual void periodicTask() = 0;

	virtual void terminationTask() = 0;

};

}

#endif /* HIERARCHYMANAGER_H_ */
