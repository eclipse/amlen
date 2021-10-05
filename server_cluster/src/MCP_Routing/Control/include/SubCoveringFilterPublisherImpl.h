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

#ifndef MCP_SUBCOVERINGFILTERPUBLISHERIMPL_H_
#define MCP_SUBCOVERINGFILTERPUBLISHERIMPL_H_

#include <string>
#include <map>
#include <set>

//#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "MembershipService.h"

#include "ByteBuffer.h"
#include "SubCoveringFilterPublisher.h"
#include "FilterTags.h"
#include "MCPConfig.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

class SubCoveringFilterPublisherImpl: public SubCoveringFilterPublisher, protected FilterTags, public spdr::ScTraceContext
{
private:
	static spdr::ScTraceComponent* tc_;

public:
	SubCoveringFilterPublisherImpl(const std::string& inst_ID, spdr::MembershipService& memService);
	virtual ~SubCoveringFilterPublisherImpl();

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual uint64_t publishBloomFilterBase(
			const std::string& tag,
			const mcc_hash_HashType_t bfType,
			const int16_t numHash,
			const int32_t numBins,
			const char* buffer);

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual uint64_t publishBloomFilterUpdate(const std::string& tag,
			const std::vector<int32_t>& binUpdates);

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual uint64_t removeBloomFilter(const std::string& tag);


	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual uint32_t getNumBloomFilterUpdates(const std::string& tag) const;

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual uint32_t getSizeBytesBloomFilterUpdates(const std::string& tag) const;

	virtual uint32_t getSizeBytesBloomFilterBase(const std::string& tag) const;

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual int publishWCSubscriptionPatternBase(
			const std::vector<SubscriptionPatternUpdate>& subPatternBase,
			uint64_t* sqn);

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual int publishWCSubscriptionPatternUpdate(
			const std::vector<SubscriptionPatternUpdate>& subPatternUpdate,
			uint64_t* sqn);

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual uint32_t getNumWCSubscriptionPatternUpdates() const;

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual uint32_t getSizeBytesWCSubscriptionPatternUpdates() const;

	virtual uint32_t getSizeBytesWCSubscriptionPatternBase() const;

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual int publishSubscriptionStats(
			const RemoteSubscriptionStats& stats,
			uint64_t* sqn);

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual int publishRegularCoveringFilterBase(
			const std::vector<RegularCoveringFilterUpdate>& rcfBase,
			uint64_t* sqn);

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual int publishRegularCoveringFilterUpdate(
			const std::vector<RegularCoveringFilterUpdate>& rcfUpdate,
			uint64_t* sqn);

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual uint32_t getNumRegularCoveringFilterUpdates() const;

	/*
	 * @see SubCoveringFilterPublisher
	 */
	virtual uint32_t getSizeBytesRegularCoveringFilterUpdates() const;

	virtual uint32_t getSizeBytesRegularCoveringFilterBase() const;

	void publishForwardingAddress(const std::string& fwdAddr, int fwdPort, uint8_t fUseTLS);

	void publishLocalServerInfo(const std::string& serverName);

	virtual int publishRetainedStats(
	        const RetainedStatsMap& retainedStats, uint64_t* sqn);

    virtual int publishMonitoringStatus(
            ismCluster_HealthStatus_t  healthStatus,
            ismCluster_HaStatus_t haStatus,
            uint64_t* sqn);

    virtual int publishRemovedServers(
            const RemoteServerVector& servers, uint64_t* sqn);

    virtual int publishRestoredNotInView(
            const RemoteServerVector& servers, uint64_t* sqn);


private:
	spdr::MembershipService& membershipService;
	mutable boost::mutex mutex;
	mcp::ByteBuffer_SPtr byteBuffer;
	std::set<std::string> permitted_BF_Tags;
	uint64_t sqn_;
	uint64_t sqn_retained_stats_;
	uint64_t sqn_monitoring_status_;

	uint64_t sqn_removed_servers_;

	uint64_t sqn_restored_notin_view_;

	struct SqnInfo
	{
		uint64_t base;
		uint64_t last_update;
		uint32_t num_updates;
		uint32_t updates_size_bytes;
		uint32_t base_size_bytes;

		SqnInfo() : base(0), last_update(0), num_updates(0), updates_size_bytes(0), base_size_bytes(0) {}
	};

	/* BF:
	 * A map from tag to <first,last> sequence numbers.
	 * For BF the first is the base number, the second is the last update published.
	 * If first==second there was no update.
	 * The first sequence number is 1'.
	 */
	typedef std::map<std::string, SqnInfo > BFTagInfo_Map;
	BFTagInfo_Map bfTagInfoMap;

	/* WC-P: Wild-card patterns
	 * A map from the index to the sequence number
	 */
	typedef std::map<uint32_t,uint64_t> WCPatternInfo_Map;
	WCPatternInfo_Map wc_patternInfo_map;
	//uint64_t wc_pattern_sqn;

	SqnInfo wcspSqnInfo_;

	SqnInfo rcfSqnInfo_;


};

} /* namespace mcp */

#endif /* MCP_SUBCOVERINGFILTERPUBLISHERIMPL_H_ */
