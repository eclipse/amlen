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

#ifndef MCP_LOCALEXACTSUBMANAGER_H_
#define MCP_LOCALEXACTSUBMANAGER_H_

#include <string>

#include <boost/unordered_set.hpp>

#include "MCPConfig.h"
#include "LocalSubManager.h"
#include "SubCoveringFilterPublisher.h"
#include "CountingBloomFilter.h"
#include "BloomFilter.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

class LocalExactSubManager : public spdr::ScTraceContext
{
private:
	static spdr::ScTraceComponent* tc_;

public:
	LocalExactSubManager(const std::string& inst_ID, const MCPConfig& mcpConfig, LocalSubManager& localSubManager);
	virtual ~LocalExactSubManager();

	MCPReturnCode subscribe(const char* subscription);

	MCPReturnCode unsubscribe(const char* subscription);

	MCPReturnCode setSubCoveringFilterPublisher(SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher);

	MCPReturnCode start();

	MCPReturnCode recoveryCompleted();

	MCPReturnCode close();

	MCPReturnCode publishLocalExactBF();

private:
	const MCPConfig& config;
	LocalSubManager& localSubManager;
	SubCoveringFilterPublisher_SPtr filterPublisher;

	//State
	bool m_started;
	bool m_closed;
	bool m_recovered;

	typedef boost::unordered_set<std::string> SubscribedTopicsSet;
	//typedef std::set<string> SubscribedTopicsSet;

	//=== Exact subscriptions ===
	SubscribedTopicsSet m_subscribedTopics;
	CountingBloomFilter_SPtr m_cbf;
	BloomFilter_SPtr m_bf;
	uint64_t m_bf_base_sqn;
	uint64_t m_bf_last_sqn;
	/* the cumulative number of index updates published and pending (i.e. uint32_t items) */
	int m_numUpdates;

	std::vector<int> m_bf_updates_vec;
	bool m_republish_base;

	MCPReturnCode pushBloomFilterBase();
};

} /* namespace mcp */

#endif /* MCP_LOCALEXACTSUBMANAGER_H_ */
