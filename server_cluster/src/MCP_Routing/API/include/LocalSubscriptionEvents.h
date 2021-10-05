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

#ifndef MCP_LOCALSUBSCRIPTIONEVENTS_H_
#define MCP_LOCALSUBSCRIPTIONEVENTS_H_

#include <boost/cstdint.hpp>

#include "cluster.h"
#include "MCPReturnCode.h"

namespace mcp
{

/**
 * An interface for updating MCP on local subscriptions.
 *
 * It is assumed that the engine counts the number of subscriptions on any given topic,
 * and calls "subscribe()" on the first subscription, and "unsubscribe()" when there are no
 * longer any subscriptions.
 */
class LocalSubscriptionEvents
{
public:
	LocalSubscriptionEvents();
	virtual ~LocalSubscriptionEvents();

	/**
	 * Local subscribe events.
	 *
	 * Indicates there is at least a single local subscription on filter specified in the array.
	 *
	 * Called once per filter, when the first local subscription is created.
	 *
	 * @param pSubInfo an array of subscription info structures.
	 * @param numSubs the length of the array.
	 * @return
	 */
	virtual MCPReturnCode addSubscriptions(
			const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs) = 0;

	/**
	 * Local unsubscribe events.
	 *
	 * Indicates there are no longer local subscriptions on the filters specified in the array.
	 *
	 * @param pSubInfo an array of subscription info structures.
	 * @param numSubs the length of the array.
	 * @return
	 */
	virtual MCPReturnCode removeSubscriptions(
			const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs) = 0;

};

} /* namespace mcp */

#endif /* MCP_LOCALSUBSCRIPTIONEVENTS_H_ */
