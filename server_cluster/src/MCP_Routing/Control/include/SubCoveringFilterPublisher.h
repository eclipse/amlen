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

#ifndef SUBCOVERINGFILTERPUBLISHER_H_
#define SUBCOVERINGFILTERPUBLISHER_H_

#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

#include "cluster.h"
#include "MCPTypes.h"
#include "SubscriptionPattern.h"
#include "MCPExceptions.h"
#include "FilterTags.h"
#include "RemoteSubscriptionStats.h"
#include "hashFunction.h"
#include "RemoteServerRecord.h"

namespace mcp
{

/**
 * An interface for publishing subscription covering filters.
 *
 * Can publish filters of types:
 * (1) Bloom filters for exact subscriptions
 * (2) Bloom filters for wild-card subscriptions
 * (3) Wild-card subscription patterns
 * (4) Regular covering filters.
 *
 * All the "publish" and "update" and "remove" methods produce strictly increasing sequence numbers.
 * The sequence numbers provide ordering between all these operations which is preserved in the receiving side.
 */
class SubCoveringFilterPublisher
{

public:

	SubCoveringFilterPublisher();
	virtual ~SubCoveringFilterPublisher();



	/**
	 * TODO convert to not throw
	 *
	 * Set the base of a Bloom filter with a given tag.
	 *
	 * The tag must be one of the tags declared above, in one of
	 * static const string tag_BF_*;
	 *
	 * @param tag
	 * @param bfType 									(represented as a int16_t)
	 * @param numHash Number of hash function used 		(represented as a int16_t)
	 * @param numBins Number of bins, in bits 			(represented as a int32_t)
	 * @param buffer
	 * @return sequence number
	 *
	 * @throw MCPRuntimeError, MCPLogicError, SpiderCastLogicError, SpiderCastRuntimeError
	 */
	virtual uint64_t publishBloomFilterBase(const std::string& tag,
			const mcc_hash_HashType_t bfType, const int16_t numHash,
			const int32_t numBins, const char* buffer) = 0;

	/**
	 * TODO convert to not throw
	 *
	 * Update the base Bloom filter with the same tag.
	 *
	 * The bin update vector specifies changed bit locations (1-indexing).
	 * Positive indices should be set to 1';
	 * The absolute value of negative indices should be set to 0'.
	 *
	 * @param tag
	 * @param binUpdates
	 * @return sequence number
	 *
	 * @throw MCPRuntimeError, MCPLogicError, SpiderCastLogicError, SpiderCastRuntimeError
	 */
	virtual uint64_t publishBloomFilterUpdate(const std::string& tag,
			const std::vector<int32_t>& binUpdates) = 0;

	/**
	 * TODO convert to not throw
	 *
	 * Removes a Bloom filter base ans all its updates.
	 *
	 * Actually publishes a zero length base that is interpreted as a "remove".
	 *
	 * @param tag
	 * @return one past the sequence number of the last base/update
	 *
	 * @throw MCPRuntimeError, MCPLogicError, SpiderCastLogicError, SpiderCastRuntimeError
	 */
	virtual uint64_t removeBloomFilter(const std::string& tag) = 0;

	/**
	 *
	 * Get the number of Bloom filter updates.
	 *
	 * Corresponds to the number of attributes used for this purpose, not the size of memory.
	 *
	 * @param tag
	 * @return
	 */
	virtual uint32_t getNumBloomFilterUpdates(const std::string& tag) const = 0;

	/**
	 * Get the size in bytes of Bloom filter updates.
	 *
	 * @param tag
	 * @return
	 */
	virtual uint32_t getSizeBytesBloomFilterUpdates(const std::string& tag) const = 0;

	/**
	 * Get the size in bytes of the Bloom filter base.
	 *
	 * @param tag
	 * @return
	 */
	virtual uint32_t getSizeBytesBloomFilterBase(const std::string& tag) const = 0;


	/* ********************************************************************/
	/* Wild-card subscription patterns (SupscriptionPattern)            */
	/* ********************************************************************/

	/**
	 * A unique ID for a pattern.
	 * The ID is strictly increasing for new patterns.
	 */
	typedef uint64_t WCSupscriptionPatternID;

	/**
	 * To add a new patter, specify a pair with a unique WCSupscriptionPatternID and
	 * a pointer to the subscription pattern.
	 *
	 * To remove an existing patter, specify a pair with an existing RegularCoveringFilterID and
	 * a NULL pointer.
	 */
	typedef std::pair<WCSupscriptionPatternID, SubscriptionPattern_SPtr> SubscriptionPatternUpdate;

	/**
	 *
	 * @param subPatternBase
	 * @param sqn
	 * @return
	 */
	virtual int publishWCSubscriptionPatternBase(
			const std::vector<SubscriptionPatternUpdate>& subPatternBase,
			uint64_t* sqn) = 0;

	/**
	 *
	 * @param subPatternBase
	 * @param sqn
	 * @return
	 */
	virtual int publishWCSubscriptionPatternUpdate(
			const std::vector<SubscriptionPatternUpdate>& subPatternUpdate,
			uint64_t* sqn) = 0;

	/**
	 * Get the number of SupscriptionPattern updates.
	 *
	 * Corresponds to the number of attributes used for this purpose, not the size of memory.
	 *
	 * @return
	 */
	virtual uint32_t getNumWCSubscriptionPatternUpdates() const = 0;

	/**
	 * Get the size in bytes of the SupscriptionPattern updates.
	 *
	 * @return
	 */
	virtual uint32_t getSizeBytesWCSubscriptionPatternUpdates() const = 0;

	/**
	 * Get the size in bytes of the SupscriptionPattern base.
	 *
	 * @return
	 */
	virtual uint32_t getSizeBytesWCSubscriptionPatternBase() const = 0;

	/* ***********************************************************************/
	/* Regular Covering Filters (RCFs)                                       */
	/* Are explicit wild card or exact subscriptions, in plain text          */
	/* ***********************************************************************/

	/**
	 * A unique ID for an RCF.
	 * The ID is strictly increasing for new subscriptions.
	 */
	typedef uint64_t RegularCoveringFilterID;
	/**
	 * To add a new RCF, specify a pair with a unique RegularCoveringFilterID and
	 * a pointer to the subscription string.
	 *
	 * To remove an existing RCF, specify a pair with an existing RegularCoveringFilterID and
	 * a NULL pointer.
	 */
	typedef std::pair<RegularCoveringFilterID, String_SPtr> RegularCoveringFilterUpdate;

	/**
	 * Publish the base of the RCFs.
	 *
	 * The base RegularCoveringFilterUpdate elements have no NULL strings.
	 * The RegularCoveringFilterID is strictly increasing for new subscriptions.
	 *
	 * @param rcfBase
	 * @return return code
	 */
	virtual int publishRegularCoveringFilterBase(
			const std::vector<RegularCoveringFilterUpdate>& rcfBase,
			uint64_t* sqn) = 0;

	/**
	 * Publish an update of RCFs.
	 *
	 * The update vector is in the order of subscribe/unsubscribe events.
	 *
	 * @param rcfUpdate
	 * @return return code
	 */
	virtual int publishRegularCoveringFilterUpdate(
			const std::vector<RegularCoveringFilterUpdate>& rcfUpdate,
			uint64_t* sqn) = 0;

	/**
	 * Get the number of RCF updates.
	 *
	 * Corresponds to the number of attributes used for this purpose, not the size of memory.
	 *
	 * @return
	 */
	virtual uint32_t getNumRegularCoveringFilterUpdates() const = 0;

	/**
	 * Get the size in bytes of RCF updates.
	 *
	 * @return
	 */
	virtual uint32_t getSizeBytesRegularCoveringFilterUpdates() const = 0;

	/**
	 * Get the size in bytes of the RCF base.
	 *
	 * @return
	 */
	virtual uint32_t getSizeBytesRegularCoveringFilterBase() const = 0;

	/**
	 * Publish the wildcard subscription patter statistics.
	 *
	 * @param stats
	 * @param sqn
	 * @return
	 */
	virtual int publishSubscriptionStats(
			const RemoteSubscriptionStats& stats, uint64_t* sqn) = 0;

	/*************************************************************************
	 * Consistent view, persistent remove
	 *************************************************************************/

    virtual int publishRemovedServers(const RemoteServerVector& servers, uint64_t* sqn) = 0;

    virtual int publishRestoredNotInView(const RemoteServerVector& servers, uint64_t* sqn) = 0;

	/* ***********************************************************************/
	/* Retained messages statistics                                          */
	/* ***********************************************************************/

	struct RetainedStatsValue
	{
		void *pData;
		uint32_t dataLength;
	};

	typedef std::map<std::string, RetainedStatsValue> RetainedStatsMap;

	/**
	 * Publish the retained message statistics.
	 *
	 * The sequence number is independent from the SCF sequence numbers.
	 *
	 * @param retainedStats
	 * @param sqn
	 *
	 * @return ISMRC_OK on success
	 */
	virtual int publishRetainedStats(
	        const RetainedStatsMap& retainedStats, uint64_t* sqn) = 0;

    /* ***********************************************************************/
    /* Monitoring                                                            */
    /* ***********************************************************************/

	/**
	 * Publish the monitoring status.
	 *
	 * @param healthStatus
	 * @param haStatus
	 * @param sqn
	 * @return
	 */
	virtual int publishMonitoringStatus(
	        ismCluster_HealthStatus_t  healthStatus,
	        ismCluster_HaStatus_t haStatus,
	        uint64_t* sqn) = 0;
};

typedef boost::shared_ptr<SubCoveringFilterPublisher> SubCoveringFilterPublisher_SPtr;

} /* namespace mcp */

#endif /* SUBCOVERINGFILTERPUBLISHER_H_ */
