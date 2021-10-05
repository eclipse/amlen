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
 * SpiderCastConfig.h
 *
 *  Created on: 28/01/2010
 */

#ifndef SPIDERCASTCONFIG_H_
#define SPIDERCASTCONFIG_H_

#include <vector>

#include "Definitions.h"
#include "BasicConfig.h"
#include "NodeID.h"

namespace spdr
{

/**
 * Documented configuration property keys.
 *
 * The global variables under this namespace ending with the suffix _PROP_KEY, are the
 * documented public property keys that are accepted by SpiderCastConfig.
 * The corresponding variables that end with the suffix _DEFVALUE are the default
 * values of those properties.
 *
 * @see SpiderCastConfig
 */
namespace config
{

/**
 * SpiderCast wire format and protocol version.
 */
const uint16_t SPIDERCAST_VERSION = 0x0001;

//=== General ===============================================

/**
 * Node name.
 *
 * Mandatory property.
 *
 * The node name is a unique identifier of the node.
 *
 * Two nodes with the same name cannot co-exist on the same bus,
 * nor on two buses that share a common first level.
 *
 * Value - a string with no white spaces, only graphical characters.
 * In addition the name cannot contain control characters, commas,
 * double quotation marks, backslashes or equal signs.
 *
 * The string '$ANY' cannot be used as it is reserved for marking
 * nameless discovery nodes in the bootstrap set.
 *
 * @see SpiderCastFactory
 *
 */
const String NodeName_PROP_KEY = "spidercast.NodeName";

/**
 * Bus name.
 *
 * Mandatory property.
 *
 * The bus name identifies an administrative unit of nodes that
 * connect and maintains full membership.
 *
 * Bus names are hierarchical. The level separator is '/' (forward slash).
 * Each level must contain only graphical letters (apart from '/').
 * Bus name must start with a '/'.
 *
 * Currently only two levels are supported.
 *
 * Example:
 * '/mycluster', '/mycluster/cell2'.
 *
 *
 * Value - a string with no white spaces, only graphical characters.
 */
const String BusName_PROP_KEY = "spidercast.BusName";

/**
 * Ensures that the node chooses an incarnation number higher than a given value.
 *
 * Optional property.
 *
 * The node usually chooses an incarnation number which is the time elapsed in
 * microseconds since the Unix (POSIX time) epoch (1.Jan.1970). However if V,
 * the value given is higher or equal to that, it will use (V+1) as the
 * incarnation number.
 *
 * This can be use to ensure strictly increasing incarnation numbers when
 * clocks are not synchronized between two alternate servers for the same
 * node.
 *
 * Value: int64_t - values<0 are ignored; values >=0 are applied.
 *
 * if missing: set to (-1).
 *
 * It is an error to provide both ForceIncarnationNumber_PROP_KEY >=0 and
 * ChooseIncarnationNumberHigherThan_PROP_KEY >=0.
 */
const String ChooseIncarnationNumberHigherThan_PROP_KEY = "spidercast.ChooseIncarnationNumberHigherThan";

const int64_t ChooseIncarnationNumberHigherThan_DEFVALUE = -1;

/**
 * Force the node to use a given incarnation number.
 *
 * Optional property.
 *
 * The node usually chooses an incarnation number which is the time elapsed in
 * microseconds since the Unix (POSIX time) epoch (1.Jan.1970). With this property
 * the incarnation number is forced to the given value, if it is >=0.
 *
 * Value: int64_t - values <0 are ignored; values >=0 are applied.
 *
 * if missing: set to (-1).
 *
 * It is an error to provide both ForceIncarnationNumber_PROP_KEY >=0 and
 * ChooseIncarnationNumberHigherThan_PROP_KEY >=0.
 */
const String ForceIncarnationNumber_PROP_KEY = "spidercast.ForceIncarnationNumber";

const int64_t ForceIncarnationNumber_DEFVALUE = -1;

//=== Comm ==================================================


/**
 * Node's network interfaces (IP address) and their network-ID.
 *
 * Mandatory property.
 *
 * One or more network interfaces (at least one). The node will declare
 * those addresses as the addresses in which it can be contacted. The network-ID
 * of an interface is a string identifier for the network that interface is on.
 *
 * Nodes with two interfaces with the same network-ID can communicate with
 * one another using those interfaces. An empty string implicitly means "global" scope.
 *
 * Value - a string with multiple interface-scope pairs, separated by comma:
 * <br>
 * <i>ifc1, scope1, ifc2, scope2, ... ifcN, scopeN</i>
 *
 * Each interface entry (ifcK) with either an explicit IP address (e.g. 1.2.3.4), or a
 * host name. The (ifcK) and (scopeK) entries should not contain white spaces or commas.
 *
 * Note that these are the external addresses that are disseminated to other peers so
 * that the current node could be contacted by them. In case of a NAT/firewall deployment,
 * these may be different than the local bind interface address.
 *
 * Example: a single interface with a scope: <br>
 * 12.22.33.44, Torus-1
 *
 * Example: a single interface with a global scope: <br>
 * 172.16.33.44,
 *
 * Example: a combination of both: <br>
 * 172.16.33.44, , 12.22.33.44, Torus-1
 *
 * The order in which the network interfaces are declared is significant.
 * A node (i) will try to connect to another node (j) using the first IP address
 * with a scope that matches, where each interface of i (in order) is matched
 * against all interfaces of j (in order). For example:
 *
 * <UL>
 * 	<LI> Node1 has: IP1, ScopeA, IP2, ScopeB, IP3, ScopeC
 * 	<LI> Node2 has: IP4, ScopeX, IP5, ScopeA, IP6, ScopeB
 * </UL>
 *
 * Node1 will connect to Node2 on IP5, ScopeA;
 *
 * Node2 will connect to Node1 on IP2, ScopeA;
 *
 */
const String NetworkInterface_PROP_KEY = "spidercast.comm.NetworkInterface";

/**
 * TCP Receiver port.
 * <p>
 * Mandatory property.
 * <p>
 * The TCP port that other peers use in order to contact this node.
 * In case of a NAT/firewall deployment,
 * this port may be different from the local bind port.
 *
 * Value - an integer carrying a legal (TCP) port number in the range
 * [1-65535].
 */
const String TCPReceiverPort_PROP_KEY = "spidercast.comm.TCPReceiverPort";

/**
 * The network interface the TCP server will bind to.
 * <p>
 * Optional.
 * <p>
 * If missing, the TCP socket will bind to all interfaces.
 * If it exists, the value of BindAllInterfaces_PROP_KEY can be used to override and
 * bind to all interfaces anyway.
 * <p>
 * Value - a single interface with either an explicit IP address (e.g. 1.2.3.4, ::1), or a
 * host name, or an interface name (e.g. eth0).
 */
const String BindNetworkInterface_PROP_KEY = "spidercast.comm.BindNetworkInterface";

/**
 * The port the TCP server will bind to.
 * <p>
 * Optional.
 * <p>
 * If missing, the value of TCPReceiverPort_PROP_KEY is used.
 * <p>
 * Value - an integer carrying a legal (TCP) port number in the range
 * [1-65535].
 */
const String BindTCPReceiverPort_PROP_KEY = "spidercast.comm.BindTCPReceiverPort";

/**
 * Bind all interfaces.
 * <p>
 * Optional property.
 * <p>
 * Whether to bind the TCP server socket to all interfaces. If true, SpiderCast will accept
 * connections from all interfaces with the specified port.
 *
 * If a bind interface is NOT given using BindNetworkInterface_PROP_KEY,
 * this value is set to TRUE.
 *
 * Only when a bind interface IS given using BindNetworkInterface_PROP_KEY,
 * The value in BindAllInterfaces_PROP_KEY may be used to override defaults behavior and
 * specify whether to bind to all interface.
 * <p>
 * Value - boolean.
 * <p>
 * Default - false; If a BindNetworkInterface_PROP_KEY is not provided: overrides to true.
 */
const String BindAllInterfaces_PROP_KEY = "spidercast.comm.BindAllInterfaces";
/**
 * Default value of BindAllInterfaces_PROP_KEY.
 */
const bool BindAllInterfaces_DEFVALUE = false;

/**
 * Heart-beat interval, in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * Failure detection is implemented by sending heart-beat messages to neighbors
 * at regular intervals. If a heart-beat is not received for a period longer than
 * the heart-beat-timeout, the counterpart node is suspected as a failure.
 * <p>
 * Value - an integer >0.
 * <p>
 * Default: 1000.
 *
 * @see HeartbeatTimeoutMillis_PROP_KEY
 */
const String HeartbeatIntervalMillis_PROP_KEY =
		"spidercast.comm.HeartbeatIntervalMillis";
/**
 * Default value of HeartbeatIntervalMillis_PROP_KEY.
 */
const int32_t HeartbeatIntervalMillis_DEFVALUE = 1000;

/**
 * Heart-beat timeout, in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * Failure detection is implemented by sending heart-beat messages to neighbors
 * at regular intervals. If a heart-beat is not received for a period longer than
 * the heart-beat-timeout, the counterpart node is suspected as a failure.
 * <p>
 * Value - an integer > HeartbeatIntervalMillis.
 * <p>
 * Default: 20000.
 *
 * @see HeartbeatIntervalMillis_PROP_KEY
 */
const String HeartbeatTimeoutMillis_PROP_KEY =
		"spidercast.comm.HeartbeatTimeoutMillis";
/**
 * Default value of HeartbeatTimeoutMillis_PROP_KEY.
 */
const int32_t HeartbeatTimeoutMillis_DEFVALUE = 20000;

/**
 * Connection establish timeout, in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * How much time to try and establish a connection to a target.
 * Zero means return with failure immediately if the first connect attempt failed.
 *
 * <p>
 * Value - an integer >= 0.
 * <p>
 * Default: 5000.
 */
const String ConnectionEstablishTimeoutMillis_PROP_KEY =
		"spidercast.comm.ConnectionEstablishTimeoutMillis";
/**
 * Default value of ConnectionEstablishTimeoutMillis_PROP_KEY.
 */
const int32_t ConnectionEstablishTimeoutMillis_DEFVALUE = 5000;

/**
 * Maximal memory allowed for Comm provider, in Mega Bytes.
 * <p>
 * Optional property.
 * <p>
 * The default Comm provider is RUM, which handles all connections, and implements reliable
 * message oriented communication channels on top of TCP (which is stream oriented). This value
 * defines how much memory is available for buffering in that layer.
 * <p>
 * Value - for RUM the Min. is 20M and Max. is 4000M on 32bit machines.
 * <p>
 * Default: 20.
 */
const String MaxMemoryAllowedMBytes_PROP_KEY =
		"spidercast.comm.MaxMemoryAllowedMBytes";
/**
 * Default value of MaxMemoryAllowedMBytes_PROP_KEY.
 */
const int32_t MaxMemoryAllowedMBytes_DEFVALUE = 20;

/**
 * Use SSL when establishing connections.
  * <p>
 * Optional property.
 * <p>
 * The default Comm provider is RUM, which handles all connections, and implements reliable
 * message oriented communication channels on top of TCP or SSL (which is stream oriented). This value
 * defines whether RUM will use the SSL/TLS protocol when establishing connections. When this
 * value is false RUM uses TCP.
 * <p>
 * Value - boolean.
 * <p>
 * Default: false.
 */
const String UseSSL_PROP_KEY =
		"spidercast.comm.UseSSL";

const String RequireCerts_PROP_KEY =
        "spidercast.comm.RequireCerts";

const bool UseSSL_DEFVALUE = false;

/**
 * RUM log level.
 * <p>
 * Optional property.
 * <p>
 * Value - an integer (-1), 0-9.
 * For: None=0, Status=1, Fatal=2, Error=3, Warning=4, Informational=5, Event=6, Debug=7, Trace=8, Extended-trace=9.
 * Value (-1)="Global", which means that the a new RUM instance inherits its log level from from the global setting,
 * as set by rumChangeLogLevel() in the RUM API. The RUM "Global" log level defaults to 5 (Informational).
 *
 * Default: (-1) (Global) which defaults to Info (5) unless changed.
 */
const String RUMLogLevel_PROP_KEY = "spidercast.comm.RUMLogLevel";
/**
 * Default value of RUMLogLevel_PROP_KEY
 */
const int32_t RUMLogLevel_DEFVALUE = -1;




//=== Multicast discovery parameters ===
/**
 * Discovery multicast group address, IPv4.
 *
 * This is a well known address the nodes of a SpiderCast bus use in order to discover each other.
 * <p>
 * Optional property.
 * <p>
 * Value - a string encoding a legal multicast address in dotted decimal form (IPv4).
 *
 * Default: 239.2.2.2
 *
 */
const String DiscoveryMulticastGroupAddressIPv4_PROP_KEY = "spidercast.comm.DiscoveryMulticastGroupAddressIPv4";
/**
 * Default value of RUMLogLevel_PROP_KEY
 */
const String DiscoveryMulticastGroupAddressIPv4_DEFVALUE = "239.2.2.2";

/**
 * Discovery multicast group address, IPv6.
 *
 * This is a well known address the nodes of a SpiderCast bus use in order to discover each other.
 * <p>
 * Optional property.
 * <p>
 * Value - a string encoding a legal multicast address in hexadecimal notation (IPv6).
 *
 * Default: FF18::2222
 */
const String DiscoveryMulticastGroupAddressIPv6_PROP_KEY = "spidercast.comm.DiscoveryMulticastGroupAddressIPv6";
/**
 * Default value of RUMLogLevel_PROP_KEY
 */
const String DiscoveryMulticastGroupAddressIPv6_DEFVALUE = "FF18::2222";

/**
 * Discovery multicast port.
 * <p>
 * Optional property.
 * <p>
 * This is the receiver port.
 * <p>
 * Value - an integer carrying a legal (UDP) port number in the range [1-65535].
 * Default: 40123.
 */
const String DiscoveryMulticastPort_PROP_KEY = "spidercast.comm.DiscoveryMulticastPort";

/**
 * Default value of DiscoveryMulticastPort_PROP_KEY
 */
const int32_t DiscoveryMulticastPort_DEFVALUE = 40123;

/**
 * Discovery multicast in/out interface.
 * <p>
 * Optional property.
 * <p>
 * This indicates the interface on which multicast packets will be transmitted,
 * and the interface from which they will be received.
 * Value - empty, or a string encoding a legal IP address in dotted decimal form (IPv4),
 * or hexadecimal notation (for IPv6), or an interface name (e.g. eth0).
 *
 * Default: "", meaning receive from all, and transmit to the default of the local machine, as decided by the kernel
 *
 */
const String DiscoveryMulticastInOutInterface_PROP_KEY = "spidercast.comm.DiscoveryMulticastInOutInterface";

/**
 * Default value of DiscoveryMulticastInOutInterface_PROP_KEY
 */
const String DiscoveryMulticastInOutInterface_DEFVALUE = "";

/**
 * Discovery multicast hops.
 * <p>
 * Optional property.
 * <p>
 * Value - an integer carrying a legal hop count in the range [0-255].
 * Default: 1 (link local).
 */
const String DiscoveryMulticastHops_PROP_KEY = "spidercast.comm.DiscoveryMulticastHops";
/**
 * Default value of DiscoveryMulticastHops_PROP_KEY
 */
const int32_t DiscoveryMulticastHops_DEFVALUE = 1;

/**
 * UDP packet size.
 *
 * Relevant for UDP discovery and Multicast discovery.
 * <p>
 * Optional property.
 * <p>
 * Value - an integer.
  * Default: 8 KB
 */
const String UDPPacketSizeBytes_PROP_KEY = "spidercast.comm.UDPPacketSizeBytes";
/**
 * Default value of UDPPacketSizeBytes_PROP_KEY
 */
const int32_t UDPPacketSizeBytes_DEFVALUE = 8 * 1024;

/**
 * UDP send buffer size.
 *
 * Relevant for UDP discovery and Multicast discovery.
 * <p>
 * Optional property.
 * <p>
 * Value - an integer, >= 64KB
 * Default: 1 MB
 */
const String UDPSendBufferSizeBytes_PROP_KEY = "spidercast.comm.UDPSendBufferSizeBytes";

const int32_t UDPSendBufferSizeBytes_DEFVALUE = 1024 * 1024;

/**
 * UDP receive buffer size.
 *
 * Relevant for UDP discovery and Multicast discovery.
 * <p>
 * Optional property.
 * <p>
 * Value - an integer, >= 64KB
 * Default: 1 MB
 */
const String UDPReceiveBufferSizeBytes_PROP_KEY = "spidercast.comm.UDPReceiveBufferSizeBytes";

const int32_t UDPReceiveBufferSizeBytes_DEFVALUE = 1024 * 1024;

//=== Membership ============================================

/**
 * Membership gossip interval in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 1000 ms.
 */
const String MembershipGossipIntervalMillis_PROP_KEY =
		"spidercast.membership.GossipIntervalMillis";
/**
 * Default value of MembershipGossipIntervalMillis_PROP_KEY.
 */
const int32_t MembershipGossipIntervalMillis_DEFVALUE = 1000;

/**
 * Node history retention time in seconds.
 * <p>
 * Optional property.
 * Must be >=2 sec.
 * <p>
 * Value - integer.
 * <p>
 * Default: 3600*24*30 sec.
 */
const String NodeHistoryRetentionTimeSec_PROP_KEY =
		"spidercast.membership.NodeHistoryRetentionTimeSec";
/**
 * Default value of NodeHistoryRetentionTimeSec_PROP_KEY.
 */
const int32_t NodeHistoryRetentionTimeSec_DEFVALUE = 3600*24*30;

/**
 * Suspicion threshold.
 *
 * Optional. Must be >=1.
 * <p>
 * Value - integer.
 * <p>
 * Default: 1.
 */
const String SuspicionThreshold_PROP_KEY =
		"spidercast.membership.SuspicionThreshold";
/**
 * Default value of SuspicionThreshold_PROP_KEY.
 */
const int32_t SuspicionThreshold_DEFVALUE = 1;

/**
 * Whether the bootstrap set given to SpiderCast represents an expected full view.
 *
 * Optional property.
 * <p>
 * Value - boolean.
 * <p>
 * Default: false.
 */
const String FullViewBootstrapSet_PROP_KEY =
		"spidercast.membership.FullViewBootstrapSet";

const bool FullViewBootstrapSet_DEFVALUE = false;

///**
// * How to use a full view bootstrap.
// * <p>
// * Optional property.
// * <p>
// * Can be either for Random- or Tree-based discovery.
// * Valid only when FullViewBootstrapSet_PROP_KEY is set "true".
// * <p>
// * Value - String.
// * "Random" or
// * "Tree [degree]". degree (int >1) is optional.
// * <p>
// * Default: "Random".
// */
//const String FullViewBootstrapPolicy_PROP_KEY =
//		"spidercast.membership.FullViewBootstrapPolicy";
//
//const String FullViewBootstrapPolicy_Random_VALUE = "Random";
//const String FullViewBootstrapPolicy_Random1_VALUE = "Random1";
//const String FullViewBootstrapPolicy_Tree_VALUE = "Tree";
//
//const String FullViewBootstrapPolicy_DEFVALUE = FullViewBootstrapPolicy_Random_VALUE;

///**
// * The time after which discovery policy returns to Random.
// */
//const String FullViewBootstrapTimeoutSec_PROP_KEY =
//		"spidercast.membership.FullViewBootstrapTimeoutSec";
//
//const int FullViewBootstrapTimeoutSec_DEFVALUE = 30;

/**
 * Whether high priority monitoring is permitted on this node.
 *
 * Optional property.
 * <p>
 * Value - boolean.
 * <p>
 * Default: false.
 */
const String HighPriorityMonitoringEnabled_PROP_KEY =
		"spidercast.membership.HighPriorityMonitoringEnabled";

const bool HighPriorityMonitoringEnabled_DEFVALUE = false;

/**
 * Whether to retain the attributes of dead nodes and continue to disseminate them.
 *
 * Optional property.
 * <p>
 * Value - boolean.
 * <p>
 * Default: false.
 */
const String RetainAttributesOnSuspectNodesEnabled_PROP_KEY =
		"spidercast.membership.RetainAttributesOnSuspectNodesEnabled";

const bool RetainAttributesOnSuspectNodesEnabled_DEFVALUE = false;


//=== Topology ==============================================

/**
 * Topology periodic task interval in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 1000 ms.
 */
const String TopologyPeriodicTaskIntervalMillis_PROP_KEY =
		"spidercast.topology.PeriodicTaskIntervalMillis";
/**
 * Default value of TopologyPeriodicTaskIntervalMillis_PROP_KEY.
 */
const int32_t TopologyPeriodicTaskIntervalMillis_DEFVALUE = 1000;

/**
 * Frequent discovery task interval in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 1000 ms.
 */
const String FrequentDiscoveryIntervalMillis_PROP_KEY =
		"spidercast.topology.FrequentDiscoveryIntervalMillis";
/**
 * Default value of FrequentDiscoveryIntervalMillis_PROP_KEY.
 */
const int32_t FrequentDiscoveryIntervalMillis_DEFVALUE = 1000;

/**
 * Frequent discovery task minimal duration in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 60000 ms.
 */
const String FrequentDiscoveryMinimalDurationMillis_PROP_KEY =
		"spidercast.topology.FrequentDiscoveryMinimalDurationMillis";
/**
 * Default value of FrequentDiscoveryMinimalDurationMillis_PROP_KEY.
 */
const int32_t FrequentDiscoveryMinimalDurationMillis_DEFVALUE = 60000;

/**
 * Normal discovery task interval in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 30000 ms.
 */
const String NormalDiscoveryIntervalMillis_PROP_KEY =
		"spidercast.topology.NormalDiscoveryIntervalMillis";
/**
 * Default value of NormalDiscoveryIntervalMillis_PROP_KEY.
 */
const int32_t NormalDiscoveryIntervalMillis_DEFVALUE = 30000;

/**
 * Discovery protocol.
 * <p>
 * Optional property.
 * <p>
 * Value - String (TCP / UDP / TCP_UDP / Multicast_TCP / Multicast_TCP_UDP).
 * <p>
 * Default: TCP.
 */
const String DiscoveryProtocol_PROP_KEY =
		"spidercast.topology.DiscoveryProtocol";

const String DiscoveryProtocol_TCP_VALUE = "TCP";
const String DiscoveryProtocol_UDP_VALUE = "UDP";
const String DiscoveryProtocol_TCP_UDP_VALUE = "TCP_UDP";
const String DiscoveryProtocol_Multicast_TCP_VALUE = "Multicast_TCP";
const String DiscoveryProtocol_Multicast_TCP_UDP_VALUE = "Multicast_TCP_UDP";

/**
 * Default value of DiscoveryProtocol_PROP_KEY.
 */
const String DiscoveryProtocol_DEFVALUE = DiscoveryProtocol_TCP_VALUE;

/**
 * Structured topology enabled.
 * <p>
 * Optional property.
 *
  * <p>
 * Value - bool.
 * <p>
 * Default: true.
 */
const String StructTopoEnabled_PROP_KEY =
		"spidercast.topology.StructEnabled";
/**
 * Default value of StructTopoEnabled_PROP_KEY.
 */
const bool StructTopoEnabled_DEFVALUE = true;

/**
 * structured topo node degree target.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 3.
 */
const String StructDegreeTarget_PROP_KEY =
		"spidercast.topology.StructDegreeTarget";
/**
 * Default value of StructDegreeTarget_PROP_KEY.
 */
const int32_t StructDegreeTarget_DEFVALUE = 5;


/**
 * Random node degree target.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 3.
 */
const String RandomDegreeTarget_PROP_KEY =
		"spidercast.topology.RandomDegreeTarget";
/**
 * Default value of RandomDegreeTarget_PROP_KEY.
 */
const int32_t RandomDegreeTarget_DEFVALUE = 3;

/**
 * Random node degree margin (above target).
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 2.
 */
const String RandomDegreeMargin_PROP_KEY =
		"spidercast.topology.RandomDegreeMargin";
/**
 * Default value of RandomDegreeMargin_PROP_KEY.
 */
const int32_t RandomDegreeMargin_DEFVALUE = 2;

//=== Routing ==============================================

/**
 * Routing enabled.
 * <p>
 * Optional property.
 *
 * When routing is disabled, the overlay routing component and its thread are not started.
 * Messaging services are unavailable when routing is disabled.
 * <p>
 * Value - bool.
 * <p>
 * Default: true.
 */
const String RoutingEnabled_PROP_KEY =
		"spidercast.routing.Enabled";
/**
 * Default value of RoutingEnabled_PROP_KEY.
 */
const bool RoutingEnabled_DEFVALUE = true;

//=== Messaging ============================================

//=== Reliability Mode ===

/**
 * The reliability mode the publisher is using.
 *
 * Optional property.
 *
 * Value: String.
 *
 * Default: BestEffort.
 *
 * Supported: BestEffort.
 *
 * @see TopicPublisher
 */
const String PublisherReliabilityMode_PROP_KEY =
		"spidercast.messaging.publisher.ReliabilityMode";

/**
 * The value of a "best effort" reliability mode.
 */
const String ReliabilityMode_BestEffort_VALUE = "BestEffort";
/**
 * The value of a "NACK-based" reliability mode.
 */
const String ReliabilityMode_NackBased_VALUE = "NackBased";

const String PublisherReliabilityMode_DEFVALUE = ReliabilityMode_BestEffort_VALUE;


/**
 * Topic global scope.
 *
 * Whether topic scope is global or local.
 *
 * Optional property.
 *
 * Value: boolean.
 *
 * Default: false (local).
 *
 * @see Topic
 */
const String TopicGlobalScope_Prop_Key =
		"spidercast.messaging.topic.GlobalScope";

const bool TopicGlobalScope_DEFVALUE = false;


//=== Leader Election ======================================

/**
 * Leader election enabled.
 *
 * If this property is true, the node can create the leader election service.
 * If this property is false, it cannot. The state of this property in a given
 * node does not affect the ability of another node to create the leader
 * election service and use it.
 *
 * Optional property.
 *
 * Value: bool.
 *
 * Default: true.
 */
const String LeaderElectionEnabled_PROP_KEY =
		"spidercast.leader.election.Enabled";
const bool LeaderElectionEnabled_DEFVALUE = true;

/**
 * Leader election warm up timeout, in milliseconds.
 *
 * How much time will a node wait upon creation of the service,
 * in order to forge a candidate view, before
 * it proposes itself as a leader, in case it is the preferred candidate.
 *
 * In order to minimize leader changes upon join, the value should be
 * at least 2x the gossip interval.
 *
 * Optional property.
 */
const String LeaderElectionWarmupTimeoutMillis_PROP_KEY =
		"spidercast.leader.election.WarmupTimeoutMillis";
const int32_t LeaderElectionWarmupTimeoutMillis_DEFVALUE = 5000;


//=== Hierarchy ============================================

/**
 * Number of delegates per base zone.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 1.
 */
const String HierarchyNumberOfDelegates_PROP_KEY =
		"spidercast.hierarchy.NumberOfDelegates";
/**
 * Default value of HierarchyNumberOfDelegates_PROP_KEY.
 */
const int32_t HierarchyNumberOfDelegates_DEFVALUE = 1;

/**
 * Number of supervisors per base zone.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 1.
 */
const String HierarchyNumberOfSupervisors_PROP_KEY =
		"spidercast.hierarchy.NumberOfSupervisors";
/**
 * Default value of HierarchyNumberOfSupervisors_PROP_KEY.
 */
const int32_t HierarchyNumberOfSupervisors_DEFVALUE = 1;

/**
 * Number of active delegates per base zone.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 1.
 */
const String HierarchyNumberOfActiveDelegates_PROP_KEY =
		"spidercast.hierarchy.NumberOfActiveDelegates";
/**
 * Default value of HierarchyNumberOfActiveDelegates_PROP_KEY.
 */
const int32_t HierarchyNumberOfActiveDelegates_DEFVALUE = 1;

/**
 * Ask delegate to include attributes in the periodic embership updates.
 * <p>
 * Optional property.
 * <p>
 * Value - boolean.
 * <p>
 * Default: true.
 */
const String HierarchyIncludeAttributes_PROP_KEY =
		"spidercast.hierarchy.IncludeAttributes";
/**
 * Default value of HierarchyIncludeAttributes_PROP_KEY.
 */
const bool HierarchyIncludeAttributes_DEFVALUE = true;


/**
 * TimeOut for foreign zone membership requests
 * <p>
 * Optional property.
 * <p>
 * Value - int.
 * <p>
 * Default: 2500.
 */
const String HierarchyForeignZoneMemberhipTimeOut_PROP_KEY =
		"spidercast.hierarchy.ForeignZoneMemberhipTimeOut";
/**
 * Default value of HierarchyMemberhipUpdateAggregationInterval_PROP_KEY.
 */
const int32_t HierarchyForeignZoneMemberhipTimeOut_DEFVALUE = 2500;


/**
 * aggregation time in milliseconds of membership updates between the delegate and its supervisor
 * <p>
 * Optional property.
 * <p>
 * Value - short.
 * <p>
 * Default: 10.
 */
const String HierarchyMemberhipUpdateAggregationInterval_PROP_KEY =
		"spidercast.hierarchy.MemberhipUpdateAggregationInterval";
/**
 * Default value of HierarchyMemberhipUpdateAggregationInterval_PROP_KEY.
 */
const int16_t HierarchyMemberhipUpdateAggregationInterval_DEFVALUE = 10;

/**
 *
 */
const String HierarchyConnectIntervalMillis_PROP_KEY =
		"spidercast.hierarchy.ConnectIntervalMillis";
const int32_t HierarchyConnectIntervalMillis_DEFVALUE = 200;

/**
 *
 */
const String HierarchySupervisorQuarantineIntervalMillis_PROP_KEY =
		"spidercast.hierarchy.SupervisorQuarantineIntervalMillis";
const int32_t HierarchySupervisorQuarantineIntervalMillis_DEFVALUE = 10000;

/**
 * Enable hierarchy.
 *
 * Allows to disable the hierarchical component.
 * <p>
 * Optional property.
 * <p>
 * Value - boolean.
 * <p>
 * Default: true.
 */
const String HierarchyEnabled_PROP_KEY = "spidercast.hierarchy.Enabled";
/**
 * Default value of HierarchyNumberOfSupervisors_PROP_KEY.
 */
const bool HierarchyEnabled_DEFVALUE = true;

//=== Statistics ============================================

/**
 * Statistics enabled.
 *
 * Enables periodic statistics in the log file.
 * <p>
 * Optional property.
 * <p>
 * Value - boolean.
 * <p>
 * Default: true.
 */
const String StatisticsEnabled_PROP_KEY = "spidercast.statistics.Enabled";

/**
 * Default value of StatisticsEnabled_PROP_KEY.
 */
const int32_t StatisticsEnabled_DEFVALUE = true;

/**
 * Statistics period in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 10000.
 */
const String StatisticsPeriodMillis_PROP_KEY =
		"spidercast.statistics.PeriodMillis";

/**
 * Default value of StatisticsPeriodMillis_PROP_KEY.
 */
const int32_t StatisticsPeriodMillis_DEFVALUE = 30000;

/**
 * Task tardiness threshold in milliseconds.
 * <p>
 * Optional property.
 * <p>
 * Value - integer.
 * <p>
 * Default: 1000.
 */
const String StatisticsTaskTardinessThresholdMillis_PROP_KEY =
		"spidercast.statistics.TaskTardinessThresholdMillis";

/**
 * Default value of StatisticsPeriodMillis_PROP_KEY.
 */
const int32_t StatisticsTaskTardinessThresholdMillis_DEFVALUE = 1000;

//=== Debug ===========================================================

/**
 * Enable CRC on membership and topology messages.
 * <p>
 * Optional property.
 * <p>
 * This property must have the same value on all the nodes in the cluster.
 * <p>
 * Value - Boolean.
 * Default: false;
 */
const String CRC_MemTopoMsg_Enabled_PROP_KEY = "spidercast.debug.CRC.MemTopoMsg.Enabled";

const bool CRC_MemTopoMsg_Enabled_DEFVALUE = false;

} //namespace config

/**
 * SpiderCast main configuration object.
 * <p>
 * This object encapsulates all the configuration details needed to create a SpiderCast node.
 * An implementation of this object is created using SpiderCastFactory#createSpiderCastConfig().
 * <p>
 *
 * <b>The properties</b><br>
 *
 * One of the arguments to the factory method that creates this object is a PropertyMap
 * that holds key-value pairs. These properties configure every aspect of SpiderCast behavior.
 * <p>
 * The global variables under namespace spdr::config ending with the suffix _PROP_KEY, are the
 * documented public property keys that are accepted by SpiderCastConfig.
 * The corresponding variables that end with the suffix _DEFVALUE are the default
 * values of those properties.
 * <p>
 * Mandatory properties are:
 * <UL>
 * 	<LI> NodeName_PROP_KEY,
 * 	<LI> BusName_PROP_KEY,
 * 	<LI> NetworkInterface_PROP_KEY,
 * 	<LI> TCPReceiverPort_PROP_KEY.
 * </UL>
 *
 * <p>
 *
 * <b>The bootstrap set</b><br>
 *
 * The other parameter to the factory method is the bootstrap set. The bootstrap set contains a
 * set of NodeID's, which are used for discovery of overlay peers. The bootstrap set must contain
 * at least a single live node for the current node to be able to join the overlay.
 * <p>
 * In order for the overlay to be connected, the bootstrap sets of the nodes joining the overlay must
 * "overlap" in some informal sense.
 * <p>
 * Formally, let us represent the nodes and their (live) bootstrap-set nodes by a bipartite graph, where every
 * node points to its (live) bootstrap nodes. The bipartite graph should contain a single connected component
 * at some point in time, for a sufficient amount of time, for the nodes to discover each other and form
 * a connected overlay.
 * <p>
 * The factory SpiderCastFactory contains convenience methods for creating NodeID's from a string,
 * and for loading the bootstrap set from an input stream:
 * <UL>
 * 	<LI> SpiderCastFactory#createNodeID_SPtr()
 * 	<LI> SpiderCastFactory#createNodeID_SPtr()
 * 	<LI> SpiderCastFactory#loadBootstrapSet()
 * </UL>
 *
 * <p>
 *
 * @see SpiderCastFactory
 * @see PropertyMap
 * @see spdr::config
 * @see NodeID
 *
 *
 */
class SpiderCastConfig : public BasicConfig
{
public:
	virtual ~SpiderCastConfig();

	virtual PropertyMap getPropertyMap() const = 0;

	virtual std::vector<NodeID_SPtr > getBootStrapSet() const = 0;

	/**
	 * Verify the existence of the BindNetworkInterface property value using NETLINK API to the kernel.
	 * Verify the existence of the DiscoveryMulticastInOutInterface property value using NETLINK API to the kernel.
	 *
	 * @see BindNetworkInterface_PROP_KEY
	 * @see DiscoveryMulticastInOutInterface_PROP_KEY
	 *
	 * @param errMsg from kernel
	 * @param errCode from kernel
	 * @return true if OK, false if failed
	 */
	virtual bool verifyBindNetworkInterface(std::string& errMsg, int& errCode) = 0;

	/**
	 * A string representation of the configuration.
	 *
	 * @return
	 */
	virtual String toString() const;

protected:
	/**
	 * Construct from properties.
	 * @param properties
	 * @return
	 */
	SpiderCastConfig(const PropertyMap& properties);
	/**
	 * Copy constructor.
	 * @param config
	 * @return
	 */
	SpiderCastConfig(const SpiderCastConfig& config);

};

/**
 * A shared pointer to a SpiderCastConfig instance.
 */
typedef boost::shared_ptr<SpiderCastConfig>	SpiderCastConfig_SPtr;

} //namespace spdr

#endif /* SPIDERCASTCONFIG_H_ */
