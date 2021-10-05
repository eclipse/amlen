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

#ifndef MCP_FILTERTAGS_H_
#define MCP_FILTERTAGS_H_

#include <string>

namespace mcp
{

class FilterTags
{
protected:
	const static char BF_Base_Suffix = 'B';
	const static char BF_Update_Suffix = 'U';

public:
	const static std::string BF_Prefix;

	/**
	 * Identifies the tag for the Bloom filter used for exact subscriptions.
	 */
	const static std::string BF_ExactSub;

	/**
	 * Identifies the tag for the Bloom filter used for wild-card subscriptions.
	 */
	const static std::string BF_WildcardSub;

	/**
	 * The tag for the forwarding address of this node.
	 */
	const static std::string Fwd_Endpoint;

	const static std::string LocalServerInfo;

	const static std::string RetainedStats;

	const static std::string MonitoringStatus;

	const static std::string RemovedServersList;

	const static std::string RestoredNotInView;

	const static std::string BF_ExactSub_Base;
	const static std::string BF_ExactSub_Update;

	const static std::string BF_WildcardSub_Base;
	const static std::string BF_WildcardSub_Update;

	const static std::string BF_Wildcard_SubscriptionPattern;
	const static std::string BF_Wildcard_SubscriptionPattern_Base;
	const static std::string BF_Wildcard_SubscriptionPattern_Update;

	const static std::string RCF_Prefix;
	const static std::string RCF_Base;
	const static std::string RCF_Update;

	const static std::string WCSub_Stats;
protected:
	FilterTags();
	virtual ~FilterTags();
};

} /* namespace mcp */

#endif /* MCP_FILTERTAGS_H_ */
