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

#ifndef REMOTESUBSCRIPTIONSTATS_H_
#define REMOTESUBSCRIPTIONSTATS_H_

#include <vector>
#include <boost/cstdint.hpp>
#include "SubscriptionPattern.h"

namespace mcp
{

class RemoteSubscriptionStats
{
public:
	RemoteSubscriptionStats();
	virtual ~RemoteSubscriptionStats();

	uint32_t wildcardSubscriptions_NumOnBloomFilter;
	uint32_t wildcardSubscriptions_NumOnTopicTree;

	typedef std::pair< SubscriptionPattern_SPtr, uint32_t > SubscriptionPatternFrequency;

	/* K most frequent patterns on the topic-tree */
	std::vector<SubscriptionPatternFrequency > topicTree_Top;

	/* K least frequent patterns on the bloom filter */
	std::vector<SubscriptionPatternFrequency > bloomFilter_Bottom;

	std::string toString() const;

};

} /* namespace mcp */

#endif /* REMOTESUBSCRIPTIONSTATS_H_ */
