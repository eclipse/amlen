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

#include <SubCoveringFilterWireFormat.h>

namespace mcp
{

const uint16_t SubCoveringFilterWireFormat::ATTR_VERSION = 1;
const uint16_t SubCoveringFilterWireFormat::STORE_VERSION = 1;


SubCoveringFilterWireFormat::SubCoveringFilterWireFormat()
{
}

SubCoveringFilterWireFormat::~SubCoveringFilterWireFormat()
{
}

void SubCoveringFilterWireFormat::writeSubscriptionPattern(const uint32_t wireFormatVer, const SubscriptionPattern& pattern, ByteBuffer_SPtr buffer)
{
	//  Where the wire format of a pattern is:
	//  uint16_t #Plus-locations
	//	uint16_t location x (#Plus-location)
	//	uint16_t hash-location
	//	uint16_t last-level

	const std::vector<uint16_t>& pluses = pattern.getPlusLocations();
	buffer->writeShort(static_cast<int16_t>(pluses.size()));
	for (std::size_t j=0; j<pluses.size(); ++j )
	{
		buffer->writeShort(static_cast<int16_t>(pluses[j]));
	}
	buffer->writeShort(static_cast<int16_t>(pattern.getHashLocation()));
	buffer->writeShort(static_cast<int16_t>(pattern.getLastLevel()));
}

void SubCoveringFilterWireFormat::readSubscriptionPattern(const uint32_t wireFormatVer, ByteBufferReadOnlyWrapper& buffer, SubscriptionPattern_SPtr& pattern)
{
	uint16_t n_plus = static_cast<uint16_t>(buffer.readShort());
	std::vector<uint16_t> plus_loc;
	for (uint16_t i=0; i<n_plus; ++i)
	{
		plus_loc.push_back( static_cast<uint16_t>(buffer.readShort()) );
	}
	uint16_t hash_loc = static_cast<uint16_t>(buffer.readShort());
	uint16_t last_level = static_cast<uint16_t>(buffer.readShort());
	pattern.reset(new SubscriptionPattern(plus_loc,hash_loc, last_level));
}

int SubCoveringFilterWireFormat::writeSubscriptionStats(const uint32_t wireFormatVer, const RemoteSubscriptionStats& stats, ByteBuffer_SPtr buffer)
{
	buffer->writeInt(static_cast<int32_t>(stats.wildcardSubscriptions_NumOnBloomFilter));
	buffer->writeInt(static_cast<int32_t>(stats.wildcardSubscriptions_NumOnTopicTree));
	buffer->writeInt(static_cast<int32_t>(stats.topicTree_Top.size()));
	for (uint32_t i=0; i<stats.topicTree_Top.size(); ++i)
	{
		if (stats.topicTree_Top[i].first)
		{
			SubCoveringFilterWireFormat::writeSubscriptionPattern(wireFormatVer, *(stats.topicTree_Top[i].first),buffer);
			buffer->writeInt(static_cast<int32_t>(stats.topicTree_Top[i].second));
		}
		else
		{
			return ISMRC_NullArgument;
		}
	}
	buffer->writeInt(static_cast<int32_t>(stats.bloomFilter_Bottom.size()));
	for (uint32_t i=0; i<stats.bloomFilter_Bottom.size(); ++i)
	{
		if (stats.bloomFilter_Bottom[i].first)
		{
			SubCoveringFilterWireFormat::writeSubscriptionPattern(wireFormatVer, *(stats.bloomFilter_Bottom[i].first),buffer);
			buffer->writeInt(static_cast<int32_t>(stats.bloomFilter_Bottom[i].second));
		}
		else
		{
			return ISMRC_NullArgument;
		}
	}

	return ISMRC_OK;
}

int SubCoveringFilterWireFormat::readSubscriptionStats(const uint32_t wireFormatVer, ByteBufferReadOnlyWrapper& buffer, RemoteSubscriptionStats* pStats)
{
	using namespace std;

	pStats->wildcardSubscriptions_NumOnBloomFilter = static_cast<uint32_t>(buffer.readInt());
	pStats->wildcardSubscriptions_NumOnTopicTree = static_cast<uint32_t>(buffer.readInt());
	uint32_t num_tt_top = static_cast<uint32_t>(buffer.readInt());
	pStats->topicTree_Top.clear();
	for (uint32_t i=0; i<num_tt_top; ++i )
	{
		SubscriptionPattern_SPtr pattern;
		SubCoveringFilterWireFormat::readSubscriptionPattern(wireFormatVer, buffer,pattern);
		uint32_t freq = static_cast<uint32_t>(buffer.readInt());
		pStats->topicTree_Top.push_back(make_pair(pattern,freq));
	}
	uint32_t num_bf_bottom = static_cast<uint32_t>(buffer.readInt());
	for (uint32_t i=0; i<num_bf_bottom; ++i )
	{
		SubscriptionPattern_SPtr pattern;
		SubCoveringFilterWireFormat::readSubscriptionPattern(wireFormatVer, buffer,pattern);
		uint32_t freq = static_cast<uint32_t>(buffer.readInt());
		pStats->bloomFilter_Bottom.push_back(make_pair(pattern,freq));
	}

	return ISMRC_OK;
}

} /* namespace spdr */
