/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

//****************************************************************************
/// @file  commonMemHandlerTypes.h
/// @brief Defines constants for each type of memory tracked by utils

/// @remark as well as the normal include of this file, commonMemHandler.c includes
/// this again as part of initialising the structure that describes the memory
/// Hence this file is fragile and should not be included directly, non-memHandler
/// code should include commonMemHandler.h

/// @remark THIS FILE SHOULD **NOT** BE INCLUDED DIRECTLY!
//   non-memHandler code should include commonMemHandler.h
//****************************************************************************

#ifdef COMMON_MEMHANDLER_RICH_TYPE_STRUCTURE
//Define the details structures used internally by the memory handler
typedef enum tag_ism_common_lowMemBehaviour_t {
  commonMem_DuringLowMemDisableEarly,
  commonMem_DuringLowMemDisable,
  commonMem_DuringLowMemEnabled,
  commonMem_DuringLowMemALWAYSEnable  //Use of this is a "code smell". It's memory we'd rather
                                 //end the process than not get. We should not have such mallocs
} ism_common_lowMemBehaviour_t;


typedef struct tag_ism_common_memTypeInfo_t  {
    const char *typeName;
    ism_common_memoryGroup group;
    const char *description;
    ism_common_lowMemBehaviour_t behaviour;
} ism_common_memTypeInfo_t;

typedef struct tag_ism_common_memGroupInfo_t {
    const char *description;
    bool printable;
} ism_common_memGroupInfo_t;

#define ISM_COMMON_MEMORY_GROUP_START( numgroups ) ism_common_memGroupInfo_t ism_common_memInfo[numgroups] = {
#define ISM_COMMON_MEMORY_GROUP( group, description, printable) {description, printable},
#define ISM_COMMON_MEMORY_GROUP_END(type) };

#define ISM_COMMON_MEMORY_TYPE_START( numtypes ) ism_common_memTypeInfo_t ism_common_memTypeInfo[numtypes] = {
#define ISM_COMMON_MEMORY_TYPE( type, group, description, lowMemBehaviour) {#type, group, description, lowMemBehaviour},
#define ISM_COMMON_MEMORY_TYPE_END(type) };

#else
//Set up the defines used to define the types and groups enum that is "publicly available"

#define ISM_COMMON_MEMORY_GROUP_START( numgroups ) typedef enum tag_memoryGroup {
#define ISM_COMMON_MEMORY_GROUP( group, description, printable) group,
#define ISM_COMMON_MEMORY_GROUP_END(type) type } ism_common_memoryGroup;

#define ISM_COMMON_MEMORY_TYPE_START( numtypes ) typedef enum tag_memoryType {
#define ISM_COMMON_MEMORY_TYPE( type, group, description, lowMemBehaviour) type,
#define ISM_COMMON_MEMORY_TYPE_END(type) type } ism_common_memoryType;
#endif

    ISM_COMMON_MEMORY_GROUP_START(ism_common_mem_numgroups)
    ISM_COMMON_MEMORY_GROUP(ism_memory_Transport,             "NetworkTransport",     true)
    ISM_COMMON_MEMORY_GROUP(ism_memory_Generic,               "Miscellaneous",        true)
    ISM_COMMON_MEMORY_GROUP(ism_memory_Utilities,             "Utilities",            true)
    ISM_COMMON_MEMORY_GROUP(ism_memory_Test,                  "UnitTesting",          false)
    ISM_COMMON_MEMORY_GROUP(ism_memory_Proxy,                 "Proxy",                true)
    ISM_COMMON_MEMORY_GROUP(ism_memory_SaslScram,             "Sasl-SCRAM",           true)
    ///Add new groups above this line...
    ISM_COMMON_MEMORY_GROUP_END(ism_common_mem_numgroups)

    ISM_COMMON_MEMORY_TYPE_START(ism_common_mem_numtypes)
    ISM_COMMON_MEMORY_TYPE(ism_memory_cpp,                       ism_memory_Generic,             "SpecialValueForCPP",                  commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_transportBuffers,          ism_memory_Transport,           "NetworkBuffers",                      commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_tls,                       ism_memory_Transport,           "TLSBuffers",                          commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_http,                      ism_memory_Transport,           "HTTPBuffers",                         commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_clusterfwd,                ism_memory_Transport,           "ClusterForwarder",                    commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_concatAlloc,               ism_memory_Generic,             "ExtensionBuffers",                    commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_misc,                ism_memory_Generic,             "ServerUtilsDefault",                  commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_admin_misc,                ism_memory_Generic,             "ServerAdminDefault",                  commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_main_misc,                 ism_memory_Generic,             "ServerMainDefault",                   commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_engine_misc,               ism_memory_Generic,             "ServerEngineDefault",                 commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_snmp_misc,                 ism_memory_Generic,             "ServerSnmpDefault",                   commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_ismc_misc,                 ism_memory_Generic,             "ServerIsmcDefault",                   commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_monitoring_misc,           ism_memory_Generic,             "ServerMonitoringDefault",             commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_cluster_misc,              ism_memory_Generic,             "ServerClusterDefault",                commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_spidercast_misc,           ism_memory_Generic,             "ServerSpidercastDefault",             commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_bedrock_misc,              ism_memory_Generic,             "ServerBedrockDefault",                commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_utils,               ism_memory_Proxy,               "ServerProxyUtilities",                commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_trace_misc,                ism_memory_Generic,             "ServerTraceDefault",                  commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_store_misc,                ism_memory_Generic,             "ServerStoreDefault",                  commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_protocol_misc,             ism_memory_Generic,             "ServerProtocolDefault",               commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_mqcbridge_misc,            ism_memory_Generic,             "ServerMqcbridgeDefault",              commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_br_misc,             ism_memory_Generic,             "ServerProxyBrDefault",                commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_parser,              ism_memory_Utilities,           "UtilitiesParser",                     commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_alloc_buffer,              ism_memory_Utilities,           "AllocBuffers",                        commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_unit_test,                 ism_memory_Test,                "UnitTests",                           commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_to_lower,            ism_memory_Utilities,           "ServerUtilsTolower",                  commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_ssl_functions,             ism_memory_Utilities,           "SslFunctions",                        commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_transportProfile,          ism_memory_Transport,           "SecurityProfiles",                    commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_auth,                ism_memory_Proxy,               "ProxyAuth",                           commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_device_auth,         ism_memory_Proxy,               "ProxyDeviceAuth",                     commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_eventstreams,        ism_memory_Proxy,               "ProxyEventStreams",                   commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_kafka_message,       ism_memory_Proxy,               "ProxyKafkaMessages",                  commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_kafka,               ism_memory_Proxy,               "ProxyKafka",                          commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mqtt_auth,           ism_memory_Proxy,               "ProxyMQTTAuth",                       commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mqtt,                ism_memory_Proxy,               "ProxyMQTTGeneral",                    commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mqtt_subscribe,      ism_memory_Proxy,               "ProxyMQTTSubscriptions",              commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mqtt_savedata,       ism_memory_Proxy,               "ProxyMQTTSavedata",                   commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mqtt_connection,     ism_memory_Proxy,               "ProxyMQTTConnection",                 commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mqtt_receive,        ism_memory_Proxy,               "ProxyMQTTRecieve",                    commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mqtt_request,        ism_memory_Proxy,               "ProxyMQTTRequest",                    commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mqtt_send,           ism_memory_Proxy,               "ProxyMQTTSend",                       commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_tcp,                 ism_memory_Proxy,               "ProxyTCP",                            commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_kafka_global,        ism_memory_Proxy,               "ProxyKafkaGlobalState",               commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_routing,             ism_memory_Proxy,               "ProxyRouting",                        commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mux_stats,           ism_memory_Proxy,               "ProxyMuxStats",                       commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mux_virtual_connection, ism_memory_Proxy,            "ProxyMuxVirtualConnections",          commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_mux_connection,      ism_memory_Proxy,               "ProxyMuxConnections",                 commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_http,                ism_memory_Proxy,               "ProxyHTTP",                           commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_tenant,              ism_memory_Proxy,               "ProxyTenant",                         commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_transport,           ism_memory_Proxy,               "ProxyTransport",                      commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_transport_endpoint,  ism_memory_Proxy,               "ProxyTransportEndpoints",             commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_stats,               ism_memory_Proxy,               "ProxyStats",                          commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_javaconfig,          ism_memory_Proxy,               "ProxyJavaConfig",                     commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_proxy_config,              ism_memory_Proxy,               "ProxyConfig",                         commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_saslScram,                 ism_memory_SaslScram,           "SaslSCRAM",                           commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_saslScramProfile,          ism_memory_SaslScram,           "SaslSCRAMProfile",                    commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_bufferPools,               ism_memory_Utilities,           "BufferPools",                         commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_rehash,              ism_memory_Utilities,           "Certificate Rehashing",               commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_filter,              ism_memory_Utilities,           "Filters",                             commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_selector,            ism_memory_Utilities,           "Selectors",                           commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_props,               ism_memory_Utilities,           "Properties",                          commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_log,                 ism_memory_Utilities,           "Logging",                             commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_trace,               ism_memory_Utilities,           "Tracing",                             commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_map,                 ism_memory_Utilities,           "Hash maps",                           commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_array,               ism_memory_Utilities,           "Arrays",                              commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_list,                ism_memory_Utilities,           "Lists",                               commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_sslutils,            ism_memory_Utilities,           "SSL Utility Functions",               commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_xml,                 ism_memory_Utilities,           "XML Parsing",                         commonMem_DuringLowMemEnabled)
    ISM_COMMON_MEMORY_TYPE(ism_memory_utils_throttle,            ism_memory_Utilities,           "Throttling",                          commonMem_DuringLowMemEnabled)
    ///Add new types above this line... 
    ISM_COMMON_MEMORY_TYPE_END(ism_common_mem_numtypes)

#undef ISM_COMMON_MEMORY_GROUP_START
#undef ISM_COMMON_MEMORY_GROUP
#undef ISM_COMMON_MEMORY_GROUP_END

#undef ISM_COMMON_MEMORY_TYPE_START
#undef ISM_COMMON_MEMORY_TYPE
#undef ISM_COMMON_MEMORY_TYPE_END
