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

#ifndef MCP_REMOTETOPICTREESUBSCRIPTIONEVENTS_H_
#define MCP_REMOTETOPICTREESUBSCRIPTIONEVENTS_H_

#include "MCPTypes.h"

namespace mcp
{

class RemoteTopicTreeSubscriptionEvents
{
public:
	RemoteTopicTreeSubscriptionEvents();
	virtual ~RemoteTopicTreeSubscriptionEvents();

	virtual int addSubscriptions(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			char                             **pSubscriptions,
			size_t                             subscriptionsLength) = 0;

	virtual int removeSubscriptions(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			char                             **pSubscriptions,
			size_t                             subscriptionsLength) = 0;
};

} /* namespace mcp */

#endif /* MCP_REMOTETOPICTREESUBSCRIPTIONEVENTS_H_ */
