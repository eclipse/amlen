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
 * SpiderCastConfigImpl.cpp
 *
 *  Created on: 24/02/2010
 */

#include <iostream>
#include <locale>
#include <vector>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>

#include "SpiderCastConfigImpl.h"
#include "CommUtils.h"

namespace spdr
{
//using std::cout;
//using std::endl;


SpiderCastConfigImpl::SpiderCastConfigImpl(SpiderCastConfig& config)
		throw (IllegalConfigException) :
	SpiderCastConfig(config)
{
	//cout << "SpiderCastConfigImpl(SpiderCastConfig&) >" << endl;
	try
	{
		SpiderCastConfigImpl& configImpl =
				dynamic_cast<SpiderCastConfigImpl&> (config);
		//copy properties
		nodeName = configImpl.nodeName;
		busName = configImpl.busName;
		busName_SPtr = BusName_SPtr(new BusName(configImpl.busName.c_str()));

		chooseIncarnationNumberHigherThen = configImpl.chooseIncarnationNumberHigherThen;
		forceIncarnationNumber = configImpl.forceIncarnationNumber;

		networkAddresses = configImpl.networkAddresses;
		tcpRceiverPort = configImpl.tcpRceiverPort;
		bindAllInterfaces = configImpl.bindAllInterfaces;
		bindNetworkAddress = configImpl.bindNetworkAddress;
		bindTcpRceiverPort = configImpl.bindTcpRceiverPort;

		heartbeatIntervalMillis = configImpl.heartbeatIntervalMillis;
		heartbeatTimeoutMillis = configImpl.heartbeatTimeoutMillis;
		connectionEstablishTimeoutMillis
				= configImpl.connectionEstablishTimeoutMillis;
		maxMemoryAllowedMBytes = configImpl.maxMemoryAllowedMBytes;
		useSSL = configImpl.useSSL;
		rumLogLevel = configImpl.rumLogLevel;

		multicastGroupAddress_v4 = configImpl.multicastGroupAddress_v4;
		multicastGroupAddress_v6 = configImpl.multicastGroupAddress_v6;
		multicastPort = configImpl.multicastPort;
		multicastOutboundInterface = configImpl.multicastOutboundInterface;
		multicastHops = configImpl.multicastHops;

		udpPacketSizeBytes = configImpl.udpPacketSizeBytes;
		udpSendBufferSizeBytes = configImpl.udpSendBufferSizeBytes;
		udpReceiveBufferSizeBytes = configImpl.udpReceiveBufferSizeBytes;

		crcMemTopoMsgEnabled = configImpl.crcMemTopoMsgEnabled;

		membershipGossipIntervalMillis
				= configImpl.membershipGossipIntervalMillis;
		nodeHistoryRetentionTimeSec = configImpl.nodeHistoryRetentionTimeSec;
		suspicionThreshold = configImpl.suspicionThreshold;
		fullViewBootstrapSet = configImpl.fullViewBootstrapSet;
		fullViewBootstrapSetPolicy = configImpl.fullViewBootstrapSetPolicy;
		fullViewBootstrapHierarchyDegree = configImpl.fullViewBootstrapHierarchyDegree;
		fullViewBootstrapTimeoutSec = configImpl.fullViewBootstrapTimeoutSec;
		highPriorityMonitoringEnabled = configImpl.highPriorityMonitoringEnabled;
		retainAttributesOnSuspectNodesEnabled = configImpl.retainAttributesOnSuspectNodesEnabled;

		topologyPeriodicTaskIntervalMillis
				= configImpl.topologyPeriodicTaskIntervalMillis;
		frequentDiscoveryIntervalMillis
				= configImpl.frequentDiscoveryIntervalMillis;
		frequentDiscoveryMinimalDurationMillis
				= configImpl.frequentDiscoveryMinimalDurationMillis;
		discoveryProtocol = configImpl.discoveryProtocol;
		tcpDiscovery = configImpl.tcpDiscovery;
		udpDiscovery = configImpl.udpDiscovery;
		multicastDiscovery = configImpl.multicastDiscovery;
		normalDiscoveryIntervalMillis
				= configImpl.normalDiscoveryIntervalMillis;
		randomDegreeTarget = configImpl.randomDegreeTarget;
		randomDegreeMargin = configImpl.randomDegreeMargin;
		strucuTopoEnabled = configImpl.strucuTopoEnabled;
		structDegreeTarget = configImpl.structDegreeTarget;

		publisherReliabilityMode = configImpl.publisherReliabilityMode;
		publisherRoutingProtocol = configImpl.publisherRoutingProtocol;
		topicGlobalScope = configImpl.topicGlobalScope;

		routingEnabled = configImpl.routingEnabled;

		leaderElectionEnabled = configImpl.leaderElectionEnabled;
		leaderElectionWarmupTimeoutMillis = configImpl.leaderElectionWarmupTimeoutMillis;

		numberOfDelegates = configImpl.numberOfDelegates;
		numberOfSupervisors = configImpl.numberOfSupervisors;
		numberOfActiveDelegates = configImpl.numberOfActiveDelegates;
		includeAttributes = configImpl.includeAttributes;
		hier_memberhipUpdateAggregationInterval
				= configImpl.hier_memberhipUpdateAggregationInterval;
		hier_connectIntervalMillis = configImpl.hier_connectIntervalMillis;
		hier_quarantineTimeMillis = configImpl.hier_quarantineTimeMillis;
		hierarchyEnabled = configImpl.hierarchyEnabled;
		foreignZoneMembershipTimeOut = configImpl.foreignZoneMembershipTimeOut;

		statisticsEnabled = configImpl.statisticsEnabled;
		statisticsPeriodMillis = configImpl.statisticsPeriodMillis;
		statisticsTaskTardinessThresholdMillis
				= configImpl.statisticsTaskTardinessThresholdMillis;

		debugFailFast = configImpl.debugFailFast;

		//copy bs-set
		bootstrap_set = configImpl.bootstrap_set;
		supervisor_bootstrap_set = configImpl.supervisor_bootstrap_set;
		myNodeID_SPtr = configImpl.myNodeID_SPtr;

	} catch (std::bad_cast& e)
	{
		String what(e.what());
		what += "; Always create SpiderCastConfig using factory.";
		throw IllegalConfigException(what);
	}

	//cout << "SpiderCastConfigImpl(SpiderCastConfig&) <" << endl;
}

SpiderCastConfigImpl::SpiderCastConfigImpl(const PropertyMap& properties,
		const std::vector<NodeID_SPtr>& bootstrapSet)
		throw (IllegalConfigException) :
	SpiderCastConfig(properties),
	networkAddresses()
{
	copyBootstrap(bootstrapSet, bootstrap_set);

	initProperties();

	NetworkEndpoints ne(getNetworkAddresses(), getTcpRceiverPort());
	myNodeID_SPtr = NodeIDImpl_SPtr(new NodeIDImpl(getNodeName(), ne));
}

SpiderCastConfigImpl::SpiderCastConfigImpl(const PropertyMap& properties,
		const std::vector<NodeID_SPtr>& bootstrapSet,
		const std::vector<NodeID_SPtr>& supervisorBootstrapSet)
		throw (IllegalConfigException) :
	SpiderCastConfig(properties),
	networkAddresses()
{
	copyBootstrap(bootstrapSet, bootstrap_set);
	copyBootstrap(supervisorBootstrapSet, supervisor_bootstrap_set);

	initProperties();

	NetworkEndpoints ne(getNetworkAddresses(), getTcpRceiverPort());
	myNodeID_SPtr = NodeIDImpl_SPtr(new NodeIDImpl(getNodeName(), ne));
}

void SpiderCastConfigImpl::copyBootstrap(const std::vector<NodeID_SPtr>& in,
		std::vector<NodeIDImpl_SPtr>& out) throw (IllegalConfigException)
{
	std::vector<NodeID_SPtr>::const_iterator pos;
	for (pos = in.begin(); pos != in.end(); ++pos)
	{
		if (*pos)
		{
			const NodeIDImpl& id = dynamic_cast<const NodeIDImpl&> (*(*pos));
			NodeIDImpl_SPtr id_impl_SPtr = NodeIDImpl_SPtr(new NodeIDImpl(id));
			if (id_impl_SPtr)
			{
				out.push_back(id_impl_SPtr);
			}
			else
			{
				String what("NodeID cannot be converted to NodeIDImpl, create NodeID using factory");
				throw IllegalConfigException(what);
			}
		}
	}
}

void SpiderCastConfigImpl::initProperties() throw (IllegalConfigException)
{
	using namespace std;

	nodeName = getMandatoryProperty(config::NodeName_PROP_KEY);
	validateNodeName(nodeName);

	busName = getMandatoryProperty(config::BusName_PROP_KEY);
	try
	{
		busName_SPtr = BusName_SPtr(new BusName(busName.c_str())); //enforce naming conventions

	}
	catch (IllegalArgumentException& ex)
	{
		string what(config::BusName_PROP_KEY);
		what.append(" illegal name: ").append(ex.what());
		throw IllegalConfigException(what);
	}

	if (busName_SPtr->getLevel() < 1 || busName_SPtr->getLevel() > 2) //and number of levels
	{
		string what(config::BusName_PROP_KEY);
		what.append(" illegal name - only two levels are supported: ").append(
				busName);
		throw IllegalConfigException(what);
	}

	chooseIncarnationNumberHigherThen = std::max(-1L, getOptionalInt64Property(
			config::ChooseIncarnationNumberHigherThan_PROP_KEY,
			config::ChooseIncarnationNumberHigherThan_DEFVALUE));
	forceIncarnationNumber = std::max(-1L, getOptionalInt64Property(
			config::ForceIncarnationNumber_PROP_KEY,
			config::ForceIncarnationNumber_DEFVALUE));
	if (chooseIncarnationNumberHigherThen >= 0 && forceIncarnationNumber >= 0)
	{
		ostringstream what;
		what << "Both " << config::ChooseIncarnationNumberHigherThan_PROP_KEY
				<< " >=0 (" << chooseIncarnationNumberHigherThen << "), AND "
				<< config::ForceIncarnationNumber_PROP_KEY << " >=0 ("
				<< forceIncarnationNumber << "); is forbidden.";
		throw IllegalConfigException(what.str());
	}
	prop.setProperty(config::ChooseIncarnationNumberHigherThan_PROP_KEY,
			boost::lexical_cast<string>(chooseIncarnationNumberHigherThen));
	prop.setProperty(config::ForceIncarnationNumber_PROP_KEY,
			boost::lexical_cast<string>(forceIncarnationNumber));

	//=== Comm ===
	String networkInterface = getMandatoryProperty(
			config::NetworkInterface_PROP_KEY);

	networkAddresses = parseNetworkInterface(networkInterface);

	bindNetworkAddress = getOptionalProperty(config::BindNetworkInterface_PROP_KEY,"");

//	if (!bindNetworkAddress.empty())
//	{
//		CommUtils::NICInfo nic_info;
//		int errCode = 0;
//		std::string errMsg;
//
//		bool fail = ( !CommUtils::get_nic_info(bindNetworkAddress.c_str(), &nic_info, &errCode, &errMsg) || nic_info.index == 0);
//		if (fail)
//		{
//			sleep(5);
//			fail = ( !CommUtils::get_nic_info(bindNetworkAddress.c_str(), &nic_info, &errCode, &errMsg) || nic_info.index == 0);
//			if (fail)
//			{
//				ostringstream what;
//				what << config::BindNetworkInterface_PROP_KEY  << " cannot be associated with a network interface (NIC), " << bindNetworkAddress
//						<< "; error code=" << errCode << ", error message=" << errMsg;
//				throw IllegalConfigException(what.str());
//			}
//		}
//	}

	//	try
	//	{
	//		if (!bindNetworkAddress.empty())
	//			boost::asio::ip::address::from_string(bindNetworkAddress);
	//	}
	//	catch (std::exception& e)
	//	{
	//		ostringstream what;
	//		what << config::BindNetworkInterface_PROP_KEY  << " cannot be converted to an IP address, " << bindNetworkAddress ;
	//		throw IllegalConfigException(what.str());
	//	}
	prop.setProperty(config::BindNetworkInterface_PROP_KEY,bindNetworkAddress);

	int32_t port = getMandatoryInt32Property(config::TCPReceiverPort_PROP_KEY);
	if (port >= (64*1024) || port < 1)
	{
		String what(config::TCPReceiverPort_PROP_KEY + " out of range [1,65535]: " + boost::lexical_cast<String>(port));
		throw IllegalConfigException(what);
	}
	else
	{
		tcpRceiverPort = static_cast<uint16_t>(port);
	}

	port = getOptionalInt32Property(
			config::BindTCPReceiverPort_PROP_KEY,tcpRceiverPort);
	if (port >= (64*1024) || port < 1)
	{
		String what(config::BindTCPReceiverPort_PROP_KEY + " out of range [1,65535]: " + boost::lexical_cast<String>(port));
		throw IllegalConfigException(what);
	}
	else
	{
		bindTcpRceiverPort = static_cast<uint16_t>(port);
	}
	prop.setProperty(config::BindTCPReceiverPort_PROP_KEY,
			boost::lexical_cast<String>(port));


	if (bindNetworkAddress.empty())
	{
		bindAllInterfaces = true;
	}
	else
	{
		bindAllInterfaces = getOptionalBooleanProperty(
				config::BindAllInterfaces_PROP_KEY,
				config::BindAllInterfaces_DEFVALUE);
	}
	prop.setProperty(config::BindAllInterfaces_PROP_KEY,
			(bindAllInterfaces ? "true" : "false"));

	heartbeatIntervalMillis = getOptionalInt32Property(
			config::HeartbeatIntervalMillis_PROP_KEY,
			config::HeartbeatIntervalMillis_DEFVALUE);
	if (heartbeatIntervalMillis <= 0)
	{
		String what(config::HeartbeatIntervalMillis_PROP_KEY);
		what.append(" must be >0");
		throw IllegalConfigException(what);
	}

	{
		ostringstream oss;
		oss << heartbeatIntervalMillis;
		prop.setProperty(config::HeartbeatIntervalMillis_PROP_KEY, oss.str());
	}

	heartbeatTimeoutMillis = getOptionalInt32Property(
			config::HeartbeatTimeoutMillis_PROP_KEY,
			config::HeartbeatTimeoutMillis_DEFVALUE);
	if (heartbeatTimeoutMillis <= heartbeatIntervalMillis)
	{
		String what(config::HeartbeatTimeoutMillis_PROP_KEY);
		what.append(" must be > ");
		what.append(config::HeartbeatIntervalMillis_PROP_KEY);
		throw IllegalConfigException(what);
	}

	{
		ostringstream oss;
		oss << heartbeatTimeoutMillis;
		prop.setProperty(config::HeartbeatTimeoutMillis_PROP_KEY, oss.str());
	}

	connectionEstablishTimeoutMillis = getOptionalInt32Property(
			config::ConnectionEstablishTimeoutMillis_PROP_KEY,
			config::ConnectionEstablishTimeoutMillis_DEFVALUE);
	if (connectionEstablishTimeoutMillis < 0)
	{
		String what(config::ConnectionEstablishTimeoutMillis_PROP_KEY);
		what.append(" must be >=0");
		throw IllegalConfigException(what);
	}

	{
		ostringstream oss;
		oss << connectionEstablishTimeoutMillis;
		prop.setProperty(config::ConnectionEstablishTimeoutMillis_PROP_KEY,
				oss.str());
	}

	maxMemoryAllowedMBytes = getOptionalInt32Property(
			config::MaxMemoryAllowedMBytes_PROP_KEY,
			config::MaxMemoryAllowedMBytes_DEFVALUE);
	{
		ostringstream oss;
		oss << maxMemoryAllowedMBytes;
		prop.setProperty(config::MaxMemoryAllowedMBytes_PROP_KEY, oss.str());
	}
	if (maxMemoryAllowedMBytes < 20 || maxMemoryAllowedMBytes > 4000)
	{
		String what(config::MaxMemoryAllowedMBytes_PROP_KEY);
		what.append(" out of range [20,4000]");
		throw IllegalConfigException(what);
	}

	useSSL = getOptionalBooleanProperty(config::UseSSL_PROP_KEY, config::UseSSL_DEFVALUE);
	prop.setProperty(config::UseSSL_PROP_KEY, (useSSL ? "true" : "false"));

	rumLogLevel = getOptionalInt32Property(config::RUMLogLevel_PROP_KEY,
			config::RUMLogLevel_DEFVALUE);
	{
		ostringstream oss;
		oss << rumLogLevel;
		prop.setProperty(config::RUMLogLevel_PROP_KEY, oss.str());
	}
	if (rumLogLevel < (-1) || rumLogLevel > 9)
	{
		String what(config::RUMLogLevel_PROP_KEY);
		what.append(" out of range [-1,9]");
		throw IllegalConfigException(what);
	}

	multicastGroupAddress_v4 = getOptionalProperty(
			config::DiscoveryMulticastGroupAddressIPv4_PROP_KEY,
			config::DiscoveryMulticastGroupAddressIPv4_DEFVALUE);
	{
		try
		{
			boost::asio::ip::address_v4 mcg_v4 = boost::asio::ip::address_v4::from_string(multicastGroupAddress_v4);
			if (!mcg_v4.is_multicast())
			{
				ostringstream what;
				what << config::DiscoveryMulticastGroupAddressIPv4_PROP_KEY  << " is not an IPv4 multicast address, " << multicastGroupAddress_v4;
				throw IllegalConfigException(what.str());
			}
		}
		catch (exception& e)
		{
			ostringstream what;
			what << config::DiscoveryMulticastGroupAddressIPv4_PROP_KEY  << " cannot be converted to an IPv4 address, " << multicastGroupAddress_v4;
			throw IllegalConfigException(what.str());
		}

		prop.setProperty(config::DiscoveryMulticastGroupAddressIPv4_PROP_KEY, multicastGroupAddress_v4);
	}

	multicastGroupAddress_v6 = getOptionalProperty(
			config::DiscoveryMulticastGroupAddressIPv6_PROP_KEY,
			config::DiscoveryMulticastGroupAddressIPv6_DEFVALUE);
	{
		try
		{
			boost::asio::ip::address_v6 mcg_v6 = boost::asio::ip::address_v6::from_string(multicastGroupAddress_v6);
			if (!mcg_v6.is_multicast())
			{
				ostringstream what;
				what << config::DiscoveryMulticastGroupAddressIPv6_PROP_KEY  << " is not an IPv6 multicast address, " << multicastGroupAddress_v6;
				throw IllegalConfigException(what.str());
			}
		}
		catch (exception& e)
		{
			ostringstream what;
			what << config::DiscoveryMulticastGroupAddressIPv6_PROP_KEY  << " cannot be converted to an IPv6 address, " << multicastGroupAddress_v6;
			throw IllegalConfigException(what.str());
		}

		prop.setProperty(config::DiscoveryMulticastGroupAddressIPv6_PROP_KEY, multicastGroupAddress_v6);
	}

	multicastPort = getOptionalInt32Property(
			config::DiscoveryMulticastPort_PROP_KEY,
			config::DiscoveryMulticastPort_DEFVALUE);
	if (multicastPort < 1 || multicastPort > 65535)
	{
		ostringstream what;
		what << config::DiscoveryMulticastPort_PROP_KEY  << " out of range [1,65535]: " << multicastPort ;
		throw IllegalConfigException(what.str());
	}
	prop.setProperty(config::DiscoveryMulticastPort_PROP_KEY, boost::lexical_cast<std::string>(multicastPort));

	multicastOutboundInterface = getOptionalProperty(
			config::DiscoveryMulticastInOutInterface_PROP_KEY,
			config::DiscoveryMulticastInOutInterface_DEFVALUE);
//	if (!multicastOutboundInterface.empty())
//	{
//		CommUtils::NICInfo nic_info;
//		int errCode = 0;
//		std::string errMsg;
//		if( !CommUtils::get_nic_info(multicastOutboundInterface.c_str(), &nic_info, &errCode, &errMsg) || nic_info.index == 0)
//		{
//			ostringstream what;
//			what << config::DiscoveryMulticastInOutInterface_PROP_KEY  << " does not identify an interface on the local machine: " << multicastOutboundInterface
//					<< "; error code=" << errCode << ", error message=" << errMsg;
//			throw IllegalConfigException(what.str());
//
//		}
//	}
	prop.setProperty(config::DiscoveryMulticastInOutInterface_PROP_KEY, multicastOutboundInterface);

	multicastHops = getOptionalInt32Property(
			config::DiscoveryMulticastHops_PROP_KEY,
			config::DiscoveryMulticastHops_DEFVALUE);
	if (multicastHops < 0 || multicastHops > 255)
	{
		String what(config::DiscoveryMulticastHops_PROP_KEY);
		what.append(" out of range [0,255]");
		throw IllegalConfigException(what);
	}
	prop.setProperty(config::DiscoveryMulticastHops_PROP_KEY,
			boost::lexical_cast<std::string>(multicastHops));

	udpPacketSizeBytes = getOptionalInt32Property(
			config::UDPPacketSizeBytes_PROP_KEY,
			config::UDPPacketSizeBytes_DEFVALUE);
	if (udpPacketSizeBytes<512 || udpPacketSizeBytes>=(64*1024))
	{
		String what(config::UDPPacketSizeBytes_PROP_KEY);
		what.append(" out of range [512,65535]");
		throw IllegalConfigException(what);
	}
	prop.setProperty(config::UDPPacketSizeBytes_PROP_KEY,
			boost::lexical_cast<std::string>(udpPacketSizeBytes));

	udpReceiveBufferSizeBytes = getOptionalInt32Property(
			config::UDPReceiveBufferSizeBytes_PROP_KEY,
			config::UDPReceiveBufferSizeBytes_DEFVALUE);
	if (udpReceiveBufferSizeBytes<(64*1024))
	{
		String what(config::UDPReceiveBufferSizeBytes_PROP_KEY);
		what.append(" too small (must be >=65534)");
		throw IllegalConfigException(what);
	}
	prop.setProperty(config::UDPReceiveBufferSizeBytes_PROP_KEY,
			boost::lexical_cast<std::string>(udpReceiveBufferSizeBytes));

	udpSendBufferSizeBytes = getOptionalInt32Property(
			config::UDPSendBufferSizeBytes_PROP_KEY,
			config::UDPSendBufferSizeBytes_DEFVALUE);
	if (udpSendBufferSizeBytes<(64*1024))
	{
		String what(config::UDPSendBufferSizeBytes_PROP_KEY);
		what.append(" too small (must be >=65534)");
		throw IllegalConfigException(what);
	}
	prop.setProperty(config::UDPSendBufferSizeBytes_PROP_KEY,
			boost::lexical_cast<std::string>(udpSendBufferSizeBytes));


	crcMemTopoMsgEnabled = getOptionalBooleanProperty(
			config::CRC_MemTopoMsg_Enabled_PROP_KEY,
			config::CRC_MemTopoMsg_Enabled_DEFVALUE);
	prop.setProperty(config::CRC_MemTopoMsg_Enabled_PROP_KEY,
			(crcMemTopoMsgEnabled ? "true" : "false"));

	//=== Mem ===
	membershipGossipIntervalMillis = getOptionalInt32Property(
			config::MembershipGossipIntervalMillis_PROP_KEY,
			config::MembershipGossipIntervalMillis_DEFVALUE);
	{
		ostringstream oss;
		oss << membershipGossipIntervalMillis;
		prop.setProperty(config::MembershipGossipIntervalMillis_PROP_KEY,
				oss.str());
	}

	nodeHistoryRetentionTimeSec = getOptionalInt32Property(
			config::NodeHistoryRetentionTimeSec_PROP_KEY,
			config::NodeHistoryRetentionTimeSec_DEFVALUE);
	if (nodeHistoryRetentionTimeSec < 2)
	{
		String what(config::NodeHistoryRetentionTimeSec_PROP_KEY);
		what.append(" must be >2");
		throw IllegalConfigException(what);
	}
	{
		ostringstream oss;
		oss << nodeHistoryRetentionTimeSec;
		prop.setProperty(config::NodeHistoryRetentionTimeSec_PROP_KEY,
				oss.str());
	}

	suspicionThreshold = getOptionalInt32Property(
			config::SuspicionThreshold_PROP_KEY,
			config::SuspicionThreshold_DEFVALUE);
	if (suspicionThreshold < 1)
	{
		String what(config::SuspicionThreshold_PROP_KEY);
		what.append(" must be >=1");
		throw IllegalConfigException(what);
	}
	else if (suspicionThreshold > 1) //TODO change this when we have a successor list
	{
		String what(config::SuspicionThreshold_PROP_KEY);
		what.append(" must be <= 1");
		throw IllegalConfigException(what);
	}
	{
		ostringstream oss;
		oss << suspicionThreshold;
		prop.setProperty(config::SuspicionThreshold_PROP_KEY, oss.str());
	}

	fullViewBootstrapSet = getOptionalBooleanProperty(
			config::FullViewBootstrapSet_PROP_KEY,
			config::FullViewBootstrapSet_DEFVALUE);

	prop.setProperty(config::FullViewBootstrapSet_PROP_KEY,
			(fullViewBootstrapSet ? "true" : "false"));

//	String policy_tmp = getOptionalProperty(
//			config::FullViewBootstrapPolicy_PROP_KEY,
//			config::FullViewBootstrapPolicy_DEFVALUE);
//	parsePolicy(policy_tmp, fullViewBootstrapSetPolicy, fullViewBootstrapHierarchyDegree);
//
//	if (fullViewBootstrapSetPolicy == config::FullViewBootstrapPolicy_Random_VALUE
//			//||	fullViewBootstrapSetPolicy == config::FullViewBootstrapPolicy_Tree_VALUE
//			)
//	{
//		prop.setProperty(config::FullViewBootstrapPolicy_PROP_KEY, fullViewBootstrapSetPolicy);
//	}
//	else
//	{
//		String what(config::FullViewBootstrapPolicy_PROP_KEY);
//		what.append(" must be: ");
//		what.append(config::FullViewBootstrapPolicy_Random_VALUE);
//		what.append(" | ");
//		what.append(config::FullViewBootstrapPolicy_Tree_VALUE);
//		throw IllegalConfigException(what);
//	}

//	fullViewBootstrapTimeoutSec = getOptionalInt32Property(
//			config::FullViewBootstrapTimeoutSec_PROP_KEY,
//			config::FullViewBootstrapTimeoutSec_DEFVALUE);
//	if (fullViewBootstrapTimeoutSec < 0)
//	{
//		String what(config::FullViewBootstrapTimeoutSec_PROP_KEY);
//		what.append(" must be >= 0");
//		throw IllegalConfigException(what);
//	}
//	else
//	{
//		ostringstream oss;
//		oss << fullViewBootstrapTimeoutSec;
//		prop.setProperty(config::FullViewBootstrapTimeoutSec_PROP_KEY,oss.str());
//	}

	highPriorityMonitoringEnabled = getOptionalBooleanProperty(
			config::HighPriorityMonitoringEnabled_PROP_KEY,
			config::HighPriorityMonitoringEnabled_DEFVALUE);
	prop.setProperty(config::HighPriorityMonitoringEnabled_PROP_KEY,
			(highPriorityMonitoringEnabled ? "true" : "false"));

	retainAttributesOnSuspectNodesEnabled = getOptionalBooleanProperty(
			config::RetainAttributesOnSuspectNodesEnabled_PROP_KEY,
			config::RetainAttributesOnSuspectNodesEnabled_DEFVALUE);
	prop.setProperty(config::RetainAttributesOnSuspectNodesEnabled_PROP_KEY,
			(retainAttributesOnSuspectNodesEnabled ? "true" : "false"));

	//=== Topo ===
	topologyPeriodicTaskIntervalMillis = getOptionalInt32Property(
			config::TopologyPeriodicTaskIntervalMillis_PROP_KEY,
			config::TopologyPeriodicTaskIntervalMillis_DEFVALUE);
	{
		ostringstream oss;
		oss << topologyPeriodicTaskIntervalMillis;
		prop.setProperty(config::TopologyPeriodicTaskIntervalMillis_PROP_KEY,
				oss.str());
	}

	frequentDiscoveryIntervalMillis = getOptionalInt32Property(
			config::FrequentDiscoveryIntervalMillis_PROP_KEY,
			config::FrequentDiscoveryIntervalMillis_DEFVALUE);
	{
		ostringstream oss;
		oss << frequentDiscoveryIntervalMillis;
		prop.setProperty(config::FrequentDiscoveryIntervalMillis_PROP_KEY,
				oss.str());
	}

	frequentDiscoveryMinimalDurationMillis = getOptionalInt32Property(
			config::FrequentDiscoveryMinimalDurationMillis_PROP_KEY,
			config::FrequentDiscoveryMinimalDurationMillis_DEFVALUE);
	{
		ostringstream oss;
		oss << frequentDiscoveryMinimalDurationMillis;
		prop.setProperty(
				config::FrequentDiscoveryMinimalDurationMillis_PROP_KEY,
				oss.str());
	}

	normalDiscoveryIntervalMillis = getOptionalInt32Property(
			config::NormalDiscoveryIntervalMillis_PROP_KEY,
			config::NormalDiscoveryIntervalMillis_DEFVALUE);
	{
		ostringstream oss;
		oss << normalDiscoveryIntervalMillis;
		prop.setProperty(config::NormalDiscoveryIntervalMillis_PROP_KEY,
				oss.str());
	}

	discoveryProtocol = getOptionalProperty(config::DiscoveryProtocol_PROP_KEY,
			config::DiscoveryProtocol_DEFVALUE);

	if (discoveryProtocol == config::DiscoveryProtocol_TCP_VALUE)
	{
		tcpDiscovery = true;
		udpDiscovery = false;
		multicastDiscovery = false;
	}
	else if (discoveryProtocol == config::DiscoveryProtocol_UDP_VALUE)
	{
		tcpDiscovery = false;
		udpDiscovery = true;
		multicastDiscovery = false;
	}
	else if (discoveryProtocol == config::DiscoveryProtocol_TCP_UDP_VALUE)
	{
		tcpDiscovery = true;
		udpDiscovery = true;
		multicastDiscovery = false;
	}
	else if (discoveryProtocol == config::DiscoveryProtocol_Multicast_TCP_VALUE)
	{
		tcpDiscovery = true;
		udpDiscovery = false;
		multicastDiscovery = true;
	}
	else if (discoveryProtocol == config::DiscoveryProtocol_Multicast_TCP_UDP_VALUE)
	{
		tcpDiscovery = true;
		udpDiscovery = true;
		multicastDiscovery = true;
	}
	else
	{
		String what(config::DiscoveryProtocol_PROP_KEY);
		what.append(" Is illegal: ");
		what.append(discoveryProtocol);
		throw IllegalConfigException(what);
	}

	prop.setProperty(config::DiscoveryProtocol_PROP_KEY,
			discoveryProtocol);

	randomDegreeTarget = getOptionalInt32Property(
			config::RandomDegreeTarget_PROP_KEY,
			config::RandomDegreeTarget_DEFVALUE);
	{
		ostringstream oss;
		oss << randomDegreeTarget;
		prop.setProperty(config::RandomDegreeTarget_PROP_KEY, oss.str());
	}

	randomDegreeMargin = getOptionalInt32Property(
			config::RandomDegreeMargin_PROP_KEY,
			config::RandomDegreeMargin_DEFVALUE);
	{
		ostringstream oss;
		oss << randomDegreeMargin;
		prop.setProperty(config::RandomDegreeMargin_PROP_KEY, oss.str());
	}

	strucuTopoEnabled = getOptionalBooleanProperty(config::StructTopoEnabled_PROP_KEY,
			config::StructTopoEnabled_DEFVALUE);
	prop.setProperty(config::StructTopoEnabled_PROP_KEY, (strucuTopoEnabled ? "true"
			: "false"));

	structDegreeTarget= getOptionalInt32Property(
			config::StructDegreeTarget_PROP_KEY,
			config::StructDegreeTarget_DEFVALUE);
	{
		ostringstream oss;
		oss << structDegreeTarget;
		prop.setProperty(config::StructDegreeTarget_PROP_KEY, oss.str());
	}

	//=== Messaging

	publisherReliabilityMode = getOptionalProperty(
			config::PublisherReliabilityMode_PROP_KEY,
			config::PublisherReliabilityMode_DEFVALUE);
	if (publisherReliabilityMode != config::ReliabilityMode_BestEffort_VALUE)
	{
		String what(config::PublisherReliabilityMode_PROP_KEY);
		what.append(" must be: ");
		what.append(config::ReliabilityMode_BestEffort_VALUE);
		throw IllegalConfigException(what);
	}
	prop.setProperty(config::PublisherReliabilityMode_PROP_KEY,
			publisherReliabilityMode);

	topicGlobalScope = getOptionalBooleanProperty(
			config::TopicGlobalScope_Prop_Key,
			config::TopicGlobalScope_DEFVALUE);
	prop.setProperty(config::TopicGlobalScope_Prop_Key, topicGlobalScope ? "true" : "false");

	publisherRoutingProtocol = getOptionalProperty(
			config::PublisherRoutingProtocol_PROP_KEY,
			config::PublisherRoutingProtocol_DEFVALUE);
	validatePublisherRoutingProtocol(publisherRoutingProtocol);
	prop.setProperty(config::PublisherRoutingProtocol_PROP_KEY,
			publisherRoutingProtocol);

	//=== Routing

	routingEnabled = getOptionalBooleanProperty(config::RoutingEnabled_PROP_KEY,
			config::RoutingEnabled_DEFVALUE);
	prop.setProperty(config::RoutingEnabled_PROP_KEY, (routingEnabled ? "true"
			: "false"));

	//=== Leader election

	leaderElectionEnabled = getOptionalBooleanProperty(config::LeaderElectionEnabled_PROP_KEY,
			config::LeaderElectionEnabled_DEFVALUE);
	prop.setProperty(config::LeaderElectionEnabled_PROP_KEY,(leaderElectionEnabled ? "true" : "false"));

	leaderElectionWarmupTimeoutMillis = getOptionalInt32Property(
			config::LeaderElectionWarmupTimeoutMillis_PROP_KEY,
			config::LeaderElectionWarmupTimeoutMillis_DEFVALUE);

	{
		ostringstream oss;
		oss << leaderElectionWarmupTimeoutMillis;
		prop.setProperty(config::LeaderElectionWarmupTimeoutMillis_PROP_KEY,oss.str());
	}

	//=== Hier
	numberOfDelegates = getOptionalInt32Property(
			config::HierarchyNumberOfDelegates_PROP_KEY,
			config::HierarchyNumberOfDelegates_DEFVALUE);
	{
		ostringstream oss;
		oss << numberOfDelegates;
		prop.setProperty(config::HierarchyNumberOfDelegates_PROP_KEY, oss.str());
	}

	numberOfSupervisors = getOptionalInt32Property(
			config::HierarchyNumberOfSupervisors_PROP_KEY,
			config::HierarchyNumberOfSupervisors_DEFVALUE);
	{
		ostringstream oss;
		oss << numberOfSupervisors;
		prop.setProperty(config::HierarchyNumberOfSupervisors_PROP_KEY,
				oss.str());
	}

	numberOfActiveDelegates = getOptionalInt32Property(
			config::HierarchyNumberOfActiveDelegates_PROP_KEY,
			config::HierarchyNumberOfActiveDelegates_DEFVALUE);
	{
		ostringstream oss;
		oss << numberOfActiveDelegates;
		prop.setProperty(config::HierarchyNumberOfActiveDelegates_PROP_KEY,
				oss.str());
	}

	includeAttributes = getOptionalBooleanProperty(
			config::HierarchyIncludeAttributes_PROP_KEY,
			config::HierarchyIncludeAttributes_DEFVALUE);
	{
		ostringstream oss;
		oss << std::boolalpha << includeAttributes;
		prop.setProperty(config::HierarchyIncludeAttributes_PROP_KEY, oss.str());
	}

	hier_memberhipUpdateAggregationInterval = getOptionalInt32Property(
			config::HierarchyMemberhipUpdateAggregationInterval_PROP_KEY,
			config::HierarchyMemberhipUpdateAggregationInterval_DEFVALUE);
	{
		ostringstream oss;
		oss << hier_memberhipUpdateAggregationInterval;
		prop.setProperty(
				config::HierarchyMemberhipUpdateAggregationInterval_PROP_KEY,
				oss.str());
	}

	hier_connectIntervalMillis = getOptionalInt32Property(
			config::HierarchyConnectIntervalMillis_PROP_KEY,
			config::HierarchyConnectIntervalMillis_DEFVALUE);
	{
		ostringstream oss;
		oss << hier_connectIntervalMillis;
		prop.setProperty(
				config::HierarchyConnectIntervalMillis_PROP_KEY,
				oss.str());
	}

	hier_quarantineTimeMillis = getOptionalInt32Property(
			config::HierarchySupervisorQuarantineIntervalMillis_PROP_KEY,
			config::HierarchySupervisorQuarantineIntervalMillis_DEFVALUE);
	{
		ostringstream oss;
		oss << hier_quarantineTimeMillis;
		prop.setProperty(
				config::HierarchySupervisorQuarantineIntervalMillis_PROP_KEY,
				oss.str());
	}

	foreignZoneMembershipTimeOut = getOptionalInt32Property(
			config::HierarchyForeignZoneMemberhipTimeOut_PROP_KEY,
			config::HierarchyForeignZoneMemberhipTimeOut_DEFVALUE);
	{
		ostringstream oss;
		oss << foreignZoneMembershipTimeOut;
		prop.setProperty(config::HierarchyForeignZoneMemberhipTimeOut_PROP_KEY,
				oss.str());
	}

	hierarchyEnabled = getOptionalBooleanProperty(
			config::HierarchyEnabled_PROP_KEY,
			config::HierarchyEnabled_DEFVALUE);
	prop.setProperty(config::HierarchyEnabled_PROP_KEY,
			(hierarchyEnabled ? "true" : "false"));

	//=== Stats
	statisticsEnabled = getOptionalBooleanProperty(
			config::StatisticsEnabled_PROP_KEY,
			config::StatisticsEnabled_DEFVALUE);
	{
		ostringstream oss;
		oss << std::boolalpha << statisticsEnabled;
		prop.setProperty(config::StatisticsEnabled_PROP_KEY, oss.str());
	}

	statisticsPeriodMillis = getOptionalInt32Property(
			config::StatisticsPeriodMillis_PROP_KEY,
			config::StatisticsPeriodMillis_DEFVALUE);
	{
		ostringstream oss;
		oss << statisticsPeriodMillis;
		prop.setProperty(config::StatisticsPeriodMillis_PROP_KEY, oss.str());
	}

	statisticsTaskTardinessThresholdMillis = getOptionalInt32Property(
			config::StatisticsTaskTardinessThresholdMillis_PROP_KEY,
			config::StatisticsTaskTardinessThresholdMillis_DEFVALUE);
	{
		ostringstream oss;
		oss << statisticsTaskTardinessThresholdMillis;
		prop.setProperty(
				config::StatisticsTaskTardinessThresholdMillis_PROP_KEY,
				oss.str());
	}

	debugFailFast = getOptionalBooleanProperty(
			config::DebugFailFast_PROP_KEY,
			config::DebugFailFast_DEFVALUE);
	{
		ostringstream oss;
		oss << std::boolalpha << debugFailFast;
		prop.setProperty(config::DebugFailFast_PROP_KEY, oss.str());
	}
}

void SpiderCastConfigImpl::validatePublisherRoutingProtocol(const String& publisherRP)
{
	if (publisherRP != config::RoutingProtocol_Broadcast_VALUE &&
			publisherRP != config::RoutingProtocol_PubSub_VALUE)
	{
		String what(config::PublisherRoutingProtocol_PROP_KEY);
		what.append(" must be: ");
		what.append(config::RoutingProtocol_Broadcast_VALUE);
		what.append(" | ");
		what.append(config::RoutingProtocol_PubSub_VALUE);
		throw IllegalConfigException(what);
	}
}

SpiderCastConfigImpl::~SpiderCastConfigImpl()
{
	//cout << "~SpiderCastConfigImpl()" << endl;
}

PropertyMap SpiderCastConfigImpl::getPropertyMap() const
{
	return prop;
}

std::vector<NodeID_SPtr> SpiderCastConfigImpl::getBootStrapSet() const
{
	std::vector<NodeID_SPtr> res;

	for (std::vector<NodeIDImpl_SPtr >::const_iterator it = bootstrap_set.begin(); it < bootstrap_set.end(); ++it )
	{
		res.push_back(boost::static_pointer_cast<NodeID>(*it));
	}

	return res;
}

bool SpiderCastConfigImpl::verifyBindNetworkInterface(std::string& errMsg, int& errCode)
{
	bool ok = true;

	if (!bindNetworkAddress.empty())
	{
		CommUtils::NICInfo nic_info;
		int errorCode = 0;
		std::string errorMsg;

		bool info_ok = CommUtils::get_nic_info(bindNetworkAddress.c_str(), &nic_info, &errorCode, &errorMsg);
		ok = (  info_ok && nic_info.index != 0);
		if (!ok)
		{
			sleep(5);
			info_ok = CommUtils::get_nic_info(bindNetworkAddress.c_str(), &nic_info, &errorCode, &errorMsg);
			ok = ( info_ok && nic_info.index != 0);
			if (!ok)
			{
				std::ostringstream what;
				what << config::BindNetworkInterface_PROP_KEY  << " cannot be associated with a network interface (NIC), " << bindNetworkAddress
						<< "; get_nic_info: error code=" << errCode << ", error message=" << errMsg << "; index=" << nic_info.index;
				if (!info_ok)
				{
					errCode = errorCode;
					errMsg.assign(what.str());
				}
				else
				{
					errCode = EADDRNOTAVAIL;
					what << "; Interface not found; NICs: {";
					std::vector<CommUtils::NICInfo> nic_vec;
					CommUtils::get_all_nic_info(nic_vec);
					for (std::size_t i=0; i<nic_vec.size(); ++i)
					{
						what << nic_vec[i].toString() << " ";
					}
					what <<"}";
					errMsg.assign(what.str());
				}
			}
		}
	}

	if (!ok)
	{
		return ok;
	}

	if (!multicastOutboundInterface.empty())
	{
		CommUtils::NICInfo nic_info;
		int errorCode = 0;
		std::string errorMsg;
		bool info_ok = CommUtils::get_nic_info(multicastOutboundInterface.c_str(), &nic_info, &errorCode, &errorMsg);
		ok = info_ok && (nic_info.index != 0);
		if(!ok)
		{
			std::ostringstream what;
			what << config::DiscoveryMulticastInOutInterface_PROP_KEY  << " does not identify an interface on the local machine: " << multicastOutboundInterface
					<< "; get_nic_info: error code=" << errCode << ", error message=" << errMsg << "; index=" << nic_info.index;
			if (!info_ok)
			{
				errCode = errorCode;
				errMsg.assign(what.str());
			}
			else
			{
				errCode = EADDRNOTAVAIL;
				what << "; Interface not found; NICs: {";
				std::vector<CommUtils::NICInfo> nic_vec;
				CommUtils::get_all_nic_info(nic_vec);
				for (std::size_t i=0; i<nic_vec.size(); ++i)
				{
					what << nic_vec[i].toString() << " ";
				}
				what <<"}";
				errMsg.assign(what.str());
			}
		}
	}

	return ok;
}

String SpiderCastConfigImpl::toString() const
{
	String rv("Properties=");
	rv += SpiderCastConfig::toString();
	rv += " BootstrapSet={";
	vector<NodeID_SPtr>::size_type i = 0;
	for (vector<NodeIDImpl_SPtr>::const_iterator it = bootstrap_set.begin(); it
			!= bootstrap_set.end(); ++it)
	{
		rv += NodeIDImpl::stringValueOf((*it));

		if (i < bootstrap_set.size() - 1)
		{
			rv += "; ";
		}
		++i;
	}
	rv += "}";

	rv += " SupervisorBootstrapSet={";
	i = 0;
	for (vector<NodeIDImpl_SPtr>::const_iterator it =
			supervisor_bootstrap_set.begin(); it
			!= supervisor_bootstrap_set.end(); ++it)
	{
		rv += NodeIDImpl::stringValueOf((*it));

		if (i < supervisor_bootstrap_set.size() - 1)
		{
			rv += "; ";
		}
		++i;
	}
	rv += "}";

	return rv;
}

vector<pair<string, string> > SpiderCastConfigImpl::parseNetworkInterface(
		const String& interface) throw (IllegalConfigException)
{
	std::vector<String> tokens;
	boost::split(tokens, interface, boost::is_any_of(std::string(",")));

	if (tokens.size() % 2 != 0)
	{
		String what =
				"Illegal network interface format (odd number of tokens): "
						+ interface;
		throw IllegalConfigException(what);
	}

	vector<pair<string, string> > rv;
	for (std::vector<String>::iterator it = tokens.begin(); it != tokens.end();)
	{
		pair<string, string> p;
		p.first = *it;
		boost::trim(p.first);
		++it;
		p.second = *it;
		boost::trim(p.second);
		++it;

		if (p.first.empty())
		{
			String what = "Illegal network interface format (empty address): "
					+ interface;
			throw IllegalConfigException(what);
		}

		rv.push_back(p);
	}

	return rv;
}

void SpiderCastConfigImpl::parsePolicy(
		const String& policy_tmp,
		String& fullViewBootstrapSetPolicy,
		int32_t& fullViewBootstrapDegree)
{
	std::vector<String> tokens;
	boost::split(tokens, policy_tmp, boost::is_any_of(" \t"), boost::algorithm::token_compress_on);
	if (tokens.size() > 0)
	{
		boost::trim(tokens[0]);
		fullViewBootstrapSetPolicy = tokens[0];

		if (tokens.size() > 1)
		{
			boost::trim(tokens[1]);
			fullViewBootstrapDegree = boost::lexical_cast<int32_t>(tokens[1]);
		}
		else
		{
			fullViewBootstrapDegree = 16;
		}
	}
}

bool forbiddenInName(char c)
{
	if (c == ',' || c == '"' || c == '=' || c == '\\')
		return true;
	else
		return false;
}

void SpiderCastConfigImpl::validateNodeName(const string& name, bool allowAny)
		throw (IllegalConfigException)
{
	if (name.empty())
	{
		String what(config::NodeName_PROP_KEY);
		what.append(" must not be empty");
		throw IllegalConfigException(what);
	}

	if (!boost::algorithm::all(name, boost::algorithm::is_graph(std::locale("C")))
			|| boost::algorithm::any_of(name, forbiddenInName) )
	{
		String what(config::NodeName_PROP_KEY);
		what.append(" must contain only graphical characters, and ");
		what.append("cannot contain control characters, commas, ");
		what.append("double quotation marks, backslashes or equal signs: '");
		what.append(name).append("'");
		throw IllegalConfigException(what);
	}

	if (!allowAny && name == NodeID::NAME_ANY)
	{
		String what(config::NodeName_PROP_KEY);
		what.append(" must not be the reserved name ");
		what.append(NodeIDImpl::NAME_ANY);
		throw IllegalConfigException(what);
	}
}

}//namespace spdr
