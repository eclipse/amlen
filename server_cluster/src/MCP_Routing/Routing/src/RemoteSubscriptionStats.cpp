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

#include <sstream>
#include <RemoteSubscriptionStats.h>
#include "ByteBuffer.h"

namespace mcp
{

RemoteSubscriptionStats::RemoteSubscriptionStats() :
		wildcardSubscriptions_NumOnBloomFilter(0), wildcardSubscriptions_NumOnTopicTree(
				0)
{
}

RemoteSubscriptionStats::~RemoteSubscriptionStats()
{
}

std::string RemoteSubscriptionStats::toString() const

{
	std::ostringstream oss;
	oss << "#onBF=" << wildcardSubscriptions_NumOnBloomFilter
	        << " #onTT=" << wildcardSubscriptions_NumOnTopicTree
	        << " #TTtop=" << topicTree_Top.size()
	        << " #BFbottom=" << bloomFilter_Bottom.size() << std::endl;
	oss << "TTtop={";
	for (std::size_t i = 0; i < topicTree_Top.size(); ++i)
	{
		oss << spdr::toString(topicTree_Top[i].first) << " /"
				<< topicTree_Top[i].second << "; ";
	}
	oss << "}" << std::endl << "BFbottom={";
	for (std::size_t i = 0; i < bloomFilter_Bottom.size(); ++i)
	{
		oss << spdr::toString(bloomFilter_Bottom[i].first) << " /"
				<< bloomFilter_Bottom[i].second << "; ";
	}
	return oss.str();
}

} /* namespace mcp */
