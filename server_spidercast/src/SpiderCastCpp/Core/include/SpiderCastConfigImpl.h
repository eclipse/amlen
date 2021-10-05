// Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
/*
 * SpiderCastConfigImpl.h
 *
 *  Created on: 24/02/2010
 */

#ifndef SPIDERCASTCONFIGIMPL_H_
#define SPIDERCASTCONFIGIMPL_H_

#include "SpiderCastConfig.h"
#include "NodeIDImpl.h"
#include "BusName.h"

namespace spdr
{
using std::vector;
using std::pair;

/*
 * Undocumented configuration parameters
 */
namespace config
{
//=== Debug =========================================================

const String DebugFailFast_PROP_KEY =
		"spidercast.debug.FailFast";
const bool DebugFailFast_DEFVALUE = false;

//=== Messaging =====================================================
//=== RoutingProtocol
const String RoutingProtocol_PubSub_VALUE = "PubSub";
const String RoutingProtocol_Broadcast_VALUE = "Broadcast";

/**
 * The routing protocol the publisher is using.
 *
 * Optional property.
 *
 * Value: String.
 *
 * Default: PubSub.
 *
 * Supported: PubSub, Broadcast.
 */
const String PublisherRoutingProtocol_PROP_KEY =
		"spidercast.publisher.RoutingProtocol";
const String PublisherRoutingProtocol_DEFVALUE = RoutingProtocol_PubSub_VALUE;

}

class SpiderCastConfigImpl: public SpiderCastConfig
{

public:

	//down casting copy ctor
	SpiderCastConfigImpl(SpiderCastConfig& config)
			throw (IllegalConfigException);

	SpiderCastConfigImpl(const PropertyMap & properties, const std::vector<
			NodeID_SPtr>& bootstrapSet) throw (IllegalConfigException);

	SpiderCastConfigImpl(const PropertyMap & properties, const std::vector<
			NodeID_SPtr>& bootstrapSet,
			const std::vector<NodeID_SPtr>& supervisorBootstrapSet)
			throw (IllegalConfigException);

	virtual ~SpiderCastConfigImpl();

	bool getBindAllInterfaces() const
	{
		return bindAllInterfaces;
	}

	const String& getBusName() const
	{
		return busName;
	}

	BusName_SPtr getBusName_SPtr() const
	{
		return busName_SPtr;
	}

	const vector<pair<string, string> >& getNetworkAddresses() const
	{
		return networkAddresses;
	}

	const String& getBindNetworkAddress() const
	{
		return bindNetworkAddress;
	}

	const String& getNodeName() const
	{
		return nodeName;
	}

	uint16_t getTcpRceiverPort() const
	{
		return tcpRceiverPort;
	}

	uint16_t getBindTcpRceiverPort() const
	{
		return bindTcpRceiverPort;
	}

	int32_t getConnectionEstablishTimeoutMillis() const
	{
		return connectionEstablishTimeoutMillis;
	}

	int32_t getHeartbeatIntervalMillis() const
	{
		return heartbeatIntervalMillis;
	}

	int32_t getHeartbeatTimeoutMillis() const
	{
		return heartbeatTimeoutMillis;
	}

	int32_t getMaxMemoryAllowedMBytes() const
	{
		return maxMemoryAllowedMBytes;
	}

	bool isUseSSL() const
	{
		return useSSL;
	}

	int32_t getRumLogLevel() const
	{
		return rumLogLevel;
	}

	const string& getMulticastGroupAddressIPv4() const
	{
		return multicastGroupAddress_v4;
	}

	const string& getMulticastGroupAddressIPv6() const
	{
		return multicastGroupAddress_v6;
	}

	int32_t getMulticastPort() const
	{
		return multicastPort;
	}

	const string& getMulticastOutboundInterface() const
	{
		return multicastOutboundInterface;
	}

	int32_t getMulticastHops() const
	{
		return multicastHops;
	}

	int32_t getUDPPacketSizeBytes() const
	{
		return udpPacketSizeBytes;
	}

	int32_t getUdpReceiveBufferSizeBytes() const
	{
		return udpReceiveBufferSizeBytes;
	}

	int32_t getUdpSendBufferSizeBytes() const
	{
		return udpSendBufferSizeBytes;
	}

	const vector<NodeIDImpl_SPtr>& getBootstrapSet() const
	{
		return bootstrap_set;
	}

	const vector<NodeIDImpl_SPtr>& getSupervisorBootstrapSet() const
	{
		return supervisor_bootstrap_set;
	}

	NodeIDImpl_SPtr getMyNodeID() const
	{
		return myNodeID_SPtr;
	}

	int32_t getMembershipGossipIntervalMillis() const
	{
		return membershipGossipIntervalMillis;
	}

	int32_t getNodeHistoryRetentionTimeSec() const
	{
		return nodeHistoryRetentionTimeSec;
	}

	int32_t getSuspicionThreshold() const
	{
		return suspicionThreshold;
	}

	int32_t getFrequentDiscoveryIntervalMillis() const
	{
		return frequentDiscoveryIntervalMillis;
	}

	int32_t getFrequentDiscoveryMinimalDurationMillis() const
	{
		return frequentDiscoveryMinimalDurationMillis;
	}

	int32_t getNormalDiscoveryIntervalMillis() const
	{
		return normalDiscoveryIntervalMillis;
	}

	int32_t getRandomDegreeMargin() const
	{
		return randomDegreeMargin;
	}

	int32_t getRandomDegreeTarget() const
	{
		return randomDegreeTarget;
	}

	int32_t getStructDegreeTarget() const
	{
		return structDegreeTarget;
	}

	int32_t getTopologyPeriodicTaskIntervalMillis() const
	{
		return topologyPeriodicTaskIntervalMillis;
	}

	bool isStructTopoEnabled() const
	{
		return strucuTopoEnabled;
	}

	String getDiscoveryProtocol() const
	{
		return discoveryProtocol;
	}

	bool isTCPDiscovery() const
	{
		return tcpDiscovery;
	}

	bool isUDPDiscovery() const
	{
		return udpDiscovery;
	}

	bool isMulticastDiscovery() const
	{
		return multicastDiscovery;
	}

	//=== messaging ===
	String getPublisherReliabilityMode() const
	{
		return publisherReliabilityMode;
	}

	String getPublisherRoutingProtocol() const
	{
		return publisherRoutingProtocol;
	}

	static void validatePublisherRoutingProtocol(const String& publisherRP);

	bool isTopicGlobalScope()
	{
		return topicGlobalScope;
	}

	//=== Routing ===

	bool isRoutingEnabled() const
	{
		return routingEnabled;
	}

	//=== Leader election ===

	bool isLeaderElectionEnabled() const
	{
		return leaderElectionEnabled;
	}

	int32_t getLeaderElectionWarmupTimeoutMillis() const
	{
		return leaderElectionWarmupTimeoutMillis;
	}

	//=== hierarchy

	int32_t getNumberOfDelegates() const
	{
		return numberOfDelegates;
	}

	int32_t getNumberOfSupervisors() const
	{
		return numberOfSupervisors;
	}

	int32_t getNumberOfActiveDelegates() const
	{
		return numberOfActiveDelegates;
	}

	bool getIncludeAttributes() const
	{
		return includeAttributes;
	}

	int16_t getHierarchyMemberhipUpdateAggregationInterval() const
	{
		return hier_memberhipUpdateAggregationInterval;
	}

	int32_t getHierarchyQuarantineIntervalMillis() const
	{
		return hier_quarantineTimeMillis;
	}

	int32_t getHierarchyConnectIntervalMillis() const
	{
		return hier_connectIntervalMillis;
	}

	int16_t getHierarchyForeignZoneMemberhipTimeOut() const
	{
		return foreignZoneMembershipTimeOut;
	}

	bool isHierarchyEnabled() const
	{
		return hierarchyEnabled;
	}

	//=== stats

	bool isStatisticsEnabled() const
	{
		return statisticsEnabled;
	}

	int32_t getStatisticsPeriodMillis() const
	{
		return statisticsPeriodMillis;
	}

	int32_t getStatisticsTaskTardinessThresholdMillis() const
	{
		return statisticsTaskTardinessThresholdMillis;
	}

	bool isCRCMemTopoMsgEnabled() const
	{
		return crcMemTopoMsgEnabled;
	}

	bool isFullViewBootstrapSet() const
	{
		return fullViewBootstrapSet;
	}

	const String & getFullViewBootstrapSetPolicy() const
	{
		return fullViewBootstrapSetPolicy;
	}

	const int32_t getFullViewBootstrapTimeoutSec() const
	{
		return fullViewBootstrapTimeoutSec;
	}

	const int32_t getFullViewBootstrapHierarchyDegree() const
	{
		return fullViewBootstrapHierarchyDegree;
	}

	bool isHighPriorityMonitoringEnabled() const
	{
		return highPriorityMonitoringEnabled;
	}

	bool isRetainAttributesOnSuspectNodesEnabled() const
	{
		return retainAttributesOnSuspectNodesEnabled;
	}

	const bool isDebugFailFastEnabled() const
	{
		return debugFailFast;
	}

	virtual PropertyMap getPropertyMap() const;

	virtual std::vector<NodeID_SPtr > getBootStrapSet() const;

	virtual String toString() const;

	virtual bool verifyBindNetworkInterface(std::string& errMsg, int& errCode);

	static void validateNodeName(const std::string& name, bool allowAny = false) throw (IllegalConfigException);

	int64_t getChooseIncarnationNumberHigherThen() const
	{
		return chooseIncarnationNumberHigherThen;
	}

	int64_t getForceIncarnationNumber() const
	{
		return forceIncarnationNumber;
	}

private:
	//NOTE: remember to update copy-constructor when adding fields.

	String nodeName;
	String busName;
	BusName_SPtr busName_SPtr;

	int64_t chooseIncarnationNumberHigherThen;
	int64_t forceIncarnationNumber;

	//=== Comm ===
	vector<pair<string, string> > networkAddresses;
	uint16_t tcpRceiverPort;
	String bindNetworkAddress;
	uint16_t bindTcpRceiverPort;
	bool bindAllInterfaces;

	int32_t heartbeatIntervalMillis;
	int32_t heartbeatTimeoutMillis;
	int32_t connectionEstablishTimeoutMillis;
	int32_t maxMemoryAllowedMBytes;
	bool useSSL;
	int32_t rumLogLevel;
	string 	multicastGroupAddress_v4;
	string 	multicastGroupAddress_v6;
	int32_t multicastPort;
	string  multicastOutboundInterface;
	int32_t multicastHops;
	int32_t udpPacketSizeBytes;
	int32_t udpSendBufferSizeBytes;
	int32_t udpReceiveBufferSizeBytes;
	bool crcMemTopoMsgEnabled;

	//=== Membership ===
	int32_t membershipGossipIntervalMillis;
	int32_t nodeHistoryRetentionTimeSec;
	int32_t suspicionThreshold;
	bool fullViewBootstrapSet;
	String fullViewBootstrapSetPolicy;
	int32_t fullViewBootstrapHierarchyDegree;
	int32_t fullViewBootstrapTimeoutSec;
	bool highPriorityMonitoringEnabled;
	bool retainAttributesOnSuspectNodesEnabled;

	vector<NodeIDImpl_SPtr> bootstrap_set;
	NodeIDImpl_SPtr myNodeID_SPtr;

	//=== Hierarchy ===
	vector<NodeIDImpl_SPtr> supervisor_bootstrap_set;
	int32_t numberOfDelegates;
	int32_t numberOfSupervisors;
	int32_t numberOfActiveDelegates;
	bool includeAttributes;
	bool hierarchyEnabled;
	int32_t foreignZoneMembershipTimeOut;

	int16_t hier_memberhipUpdateAggregationInterval;
	int32_t hier_quarantineTimeMillis; //= 10000; //TODO define a config parameter
	int32_t hier_connectIntervalMillis; //= 200;//1000; //TODO define a config parameter

	//=== Topology ===
	int32_t topologyPeriodicTaskIntervalMillis;
	int32_t frequentDiscoveryIntervalMillis;
	int32_t frequentDiscoveryMinimalDurationMillis;
	int32_t normalDiscoveryIntervalMillis;
	String discoveryProtocol;
	bool tcpDiscovery;
	bool udpDiscovery;
	bool multicastDiscovery;
	int32_t randomDegreeTarget;
	int32_t randomDegreeMargin;
	bool strucuTopoEnabled;
	int32_t structDegreeTarget;

	//=== Messaging ===

	String publisherReliabilityMode;
	String publisherRoutingProtocol;
	bool topicGlobalScope;

	//=== Routing ===

	bool routingEnabled;

	//=== Leader Election ===

	bool leaderElectionEnabled;
	int32_t leaderElectionWarmupTimeoutMillis;

	//=== Statistics ===
	bool statisticsEnabled;
	int32_t statisticsPeriodMillis;
	int32_t statisticsTaskTardinessThresholdMillis;

	//=== Debug ===

	bool debugFailFast;


	//methods
	vector<pair<String, String> >
	parseNetworkInterface(const String& interface)
			throw (IllegalConfigException);

	void parsePolicy(const String& policy_tmp,
			String& fullViewBootstrapSetPolicy,
			int32_t& fullViewBootstrapHierarchyDegree);

	void copyBootstrap(const std::vector<NodeID_SPtr>& in, std::vector<
			NodeIDImpl_SPtr>& out) throw (IllegalConfigException);

	void initProperties() throw (IllegalConfigException);
};

} //namespace spdr
#endif /* SPIDERCASTCONFIGIMPL_H_ */
