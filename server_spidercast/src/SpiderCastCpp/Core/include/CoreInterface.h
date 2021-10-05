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
 * CoreInterface.h
 *
 *  Created on: 01/03/2010
 */

#ifndef SPDR_COREINTERFACE_H_
#define SPDR_COREINTERFACE_H_

#include <boost/shared_ptr.hpp>

#include "TopologyManager.h"
#include "MembershipManager.h"
#include "CommAdapter.h"
#include "TaskSchedule.h"


namespace spdr
{

class HierarchyManager;
typedef boost::shared_ptr<HierarchyManager> HierarchyManager_SPtr;

namespace route
{
	class RoutingManager;
}

namespace messaging
{
	class MessagingManager;
}

/**
 * The interface SpiderCastImpl exposes to the components.
 *
 */
class CoreInterface
{
public:
	CoreInterface();
	virtual ~CoreInterface();

	virtual MembershipManager_SPtr getMembershipManager() = 0;

	virtual TopologyManager_SPtr getTopologyManager() = 0;

	virtual CommAdapter_SPtr getCommAdapter() = 0;

	virtual HierarchyManager_SPtr getHierarchyManager() = 0;

	virtual boost::shared_ptr<route::RoutingManager> getRoutingManager() = 0;

	virtual boost::shared_ptr<messaging::MessagingManager> getMessagingManager() = 0;

	virtual TaskSchedule_SPtr getTopoMemTaskSchedule() = 0;

	/**
	 * Incarnation number is creation time of the node, expressed as the
	 * number of microseconds since 1-Jan-1970 (UTC).
	 *
	 * @return
	 */
	virtual int64_t getIncarnationNumber() const = 0;

	/**
	 * Called when a thread experiences a fatal error that needs to be reported
	 * and trigger an orderly shutdown.
	 *
	 * @param threadName
	 * @param ex
	 */
	virtual void threadFailure(const String& threadName, std::exception& ex) = 0;

	/**
	 * Called when a component Fatal_Error is received and triggers an orderly shutdown.
	 *
	 * @param errMsg
	 * @param errCode
	 */
	virtual void componentFailure(const String& errMsg, event::ErrorCode errCode) = 0;

	/**
	 * The instance ID for tracing.
	 *
	 * @return
	 */
	virtual const String& getInstanceID() const = 0;

	/**
	 * Executed by the statistics task.
	 */
	virtual void reportStats(bool labels = false) = 0;

	virtual void submitExternalEvent(event::SpiderCastEvent_SPtr event) = 0;

};

}

#endif /* SPDR_COREINTERFACE_H_ */
