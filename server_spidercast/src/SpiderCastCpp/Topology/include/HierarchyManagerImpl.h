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
 * HierarchyManagerImpl.h
 *
 *  Created on: Oct 4, 2010
 */

#ifndef HIERARCHYMANAGERIMPL_H_
#define HIERARCHYMANAGERIMPL_H_

#include <boost/noncopyable.hpp>

#include "Definitions.h"
#include "NodeIDCache.h"
#include "SpiderCastConfigImpl.h"
#include "CoreInterface.h"
#include "HierarchyManager.h"
#include "TaskSchedule.h"
#include "HierarchyPeriodicTask.h"
#include "HierarchyTerminationTask.h"
#include "HierarchyViewKeeper.h"
#include "HierarchyDelegate.h"
#include "HierarchySupervisor.h"

namespace spdr
{

class VirtualIDCache;

class HierarchyManagerImpl: boost::noncopyable,
	public HierarchyManager, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	HierarchyManagerImpl(
			const String& instID,
			const SpiderCastConfigImpl& config,
			NodeIDCache& nodeIDCache,
			VirtualIDCache& vidCache,
			CoreInterface& coreInterface);

	virtual ~HierarchyManagerImpl();

	//===

	/*
	 * @see spdr::HierarchyManager
	 */
	void start();

	/*
	 * @see spdr::HierarchyManager
	 */
	void terminate(bool soft);

	/*
	 * @see spdr::HierarchyManager
	 */
	bool isManagementZone() const;

	/*
	 * @see spdr::HierarchyManager
	 */
	void processIncomingHierarchyMessage(SCMessage_SPtr inHierarchyMsg);

	/*
	 * @see spdr::HierarchyManager
	 */
	void processIncomingCommEventMsg(SCMessage_SPtr incomingCommEvent);

	/*
	 * @see spdr::HierarchyManager
	 */
	void periodicTask();

	/*
	 * @see spdr::HierarchyManager
	 */
	void terminationTask();

	/*
	 * @see spdr::HierarchyManager
	 */
	void hierarchyViewChanged();

	/*
	 * @see spdr::HierarchyManager
	 */
	void globalViewChanged();

	/*
	 * @see spdr::HierarchyManager
	 */
	void verifyIncomingMessageAddressing(
		const String& msgSenderName, const String& connSenderName, const String& msgTargetName)
		throw (SpiderCastRuntimeError);

	/*
	 * @see spdr::HierarchyManager
	 */
	int64_t queueForeignZoneMembershipRequest(
		BusName_SPtr zoneBusName, bool includeAttributes, int timeoutMillis)
		throw(SpiderCastLogicError, SpiderCastRuntimeError);

	/*
	 * @see spdr::HierarchyManager
	 */
	int64_t queueZoneCensusRequest()
		throw(SpiderCastLogicError, SpiderCastRuntimeError);

	//===

	/**
	 * Create cross-references between the components.
	 * To be called right after construction but before start().
	 *
	 * Called only if hierarchy is enabled
	 */
	void init();

	/**
	 * Destroy cross-references between the components.
	 * To be called right before destruction.
	 */
	void destroyCrossRefs();

private:
	const String& instID_;
	const SpiderCastConfigImpl& config_;
	CoreInterface& coreInterface_;
	NodeIDCache& nodeIDCache_;

	boost::shared_ptr<HierarchyViewKeeper> viewKeeper_SPtr;

	HierarchyDelegate delegate_;
	HierarchySupervisor supervisor_;

	MembershipManager_SPtr memMngr_SPtr;
	TaskSchedule_SPtr taskSchedule_SPtr;
	AbstractTask_SPtr periodicTask_SPtr;

	//is the node in the management-zone (L1) or base-zone (L2)
	bool managementZone;
	//if in base-zone, is node a delegate candidate (non-empty L1-bootstrap)
	bool isDelegateCandidate;

	boost::recursive_mutex hier_mutex;
	bool started_;
	bool closed_;
	bool close_done_;
	bool close_soft_;

	bool first_periodic_task_;


};

}

#endif /* HIERARCHYMANAGERIMPL_H_ */
