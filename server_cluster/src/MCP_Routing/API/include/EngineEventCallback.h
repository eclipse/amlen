/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

/*
 * EngineEventCallback.h
 *  Created on: Dec 28, 2014
 */

#ifndef MCP_ENGINEEVENTCALLBACK_H_
#define MCP_ENGINEEVENTCALLBACK_H_

#include "cluster.h"
#include "ServerRegistration.h"
#include "StoreNodeData.h"
#include "RemoteTopicTreeSubscriptionEvents.h"

namespace mcp
{

class EngineEventCallback : public ServerRegistration, public StoreNodeData, public RemoteTopicTreeSubscriptionEvents
{
public:
	EngineEventCallback();
	virtual ~EngineEventCallback();

	/**
	 *
	 * @param pEngineStatistics
	 * @return
	 */
    virtual int reportEngineStatistics(ismCluster_EngineStatistics_t * pEngineStatistics) = 0;



};

} /* namespace mcp */

#endif /* MCP_ENGINEEVENTCALLBACK_H_ */
