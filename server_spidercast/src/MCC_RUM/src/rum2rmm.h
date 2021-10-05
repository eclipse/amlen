/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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

#ifndef  H_rum2rmm_H
#define  H_rum2rmm_H

#ifndef RMMCAPI_VERSION
#define RMMCAPI_VERSION RUMCAPI_VERSION
#endif

#ifndef rmm_uint64
#define rmm_uint64 rum_uint64
#endif

#ifndef rmm_int64
#define rmm_int64 rum_int64
#endif

#ifndef rmmStreamID_t
#define rmmStreamID_t rumStreamID_t
#endif

#ifndef rmmMessageID_t
#define rmmMessageID_t rumMessageID_t
#endif

#ifndef rmmMessage
#define rmmMessage rumMessage
#endif

#ifndef rmmEvent
#define rmmEvent   rumEvent
#endif

#ifndef rmmApplicationMsgFilter
#define rmmApplicationMsgFilter rumApplicationMsgFilter
#endif

#ifndef rmm_all_props_select_t
#define rmm_all_props_select_t rum_all_props_select_t
#endif

#ifndef rmmMsgProperty
#define rmmMsgProperty rumMsgProperty
#endif

#ifndef rmmMsgSelector
#define rmmMsgSelector rumMsgSelector
#endif

#ifndef  rmmRxMessage 
#define  rmmRxMessage rumRxMessage
#endif

#ifndef  rmmStreamInfo
#define  rmmStreamInfo rumStreamInfo
#endif

#ifndef  rmmPacket
#define  rmmPacket rumPacket
#endif

#ifndef  rmmLogEvent
#define  rmmLogEvent rumLogEvent
#endif

#ifndef  rmmLatencyMonParams
#define  rmmLatencyMonParams rumLatencyMonParams
#endif

#define topic_name          queue_name

#define rmm_length          rum_length
#define rmm_reserved        rum_reserved
#define rmm_pst_reserved    rum_pst_reserved
#define rmm_buff_reserved   rum_buff_reserved
#define rmm_flags_reserved  rum_flags_reserved
#define rmm_instance        rum_instance
#define rmm_expiration      rum_expiration

#ifndef  rmmTxMessage 
#define  rmmTxMessage rumTxMessage
#endif

#define rmm_get_external_time_t   rum_get_external_time_t

#define RMM_TF_MODE_DISABLED RUM_TF_MODE_DISABLED
#define RMM_TF_MODE_LABEL    RUM_TF_MODE_LABEL   
#define RMM_TF_MODE_BITMAP   RUM_TF_MODE_BITMAP  

#define RMM_MAX_BITMAP_LENGTH RUM_MAX_BITMAP_LENGTH

#define RMM_MAX_MSGS_IN_PACKET RUM_MAX_MSGS_IN_PACKET

#define RMM_MSG_PROP_INT32  RUM_MSG_PROP_INT32
#define RMM_MSG_PROP_INT64  RUM_MSG_PROP_INT64
#define RMM_MSG_PROP_DOUBLE RUM_MSG_PROP_DOUBLE
#define RMM_MSG_PROP_BYTES  RUM_MSG_PROP_BYTES

#ifndef RUM_MAX_DEBUG
#define RUM_MAX_DEBUG RUM_LOGLEV_XTRACE
#endif

#define RUM_TRUE  RMM_TRUE
#define RUM_FALSE RMM_FALSE

/* all API functions return RUM_SUCCESS on success or RUM_FAILUR on failure */
#define  RMM_SUCCESS   RUM_SUCCESS
#define  RMM_FAILURE   RUM_FAILURE
#define  RMM_BUSY      RUM_BUSY


/* --- constants used in configuration structures --- */
            
#ifndef RMM_MAX_INSTANCES
#define RMM_MAX_INSTANCES   RUM_MAX_INSTANCES
#endif
            
#ifndef RMM_MAX_TX_TOPICS
#define RMM_MAX_TX_TOPICS   RUM_MAX_TX_QUEUES
#endif
            
#ifndef RMM_MAX_RX_TOPICS
#define RMM_MAX_RX_TOPICS   RUM_MAX_RX_QUEUES
#endif
            
#ifndef RMM_MAX_TOPIC_NAME
#define RMM_MAX_TOPIC_NAME  RUM_MAX_QUEUE_NAME
#endif
            
#ifndef RMM_TOPIC_NAME_LEN
#define RMM_TOPIC_NAME_LEN  RUM_MAX_QUEUE_NAME
#endif

#ifndef RMM_API_VERSION_LEN
#define RMM_API_VERSION_LEN RUM_API_VERSION_LEN
#endif

#ifndef RMM_ADDR_STR_LEN
#define RMM_ADDR_STR_LEN  RUM_ADDR_STR_LEN
#endif

#ifndef RMM_HOSTNAME_STR_LEN
#define RMM_HOSTNAME_STR_LEN RUM_HOSTNAME_STR_LEN
#endif

#ifndef RMM_FILE_NAME_LEN
#define RMM_FILE_NAME_LEN  RUM_FILE_NAME_LEN
#endif

 /* IPVersion */
#define  RMM_IPver_IPv4   RUM_IPver_IPv4
#define  RMM_IPver_IPv6   RUM_IPver_IPv6
#define  RMM_IPver_ANY    RUM_IPver_ANY
#define  RMM_IPver_BOTH   RUM_IPver_BOTH

 /* LogLevel */
#define  RMM_NO_LOG      RUM_NO_LOG
#define  RMM_BASIC_LOG   RUM_BASIC_LOG
#define  RMM_MAX_LOG     RUM_MAX_LOG

 /* LimitTransRate */
#define  RMM_DISABLED_RATE_LIMIT  RUM_DISABLED_RATE_LIMIT
#define  RMM_STATIC_RATE_LIMIT    RUM_STATIC_RATE_LIMIT
#define  RMM_DYNAMIC_RATE_LIMIT   RUM_DYNAMIC_RATE_LIMIT

/* --- constants used in API functions --- */

  /* constants used to define Queue reliability */
#define  RMM_UNRELIABLE                RUM_UNRELIABLE
#define  RMM_RELIABLE                  RUM_RELIABLE
#define  RMM_RELIABLE_NO_HISTORY       RUM_RELIABLE_NO_HISTORY
#define  RMM_RELIABLE_FAILOVER         RUM_RELIABLE_FAILOVER
#define  RMM_RELIABLE_FAILOVER_PRIM    RUM_RELIABLE_FAILOVER_PRIM
#define  RMM_RELIABLE_FAILOVER_BU      RUM_RELIABLE_FAILOVER_BU
#define  RMM_RELIABLE_FAILOVER_BU_RPLY RUM_RELIABLE_FAILOVER_BU_RPLY


/*----    RUM Events    ----*/

/*----    Event types   ----*/
#define RMM_PACKET_LOSS         RUM_PACKET_LOSS
#define RMM_HEARTBEAT_TIMEOUT   RUM_HEARTBEAT_TIMEOUT
#define RMM_VERSION_CONFLICT    RUM_VERSION_CONFLICT
#define RMM_RELIABILITY         RUM_RELIABILITY
#define RMM_CLOSED_TRANS        RUM_CLOSED_TRANS
#define RMM_CLOSED_REC          RUM_CLOSED_REC
#define RMM_ADDRESS_CHANGED     RUM_ADDRESS_CHANGED
#define RMM_DATA_PORT_CHANGED   RUM_DATA_PORT_CHANGED
#define RMM_NEW_SOURCE          RUM_NEW_SOURCE
#define RMM_SUSPEND_REC         RUM_SUSPEND_REC
#define RMM_SUSPEND_NACK        RUM_SUSPEND_NACK
#define RMM_RESUME_REC          RUM_RESUME_REC
#define RMM_RESUME_DATA         RUM_RESUME_DATA
#define RMM_CONTROL_OPTION      RUM_CONTROL_OPTION
#define RMM_REPORT_RECEIVED     RUM_REPORT_RECEIVED
#define RMM_RECEIVE_QUEUE_TRIMMED RUM_RECEIVE_QUEUE_TRIMMED

/*--- HA Events   ----*/
#define RMM_FIRST_MESSAGE       RUM_FIRST_MESSAGE
#define RMM_LATE_JOIN_FAILURE   RUM_LATE_JOIN_FAILURE
#define RMM_MESSAGE_LOSS        RUM_MESSAGE_LOSS
#define RMM_REPAIR_DELAY        RUM_REPAIR_DELAY
#define RMM_MEMORY_ALERT_ON     RUM_MEMORY_ALERT_ON
#define RMM_MEMORY_ALERT_OFF    RUM_MEMORY_ALERT_OFF
#define RMM_RELIABILITY_CHANGED RUM_RELIABILITY_CHANGED



/*----    msg_key of LOG Events   ----*/
#define RMM_MAX_MSG_KEY       RUM_MAX_MSG_KEY
 /* debug */
#define RMM_D_GENERAL         RUM_D_GENERAL
#define RMM_D_METHOD_ENTRY    RUM_D_METHOD_ENTRY
#define RMM_D_METHOD_EXIT     RUM_D_METHOD_EXIT

/* info */
#define RMM_L_I_STATUS_INFO                RUM_L_I_STATUS_INFO
#define RMM_L_I_EVENT_INFO                 RUM_L_I_EVENT_INFO
#define RMM_L_I_TRACE_INFO                 RUM_L_I_TRACE_INFO
#define RMM_L_I_XTRACE_INFO                RUM_L_I_XTRACE_INFO
#define RMM_L_I_GENERAL_INFO               RUM_L_I_GENERAL_INFO

#define RMM_L_I_START_SERVICE              RUM_L_I_START_SERVICE
#define RMM_L_I_STOP_SERVICE               RUM_L_I_STOP_SERVICE
#define RMM_L_I_NACK                       RUM_L_I_NACK
#define RMM_L_I_BUFFER_CLEAN               RUM_L_I_BUFFER_CLEAN
#define RMM_L_I_CONTROL_DATA_RECEIVED      RUM_L_I_CONTROL_DATA_RECEIVED

/* warning */
#define RMM_L_W_EVENT_WARNING              RUM_L_W_EVENT_WARNING
#define RMM_L_W_TRACE_WARNING              RUM_L_W_TRACE_WARNING
#define RMM_L_W_GENERAL_WARNING            RUM_L_W_GENERAL_WARNING

#define RMM_L_W_SOCKET_WARNING             RUM_L_W_SOCKET_WARNING
#define RMM_L_W_QUEUE_WARNING              RUM_L_W_QUEUE_WARNING
#define RMM_L_W_NACK_WARNING               RUM_L_W_NACK_WARNING

/* error */
#define RMM_L_E_FATAL_ERROR                RUM_L_E_FATAL_ERROR
#define RMM_L_E_GENERAL_ERROR              RUM_L_E_GENERAL_ERROR
#define RMM_L_E_INTERNAL_SOFTWARE_ERROR    RUM_L_E_INTERNAL_SOFTWARE_ERROR

#define RMM_L_E_MEMORY_ALLOCATION_ERROR    RUM_L_E_MEMORY_ALLOCATION_ERROR
#define RMM_L_E_SOCKET_ERROR               RUM_L_E_SOCKET_ERROR
#define RMM_L_E_SOCKET_CREATE              RUM_L_E_SOCKET_CREATE
#define RMM_L_E_QUEUE_ERROR                RUM_L_E_QUEUE_ERROR
#define RMM_L_E_SEND_FAILURE               RUM_L_E_SEND_FAILURE
#define RMM_L_E_FILE_NOT_FOUND             RUM_L_E_FILE_NOT_FOUND
#define RMM_L_E_CONFIG_ENTRY               RUM_L_E_CONFIG_ENTRY
#define RMM_L_E_BAD_PARAMETER              RUM_L_E_BAD_PARAMETER
#define RMM_L_E_MCAST_INTERF               RUM_L_E_MCAST_INTERF
#define RMM_L_E_TTL                        RUM_L_E_TTL
#define RMM_L_E_LOCAL_INTERFACE            RUM_L_E_LOCAL_INTERFACE
#define RMM_L_E_INTERRUPT                  RUM_L_E_INTERRUPT
#define RMM_L_E_BAD_CONTROL_DATA           RUM_L_E_BAD_CONTROL_DATA
#define RMM_L_E_BAD_CONTROL_DATA_CREATION  RUM_L_E_BAD_CONTROL_DATA_CREATION
#define RMM_L_E_INTERNAL_LIMIT             RUM_L_E_INTERNAL_LIMIT
#define RMM_L_E_THREAD_ERROR               RUM_L_E_THREAD_ERROR   
#define RMM_L_E_STRUCT_INIT                RUM_L_E_STRUCT_INIT    
#define RMM_L_E_BAD_ADDRESS                RUM_L_E_BAD_ADDRESS    
#define RMM_L_E_LOAD_LIBRARY               RUM_L_E_LOAD_LIBRARY

#define RMM_L_E_INSTANCE_INVALID           RUM_L_E_INSTANCE_INVALID
#define RMM_L_E_INSTANCE_CLOSED            RUM_L_E_INSTANCE_CLOSED 
#define RMM_L_E_TOPIC_INVALID              RUM_L_E_QUEUE_INVALID   
#define RMM_L_E_TOPIC_CLOSED               RUM_L_E_QUEUE_CLOSED    
#define RMM_L_E_TOO_MANY_INSTANCES         RUM_L_E_TOO_MANY_INSTANCES
#define RMM_L_E_TOO_MANY_STREAMS           RUM_L_E_TOO_MANY_STREAMS   
#define RMM_L_E_BAD_MSG_PROP               RUM_L_E_BAD_MSG_PROP
#define RMM_L_E_BAD_API_VERSION            RUM_L_E_BAD_API_VERSION

#define RMM_LOGLEV_t       RUM_LOGLEV_t
#define RMM_LOGLEV_NONE    RUM_LOGLEV_NONE
#define RMM_LOGLEV_STATUS  RUM_LOGLEV_STATUS
#define RMM_LOGLEV_FATAL   RUM_LOGLEV_FATAL
#define RMM_LOGLEV_ERROR   RUM_LOGLEV_ERROR
#define RMM_LOGLEV_WARN    RUM_LOGLEV_WARN
#define RMM_LOGLEV_INFO    RUM_LOGLEV_INFO
#define RMM_LOGLEV_EVENT   RUM_LOGLEV_EVENT
#define RMM_LOGLEV_DEBUG   RUM_LOGLEV_DEBUG
#define RMM_LOGLEV_TRACE   RUM_LOGLEV_TRACE
#define RMM_LOGLEV_XTRACE  RUM_LOGLEV_XTRACE

typedef enum
{
   RMM_SHM_FAULT_DETECTION_DISABLED     = 0,                         /**< Shared Memory fault detection is disabled                   */
   RMM_SHM_FAULT_DETECTION_NO_REPAIR    = 1,                         /**< Shared Memory fault detection is enabled without repair.
                                                                      *   If a fault is detected LLM will mark the shared memory 
                                                                      *   region as unusable and will stop delivering data on it. 
                                                                      *   The application will be informed that the shared memory 
                                                                      *   object is unusable via an event of RMM_SHM_DEST_UNUSABLE or
                                                                      *   RMM_SHM_PORT_UNUSABLE                                       */
   RMM_SHM_FAULT_DETECTION_REPAIR       = 2                          /**< Shared Memory fault detection is enabled with repair.
                                                                      *   If a fault is detected RMM will automatically attempt to
                                                                      *   repair the problem and continue. Note that LLM can not 
                                                                      *   guarantee that the shared memory region will always be 
                                                                      *   properly repaired                                           */
}RMM_SHM_FAULT_DETECTION_POLICY_t ;

#endif
