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

#ifndef MCP_SUBCOVERINGFILTEREVENTLISTENER_H_
#define MCP_SUBCOVERINGFILTEREVENTLISTENER_H_

#include <vector>
#include <boost/shared_array.hpp>
#include "MCPTypes.h"
#include "SubCoveringFilterPublisher.h"

namespace mcp
{

class SubCoveringFilterEventListener
{
public:
	SubCoveringFilterEventListener();
	virtual ~SubCoveringFilterEventListener();

	virtual int onBloomFilterBase(
			ismCluster_RemoteServerHandle_t node,
			const std::string& tag,
			const mcc_hash_HashType_t bfType,
			const int16_t numHash,
			const int32_t numBins,
			const char* buffer) = 0;

	virtual int onBloomFilterUpdate(
			ismCluster_RemoteServerHandle_t node,
			const std::string& tag,
			const std::vector<int32_t>& binUpdates) = 0;

	virtual int onBloomFilterRemove(
			ismCluster_RemoteServerHandle_t node,
			const std::string& tag) = 0;

	virtual int onWCSubscriptionPatternBase(
			ismCluster_RemoteServerHandle_t node,
			const std::vector<SubCoveringFilterPublisher::SubscriptionPatternUpdate>& subPatternBase) = 0;

	virtual int onWCSubscriptionPatternUpdate(
			ismCluster_RemoteServerHandle_t node,
			const std::vector<SubCoveringFilterPublisher::SubscriptionPatternUpdate>& subPatternUpdate) = 0;

	virtual int onServerDelete(ismCluster_RemoteServerHandle_t node, bool recovery) = 0;

	virtual int setRouteAll(ismCluster_RemoteServerHandle_t node,
			int fRoutAll) = 0;

	struct RetainedStatsItem
	{
	    std::string uid;
	    boost::shared_array<char> data;
	    uint32_t dataLen;

	    RetainedStatsItem() : dataLen(0){}
	};

	typedef std::vector<RetainedStatsItem> RetainedStatsVector;

	/**
	 * The retained stats vector is called when there is a change in the vector of a certain remote server.
	 * The memory pointed to by the fields in each ismCluster_RetainedStats_t structure belongs to the caller and must be copied.
	 *
	 * @param node
	 * @param serverUID
	 * @param retainedStats
	 * @return
	 */
	virtual int onRetainedStatsChange(ismCluster_RemoteServerHandle_t node,
	        const std::string& serverUID,
	        RetainedStatsVector* retainedStats) = 0;

	virtual int onRetainedStatsRemove(ismCluster_RemoteServerHandle_t node,  const std::string& serverUID) = 0;

};

} /* namespace mcp */

#endif /* MCP_SUBCOVERINGFILTEREVENTLISTENER_H_ */
