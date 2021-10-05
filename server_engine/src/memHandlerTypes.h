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
/// @file  memHandlerTypes.h
/// @brief Defines constants for each type of memory used by the engine

/// @remark as well as the normal include of this file, memHandler.c includes
/// this again as part of initialising the structure that describes the memory
/// Hence this file is fragile and should not be included directly, non-memHandler
/// code should include memHandler.h

/// @remark THIS FILE SHOULD **NOT** BE INCLUDED DIRECTLY!
//   non-memHandler code should include memHandler.h
//****************************************************************************

#ifdef IEMEM_MEMHANDLER_RICH_TYPE_STRUCTURE
//Define the details structures used internally by the memory handler
typedef enum tag_iememLowMemBehaviour_t {
  iememDuringLowMemDisableEarly,
  iememDuringLowMemDisable,
  iememDuringLowMemEnabled,
  iememDuringLowMemALWAYSEnable  //Use of this is a "code smell". It's memory we'd rather
                                 //end the process than not get. We should not have such mallocs
} iememLowMemBehaviour_t;


typedef struct tag_iememMemTypeInfo_t  {
    const char *typeName;
    iemem_memoryGroup group;
    const char *description;
    iememLowMemBehaviour_t behaviour;
} iememMemTypeInfo_t;

typedef struct tag_memGroupInfo_t {
    const char *description;
} iememMemGroupInfo_t;

#define IEMEM_MEMORY_GROUP_START( numgroups ) iememMemGroupInfo_t iememMemInfo[numgroups] = {
#define IEMEM_MEMORY_GROUP( group, description) {description},
#define IEMEM_MEMORY_GROUP_END(type) };

#define IEMEM_MEMORY_TYPE_START( numtypes ) iememMemTypeInfo_t iememMemTypeInfo[numtypes] = {
#define IEMEM_MEMORY_TYPE( type, group, description, lowMemBehaviour) {#type, group, description, lowMemBehaviour},
#define IEMEM_MEMORY_TYPE_END(type) };

#else
//Set up the defines used to define the types and groups enum that is "publicly available"

#define IEMEM_MEMORY_GROUP_START( numgroups ) typedef enum tag_iemem_memoryGroup {
#define IEMEM_MEMORY_GROUP( group, description) group,
#define IEMEM_MEMORY_GROUP_END(type) type } iemem_memoryGroup;

#define IEMEM_MEMORY_TYPE_START( numtypes ) typedef enum tag_iemem_memoryType {
#define IEMEM_MEMORY_TYPE( type, group, description, lowMemBehaviour) type,
#define IEMEM_MEMORY_TYPE_END(type) type } iemem_memoryType;
#endif

    IEMEM_MEMORY_GROUP_START(iemem_numgroups)
    IEMEM_MEMORY_GROUP(iememGroupMessages,         "Message payloads")
    IEMEM_MEMORY_GROUP(iememGroupPublishSubscribe, "Publish/subscribe - topic tree and subscriptions")
    IEMEM_MEMORY_GROUP(iememGroupDestinations,     "Queues and subscription queues")
    IEMEM_MEMORY_GROUP(iememGroupCurrentActivity,  "Current activity")
    IEMEM_MEMORY_GROUP(iememGroupRecovery,         "Restart recovery")
    IEMEM_MEMORY_GROUP(iememGroupClientStates,     "Client states")
    ///Add new groups above this line...
    IEMEM_MEMORY_GROUP_END(iemem_numgroups)

    IEMEM_MEMORY_TYPE_START(iemem_numtypes)
    IEMEM_MEMORY_TYPE(iemem_messageBody,          iememGroupMessages,           "Message Bodies",                     iememDuringLowMemDisableEarly)
    IEMEM_MEMORY_TYPE(iemem_topicAnalysis,        iememGroupPublishSubscribe,   "Topic Analysis",                     iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_subsTree,             iememGroupPublishSubscribe,   "Subscriber Tree",                    iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_namedSubs,            iememGroupPublishSubscribe,   "Named Subscriptions",                iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_subscriberCache,      iememGroupPublishSubscribe,   "Subscriber Cache",                   iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_subsQuery,            iememGroupPublishSubscribe,   "Subscriber Query",                   iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_topicsTree,           iememGroupPublishSubscribe,   "Topic Tree",                         iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_topicsQuery,          iememGroupPublishSubscribe,   "Topic Query",                        iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_clientState,          iememGroupClientStates,       "Client State",                       iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_callbackContext,      iememGroupCurrentActivity,    "Callback Context",                   iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_policyInfo,           iememGroupDestinations,       "Policy Information",                 iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_queueNamespace,       iememGroupDestinations,       "Named Queues",                       iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_simpleQ,              iememGroupDestinations,       "Simple Queue Definitions",           iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_simpleQPage,          iememGroupDestinations,       "Simple Queue Pages",                 iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_intermediateQ,        iememGroupDestinations,       "Intermediate Queue Definitions",     iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_intermediateQPage,    iememGroupDestinations,       "Intermediate Queue Pages",           iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_multiConsumerQ,       iememGroupDestinations,       "Multi-consumer Queue Definitions",   iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_multiConsumerQPage,   iememGroupDestinations,       "Multi-consumer Queue Pages",         iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_lockManager,          iememGroupCurrentActivity,    "Lock Manager",                       iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_mqBridgeRecords,      iememGroupCurrentActivity,    "MQ Connectivity",                    iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_restoreTable,         iememGroupRecovery,           "Recovery Table",                     iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_externalObjs,         iememGroupCurrentActivity,    "External Engine Objects",            iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_localTransactions,    iememGroupCurrentActivity,    "Local Transactions",                 iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_globalTransactions,   iememGroupCurrentActivity,    "Global Transactions",                iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_monitoringData,       iememGroupCurrentActivity,    "Monitoring Data",                    iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_notificationData,     iememGroupCurrentActivity,    "Notification Data",                  iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_messageExpiryData,    iememGroupDestinations,       "Message Expiry Tracking",            iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_remoteServers,        iememGroupPublishSubscribe,   "Remote Server Info",                 iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_commitData,           iememGroupCurrentActivity,    "Memory allocd during commit",        iememDuringLowMemALWAYSEnable)
    IEMEM_MEMORY_TYPE(iemem_unneededRetainedMsgs, iememGroupRecovery,           "Unneeded retained messages",         iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_expiringGetData,      iememGroupCurrentActivity,    "Expiring Get Data",                  iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_exportResources,      iememGroupCurrentActivity,    "Export Data for a set of resources", iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_diagnostics,          iememGroupCurrentActivity,    "Diagnostics",                        iememDuringLowMemDisable)
    IEMEM_MEMORY_TYPE(iemem_unneededBufferedMsgs, iememGroupRecovery,           "Unneeded buffered messages",         iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_jobQueues,            iememGroupCurrentActivity,    "Inter-Thread Work Scheduling",       iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_resourceSetStats,     iememGroupCurrentActivity,    "ResourceSet Statistics",             iememDuringLowMemEnabled)
    IEMEM_MEMORY_TYPE(iemem_deferredFreeLists,    iememGroupCurrentActivity,    "Deferred Free Infrastructure",       iememDuringLowMemEnabled)
    ///Add new types above this line... And don't forget to update the memory statistics routines (memHandler.c) to include them...
    IEMEM_MEMORY_TYPE_END(iemem_numtypes)

#undef IEMEM_MEMORY_GROUP_START
#undef IEMEM_MEMORY_GROUP
#undef IEMEM_MEMORY_GROUP_END

#undef IEMEM_MEMORY_TYPE_START
#undef IEMEM_MEMORY_TYPE
#undef IEMEM_MEMORY_TYPE_END
