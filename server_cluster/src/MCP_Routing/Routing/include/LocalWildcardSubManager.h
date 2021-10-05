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

#ifndef MCP_LOCALWILDCARDSUBMANAGER_H_
#define MCP_LOCALWILDCARDSUBMANAGER_H_


#include <string>

#include <boost/unordered_set.hpp>

#include "MCPConfig.h"
#include "LocalSubManager.h"
#include "SubCoveringFilterPublisher.h"
#include "CountingBloomFilter.h"
#include "BloomFilter.h"
#include "ControlManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

#include "RemoteSubscriptionStatsListener.h"
#include "hashFunction.h"

namespace mcp
{

class LocalWildcardSubManager : public RemoteSubscriptionStatsListener, public spdr::ScTraceContext
{
private:
	static spdr::ScTraceComponent* tc_;

public:
	LocalWildcardSubManager(
			const std::string& inst_ID,
			const MCPConfig& mcpConfig,
			const std::string& server_name,
			LocalSubManager& localSubManager,
			ControlManager& controlManager);

	virtual ~LocalWildcardSubManager();

	MCPReturnCode subscribe(const char* subscription);

	MCPReturnCode unsubscribe(const char* subscription);

	MCPReturnCode setSubCoveringFilterPublisher(SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher);

	MCPReturnCode start();


	/**
	 * This will be invoked between start() and recoveryCompleted to provide the patterns assigned to the WCBF before the crash/shutdown
	 *
	 * @param patterns
	 * @return
	 */
	MCPReturnCode restoreSubscriptionPatterns(const std::vector<SubscriptionPattern_SPtr>& patterns);

	MCPReturnCode recoveryCompleted();

	MCPReturnCode close();

	MCPReturnCode publishLocalUpdates();


	/*
	 * @see RemoteSubscriptionStatsListener
	 */
	virtual int update(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID,
			const RemoteSubscriptionStats& stats);
	/*
	 * @see RemoteSubscriptionStatsListener
	 */
	virtual int disconnected(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID);
	/*
	 * @see RemoteSubscriptionStatsListener
	 */
	virtual int connected(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID);
	/*
	 * @see RemoteSubscriptionStatsListener
	 */
	virtual int remove(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID);


private:

	std::string myName;
	uint32_t myNameHash ;

	typedef std::map<std::string,uint64_t> SubscribedTopicsMap;
	struct SubscriptionPatternInfo
	{
		uint64_t	id;
		uint32_t	count;
		bool        inBF;
		SubscriptionPattern_SPtr pattern;
		SubscribedTopicsMap m_subscribedTopics_WC;
		SubscriptionPatternInfo *prev ;
		SubscriptionPatternInfo *next ;

		SubscriptionPatternInfo() : id(0), count(0), inBF(false), prev(NULL), next(NULL) {}
	};
	SubscriptionPatternInfo *firstSpi ;
	SubscriptionPatternInfo *lastSpi ;

	uint32_t wcttRemote;
	uint32_t wcttLocal;
	uint32_t wcbfLocal;
	uint32_t wcttLocal_last_published;
	uint32_t wcbfLocal_last_published;
	uint64_t wctt_baseSqn;
	uint64_t wctt_updtSqn;
	uint32_t wctt_baseSize;
	uint32_t wctt_updtSize;
	uint64_t rcfID;
	uint64_t patID;
	uint64_t statSqn;
	uint32_t nInBF;
	uint64_t pat_baseSqn;
	uint64_t pat_updtSqn;

	const MCPConfig& config;
	LocalSubManager& localSubManager;
	ControlManager& controlManager;
	SubCoveringFilterPublisher_SPtr filterPublisher;

	//State
	bool m_started;
	bool m_closed;
	bool m_recovered;
	bool m_1st_publish;
	bool reschedule;

	//typedef boost::unordered_set<std::string> SubscribedTopicsSet;

//	SubscribedTopicsSet m_subscribedTopics_WC;
	CountingBloomFilter_SPtr m_cbf_WC;
	BloomFilter_SPtr m_bf_WC;
	uint64_t m_bf_WC_base_sqn;
	uint64_t m_bf_WC_last_sqn;
	/* the cumulative number of index updates published and pending (i.e. uint32_t items) */
	int m_numUpdates_WC;

	std::vector<int> m_bf_WC_updates_vec;
	bool m_republish_base_WC;

	//=== Wild-card patterns ===
	typedef std::map<SubscriptionPattern, SubscriptionPatternInfo * > SubscriptionPatternMap;
	SubscriptionPatternMap m_subscriptionPattern_Map;

	//a stack of free indices. indices are used to create attribute keys, preventing tomb-stone blow-out
//	uint32_t m_subscriptionPattern_max_index;
//	std::vector<uint32_t> m_subscriptionPattern_free_indices;

	/* the queue of patterns that need to be published */
	typedef SubCoveringFilterPublisher::RegularCoveringFilterUpdate RegularCoveringFilterUpdate;

	typedef SubCoveringFilterPublisher::SubscriptionPatternUpdate SubscriptionPatternUpdate;
	typedef std::vector< SubscriptionPatternUpdate > SubscriptionPatternQueue;
	SubscriptionPatternQueue m_subscriptionPattern_publish_queue;

	typedef std::vector< RegularCoveringFilterUpdate > RegularCoveringFilterQueue;
	RegularCoveringFilterQueue rcf_publish_queue;

	struct ByCount
	{
		uint32_t count;
		uint32_t hash;
		std::string *name;
		SubscriptionPattern_SPtr pat;
		uint16_t index;
	    bool operator< (const ByCount &x) const
	    {
	    	if (count < x.count) return true;
	    	if (count > x.count) return false;
	    	if (hash < x.hash) return true;
	    	if (hash > x.hash) return false;
	    	return (*name<*x.name) ;
	    }
	};

	uint8_t *isConn ;
	size_t isConnSize;
	MCPReturnCode isConnMakeRoom(uint16_t index);
	int isConnected(uint16_t index);

	struct RemoteStatsInfo
	{
		uint32_t hash;
		std::string name;
		RemoteSubscriptionStats stats;
	};

	typedef std::map<uint16_t,RemoteStatsInfo> RemoteStatsMap;
	RemoteStatsMap remoteStats;

	MCPReturnCode publishLocalWildcardBF();

	MCPReturnCode publishLocalWildcardPatterns();

	MCPReturnCode publishRegularCoveringFilters();

	MCPReturnCode publishStats();

	MCPReturnCode publishAll();

	MCPReturnCode storeSubscriptionPatterns();

};

} /* namespace mcp */

#endif /* MCP_LOCALWILDCARDSUBMANAGER_H_ */
