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

#ifndef __MONOBJS_DEFINED
#define __MONOBJS_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef XAPI
    #define XAPI extern
#endif

#include <ismutil.h>
#include <security.h>
#include <monserialization.h>
#include <transport.h>
#include <stddef.h>

typedef struct
{
	char * Name;
	char * IPAddr;
	char * Enabled;
	char * Total;
	char * Active;
	char * Messages;
	char * Bytes;
	char * LastErrorCode;
	ism_time_t  ConfigTime;
	//ism_time_t  ResetTime;
	char * BadConnections;
}ismEndpointMonDataOut;

typedef struct
{
	uint64_t ActiveConnections;
	uint64_t TotalConnections;
	uint64_t MsgRead;
    uint64_t MsgWrite;
    uint64_t BytesRead ;
    uint64_t BytesWrite;
    uint64_t BadConnCount;
    int		 TotalEndpoints;
    uint64_t BufferedMessages;
    uint64_t RetainedMessages;
    uint64_t ExpiredMessages;
    uint64_t ClientSessions;
    uint64_t ExpiredClientSessions;
    uint64_t Subscriptions;
}ismConnectionVolumeData;

#define DEFINE_CONNECTION_VOLUME_MONITORING_OBJ(mon_obj)                                                       \
const ism_mon_obj_def_t mon_obj [] = {                                                                         \
		ISMMONOBJ(ismConnectionVolumeData, L, L, ActiveConnections, 			ActiveConnections,		OID_STR,	1000, 1, ActiveConnections)     		\
		ISMMONOBJ(ismConnectionVolumeData, L, L, TotalConnections, 				TotalConnections,		OID_STR,	1000, 1, TotalConnections)     		\
		ISMMONOBJ(ismConnectionVolumeData, L, L, MsgRead, 						MsgRead,				OID_STR,	1000, 1, MsgRead)     		\
		ISMMONOBJ(ismConnectionVolumeData, L, L, MsgWrite, 						MsgWrite,				OID_STR,	1000, 1, MsgWrite)		     \
		ISMMONOBJ(ismConnectionVolumeData, L, L, BytesRead, 					BytesRead,				OID_STR,	1000, 1, BytesRead)		     \
		ISMMONOBJ(ismConnectionVolumeData, L, L, BytesWrite, 					BytesWrite,				OID_STR,	1000, 1, BytesWrite)		     \
		ISMMONOBJ(ismConnectionVolumeData, L, L, BadConnCount, 					BadConnCount,			OID_STR,	1000, 1, BadConnCount)		     \
		ISMMONOBJ(ismConnectionVolumeData, I, I, TotalEndpoints, 				TotalEndpoints,			OID_STR,	1000, 1, TotalEndpoints)		     \
		ISMMONOBJ(ismConnectionVolumeData, L, L, BufferedMessages, 				BufferedMessages,		OID_STR,	1000, 1, BufferedMessages)		     \
		ISMMONOBJ(ismConnectionVolumeData, L, L, RetainedMessages, 				RetainedMessages,		OID_STR,	1000, 1, RetainedMessages)		     \
		ISMMONOBJ(ismConnectionVolumeData, L, L, ExpiredMessages, 				ExpiredMessages,		OID_STR,	1000, 1, ExpiredMessages)		     \
        ISMMONOBJ(ismConnectionVolumeData, L, L, ClientSessions,                ClientSessions,         OID_STR,    1000, 1, ClientSessions)            \
        ISMMONOBJ(ismConnectionVolumeData, L, L, ExpiredClientSessions,         ExpiredClientSessions,  OID_STR,    1000, 1, ExpiredClientSessions)            \
        ISMMONOBJ(ismConnectionVolumeData, L, L, Subscriptions,                 Subscriptions,          OID_STR,    1000, 1, Subscriptions)            \
		{0,0,0,0,0,0,0}                                                                                        \
};

#ifdef AUTH_TIME_STAT_ENABLED
#define DEFINE_CONNECTION_SECURITY_MONITORING_OBJ(mon_obj)                                                     \
const ism_mon_obj_def_t mon_obj [] = {                                                                         \
		ISMMONOBJ(security_stat_t, L, L,   authen_passed, 		AuthenticationSuccessCount,			OID_STR,	1000, 1, AuthenticationSuccessCount)     		\
		ISMMONOBJ(security_stat_t, L, L,   authen_failed, 		AuthenticationFailedCount,			OID_STR,	1000, 1, AuthenticationFailedCount)     		\
		ISMMONOBJ(security_stat_t, L, L,   author_conn_passed,	ConnectionAuthorizationSuccessCount,		OID_STR,	1000, 1, ConnectionAuthorizationSuccessCount)   \
		ISMMONOBJ(security_stat_t, L, L,   author_conn_failed, 	ConnectionAuthorizationFailedCount,		OID_STR,	1000, 1, ConnectionAuthorizationFailedCount)    \
		ISMMONOBJ(security_stat_t, L, L,   author_msgn_passed, 	MessagingAuthorizationSuccessCount,		OID_STR,	1000, 1, MessagingAuthorizationSuccessCount)    \
		ISMMONOBJ(security_stat_t, L, L,   author_msgn_failed, 	MessagingAuthorizationFailedCount,		OID_STR,	1000, 1, MessagingAuthorizationFailedCount)     \
		ISMMONOBJ(security_stat_t, SP, SP, author_total_time, 	AuthorizationTotalTime,		OID_STR,	1000, 1, MessagingAuthorizationFailedCount)     \
};
#else
#define DEFINE_CONNECTION_SECURITY_MONITORING_OBJ(mon_obj)                                                     \
const ism_mon_obj_def_t mon_obj [] = {                                                                         \
		ISMMONOBJ(security_stat_t, L, L,   authen_passed, 		AuthenticationSuccessCount,			OID_STR,	1000, 1, AuthenticationSuccessCount)     		\
		ISMMONOBJ(security_stat_t, L, L,   authen_failed, 		AuthenticationFailedCount,			OID_STR,	1000, 1, AuthenticationFailedCount)     		\
		ISMMONOBJ(security_stat_t, L, L,   author_conn_passed,	ConnectionAuthorizationSuccessCount,		OID_STR,	1000, 1, ConnectionAuthorizationSuccessCount)   \
		ISMMONOBJ(security_stat_t, L, L,   author_conn_failed, 	ConnectionAuthorizationFailedCount,		OID_STR,	1000, 1, ConnectionAuthorizationFailedCount)    \
		ISMMONOBJ(security_stat_t, L, L,   author_msgn_passed, 	MessagingAuthorizationSuccessCount,		OID_STR,	1000, 1, MessagingAuthorizationSuccessCount)    \
		ISMMONOBJ(security_stat_t, L, L,   author_msgn_failed, 	MessagingAuthorizationFailedCount,		OID_STR,	1000, 1, MessagingAuthorizationFailedCount)     \
};
#endif

typedef struct
{
	int    Duration;
	uint64_t * Interval;
	char * StoreMemUsagePct;
	char * StoreDiskUsagePct;
	char * DiskFreeSpaceBytes;
	char * MemoryUsedPercent;
	char * DiskUsedPercent;
	char * DiskFreeBytes;
	char * IncomingMessageAcksBytes;
	char * ClientStatesBytes;
	char * QueuesBytes;
	char * SubscriptionsBytes;
	char * TopicsBytes;
	char * TransactionsBytes;
	char * MQConnectivityBytes;
	char * MemoryTotalBytes;
	char * Pool1TotalBytes;
	char * Pool1UsedBytes;
	char * Pool1RecordsLimitBytes;
	char * Pool1RecordsUsedBytes;
	char * Pool2TotalBytes;
	char * Pool2UsedBytes;
	char * Pool1UsedPercent;
	char * Pool2UsedPercent;

	
	
}ismStoreMonHistoryDataOut;

#define DEFINE_STORE_MONITORING__HISTORY_OBJ(mon_obj)                                                       \
const ism_mon_obj_def_t mon_obj [] = {                                                                         \
		ISMMONOBJ(ismStoreMonHistoryDataOut, I, I,   Duration, 									Duration,	OID_STR,	1000, 1, Enabled)  \
		ISMMONOBJ(ismStoreMonHistoryDataOut, L, L,   Interval,									Interval, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, StoreMemUsagePct, 						StoreMemUsagePct,	OID_STR,	1000, 1, Enabled)  \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, StoreDiskUsagePct,						StoreDiskUsagePct, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, DiskFreeSpaceBytes,					DiskFreeSpaceBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, MemoryUsedPercent, 					MemoryUsedPercent,	OID_STR,	1000, 1, Enabled)  \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, DiskUsedPercent,						DiskUsedPercent, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, DiskFreeBytes,							DiskFreeBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, IncomingMessageAcksBytes,				IncomingMessageAcksBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, ClientStatesBytes,						ClientStatesBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, QueuesBytes,							QueuesBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, SubscriptionsBytes,					SubscriptionsBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, TopicsBytes,							TopicsBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, TransactionsBytes,						TransactionsBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, MQConnectivityBytes,					MQConnectivityBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, MemoryTotalBytes,						MemoryTotalBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, Pool1TotalBytes,						Pool1TotalBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, Pool1UsedBytes,						Pool1UsedBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, Pool1RecordsLimitBytes,				Pool1RecordsLimitBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, Pool1RecordsUsedBytes,					Pool1RecordsUsedBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, Pool2TotalBytes,						Pool2TotalBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, Pool2UsedBytes,						Pool2UsedBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, Pool1UsedPercent,						Pool1UsedPercent, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismStoreMonHistoryDataOut, HIST, HIST, Pool2UsedPercent,						Pool2UsedPercent, 	OID_STR,	1000, 1, Total)    \
		{0,0,0,0,0,0,0}                                                                                        \
};

typedef struct
{
	int    Duration;
	uint64_t * Interval;
	char * MemoryTotalBytes;
	char * MemoryFreeBytes;
	char * MemoryFreePercent;
	char * ServerVirtualMemoryBytes;
	char * ServerResidentSetBytes;
	char * GroupMessagePayloads;
	char * GroupPublishSubscribe;
	char * GroupDestinations;
	char * GroupCurrentActivity;
	char * GroupClientStates;
	
}ismMemoryMonHistoryDataOut;

#define DEFINE_MEMORY_MONITORING__HISTORY_OBJ(mon_obj)                                                       \
const ism_mon_obj_def_t mon_obj [] = {                                                                         \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, I, I,   Duration, 								Duration,	OID_STR,	1000, 1, Enabled)  \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, L, L,   Interval,									Interval, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, MemoryTotalBytes, 					MemoryTotalBytes,	OID_STR,	1000, 1, Enabled)  \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, MemoryFreeBytes,						MemoryFreeBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, MemoryFreePercent,					MemoryFreePercent, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, ServerVirtualMemoryBytes,				ServerVirtualMemoryBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, ServerResidentSetBytes,				ServerResidentSetBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, GroupMessagePayloads,					MessagePayloads, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, GroupPublishSubscribe,				PublishSubscribe, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, GroupDestinations,					Destinations, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, GroupCurrentActivity,					CurrentActivity, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonHistoryDataOut, HIST, HIST, GroupClientStates,					ClientStates, 	OID_STR,	1000, 1, Total)    \
		{0,0,0,0,0,0,0}                                                                                        \
};

typedef struct
{
	uint64_t MemoryTotalBytes;
	uint64_t MemoryFreeBytes;
	uint64_t MemoryFreePercent;
	uint64_t ServerVirtualMemoryBytes;
	uint64_t ServerResidentSetBytes;
	uint64_t MessagePayloads;
	uint64_t PublishSubscribe;
	uint64_t Destinations;
	uint64_t CurrentActivity;
}ismMemoryMonDataOut;


#define DEFINE_MEMORY_MONITORING_OBJ(mon_obj)                                                       \
const ism_mon_obj_def_t mon_obj [] = {                                                                         \
		ISMMONOBJ(ismMemoryMonDataOut, L, L, MemoryTotalBytes, 					MemoryTotalBytes,	OID_STR,	1000, 1, Enabled)  \
		ISMMONOBJ(ismMemoryMonDataOut, L, L, MemoryFreeBytes,					MemoryFreeBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonDataOut, L, L, MemoryFreePercent,					MemoryFreePercent, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonDataOut, L, L, ServerVirtualMemoryBytes,			ServerVirtualMemoryBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonDataOut, L, L, ServerResidentSetBytes,			ServerResidentSetBytes, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonDataOut, L, L, MessagePayloads,					MessagePayloads, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonDataOut, L, L, PublishSubscribe,					PublishSubscribe, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonDataOut, L, L, Destinations,						Destinations, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismMemoryMonDataOut, L, L, CurrentActivity,					CurrentActivity, 	OID_STR,	1000, 1, Total)    \
		{0,0,0,0,0,0,0}                                                                                        \
};

typedef struct
{
	uint64_t DiskUsedPercent;				/* SubType=Current only */
	uint64_t DiskFreeBytes;					/* SubType=Current only */
	uint64_t MemoryUsedPercent;
	uint64_t MemoryTotalBytes;
	uint64_t Pool1TotalBytes;
	uint64_t Pool1UsedBytes;
	uint64_t Pool1UsedPercent;
	uint64_t Pool1RecordsLimitBytes;
	uint64_t Pool1RecordsUsedBytes;
	uint64_t Pool1RecordSizeBytes;			/* SubType=MemoryDetail only */
	uint64_t Pool2TotalBytes;
	uint64_t Pool2UsedBytes;
	uint64_t Pool2UsedPercent;
	uint64_t ClientStatesBytes;				/* SubType=MemoryDetail only */
	uint64_t SubscriptionsBytes;			/* SubType=MemoryDetail only */
	uint64_t TopicsBytes;					/* SubType=MemoryDetail only */
	uint64_t TransactionsBytes;				/* SubType=MemoryDetail only */
	uint64_t QueuesBytes;					/* SubType=QueuesBytes only */
	uint64_t MQConnectivityBytes;			/* SubType=QueuesBytes only */
	uint64_t IncomingMessageAcksBytes;		/* SubType=QueuesBytes only */
}ismStoreMonDataOut;


#define DEFINE_STORE_MONITORING_OBJ(mon_obj)                                                       \
const ism_mon_obj_def_t mon_obj [] = {                                                                         \
		ISMMONOBJ(ismStoreMonDataOut, L, L, DiskUsedPercent, 				DiskUsedPercent,		OID_STR,	1000, 1, DiskUsedPercent)  \
		ISMMONOBJ(ismStoreMonDataOut, L, L, DiskFreeBytes, 					DiskFreeBytes,			OID_STR,	1000, 1, DiskFreeBytes)  \
		ISMMONOBJ(ismStoreMonDataOut, L, L, MemoryUsedPercent, 				MemoryUsedPercent,		OID_STR,	1000, 1, MemoryUsedPercent)  \
		ISMMONOBJ(ismStoreMonDataOut, L, L, MemoryTotalBytes,				MemoryTotalBytes, 		OID_STR,	1000, 1, MemoryTotalBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1TotalBytes,				Pool1TotalBytes, 	    OID_STR,	1000, 1, Pool1TotalBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1UsedBytes,					Pool1UsedBytes, 	    OID_STR,	1000, 1, Pool1UsedBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1UsedPercent,				Pool1UsedPercent, 	    OID_STR,	1000, 1, Pool1UsedPercent)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1RecordsLimitBytes,			Pool1RecordsLimitBytes, OID_STR,	1000, 1, Pool1RecordsLimitBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1RecordsUsedBytes,			Pool1RecordsUsedBytes, 	OID_STR,	1000, 1, Pool1RecordsUsedBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool2TotalBytes,				Pool2TotalBytes, 	    OID_STR,	1000, 1, Pool2TotalBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool2UsedBytes,					Pool2UsedBytes, 	    OID_STR,	1000, 1, Pool2UsedBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool2UsedPercent,				Pool2UsedPercent, 	    OID_STR,	1000, 1, Pool2UsedPercent)    \
		{0,0,0,0,0,0,0}                                                                                        \
};

#define DEFINE_STORE_MEM_DETAIL_MONITORING_OBJ(mon_obj)                                                       \
const ism_mon_obj_def_t mon_obj [] = {                                                                         \
		ISMMONOBJ(ismStoreMonDataOut, L, L, MemoryUsedPercent, 				MemoryUsedPercent,			OID_STR,	1000, 1, MemoryUsedPercent)  \
		ISMMONOBJ(ismStoreMonDataOut, L, L, MemoryTotalBytes,				MemoryTotalBytes, 			OID_STR,	1000, 1, MemoryTotalBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1TotalBytes,				Pool1TotalBytes, 	    	OID_STR,	1000, 1, Pool1TotalBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1UsedBytes,					Pool1UsedBytes, 	    	OID_STR,	1000, 1, Pool1UsedBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1UsedPercent,				Pool1UsedPercent, 	    	OID_STR,	1000, 1, Pool1UsedPercent)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1RecordSizeBytes,			Pool1RecordSizeBytes, 		OID_STR,	1000, 1, Pool1RecordSizeBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1RecordsLimitBytes,			Pool1RecordsLimitBytes, 	OID_STR,	1000, 1, Pool1RecordsLimitBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool1RecordsUsedBytes,			Pool1RecordsUsedBytes, 		OID_STR,	1000, 1, Pool1RecordsUsedBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, ClientStatesBytes,				ClientStatesBytes, 	    	OID_STR,	1000, 1, ClientStatesBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, SubscriptionsBytes,				SubscriptionsBytes,			OID_STR,	1000, 1, SubscriptionsBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, TopicsBytes,					TopicsBytes, 	    		OID_STR,	1000, 1, TopicsBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, TransactionsBytes,				TransactionsBytes, 	    	OID_STR,	1000, 1, TransactionsBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, QueuesBytes,					QueuesBytes, 	    		OID_STR,	1000, 1, QueuesBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, MQConnectivityBytes,			MQConnectivityBytes, 		OID_STR,	1000, 1, MQConnectivityBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool2TotalBytes,				Pool2TotalBytes, 	    	OID_STR,	1000, 1, Pool2TotalBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool2UsedBytes,					Pool2UsedBytes, 	    	OID_STR,	1000, 1, Pool2UsedBytes)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, Pool2UsedPercent,				Pool2UsedPercent, 	    	OID_STR,	1000, 1, Pool2UsedPercent)    \
		ISMMONOBJ(ismStoreMonDataOut, L, L, IncomingMessageAcksBytes,		IncomingMessageAcksBytes,	OID_STR,	1000, 1, IncomingMessageAcksBytes)    \
		{0,0,0,0,0,0,0}                                                                                        \
};

typedef struct
{
	char * Endpoint;
	char * Type;
	int    Duration;
	uint64_t * Interval;
	char * Data;
}ismEndpointMonHistoryDataOut;

#define DEFINE_ENDPOINT_MONITORING__HISTORY_OBJ(mon_obj)                                                       \
const ism_mon_obj_def_t mon_obj [] = {                                                                         \
		ISMMONOBJ(ismEndpointMonHistoryDataOut, SP, SP, Endpoint, 	Endpoint,	OID_STR,	1000, 1, Name)     \
		ISMMONOBJ(ismEndpointMonHistoryDataOut, SP, SP, Type, 		Type,		OID_STR,	1000, 1, IPAddr)   \
		ISMMONOBJ(ismEndpointMonHistoryDataOut, I, I, Duration, 	Duration,	OID_STR,	1000, 1, Enabled)  \
		ISMMONOBJ(ismEndpointMonHistoryDataOut, L, L, Interval,		Interval, 	OID_STR,	1000, 1, Total)    \
		ISMMONOBJ(ismEndpointMonHistoryDataOut, SP, SP, Data, 		Data,		OID_STR,	1000, 1, Active)   \
		{0,0,0,0,0,0,0}                                                                                        \
};


#define DEFINE_ENDPOINT_MONITORING_OBJ(mon_obj)                                                                             \
const ism_mon_obj_def_t mon_obj [] = {                                                                                      \
		ISMMONOBJ(ismEndpointMonDataOut, SP, SP, Name, 			    Name,			OID_STR,	1000, 1, Name)              \
		ISMMONOBJ(ismEndpointMonDataOut, SP, SP, IPAddr, 			IPAddr,			OID_STR,	1000, 1, IPAddr)            \
		ISMMONOBJ(ismEndpointMonDataOut, SP, SP, Enabled, 			Enabled,		OID_STR,	1000, 1, Enabled)           \
		ISMMONOBJ(ismEndpointMonDataOut, SP, SP, Total,			    Total, 			OID_STR,	1000, 1, Total)             \
		ISMMONOBJ(ismEndpointMonDataOut, SP, SP, Active, 			Active,			OID_STR,	1000, 1, Active)            \
		ISMMONOBJ(ismEndpointMonDataOut, SP, SP, Messages, 		    Messages,		OID_STR,	1000, 1, Messages)          \
		ISMMONOBJ(ismEndpointMonDataOut, SP, SP, Bytes, 			Bytes,			OID_STR,	1000, 1, Bytes)             \
		ISMMONOBJ(ismEndpointMonDataOut, SP, SP, LastErrorCode, 	LastErrorCode,	OID_STR,	1000, 1, LastErrorCode)     \
		ISMMONOBJ(ismEndpointMonDataOut, ISMTIME, ISMTIME, ConfigTime, 		ConfigTime,		OID_STR,	1000, 1, ConfigTime)        \
		ISMMONOBJ(ismEndpointMonDataOut, SP, SP, BadConnections,	BadConnections,	OID_STR,	1000, 1, BadConnections)	\
		{0,0,0,0,0,0,0}                                                                                                     \
};

#define DEFINE_CONNECTION_MONITORING_OBJ(mon_obj)                                                                                         \
const ism_mon_obj_def_t mon_obj [] = {                                                                                                    \
        ISMMONOBJ(ism_connect_mon_t,SP, SP, name,             Name,                   OID_STR,    1000, 1, connection name)               \
        ISMMONOBJ(ism_connect_mon_t,SP, SP, protocol,         Protocol,               OID_STR,    1000, 1, The name of the protocol)      \
        ISMMONOBJ(ism_connect_mon_t,SP, SP, client_addr,      ClientAddr,             OID_STR,    1000, 1, client_addr)                   \
        ISMMONOBJ(ism_connect_mon_t,SP, SP, userid,           UserId,                 OID_STR,    1000, 1, userid)                        \
        ISMMONOBJ(ism_connect_mon_t,SP, SP, listener,         Endpoint,               OID_STR,    1000, 1, listener)                      \
        ISMMONOBJ(ism_connect_mon_t,I, I, port,               Port,                   OID_STR,    1000, 1, port)                          \
        ISMMONOBJ(ism_connect_mon_t,I, I, index,              Index,                  OID_STR,    1000, 0, index)                         \
        ISMMONOBJ(ism_connect_mon_t,VP, VP, resv1,            resv1,                  OID_STR,    1000, 0, index)                         \
        ISMMONOBJ(ism_connect_mon_t,ISMTIME, ISMTIME, connect_time,       ConnectTime,            OID_STR,    1000, 1, connect_time)                  \
        ISMMONOBJ(ism_connect_mon_t,L, L, duration,           Duration,               OID_STR,    1000, 1, duration)                      \
        ISMMONOBJ(ism_connect_mon_t,L, L, read_bytes,         ReadBytes,              OID_STR,    1000, 1, read_bytes)                    \
        ISMMONOBJ(ism_connect_mon_t,L, L, read_msg,           ReadMsg,                OID_STR,    1000, 1, read_msg)                      \
        ISMMONOBJ(ism_connect_mon_t,L, L, write_bytes,        WriteBytes,             OID_STR,    1000, 1, write_bytes)                   \
        ISMMONOBJ(ism_connect_mon_t,L, L, write_msg,          WriteMsg,               OID_STR,    1000, 1, write_msg)                     \
        ISMMONOBJ(ism_connect_mon_t,I, I, sendQueueSize,      SendQueueSize,          OID_STR,    1000, 0, wsendQueueSize)                \
        ISMMONOBJ(ism_connect_mon_t,I, I, isSuspended,        IsSuspended,            OID_STR,    1000, 0, isSuspended)                   \
        {0,0,0,0,0,0,0}                                                                                                                   \
};

/* Subscription monitoring data - returned by Engine API in ismEngine_SubscriptionMonitor_t structure */
#define DEFINE_SUBSCRIPTION_MONITORING_OBJ(mon_obj)                                                                                                          \
ism_mon_obj_def_t mon_obj [] = {                                                                                                                             \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, SP,     SP,        subName,                  SubName,            OID_STR,    1000, 1, SubscriptionName)   \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, SKIPSP, SKIPSP,    configType,               ConfigType,         OID_STR,    1000, 1, ConfigType)         \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, SP,     SP,        topicString,              TopicString,        OID_STR,    1000, 1, TopicString)        \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, SP,     SP,        clientId,                 ClientID,           OID_STR,    1000, 1, ClientID)           \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, BOOL,   BOOL,      durable,                  IsDurable,          OID_STR,    1000, 1, IsDurable)          \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.BufferedMsgs,       BufferedMsgs,       OID_STR,    1000, 1, BufferedMsgs)       \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.BufferedMsgsHWM,    BufferedMsgsHWM,    OID_STR,    1000, 1, BufferedMsgsHWM)    \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, D,      PERCENT,   stats.BufferedPercent,    BufferedPercent,    OID_STR,    1000, 1, BufferedPercent)    \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.InflightEnqs,       InflightEnqs,       OID_STR,    1000, 0, InflightEnqueues)   \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.InflightDeqs,       InflightDeqs,       OID_STR,    1000, 0, InflightDequeues)   \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.EnqueueCount,       EnqueueCount,       OID_STR,    1000, 0, EnqueueCount)       \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.DequeueCount,       DequeueCount,       OID_STR,    1000, 0, DequeueCount)       \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.QAvoidCount,        QAvoidCount,        OID_STR,    1000, 0, QAvoidCount)        \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.MaxMessages,        MaxMessages,        OID_STR,    1000, 1, MaxMessages)        \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.ProducedMsgs,       PublishedMsgs,      OID_STR,    1000, 1, PublishedMsgs)      \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.RejectedMsgs,       RejectedMsgs,       OID_STR,    1000, 1, RejectedMsgs)       \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, D,      PERCENT,   stats.BufferedHWMPercent, BufferedHWMPercent, OID_STR,    1000, 1, BufferedHWMPercent) \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, BOOL,   BOOL,      shared,                   IsShared,           OID_STR,    1000, 1, IsShared)           \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, I,      I,         consumers,                Consumers,          OID_STR,    1000, 1, Consumers)          \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.DiscardedMsgs,      DiscardedMsgs,      OID_STR,    1000, 1, DiscardedMsgs)      \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, UL,     UL,        stats.ExpiredMsgs,        ExpiredMsgs,        OID_STR,    1000, 1, ExpiredMsgs)        \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, SP,     SP,        messagingPolicyName,      MessagingPolicy,    OID_STR,    1000, 1, MessagingPolicy)    \
        ISMMONOBJ(ismEngine_SubscriptionMonitor_t, RSETID, RSETID,    resourceSetId,            ResourceSetID,      OID_STR,    1000, 0, ResourceSetID)      \
        {0,0,0,0,0,0,0}                                                                                                                                      \
};

/* ResourceSet monitoring data - returned by Engine API in ismEngine_ResourceSetMonitor_t structure */
#define DEFINE_RESOURCESET_MONITORING_OBJ(mon_obj)                                                                                                                                                               \
ism_mon_obj_def_t mon_obj [] = {                                                                                                                                                                                 \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, RSETID,  RSETID,  resourceSetId,                                ResourceSetID,                       OID_STR,    1000, 1, ResourceSetID)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, ISMTIME, ISMTIME, resetTime,                                    ResetTime,                           OID_STR,    1000, 1, ResetTime)                           \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, ISMTIME, ISMTIME, reportTime,                                   ReportTime,                          OID_STR,    1000, 1, ReportTime)                          \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.TotalMemoryBytes,                       TotalMemoryBytes,                    OID_STR,    1000, 1, TotalMemoryBytes)                    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.Subscriptions,                          Subscriptions,                       OID_STR,    1000, 1, Subscriptions)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentNonSharedSubscriptions,       PersistentNonSharedSubscriptions,    OID_STR,    1000, 1, PersistentNonSharedSubscriptions)    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.NonpersistentNonSharedSubscriptions,    NonpersistentNonSharedSubscriptions, OID_STR,    1000, 1, NonpersistentNonSharedSubscriptions) \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentSharedSubscriptions,          PersistentSharedSubscriptions,       OID_STR,    1000, 1, PersistentSharedSubscriptions)       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.NonpersistentSharedSubscriptions,       NonpersistentSharedSubscriptions,    OID_STR,    1000, 1, NonpersistentSharedSubscriptions)    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.BufferedMsgs,                           BufferedMsgs,                        OID_STR,    1000, 1, BufferedMsgs)                        \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.DiscardedMsgsSinceRestart,              DiscardedMsgsSinceRestart,           OID_STR,    1000, 1, DiscardedMsgsSinceRestart)           \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.DiscardedMsgs,                          DiscardedMsgs,                       OID_STR,    1000, 1, DiscardedMsgs)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.RejectedMsgsSinceRestart,               RejectedMsgsSinceRestart,            OID_STR,    1000, 1, RejectedMsgsSinceRestart)            \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.RejectedMsgs,                           RejectedMsgs,                        OID_STR,    1000, 1, RejectedMsgs)                        \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.RetainedMsgs,                           RetainedMsgs,                        OID_STR,    1000, 1, RetainedMsgs)                        \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.WillMsgs,                               WillMsgs,                            OID_STR,    1000, 1, WillMsgs)                            \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.BufferedMsgBytes,                       BufferedMsgBytes,                    OID_STR,    1000, 1, BufferedMsgBytes)                    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentBufferedMsgBytes,             PersistentBufferedMsgBytes,          OID_STR,    1000, 1, PersistentBufferedMsgBytes)          \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.NonpersistentBufferedMsgBytes,          NonpersistentBufferedMsgBytes,       OID_STR,    1000, 1, NonpersistentBufferedMsgBytes)       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.RetainedMsgBytes,                       RetainedMsgBytes,                    OID_STR,    1000, 1, RetainedMsgBytes)                    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.WillMsgBytes,                           WillMsgBytes,                        OID_STR,    1000, 1, WillMsgBytes)                        \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentWillMsgBytes,                 PersistentWillMsgBytes,              OID_STR,    1000, 1, PersistentWillMsgBytes)              \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.NonpersistentWillMsgBytes,              NonpersistentWillMsgBytes,           OID_STR,    1000, 1, NonpersistentWillMsgBytes)           \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PublishedMsgsSinceRestart,              PublishedMsgsSinceRestart,           OID_STR,    1000, 1, PublishedMsgsSinceRestart)           \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PublishedMsgs,                          PublishedMsgs,                       OID_STR,    1000, 1, PublishedMsgs)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS0PublishedMsgs,                      QoS0PublishedMsgs,                   OID_STR,    1000, 1, QoS0PublishedMsgs)                   \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS1PublishedMsgs,                      QoS1PublishedMsgs,                   OID_STR,    1000, 1, QoS1PublishedMsgs)                   \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS2PublishedMsgs,                      QoS2PublishedMsgs,                   OID_STR,    1000, 1, QoS2PublishedMsgs)                   \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PublishedMsgBytesSinceRestart,          PublishedMsgBytesSinceRestart,       OID_STR,    1000, 1, PublishedMsgBytesSinceRestart)       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PublishedMsgBytes,                      PublishedMsgBytes,                   OID_STR,    1000, 1, PublishedMsgBytes)                   \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS0PublishedMsgBytes,                  QoS0PublishedMsgBytes,               OID_STR,    1000, 1, QoS0PublishedMsgBytes)               \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS1PublishedMsgBytes,                  QoS1PublishedMsgBytes,               OID_STR,    1000, 1, QoS1PublishedMsgBytes)               \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS2PublishedMsgBytes,                  QoS2PublishedMsgBytes,               OID_STR,    1000, 1, QoS2PublishedMsgBytes)               \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.MaxPublishRecipientsSinceRestart,       MaxPublishRecipientsSinceRestart,    OID_STR,    1000, 1, MaxPublishRecipientsSinceRestart)    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.MaxPublishRecipients,                   MaxPublishRecipients,                OID_STR,    1000, 1, MaxPublishRecipients)                \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.ConnectionsSinceRestart,                ConnectionsSinceRestart,             OID_STR,    1000, 1, ConnectionsSinceRestart)             \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.Connections,                            Connections,                         OID_STR,    1000, 1, Connections)                         \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.ActiveClients,                          ActiveClients,                       OID_STR,    1000, 1, ActiveClients)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.ActivePersistentClients,                ActivePersistentClients,             OID_STR,    1000, 1, ActivePersistentClients)             \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.ActiveNonpersistentClients,             ActiveNonpersistentClients,          OID_STR,    1000, 1, ActiveNonpersistentClients)          \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentClientStates,                 PersistentClientStates,              OID_STR,    1000, 1, PersistentClientStates)              \
        {0,0,0,0,0,0,0}                                                                                                                                                                                          \
};

/* ResultType=Standard subset of ResourceSet monitoring data */
#define DEFINE_RESOURCESET_MONITORING_OBJ_STANDARD_SUBSET(mon_obj)                                                                                                                                               \
ism_mon_obj_def_t mon_obj [] = {                                                                                                                                                                                 \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, RSETID,  RSETID,  resourceSetId,                                ResourceSetID,                       OID_STR,    1000, 1, ResourceSetID)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, ISMTIME, ISMTIME, resetTime,                                    ResetTime,                           OID_STR,    1000, 1, ResetTime)                           \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, ISMTIME, ISMTIME, reportTime,                                   ReportTime,                          OID_STR,    1000, 1, ReportTime)                          \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.TotalMemoryBytes,                       TotalMemoryBytes,                    OID_STR,    1000, 1, TotalMemoryBytes)                    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.Subscriptions,                          Subscriptions,                       OID_STR,    1000, 1, Subscriptions)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentNonSharedSubscriptions,       PersistentNonSharedSubscriptions,    OID_STR,    1000, 1, PersistentNonSharedSubscriptions)    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.NonpersistentNonSharedSubscriptions,    NonpersistentNonSharedSubscriptions, OID_STR,    1000, 1, NonpersistentNonSharedSubscriptions) \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentSharedSubscriptions,          PersistentSharedSubscriptions,       OID_STR,    1000, 1, PersistentSharedSubscriptions)       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.NonpersistentSharedSubscriptions,       NonpersistentSharedSubscriptions,    OID_STR,    1000, 1, NonpersistentSharedSubscriptions)    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.BufferedMsgs,                           BufferedMsgs,                        OID_STR,    1000, 1, BufferedMsgs)                        \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.DiscardedMsgsSinceRestart,              DiscardedMsgsSinceRestart,           OID_STR,    1000, 0, DiscardedMsgsSinceRestart)           \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.DiscardedMsgs,                          DiscardedMsgs,                       OID_STR,    1000, 1, DiscardedMsgs)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.RejectedMsgsSinceRestart,               RejectedMsgsSinceRestart,            OID_STR,    1000, 0, RejectedMsgsSinceRestart)            \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.RejectedMsgs,                           RejectedMsgs,                        OID_STR,    1000, 1, RejectedMsgs)                        \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.RetainedMsgs,                           RetainedMsgs,                        OID_STR,    1000, 1, RetainedMsgs)                        \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.WillMsgs,                               WillMsgs,                            OID_STR,    1000, 1, WillMsgs)                            \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.BufferedMsgBytes,                       BufferedMsgBytes,                    OID_STR,    1000, 1, BufferedMsgBytes)                    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentBufferedMsgBytes,             PersistentBufferedMsgBytes,          OID_STR,    1000, 1, PersistentBufferedMsgBytes)          \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.NonpersistentBufferedMsgBytes,          NonpersistentBufferedMsgBytes,       OID_STR,    1000, 1, NonpersistentBufferedMsgBytes)       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.RetainedMsgBytes,                       RetainedMsgBytes,                    OID_STR,    1000, 1, RetainedMsgBytes)                    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.WillMsgBytes,                           WillMsgBytes,                        OID_STR,    1000, 1, WillMsgBytes)                        \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentWillMsgBytes,                 PersistentWillMsgBytes,              OID_STR,    1000, 1, PersistentWillMsgBytes)              \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.NonpersistentWillMsgBytes,              NonpersistentWillMsgBytes,           OID_STR,    1000, 1, NonpersistentWillMsgBytes)           \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PublishedMsgsSinceRestart,              PublishedMsgsSinceRestart,           OID_STR,    1000, 0, PublishedMsgsSinceRestart)           \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PublishedMsgs,                          PublishedMsgs,                       OID_STR,    1000, 1, PublishedMsgs)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS0PublishedMsgs,                      QoS0PublishedMsgs,                   OID_STR,    1000, 1, QoS0PublishedMsgs)                   \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS1PublishedMsgs,                      QoS1PublishedMsgs,                   OID_STR,    1000, 1, QoS1PublishedMsgs)                   \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS2PublishedMsgs,                      QoS2PublishedMsgs,                   OID_STR,    1000, 1, QoS2PublishedMsgs)                   \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PublishedMsgBytesSinceRestart,          PublishedMsgBytesSinceRestart,       OID_STR,    1000, 0, PublishedMsgBytesSinceRestart)       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PublishedMsgBytes,                      PublishedMsgBytes,                   OID_STR,    1000, 1, PublishedMsgBytes)                   \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS0PublishedMsgBytes,                  QoS0PublishedMsgBytes,               OID_STR,    1000, 1, QoS0PublishedMsgBytes)               \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS1PublishedMsgBytes,                  QoS1PublishedMsgBytes,               OID_STR,    1000, 1, QoS1PublishedMsgBytes)               \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.QoS2PublishedMsgBytes,                  QoS2PublishedMsgBytes,               OID_STR,    1000, 1, QoS2PublishedMsgBytes)               \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.MaxPublishRecipientsSinceRestart,       MaxPublishRecipientsSinceRestart,    OID_STR,    1000, 0, MaxPublishRecipientsSinceRestart)    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.MaxPublishRecipients,                   MaxPublishRecipients,                OID_STR,    1000, 1, MaxPublishRecipients)                \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.ConnectionsSinceRestart,                ConnectionsSinceRestart,             OID_STR,    1000, 0, ConnectionsSinceRestart)             \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.Connections,                            Connections,                         OID_STR,    1000, 1, Connections)                         \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.ActiveClients,                          ActiveClients,                       OID_STR,    1000, 1, ActiveClients)                       \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.ActivePersistentClients,                ActivePersistentClients,             OID_STR,    1000, 1, ActivePersistentClients)             \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.ActiveNonpersistentClients,             ActiveNonpersistentClients,          OID_STR,    1000, 1, ActiveNonpersistentClients)          \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,      UL,      stats.PersistentClientStates,                 PersistentClientStates,              OID_STR,    1000, 1, PersistentClientStates)              \
        {0,0,0,0,0,0,0}                                                                                                                                                                                          \
};

/* ResultType=TotalMemoryBytes subset of ResourceSet monitoring data */
#define DEFINE_RESOURCESET_MONITORING_OBJ_TOTALMEMORYBYTES_SUBSET(mon_obj)                                                                                  \
ism_mon_obj_def_t mon_obj [] = {                                                                                                                      \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, RSETID, RSETID,   resourceSetId,            ResourceSetID,               OID_STR,    1000, 1, ResourceSetID)    \
        ISMMONOBJ(ismEngine_ResourceSetMonitor_t, UL,     UL,       stats.TotalMemoryBytes,   TotalMemoryBytes,            OID_STR,    1000, 1, TotalMemoryBytes) \
        {0,0,0,0,0,0,0}                                                                                                                                     \
};

/* Queue monitoring data - returned by Engine API in ismEngine_QueueMonitor_t structure */
#define DEFINE_QUEUE_MONITORING_OBJ(mon_obj)                                                                                                   \
ism_mon_obj_def_t mon_obj [] = {                                                                                                         \
        ISMMONOBJ(ismEngine_QueueMonitor_t, SP,  SP,  queueName,                 Name,                OID_STR,    1000, 1, QueueName)          \
        ISMMONOBJ(ismEngine_QueueMonitor_t, U,   U,   producers,                 Producers,           OID_STR,    1000, 1, Producers)          \
        ISMMONOBJ(ismEngine_QueueMonitor_t, U,   U,   consumers,                 Consumers,           OID_STR,    1000, 1, Consumers)          \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.BufferedMsgs,        BufferedMsgs,        OID_STR,    1000, 1, BufferedMsgs)       \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.BufferedMsgsHWM,     BufferedMsgsHWM,     OID_STR,    1000, 1, BufferedMsgsHWM)    \
        ISMMONOBJ(ismEngine_QueueMonitor_t, D,   D,   stats.BufferedPercent,     BufferedPercent,     OID_STR,    1000, 1, BufferedPercent)    \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.InflightEnqs,        InflightEnqs,        OID_STR,    1000, 0, InflightEnqueues)   \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.InflightDeqs,        InflightDeqs,        OID_STR,    1000, 0, InflightDequeues)   \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.EnqueueCount,        EnqueueCount,        OID_STR,    1000, 0, EnqueueCount)       \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.DequeueCount,        DequeueCount,        OID_STR,    1000, 0, DequeueCount)       \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.QAvoidCount,         QAvoidCount,         OID_STR,    1000, 0, QAvoidCount)        \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.MaxMessages,         MaxMessages,         OID_STR,    1000, 1, MaxMessages)        \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.ProducedMsgs,        ProducedMsgs,        OID_STR,    1000, 1, ProducedMsgs)       \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.ConsumedMsgs,        ConsumedMsgs,        OID_STR,    1000, 1, ConsumedMsgs)       \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.RejectedMsgs,        RejectedMsgs,        OID_STR,    1000, 1, RejectedMsgs)       \
        ISMMONOBJ(ismEngine_QueueMonitor_t, D,   D,   stats.BufferedHWMPercent,  BufferedHWMPercent,  OID_STR,    1000, 1, BufferedHWMPercent) \
        ISMMONOBJ(ismEngine_QueueMonitor_t, UL,  UL,  stats.ExpiredMsgs,         ExpiredMsgs,         OID_STR,    1000, 1, ExpiredMsgs)        \
        {0,0,0,0,0,0,0}                                                                                                                        \
};


/* Topic monitoring data - returned by Engine API in ismEngine_TopicMonitor_t structure */
#define DEFINE_TOPIC_MONITORING_OBJ(mon_obj)                                                                                                  \
ism_mon_obj_def_t mon_obj [] = {                                                                                                        \
        ISMMONOBJ(ismEngine_TopicMonitor_t, SP,      SP,      topicString,           TopicString,       OID_STR,    1000, 1, TopicString)     \
        ISMMONOBJ(ismEngine_TopicMonitor_t, UL,      UL,      subscriptions,         Subscriptions,     OID_STR,    1000, 1, Subscriptions)   \
        ISMMONOBJ(ismEngine_TopicMonitor_t, ISMTIME, ISMTIME, stats.ResetTime,       ResetTime,         OID_STR,    1000, 1, ResetTime)       \
        ISMMONOBJ(ismEngine_TopicMonitor_t, UL,      UL,      stats.PublishedMsgs,   PublishedMsgs,     OID_STR,    1000, 1, PublishedMsgs)   \
        ISMMONOBJ(ismEngine_TopicMonitor_t, UL,      UL,      stats.RejectedMsgs,    RejectedMsgs,      OID_STR,    1000, 1, RejectedMsgs)    \
        ISMMONOBJ(ismEngine_TopicMonitor_t, UL,      UL,      stats.FailedPublishes, FailedPublishes,   OID_STR,    1000, 1, FailedPublishes) \
        {0,0,0,0,0,0,0}                                                                                                                       \
};


/* MQTT Client monitoring data - returned by Engine API in ismEngine_ClientStateMonitor_t structure */
#define DEFINE_MQTTCLIENT_MONITORING_OBJ(mon_obj)                                                                                                     \
ism_mon_obj_def_t mon_obj [] = {                                                                                                                      \
        ISMMONOBJ(ismEngine_ClientStateMonitor_t, SP,      SP,       ClientId,             ClientID,          OID_STR,    1000, 1, ClientID)          \
        ISMMONOBJ(ismEngine_ClientStateMonitor_t, BOOL,    BOOL,     fIsConnected,         IsConnected,       OID_STR,    1000, 1, IsConnected)       \
        ISMMONOBJ(ismEngine_ClientStateMonitor_t, ISMTIME, ISMTIME,  LastConnectedTime,    LastConnectedTime, OID_STR,    1000, 1, LastConnectedTime) \
        ISMMONOBJ(ismEngine_ClientStateMonitor_t, ISMTIME, ISMTIME,  ExpiryTime,           ExpiryTime,        OID_STR,    1000, 1, ExpiryTime)        \
        ISMMONOBJ(ismEngine_ClientStateMonitor_t, ISMTIME, ISMTIME,  WillDelayExpiryTime,  WillMsgDelayTime,  OID_STR,    1000, 1, WillMsgDelayTime)  \
        ISMMONOBJ(ismEngine_ClientStateMonitor_t, RSETID,  RSETID,   ResourceSetId,        ResourceSetID,     OID_STR,    1000, 0, ResourceSetID)     \
        {0,0,0,0,0,0,0}                                                                                                                               \
};

/* Transaction monitoring data - returned by Engine API in ismEngine_TransactionMonitor_t structure */
#define DEFINE_TRANSACTION_MONITORING_OBJ(mon_obj)                                                                                                \
ism_mon_obj_def_t mon_obj [] = {                                                                                                                  \
        ISMMONOBJ(ismEngine_TransactionMonitor_t, XID,     XID,      XID,               XID,               OID_STR,    1000, 1, XID)              \
        ISMMONOBJ(ismEngine_TransactionMonitor_t, I,       I,        state,             state,             OID_STR,    1000, 1, state)            \
        ISMMONOBJ(ismEngine_TransactionMonitor_t, ISMTIME, ISMTIME,  stateChangedTime,  stateChangedTime,  OID_STR,    1000, 1, stateChangedTime) \
        {0,0,0,0,0,0,0}                                                                                                                           \
};

/* DestinationMappingRule monitoring data. Returned by MQ Connectivity in mqcStatistics_t */

#define DEFINE_DESTINATIONMAPPINGRULE_MONITORING_OBJ(mon_obj)                                                                        \
const ism_mon_obj_def_t mon_obj [] = {                                                                                               \
        ISMMONOBJ(mqcStatistics_t, SP, SP, RuleName,               RuleName,               OID_STR, 1000, 1, RuleName)               \
        ISMMONOBJ(mqcStatistics_t, SP, SP, QueueManagerConnection, QueueManagerConnection, OID_STR, 1000, 1, QueueManagerConnection) \
        ISMMONOBJ(mqcStatistics_t, UL, UL, commitCount,            CommitCount,            OID_STR, 1000, 1, CommitCount)            \
        ISMMONOBJ(mqcStatistics_t, UL, UL, rollbackCount,          RollbackCount,          OID_STR, 1000, 1, RollbackCount)          \
        ISMMONOBJ(mqcStatistics_t, UL, UL, committedMessageCount,  CommittedMessageCount,  OID_STR, 1000, 1, CommittedMessageCount)  \
        ISMMONOBJ(mqcStatistics_t, UL, UL, persistentCount,        PersistentCount,        OID_STR, 1000, 1, PersistentCount)        \
        ISMMONOBJ(mqcStatistics_t, UL, UL, nonpersistentCount,     NonpersistentCount,     OID_STR, 1000, 1, NonpersistentCount)     \
        ISMMONOBJ(mqcStatistics_t, U,  U,  status,                 Status,                 OID_STR, 1000, 1, Status)                 \
        ISMMONOBJ(mqcStatistics_t, SP, SP, pExpandedMessage,       ExpandedMessage,        OID_STR, 1000, 1, ExpandedMessage)        \
        {0,0,0,0,0,0,0}                                                                                                              \
};

#ifdef __cplusplus
}
#endif

#endif
