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

#ifndef GLOBALSUBMANAGERIMPL_H_
#define GLOBALSUBMANAGERIMPL_H_

#include <string>
#include <set>

#include <boost/thread/shared_mutex.hpp>

#include "GlobalSubManager.h"
#include "MCPConfig.h"
#include "RemoteServerInfo.h"
#include "mccLookupSet.h"
#include "GlobalRetainedStatsManager.h"


#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

class GlobalSubManagerImpl: public GlobalSubManager, public spdr::ScTraceContext
{
private:
	static spdr::ScTraceComponent* tc_;

public:
	GlobalSubManagerImpl(const std::string& inst_ID, const MCPConfig& mcpConfig);
	virtual ~GlobalSubManagerImpl();

	/**
	 * Start the unit.
	 *
	 * Start the unit, starting threads if any, etc.
	 *
	 * @return return code from MCPReturnCode
	 *
	 * @see MCPReturnCode
	 */
	MCPReturnCode start();

	MCPReturnCode recoveryCompleted();

	/**
	 * Close the unit.
	 *
	 * Close the unit, stopping threads if any, releasing resources, etc.
	 * This method is idempotent, i.e. may be called multiple times with the same effect.
	 *
	 * @return return code from MCPReturnCode
	 *
	 * @see MCPReturnCode
	 */
	MCPReturnCode close(bool leave_state_error = false);

	/*
	 * @see RouteLookup
	 */
	virtual MCPReturnCode lookup(
				ismCluster_LookupInfo_t* pLookupInfo);

	/*
	 * @see SubCoveringFilterEventListener
	 */
	virtual int onBloomFilterBase(ismCluster_RemoteServerHandle_t node, const std::string& tag,
			const mcc_hash_HashType_t bfType, const int16_t numHash,
			const int32_t numBins, const char* buffer);

	/*
	 * @see SubCoveringFilterEventListener
	 */
	virtual int onBloomFilterUpdate(ismCluster_RemoteServerHandle_t node, const std::string& tag,
			const std::vector<int32_t>& binUpdates);

	/*
	 * @see SubCoveringFilterEventListener
	 */
	virtual int onBloomFilterRemove(ismCluster_RemoteServerHandle_t node, const std::string& tag);


	/*
	 * @see SubCoveringFilterEventListener
	 */
	virtual int onWCSubscriptionPatternBase(
			ismCluster_RemoteServerHandle_t node,
			const std::vector<SubCoveringFilterPublisher::SubscriptionPatternUpdate>& subPatternBase);

	/*
	 * @see SubCoveringFilterEventListener
	 */
	virtual int onWCSubscriptionPatternUpdate(
			ismCluster_RemoteServerHandle_t node,
			const std::vector<SubCoveringFilterPublisher::SubscriptionPatternUpdate>& subPatternUpdate);


	/*
	 * @see SubCoveringFilterEventListener
	 */
	virtual int onServerDelete(ismCluster_RemoteServerHandle_t node, bool recovery);


    /*
     * @see SubCoveringFilterEventListener
     */
	virtual int setRouteAll(ismCluster_RemoteServerHandle_t node,
				int fRoutAll);


    /*
     * @see SubCoveringFilterEventListener
     */
	virtual int onRetainedStatsChange(
	        ismCluster_RemoteServerHandle_t node,
	        const std::string& serverUID,
	        RetainedStatsVector* retainedStats);

    /*
     * @see SubCoveringFilterEventListener
     */
    virtual int onRetainedStatsRemove(ismCluster_RemoteServerHandle_t node,
            const std::string& serverUID);


	MCPReturnCode lookupRetainedStats(const char *pServerUID,
	            ismCluster_LookupRetainedStatsInfo_t **pLookupInfo);


private:
	const MCPConfig& mcp_config;
	boost::shared_mutex shared_mutex;

	bool closed;
	bool started;
	bool error;

	mcc_lus_LUSetHandle_t lus ;
	int nl;

	GlobalRetainedStatsManager retainedManager;

	typedef std::set<uint64_t> PatternID_Set;
	typedef std::map<ServerIndex,PatternID_Set> Pattern_IDs_Nodes_Map;
	Pattern_IDs_Nodes_Map pattern_ids_map;

	int onBloomFilterSubscriptionPatternAdd(
			ismCluster_RemoteServerHandle_t node,
			const uint64_t id,
			const SubscriptionPattern& pattern);

	int onBloomFilterSubscriptionPatternRemove(
			ismCluster_RemoteServerHandle_t node,
			const uint64_t id);
};

} /* namespace mcp */

#endif /* GLOBALSUBMANAGERIMPL_H_ */
