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

/** \file rumCapi.h
 *  \brief Application Interface for RUM messaging layer version 2.6.0
 * The interface below defines the externalized functions that a component
 * can call to initialize RUM, create, destroy and manipulate instances and
 * send and receive messages. It contains the callbacks that the component
 * must implement to receive messages and events asynchronously and the
 * structures used for configuration and normal operations.
 * This file must be included with any application that uses the RUM API.
 *
 * The prefix of all RUM function names is 'rum'. Functions associated with
 * the RUM transmitter start with 'rumT' while receiver functions start with 'rumR'.
 *
 * Conventions:
 * All functions return an integer which is either RUM_SUCCESS
 * if the function completed successfully, RUM_FAILURE if the
 * function failed, or RUM_BUSY if the function could not be completed.
 * All Functions include an output parameter int *rc where return/reason code is set
 * in case RUM_FAILURE is returned. The keys used for return codes are the log events
 * error codes (prefix RUM_L_E_).
 *
 */


#ifndef  H_rumCapi_H
#define  H_rumCapi_H

#include "llmCommonApi.h"

#   ifdef __cplusplus
    extern "C" {
#   endif


#ifndef RUMAPI_DLL

#   ifndef RUM_UNIX
#       ifdef RUM_WINDOWS
#       define RUMAPI_DLL __declspec( dllexport )
#       else
#       define RUMAPI_DLL __declspec( dllimport )
      /**< */
#       endif
# else
#       define RUMAPI_DLL
#endif
#endif

/** \brief RUM API Version
 *  This version is used to identify the version of the LLM API that is used by the application.
 *  An application should use this api_version when initializing structures using rumInitStructureParameters().
 *  Using the right api_version will ensure binary compatibility when upgrading to a new version of LLM
 *  without recompiling the application with the new LLM API
*/
#define RUMCAPI_VERSION   2200

 /** \brief define a 64 bit integer types */
#ifdef RUM_UNIX
#   ifndef   rum_uint64
#   define   rum_uint64 unsigned long long
#   define   rum_int64  long long
#   endif
#else
#   ifndef   rum_uint64
#   define   rum_uint64 unsigned __int64
#   define   rum_int64  __int64
      /**< rum 64bit integer type*/
#   endif
#endif


#ifndef   rumStreamID_t
 /** \brief define a type for stream id               */
#define   rumStreamID_t      rum_uint64
#endif
#ifndef   rumMessageID_t
 /** \brief define a type for message id              */
#define   rumMessageID_t     rum_uint64
#endif
#ifndef   rumConnectionID_t
 /** \brief define a type for connection id           */
#define   rumConnectionID_t  rum_uint64
#endif

/** \brief define a type for TurboFlow bit maps       */
#define   rumtfBitMap_t       unsigned char *

/*----    RUM C API Constants    ----*/

/**
 * All API functions return RUM_SUCCESS on success or RUM_FAILURE on failure.
 * APIs that may not complete because the application is using certain resource may return RUM_BUSY.
 */
/** \brief Indicates a successful operation           */
#define  RUM_SUCCESS                  0
/** \brief Indicates a failed operation               */
#define  RUM_FAILURE                 -1
/** \brief Indicates an operation that currently cannot be completed, the caller should try again */
#define  RUM_BUSY                    -11

/** \brief return value of the on_connection_event callback */
#define  RUM_ON_CONNECTION_REJECT    -1                  /**< indicates the connection should be rejected        */
#define  RUM_ON_CONNECTION_ACCEPT     1                  /**< indicates the connection should be accepted        */
#define  RUM_ON_CONNECTION_NULL       0                  /**< indicates no action should be taken                */


/** \brief return value of the accept_stream callback */
#define  RUM_STREAM_REJECT            0                  /**< indicates a stream should be rejected              */
#define  RUM_STREAM_ACCEPT            1                  /**< indicates a stream should be accepted              */


/* --- constants used in configuration structures --- */

/** \brief Protocol */
typedef enum
{
   RUM_TCP                   = 0,      /**< TCP based protocol   */
   RUM_IB                    = 1,      /**< In this mode data is sent and received over InfiniBand(TM) using
                                        *   native InfiniBand (IB) services.
                                        *   Important: to use this transport, IP over IB type interface must be
                                        *   defined on the IB Host Channel Adapter.                              */
   RUM_SSL                   = 2       /**< Secured TCP protocol   */
}RUM_PROTOCOL_t ;

typedef LLM_IPVERSION_t RUM_IPVERSION_t ;
#define RUM_IPver_IPv4 LLM_IPver_IPv4
#define RUM_IPver_IPv6 LLM_IPver_IPv6
#define RUM_IPver_ANY  LLM_IPver_ANY
#define RUM_IPver_BOTH LLM_IPver_BOTH

 /** \brief Log Levels */
typedef enum
{
   RUM_LOGLEV_GLOBAL         = -1,     /**<  G = Global          */
   RUM_LOGLEV_NONE           =  LLM_LOGLEV_NONE,       /**<  N = none            */
   RUM_LOGLEV_STATUS         =  LLM_LOGLEV_STATUS,     /**<  S = status          */
   RUM_LOGLEV_FATAL          =  LLM_LOGLEV_FATAL,      /**<  F = fatal           */
   RUM_LOGLEV_ERROR          =  LLM_LOGLEV_ERROR,      /**<  E = error           */
   RUM_LOGLEV_WARN           =  LLM_LOGLEV_WARN,       /**<  W = warning         */
   RUM_LOGLEV_INFO           =  LLM_LOGLEV_INFO,       /**<  I = informational   */
   RUM_LOGLEV_EVENT          =  LLM_LOGLEV_EVENT,      /**<  V = eVent           */
   RUM_LOGLEV_DEBUG          =  LLM_LOGLEV_DEBUG,      /**<  D = debug           */
   RUM_LOGLEV_TRACE          =  LLM_LOGLEV_TRACE,      /**<  T = trace           */
   RUM_LOGLEV_XTRACE         =  LLM_LOGLEV_XTRACE      /**<  X = eXtended trace  */
}RUM_LOGLEV_t;

 /** \brief Defines transmission rate control mechanism */
typedef enum
{
   RUM_DISABLED_RATE_LIMIT   = 0,      /**< No transmission rate control.                                        */
   RUM_STATIC_RATE_LIMIT     = 1,      /**< Implements a static transmission rate control.
                                        *   The transmitter would not transmit at a rate exceeding the
                                        *   rate specified by TransRateLimitKbps.                                */
   RUM_DYNAMIC_RATE_LIMIT    = 2       /**< The transmitter will adapt its transmission rate according
                                        *   to the feedback it gets from the receivers. In this case the
                                        *   transmission rate limit would not be more than the value specified
                                        *   by TransRateLimitKbps. The transmitter will try to reach an optimal
                                        *   transmission rate.                                                   */
}RUM_RATE_LIMIT_t ;

 /** \brief TransportDirection */
typedef enum
{
   RUM_Tx_Rx                 = 0,      /**< Act as both transmitter and receiver                                  */
   RUM_Tx_ONLY               = 1,      /**< Act as transmitter only                                               */
   RUM_Rx_ONLY               = 2       /**< Act as receiver only                                                  */
}RUM_TRANS_DIRECTION_t ;

 /** \brief Connection State */
typedef enum
{
   RUM_CONNECTION_STATE_ESTABLISHED    =  0,       /**< successfully established connection               */
   RUM_CONNECTION_STATE_IN_PROGRESS    =  1,       /**< connection in establishment process               */
   RUM_CONNECTION_STATE_CLOSED         = -1        /**< connection is closed                              */
}RUM_CONN_STATE_t ;


/* --- constants used in API functions --- */

/**
 *  \brief Constants used to define Transmitter Queue reliability */
typedef enum
{
   RUM_UNRELIABLE                 = 0,             /**< unreliable (send once, retransmission disabled)   */
   RUM_RELIABLE                   = 1,             /**< reliable (retransmission enabled)                 */
   RUM_RELIABLE_NO_HISTORY        = 2,             /**< reliable but no history buffers                   */
}RUM_Tx_RELIABILITY_t ;

/** \brief Constants used to define Receiver Queue reliability */
typedef enum
{
   RUM_UNRELIABLE_RCV             = RUM_UNRELIABLE,/**< unreliable reception                              */
   RUM_RELIABLE_RCV               = RUM_RELIABLE,  /**< reliable ordered reception                        */
}RUM_Rx_RELIABILITY_t ;


/** \brief connection establishment method */
typedef enum
{
   RUM_USE_EXISTING               = 1,             /**< If there is already an existing connection to the
                                                    *   same destination, use it; otherwise create a new
                                                    *   connection. Currently this mode is not supported  */
   RUM_CREATE_NEW                 = 2              /**< Create a new connection (even if one exists)      */
}RUM_ESTABLISH_METHOD_t ;

/** \brief Message property types */
typedef enum
{
   RUM_MSG_PROP_INT32         = 1,    /**< Message property of type signed 32 bit integer                 */
   RUM_MSG_PROP_INT64         = 2,    /**< Message property of type signed 64 bit integer                 */
   RUM_MSG_PROP_DOUBLE        = 3,    /**< Message property of type double                                */
   RUM_MSG_PROP_BYTES         = 4     /**< Message property of type bytes (array of bytes)                */
}RUM_MSG_PROPERTY_TYPE_t ;

 /** \brief Structure identifiers */
typedef enum
{
   RUM_SID_CONFIG                 = 1,             /**< struct rumConfig                                  */
   RUM_SID_TxQUEUEPARAMETERS      = 2,             /**< struct rumTxQueueParameters                       */
   RUM_SID_RxQUEUEPARAMETERS      = 3,             /**< struct rumRxQueueParameters                       */
   RUM_SID_ESTABLISHCONNPARAMS    = 4,             /**< struct rumEstablishConnectionParams               */
   RUM_SID_TxMESSAGE              = 5,             /**< struct rumTxMessage                               */
   RUM_SID_MSGPROPERTY            = 6,             /**< struct rumMsgProperty                             */
   RUM_SID_MSGSELECTOR            = 7,             /**< struct rumMsgSelector                             */
   RUM_SID_LATENCYMONPARAMS       = 8              /**< struct rumLatencyMonParams                        */
}RUM_STRUCTURE_ID_t ;


/*----    RUM Events    ----*/

/** \brief Event types
  * A rumEvent includes a list of parameters in the event_params field; the nparams field indicates the number of parameters
  * that are included with the event (the number varies with different event types).
  * In all events the first parameter (event_params[0]) is of type char * and contains a short description of the event (as a NULL terminated string).
  * If an event includes additional parameters they are indicated in the event description.
  * Most RUM events are delivered on the on_event callback of a queue receiver (rumQueueR). The exceptions are RUM_MEMORY_ALERT_ON/OFF
  * events which are delivered on the RUM instance (rumInstance).
  */
typedef enum
{
   RUM_PACKET_LOSS                        = 1,    /**< Unrecoverable packet loss.
                                                   *    event_params[1] holds the number of lost packets (type int).
                                                   *    Delivered on rumQueueR                                                */
   RUM_HEARTBEAT_TIMEOUT                  = 2,    /**< Long absence of heartbeat control packet.
                                                   *    Delivered on rumQueueR                                                */
   RUM_VERSION_CONFLICT                   = 3,    /**< Reception of newer protocol version packets.
                                                   *    Delivered on rumQueueR                                                */
   RUM_RELIABILITY                        = 4,    /**< Different reliability modes of the transmitter and the receiver.
                                                   *    Delivered on rumQueueR                                                */
   RUM_CLOSED_TRANS                       = 5,    /**< Queue transmission is closed (by one source).
                                                   *    Delivered on rumQueueR                                                */
   RUM_STREAM_ERROR                       = 6,    /**< Error detected on stream
                                                   *    Delivered on rumQueueR                                                */
   RUM_STREAM_BROKE                       = 7,    /**< Stream is invalid since the connection it was using broke
                                                   *    Delivered on rumQueueR                                                */
   RUM_STREAM_NOT_PRESENT                 = 8,    /**< Stream used by the sender is not accepted by the receiver. In this
                                                   *   case messages sent on the stream are discarded at the receiver so the
                                                   *   application may want to close the stream (QueueT)
                                                   *    Delivered on rumQueueR and rumQueueT                                  */
   RUM_NEW_SOURCE_FAILED                  = 9,    /**< Could not successfully create a stream which the application earlier
                                                   *   indicated it wants to accept in the accept_stream callback. Used to
                                                   *   inform the application it should not expect RUM_NEW_SOURCE event on a
                                                   *   stream it accepted in the accept_stream callback
                                                   *    Delivered on rumQueueR                                                */
   RUM_NEW_SOURCE                         = 10,   /**< A new source starts to transmit on the queue.
                                                   *    event_params[1] holds an rumStreamInfo structure (type rumStreamInfo).
                                                   *    Delivered on rumQueueR                                                */
   RUM_RECEIVE_QUEUE_TRIMMED              = 11,   /**< Packets removed from PacketQ because of time or space expiration
                                                   *    event_params[1] holds the number of trimmed packets (type int).
                                                   *    Delivered on rumQueueR                                                */
   /*--- HA Events   ----*/
   RUM_FIRST_MESSAGE                      = 20,   /**< First message from a source. First message number.
                                                   *    event_params[1] holds the message number (type RUMMessageID_t).
                                                   *    Delivered on rumQueueR                                                */
   RUM_LATE_JOIN_FAILURE                  = 21,   /**< Failed to start late join session. A late join session with a source
                                                   *   cannot be started properly if the trail of the source is higher than
                                                   *   its late join mark.
                                                   *    Delivered on rumQueueR                                                */
   RUM_MESSAGE_LOSS                       = 22,   /**< Unrecoverable message loss.
                                                   *    event_params[1] holds the number of lost messages (type int).
                                                   *    Delivered on rumQueueR                                                */
   RUM_REPAIR_DELAY                       = 23,   /**< RUM receiver did not receive a repair packets for an outstanding NAK
                                                   *   within the configured timeout
                                                   *    Delivered on rumQueueR                                                */
   RUM_MEMORY_ALERT_ON                    = 24,   /**< Receiver reception buffers are filling up and have reached a high
                                                   *   water mark.
                                                   *    event_params[1] holds the buffer pool utilization (percent) (type int)
                                                   *    Delivered on rumInstance                                             */
   RUM_MEMORY_ALERT_OFF                   = 25,   /**< Receiver reception buffers are down to normal.
                                                   *    event_params[1] holds the buffer pool utilization (percent) (type int)
                                                   *    Delivered on rumInstance                                             */
   RUM_RELIABILITY_CHANGED                = 26,   /**< Receiver detected that the reliability level of a certain stream
                                                   *   has changed.
                                                   *    event_params[1] holds the new reliability (type int).
                                                   *    Delivered on rumQueueR                                                */
   RUM_CCT_GETTIME_FAILED                  = 110   /**< Indicates that the get time from Coordinated Cluster Time failed  */
}RUM_EVENT_t ;


 /** \brief Message keys for LOG events
  *
  * Message keys codes are used for error return (reason) codes in all functions
  */
typedef enum
{
 /* debug */
   RUM_D_GENERAL                          = 2,    /** Used to report a general debug event.                                      */
   RUM_D_METHOD_ENTRY                     = 3,    /** Generated upon entering a method.                                          */
   RUM_D_METHOD_EXIT                      = 4,    /** Generated upon exiting a method.                                           */

/* info */
   RUM_L_I_STATUS_INFO                    = 10,   /**< An information report of Log Level Status.                                */
   RUM_L_I_EVENT_INFO                     = 11,   /**< An information report of Log Level Event.                                 */
   RUM_L_I_TRACE_INFO                     = 12,   /**< An information report of Log Level Trace.                                 */
   RUM_L_I_XTRACE_INFO                    = 13,   /**< An information report of Log Level extended Trace.                        */
   RUM_L_I_GENERAL_INFO                   = 14,   /**< A General information report.                                             */

   RUM_L_I_START_SERVICE                  = 20,   /**< Start of a new service or a new thread.                                   */
   RUM_L_I_STOP_SERVICE                   = 21,   /**< Stop of a service or a thread.                                            */
   RUM_L_I_NACK                           = 22,   /**< NACK related information.                                                 */
   RUM_L_I_BUFFER_CLEAN                   = 23,   /**< Informs about history cleaning at the transmitter.                        */
   RUM_L_I_CONTROL_DATA_RECEIVED          = 24,   /**< Received control message                                                  */

/* warning */
   RUM_L_W_EVENT_WARNING                  = 50,   /**< A warning report of Log Level Event.                                      */
   RUM_L_W_TRACE_WARNING                  = 51,   /**< A warning report of Log Level Trace.                                      */
   RUM_L_W_GENERAL_WARNING                = 52,   /**< A General warning report.                                                 */

   RUM_L_W_SOCKET_WARNING                 = 60,   /**< A warning related to socket create or access.                             */
   RUM_L_W_QUEUE_WARNING                  = 61,   /**< A warning related to internal RUM queue handling.                         */
   RUM_L_W_NACK_WARNING                   = 62,   /**< A warning related receiving or processing a NACK.                         */

/* error */
   RUM_L_E_FATAL_ERROR                    = 100,  /**< An error report of Log Level Fatal error.                                 */
   RUM_L_E_GENERAL_ERROR                  = 101,  /**< A General error report.                                                   */
   RUM_L_E_INTERNAL_SOFTWARE_ERROR        = 102,  /**< An error due to an internal software failure                              */


   RUM_L_E_MEMORY_ALLOCATION_ERROR        = 110,  /**< An error occurred while trying to allocate memory.                        */
   RUM_L_E_SOCKET_ERROR                   = 111,  /**< An error related to socket create or access.                              */
   RUM_L_E_SOCKET_CREATE                  = 112,  /**< An error occurred while creating a socket.                                */
   RUM_L_E_QUEUE_ERROR                    = 113,  /**< An error related to internal RUM queue handling.                          */
   RUM_L_E_SEND_FAILURE                   = 114,  /**< RUM failed to send a packet.                                              */
   RUM_L_E_FILE_NOT_FOUND                 = 115,  /**< A file (such as configuration file) was not found.                        */
   RUM_L_E_CONFIG_ENTRY                   = 116,  /**< Bad configuration entry.                                                  */
   RUM_L_E_BAD_PARAMETER                  = 117,  /**< Bad parameter passed to a function.                                       */
   RUM_L_E_MCAST_INTERF                   = 118,  /**< An error occurred while trying to use the specified multicast interface.  */
   RUM_L_E_TTL                            = 119,  /**< Error in TTL value.                                                       */
   RUM_L_E_LOCAL_INTERFACE                = 120,  /**< An error occurred while trying to set the local interface.                */
   RUM_L_E_INTERRUPT                      = 121,  /**< Service interruption error.                                               */
   RUM_L_E_BAD_CONTROL_DATA               = 122,  /**< Received bad control message                                              */
   RUM_L_E_BAD_CONTROL_DATA_CREATION      = 123,  /**< Failed to create control message                                          */
   RUM_L_E_INTERNAL_LIMIT                 = 124,  /**< Could not perform operation since an internal RUM limit has been exceeded */
   RUM_L_E_THREAD_ERROR                   = 125,  /**< Thread control error                                                      */
   RUM_L_E_STRUCT_INIT                    = 126,  /**< Error initializing a structure                                            */
   RUM_L_E_BAD_ADDRESS                    = 127,  /**< Failed to convert given string address to internal form                   */
   RUM_L_E_LOAD_LIBRARY                   = 128,  /**< Failed to load dynamic library                                            */
   RUM_L_E_PORT_BUSY                      = 129,  /**< Failed to bind to listening port                                          */

   RUM_L_E_SHM_OS_ERROR                   = 141,  /**< Shared memory related system call has failed                              */

   RUM_L_E_INSTANCE_INVALID               = 150,  /**< Invalid instance structure supplied                                       */
   RUM_L_E_INSTANCE_CLOSED                = 151,  /**< Supplied instance structure has been closed                               */
   RUM_L_E_QUEUE_INVALID                  = 152,  /**< Invalid queue structure supplied                                          */
   RUM_L_E_QUEUE_CLOSED                   = 153,  /**< Supplied queue structure has been closed                                  */
   RUM_L_E_TOO_MANY_INSTANCES             = 154,  /**< Maximum number of instances have been created                             */
   RUM_L_E_TOO_MANY_STREAMS               = 155,  /**< Maximum number of streams are in service                                  */
   RUM_L_E_CONN_INVALID                   = 156,  /**< Supplied connection does not exist or no longer valid                     */
   RUM_L_E_BAD_MSG_PROP                   = 157,  /**< Bad message property                                                      */
   RUM_L_E_BAD_API_VERSION                = 158,  /**< Bad value of API version                                                  */

   RUM_L_E_LOG_ERROR                      = 200,  /**< General error in logging utility                                          */
   RUM_L_E_LOG_INSTANCE_ALREADY_EXISTS    = 201,  /**< Logging is already configured for this instance                           */
   RUM_L_E_LOG_INSTANCE_UNKNOWN           = 202,  /**< Logging is not configured for this instance                               */
   RUM_L_E_LOG_INVALID_CONFIG_PARAM       = 203,  /**< Logging configuration parameter is not valid                              */
   RUM_L_E_LOG_MAX_NUMBER_OF_COMPONENTS_EXEEDED = 204,  /**< The maximal number of components exceeded in the logging configuration    */
   RUM_L_E_LOG_COMPONENT_NOT_REGISTERED   = 205,  /**< Logging is not configured for this component                              */
   RUM_L_E_LOG_TOO_MANY_FILTERS_DEFINED   = 206,  /**< Too many filters configured for instance logging                          */



   RUM_MAX_MSG_KEY                        = 500
}RUM_LOG_KEY_t ;

/** \brief Constants used in connection events */
typedef enum
{
   RUM_CONNECTION_ESTABLISH_SUCCESS       = 50,         /**< successfully established a new connection                           */
   RUM_CONNECTION_ESTABLISH_FAILURE       = 51,         /**< failed to establish new connection                                  */
   RUM_CONNECTION_ESTABLISH_TIMEOUT       = 52,         /**< failed to establish new connection after timeout                    */
   RUM_CONNECTION_ESTABLISH_IN_PROCESS    = 53,         /**< failed to establish new connection since another connection to the
                                                         *   same destination is in process                                      */

   RUM_NEW_CONNECTION                     = 60,         /**< new connection detected at the receiver                             */
   RUM_CONNECTION_READY                   = 61,         /**< new connection is ready at the receiver                             */
   RUM_CONNECTION_HEARTBEAT_TIMEOUT       = 62,         /**< heartbeat timeout detected on the connection                        */
   RUM_CONNECTION_BROKE                   = 63,         /**< connection broke unexpectedly                                       */
   RUM_CONNECTION_CLOSED                  = 64          /**< connection was gracefully closed                                    */
}RUM_CONN_EVENT_t ;




 /* Constants used to define limits on RUM instances and queues */

/** \brief maximal number of RUM instances that can be supported.
  *  If there are more than RUM_MAX_INSTANCES open RUM instances an additional
  *  rum instance cannot be created.                                                               */
#define RUM_MAX_INSTANCES              100

/** \brief maximal number of transmitter queues that can be supported by an RUM instance.
 *  If there are more than RUM_MAX_TX_QUEUES open queues an additional queue cannot be created
 *  Note that for efficiency a limit on the number of queues/streams is set by the advanced
 *  configuration parameter MaxStreamsPerTransmitter which is set by default to 1024               */
#define RUM_MAX_TX_QUEUES              2048

/** \brief maximal number of receiver queues that can be supported by an RUM instance.
  *  If there are more than RUM_MAX_RX_QUEUES open queues an additional queue cannot be created    */
#define RUM_MAX_RX_QUEUES              1024

 /* Constants used in the definitions of the API data structures */

/** \brief maximal length of a queue name string including NULL terminator
 * RUM_MAX_QUEUE_NAME must be a multiple of 8                                                      */
#define RUM_MAX_QUEUE_NAME             2048

/** \brief maximal number of rumRxMessage elements in a single rumPacket                           */
#define RUM_MAX_MSGS_IN_PACKET         256

/** \brief maximal length of RUM version string including NULL terminator                          */
#define RUM_API_VERSION_LEN            64

/** \brief maximal length of IP address string including NULL terminator                           */
#define RUM_ADDR_STR_LEN               64

/** \brief maximal length of host name string including NULL terminator                            */
#define RUM_HOSTNAME_STR_LEN           256

/** \brief maximal length of file name string including NULL terminator                            */
#define RUM_FILE_NAME_LEN              1024

/** \brief maximal length of bitmap used in TurboFlow bitmap mode                                  */
#define RUM_MAX_BITMAP_LENGTH          255

/*****************************************************/
/*-----  RUM C Data Structures ---------*/
/*****************************************************/

 /** \brief RUM instance handle
  *
  * Includes a unique handler identifying the RUM instance.
  */
typedef struct
{
        int instance;                  /**< RUM instance identifier  */
        char rsrv[16] ;                /**< Reserved                 */
}rumInstance ;

 /** \brief Transmitter Queue handle
  *
  * Includes the Transmitter instance and a Queue handler (unique non-negative number).
  */
typedef struct
{
        int handle ;                   /**< Transmitter Queue handle */
        int instance ;                 /**< RUM instance identifier  */
        char rsrv[16] ;                /**< Reserved                 */
}rumQueueT ;

 /** \brief Receiver Queue handle
  *
  * Includes the RUM instance and a Queue handler (unique non-negative number).
  */
typedef struct
{
        int handle ;                   /**< Receiver Queue handle    */
        int instance ;                 /**< RUM instance identifier  */
        char rsrv[16] ;                /**< Reserved                 */
}rumQueueR ;

 /** \brief Connection handle
  *
  * Includes the (unique) connection id and information about the connection.
  */
typedef struct
{
        rumConnectionID_t   connection_id ;                   /**< Unique connection id                                  */
        int                 rum_instance ;                    /**< The id of this RUM instance                           */
        RUM_CONN_STATE_t    connection_state ;                /**< State of the connection. Can be one of the states
                                                               *   defined in RUM_CONN_STATE_t                           */
        int                 local_server_port ;               /**< The server port used by this RUM instance             */
        int                 local_connection_port ;           /**< The local port used by the connection                 */
        int                 remote_server_port ;              /**< The server port that is used by the RUM instance at 
                                                               *   the remote side of the connection                     */
        int                 remote_connection_port ;          /**< The port used by the remote side of the connection    */
        char                local_addr[RUM_ADDR_STR_LEN] ;    /**< The local address of the connection                   */
        char                remote_addr[RUM_ADDR_STR_LEN] ;   /**< The remote address of the connection                  */
        int                 shared_memory_is_used ;           /**< A value of 1 indicates that RUM is using shared memory
                                                               *   for data delivery for this connection                 */
        char                rsrv[124] ;                       /**< Reserved                                              */
}rumConnection ;

/*----    Message selection related structures   ----*/

/**
 * \brief A single message property.
 * rumMsgProperty must be initialized (using rumInitStructureParameters() with id RUM_SID_MSGPROPERTY) before it is first used
 */
typedef struct
{
   int                       rum_length ;            /**< For RUM usage, length of this structure                        */
   int                       rum_reserved ;          /**< For RUM usage, must not be changed                             */
   RUM_MSG_PROPERTY_TYPE_t   type ;                  /**< Type of the property. Default RUM_MSG_PROP_INT32               */
   int                       value_length ;          /**< Length of the property value (only for RUM_MSG_PROP_BYTES)     */
   union
   {
     int          int32 ;
     rum_int64    int64 ;
     double       dbl ;
     void        *ptr ;
   } value ;                                         /**< The value of the property                                      */
   char                     *name ;                  /**< Property name                                                  */
   short                     name_length ;           /**< Length of property name                                        */
   short                     code ;                  /**< Code of the property. Codes of value less than zero (<0) are
                                                      *   reserved for RUM specific codes. A value of zero indicates no
                                                      *   code is specified. Code values greater than zero are for app.
                                                      *   proprietary codes                                              */
   LLM_PROP_KINDS_t          kind;                   /**< Extended type information                                      */
   char                      pad[sizeof(void *)] ;   /**< Reserved - Alignment pad                                       */
   char                      rsrv[64] ;              /**< Reserved                                                       */
}rumMsgProperty ;

/**
 * Callback which is used in the message selection process to evaluate ALL the properties of a message.
 * If this callback is provided in the rumMsgSelector, RUM invokes this callback with an array that includes all the properties
 * that are attached to the message. The application should evaluate the properties and indicate whether the message should be
 * accepted (return TRUE) or filtered (return FALSE).
 * Using this callback (rather than SQL92 based selector) means the application is responsible for implementing the message filtering
 * logic but provides the application much greater flexibility in processing message properties.
 *
 * @param msg_props pointer to an array of the properties included in the message.
 * @param num_props number of properties included in with the message (can be zero).
 * @param user parameter that is used to associate the callback with the called function context.
 * @return 1 to indicate TRUE, 0 to indicate FALSE.
 */
typedef int (*rum_all_props_select_t)(const rumMsgProperty *msg_props, int num_props, void *user) ;

/**
 * Callback which is used in the message selection process to evaluate a SINGLE property of a message.
 * RUM invokes this callback when a property that matches one of the rumApplicationMsgFilter provided
 * in the rumMsgSelector is included in a message.
 * @param msg_property the property on which the message selector filter should be invoked
 * @param user parameter that is used to associate the callback with the called function context.
 * @return 1 to indicate TRUE, 0 to indicate FALSE.
 */
typedef int (*rum_select_msg_t)(const rumMsgProperty *msg_property, void *user) ;

/**
 * \brief An application message filter callback.
 * Message selection is performed based on the properties delivered in each message
 */
typedef struct
{
   void                     *user ;                  /**<  User context to be delivered to the callback                   */
   rum_select_msg_t          callback ;              /**<  A callback to be used with the selector to evaluate a single
                                                      *    message property */
   int                       filter_id ;             /**< ID of the filter. Used to identify the filter in the selector
                                                      *   string, e.g., "prop_name SELECT_BY_FILTER 18"                   */
   char                      pad[sizeof(int)] ;      /**< Reserved - Alignment pad                                        */
   char                      rsrv[64] ;              /**< Reserved                                                        */
}rumApplicationMsgFilter ;

/**
* \brief An RUM message selector. <br>
 * The RUM selector uses the syntax of a JMS Message selector which is in turn based on a subset of the SQL92 conditional
 * expression syntax. The RUM selector fully implements the functionality of a JMS selector with the following exceptions: <ul>
 * <li> An identifier must begin with a letter and contain only letters, underscore characters (_), and digits.
 * <li> Compound Arithmetic Expressions are not supported - The RUM selector allows only numeric constants or property names in expressions.
 *      For example "Prop=5", and "Prop1>Prop2" are legal selectors but "Prop1=5+3" and "Prop1>Prop2+5" are not.
 * <li>  Precedence of AND over OR operation is not supported - logical expressions containing AND and OR operations are evaluated from left to right.
 *      The application should use parenthesis to enforce the right precedence.
 *      For example the selector "Prop1='A' OR Prop1='B' AND Prop2='C'" will be evaluated as "(Prop1='A' OR Prop1='B') AND Prop2='C'".
 *      Note that NOT operations have the right precedence.
 * <li> The use of ESCAPE in LIKE expressions is not supported; instead '\' (backslash) is always used as an escape character in LIKE expressions.
 * </ul>
 * The RUM selector includes several features which are not part of SQL92. The RUM selector enables specifying an identifier by code as follows:
 * "LLM_CODE_num", where num is an integer (>0).
 * In addition the RUM selector enables the application to provide its own callbacks to evaluate properties. To that end
 * one additional selector operation is added SELECT_BY_FILTER to be use with property names or codes.
 * The syntax of using this operation is:  Identifier SELECT_BY_FILTER filter_id
 *      - where Identifier is the name of a message property and filter_id is the ID of one of the application message
 *        filters provided in app_filters. Example "orderType SELECT_BY_FILTER 3" means that when a message with a property
 *        name of orderType is received the application message filter with ID 3 will be invoked by RUM to evaluate it.
 *        Example "LLM_CODE_77 SELECT_BY_FILTER 55" means that when a message with a property
 *        code 77 is received the application message filter with ID 55 will be invoked by RUM to evaluate it.
 *
 * It is possible to combine the RUM operations with standard selector operations, for example
 * "orderType SELECT_BY_FILTER 3 AND LLM_CODE_77 SELECT_BY_FILTER 55 AND age < 128"
 *
 * rumMsgSelector must be initialized (using rumInitStructureParameters() with id RUM_SID_MSGSELECTOR) before it is first used
 */
typedef struct
{
   int                       rum_length ;          /**< For RUM usage, length of this structure                        */
   int                       rum_reserved ;        /**< For RUM usage, must not be changed                             */
   char                     *selector ;            /**< The RUM selector represented as a NULL terminated string       */
   rumApplicationMsgFilter  *app_filters ;         /**< Pointer to an array of application message filter elements to
                                                    *   be used with the selector                                      */
   rum_all_props_select_t    all_props_filter ;    /**< A callback that the application can set in case it wants to
                                                    *   evaluate all properties. If all_props_filter is not NULL then
                                                    *   message selection will be done using this callback ONLY, i.e.,
                                                    *   the selector and app_filters fields are ignored.               */
   void                     *all_props_user ;      /**< User context to be delivered to the all_props_filter callback  */
   int                       app_filters_length ;  /**< Number of filter elements defined the app_filters array        */
   char                      pad[sizeof(int)] ;    /**< Reserved - Alignment pad                                       */
   char                      rsrv[64] ;            /**< Reserved                                                       */
}rumMsgSelector ;


/*----    Queue and stream structures   ----*/

 /** \brief Stream information provided in each rumRxMessage and rumPacket
  *
  * A general structure to hold stream information fields.
  */
typedef struct
{
  rumStreamID_t             stream_id ;                       /**< Stream unique ID                           */
        void              **stream_params ;                   /**< List of stream  parameters                 */
        int                 nparams ;                         /**< Number of stream parameters                */
        int                 port ;                            /**< Transmitter port                           */
        char               *source_addr ;                     /**< Transmitter address                        */
        char               *queue_name ;                      /**< Queue name                                 */
        char                pad[sizeof(void *)] ;             /**< Reserved - Alignment pad                   */
        int                 pad1 ;                            /**< Reserved - Alignment pad                   */
        int                 use_msg_properties ;              /**< Use message properties mode of stream      */
        char                rsrv[24] ;                        /**< Reserved                                   */
}rumStreamInfo ;

 /** \brief Message parameters provided to a Message Listener
  *
  *  A general structure to hold message information fields.
  */
typedef struct
{
   rumMessageID_t           msg_sqn ;                         /**< Message sequence number                    */
        char               *msg_buf ;                         /**< Message data                               */
        int                 msg_len ;                         /**< Length of message data                     */
    unsigned int            pad1 ;                            /**< Reserved - Alignment pad                   */
    rumStreamInfo          *stream_info;                      /**< stream information                         */
        int                 hdr_len ;                         /**< Length of header information               */
        char                rsrv[12] ;                        /**< Reserved                                   */
}rumRxMessage ;

/**
 * \brief Message parameters used in rumTSubmitMessage.
 *
 * The message content must include at least one byte.
 * rumTxMessage must be initialized (using rumInitStructureParameters() with id RUM_SID_TxMESSAGE) before it is first used.
 * Once initialized rumTxMessage can be used multiple times.
 */
typedef struct
{
        int                 rum_length ;                      /**< For RUM usage, length of this structure                                                 */
        int                 rum_reserved ;                    /**< For RUM usage, must not be changed                                                      */
   rumMessageID_t          *msg_sqn ;                         /**< Output parameter - if not NULL RUM returns the sequence number of the submitted message.*/
   unsigned int            *pad1 ;                            /**< Reserved - Alignment pad                                                                */
        char               *msg_buf ;                         /**< Message data                                                                            */
        int                 msg_len ;                         /**< Length of message data (must be > 0)                                                                  */
        int                 dont_batch ;                      /**< if set (value != 0) will force message buffer flush after submitting the message.
                                                               *   Default 0                                                                               */
   rumMsgProperty          *msg_props ;                       /**< Message properties array. A pointer to an array of rumMsgProperty elements that should
                                                               *   be attached to the message. This parameter is ignored if num_props is zero or less      */
        int                 num_props ;                       /**< Number of properties from the msg_props array to attach to the message. A value of zero
                                                               *   (or less) indicates properties should not be attached                                   */
        int                 num_frags ;                       /**< Number of fragments from which a message is to be assembled. If set (value > 0) RUM
                                                               *   constructs the message by reassembling num_frags message fragments from the msg_frags
                                                               *   array into a single message                                                             */
        char              **msg_frags ;                       /**< Message fragments array. A pointer to an array of buffers comprising the fragments of
                                                               *   the message. Only the first num_frags buffers are used. Fragments are reassembled
                                                               *   according to the order they appear in the msg_frags array.
                                                               *   This parameter is ignored if num_frags is zero or less                                  */
        int                *frags_lens ;                      /**< Lengths of message fragments in msg_frags. A pointer to an array of integer each holding
                                                               *   the length of the fragment in the corresponding index in the msg_frags array.
                                                               *   The length of each fragment must be at least 1 byte.
                                                               *   This parameter is ignored if num_frags is zero or less                                  */
   rumtfBitMap_t            msg_bitmap ;                      /**< TurboFlow bit-map (ignored if the queue is not in turboFlow bitmap mode)                */
        int                 bitmap_length ;                   /**< Length in bytes of msg_bitmap, must not exceed RUM_MAX_BITMAP_LENGTH                    */
        int                 pad ;                             /**< Reserved - Alignment pad                                                                */
        char                rsrv[52-2*sizeof(void *)] ;       /**< Reserved                                                                                */
}rumTxMessage ;


 /** \brief Packet information including Stream information and a list of messages */
typedef struct
{
     rum_uint64         rum_expiration;                       /**< Expiration time ; for RUM usage only                                                    */
        char           *rum_buff_reserved;                    /**< Packet pointer; for RUM usage only                                                      */
        void           *rum_pst_reserved;                     /**< Stream pointer; for RUM usage only                                                      */
   rumStreamInfo       *stream_info;                          /**< Stream information of the received packet                                               */
        int             rum_instance ;                        /**< RUM instance ; for RUM usage only                                                       */
        int             rum_flags_reserved;                   /**< Packet flags; for RUM usage only                                                        */
   unsigned int         first_packet_sqn ;                    /**< First packet sequence number of a multi packet message, that is, when the message
                                                               *   size is greater than the LLM packet size. This field indicates the packet 
                                                               *   sequence number of the first packet of the message.                                     */
   unsigned int         last_packet_sqn ;                     /**< Last packet sequence number of a multi packet message, that is, when the message
                                                               *   size is greater than the LLM packet size. This field indicates the packet 
                                                               *   sequence number of the last packet of the message.                                      */
        int             num_msgs;                             /**< Number of messages in the packet                                                        */
        char            pad[sizeof(int)] ;                    /**< Reserved - Alignment pad                                                                */
        void           *rmm_ptr_reserved ;                    /**< Reserved ; for RMM usage only                                                           */
     rumRxMessage       msgs_info[RUM_MAX_MSGS_IN_PACKET];    /**< List of messages in the packet                                                          */
        double          receive_time_stamp ;                  /**< A time stamp representing the time at which the packet was first received by RUM.
                                                               *   The time stamp is provided by the clock that is used by the RUM instance
                                                               *   for latency monitoring; this clock can be the RUM internal clock, the Coordinated
                                                               *   Cluster Time, or the external clock the application provided as the get_time
                                                               *   callback in rumLatencyMonParams. The time is provided as a double representing a
                                                               *   value in seconds. To get the current time of the clock that provided the time
                                                               *   stamp use the rumRxGetTime API with the RUM instance on which the packet was
                                                               *   received. If the application provided an external clock for latency monitoring it
                                                               *   is more efficient to call it directly (rather than using rumRxGetTime).
                                                               *   It is important to note that a time stamp is provided only when latency monitoring
                                                               *   of level RUM_LATENCY_MON_MEDIUM or higher is enabled on the queue receiver and 
                                                               *   even then not every packet will be time stamped.
                                                               *   The rate at which packets are time stamped is controlled by the receiver advanced
                                                               *   configuration parameter, LatencyMonSampleRate_rx.
                                                               *   A value of zero indicates that a time stamp was not provided for the packet.      */
        double          sender_time_stamp ;                   /**< A time stamp representing the time at which the packet was generated by the RUM 
                                                               *   sender, that is, the time at which the first message, contained in the packet, 
                                                               *   was submitted by the sending application.
                                                               *   The clock used to generate this time stamp is the sender's latency monitoring  
                                                               *   clock (internal RUM, Coordinate Cluster Time, or external application clock, see
                                                               *   receive_time_stamp for details).
                                                               *   In order to be able to calculate the delay from the receive and sender time 
                                                               *   stamps the clocks of the RUM sender and the receiver should be synchronized.
                                                               *   It is important to note that this information is provided only when latency
                                                               *   monitoring of level RUM_LATENCY_MON_HIGH is enabled on the queues of both the
                                                               *   sender and the receiver and even then only on packets which have the sender time
                                                               *   stamp. The time stamp is not delivered if the latency monitoring deliver_timestamp
                                                               *   flag on the sender queue is turned off. Senders using LLM version 2.3 or earlier  
                                                               *   do not send this time stamp.
                                                               *   The rate at which the sender's packets are time stamped is controlled by the
                                                               *   RUM advanced configuration parameter, LatencyMonSampleRate_tx.
                                                               *   A value of zero indicates that a time stamp was not provided for the packet.      */
        char            rsrv[48] ;                            /**< Reserved                                                                          */
}rumPacket ;


 /** \brief Event information */
typedef struct
{
   rumStreamID_t   stream_id ;                                /**< Stream identifier                          */
        void     **event_params ;                             /**< List of event parameters                   */
        int        nparams ;                                  /**< Number of event parameters                 */
    RUM_EVENT_t    type ;                                     /**< Event type                                 */
        int        port;                                      /**< Transmitter port                           */
        char       source_addr[RUM_ADDR_STR_LEN] ;            /**< Transmitter address                        */
        char       queue_name[RUM_MAX_QUEUE_NAME] ;           /**< Queue name                                 */
        char       pad[sizeof(void *) + sizeof(int)] ;        /**< Reserved - Alignment pad                   */
        char       rsrv[32] ;                                 /**< Reserved                                   */
}rumEvent ;

/** \brief LOG Event information */
typedef llmLogEvent_t rumLogEvent ;

/** \brief connection Event information */
typedef struct
{
        rumConnection      connection_info ;                  /**< Unique connection id                       */
        void              **event_params ;                    /**< List of event parameters                   */
        int                nparams ;                          /**< Number of event parameters                 */
        RUM_CONN_EVENT_t   type ;                             /**< Event type                                 */
        char               *connect_msg ;                     /**< Application message that is set when the
                                                               *   connection is established and is delivered
                                                               *   when the connection is first accepted. Can
                                                               *   be NULL if was not set at source           */
        int                msg_len ;                          /**< Length in bytes of connect_msg             */
        char               pad[sizeof(int)] ;                 /**< Reserved - Alignment pad                   */
        char               rsrv[64] ;                         /**< Reserved                                   */
}rumConnectionEvent ;

/**
 * \brief Stream parameters which are delivered to the accept_stream callback when a new stream is detected.
 *
 * new_stream_user is an output parameter; the application should set this parameter in order to associate
 * a different user structure with the stream. If new_stream_user is not set by the application the user
 * associated with the accept_stream callback will be used for the stream.
 */
typedef struct
{
        rumStreamID_t          stream_id ;                              /**< Unique stream id                                           */
        rumConnection         *rum_connection ;                         /**< The connection on which the stream is accepted             */
        rumEvent              *event ;                                  /**< Event associated with the new stream                       */
        void                  *new_stream_user ;                        /**< output: user to associate with the new stream or NULL      */
        RUM_Tx_RELIABILITY_t   reliability ;                            /**< reliability of the stream                                  */
        int                    pad1 ;                                   /**< Reserved - Alignment pad                                   */
        int                    is_late_join ;                           /**< is the stream a Late Join stream                           */
        int                    use_msg_properties ;                     /**< Use message properties mode of stream                      */
        char                   pad[sizeof(void *)] ;                    /**< Reserved - Alignment pad                                   */
        char                   rsrv[64] ;                               /**< Reserved                                                   */
}rumStreamParameters ;



/** Signatures of Callback Functions used in API    */

/** RUM event callback function signature.
 *  A callback function to be invoked on events related to a queue (transmitter or receiver)
 *  or an instance.
 *  The on_event() callback function needs to be implemented by the calling application.
 *  The user parameter is used to associate the queue with the called function context.
 *  The application must not change any of the fields of the event structure
 *  (event should be treated as read-only).
 */
typedef void (*rum_on_event_t)(const rumEvent *event, void *user) ;


/** RUM log event callback function signature
 *  A callback function to be called on log events generated by an RUM transmitter or receiver.
 *  The on_log_event() callback function needs to be implemented by the calling application.
 *  The user parameter is used to associate the queue with the called function context.
 *  The application must not change any of the fields of the log_event structure
 *  (log_event should be treated as read-only).
 */
typedef llm_on_log_event_t rum_on_log_event_t;

/** RUM accept stream callback function signature
 *  A callback function to be called when a new stream is detected and RUM needs to verify whether
 *  the new stream should be accepted by a receiver queue.
 *  The accept_stream callback should return RUM_STREAM_REJECT (0) if the stream should NOT be accepted.
 *  If the stream should be accepted the accept_stream callback should return RUM_STREAM_ACCEPT (1).
 *  In case of multiple StreamSet receivers, in the same RUM instance, with overlapping accept_stream rules,
 *  a stream will be assigned to the first matching StreamSet (order of evaluation is random).
 *  The accept_stream() callback function needs to be implemented by the calling application.
 *  The user parameter is used to associate the queue with the called function context.
 *  The application may change only the new_stream_user field (to associate a different user with the new stream)
 *  of the stream_params structure, all other parameters should be treated as read-only.
 *
 *  @return RUM_STREAM_REJECT if the stream should be rejected or RUM_STREAM_ACCEPT if the stream should be accepted.
 */
typedef int (*rum_accept_stream_t)(rumStreamParameters *stream_params, void *user);

/** RUM on message callback function signature
 *  A callback function to be called for every message received on the queue_r receiver.
 *  The on_message() callback function needs to be implemented by the calling application.
 *  The msg structure should be considered as read-only; the application must not change any of its fields.
 *  Memory is freed after the on_message() function returns.
 *  When an on_message callback is set on a queue the queue is set to work in asynchronous receive mode,
 *  i.e., data from this queue is delivered (only by) the on_message callback.
 *  The user parameter is used to associate the Queue (or StreamSet) receiver with the called function context.
*/
typedef void (*rum_on_message_t)(rumRxMessage *msg, void * user);

/** RUM packet callback function signature
 *  A callback function to be called for every packet received on the queue_r receiver.
 *  The on_packet() callback function needs to be implemented by the calling application.
 *  The packet structure should be considered as read-only; the application must not change any of its fields.
 *  Memory is freed after the on_packet() function returns.
 *  When an on_packet callback is set on a queue the queue is set to work in asynchronous receive mode,
 *  i.e., data from this queue is delivered (only by) the on_packet callback.
 *  Working with on_packet rather than on_message can be more efficient since the application is getting a packet which
 *  may contain more than one message.
 *  The user parameter is used to associate the Queue (or StreamSet) receiver with the called function context.
*/
typedef void (*rum_on_packet_t)(rumPacket *packet, void * user);

/** RUM announcing packet arrival callback function signature
 *  A callback to announce availability of packets on queue_r.
 *  The on_data callback will be invoked whenever a new packet is ready to be
 *  delivered to the application on queue_r. Note that the callback will be invoked
 *  regardless of whether earlier packets that have been announced already have been
 *  consumed by the application.
 *  The on_data callback cannot be set on a queue on which an on_message or an on_packet callback is set.
 *  When on_data is used the queue is set to work in synchronous receive mode,
 *  i.e., data from this queue is received by calling the rumRReceivePacket API.
 *  It is important to note that on_data is used only for notification; the application
 *  should NOT call rumRReceivePacket (and in fact no other RUM API function) from this callback.
 *  The user parameter is used to associate the Queue with the called function context.
*/
typedef void (*rum_on_data_t)(void * user);


/** RUM free memory callback function signature
 *  A callback function to inform the application that memory associated with a connection listener can be freed.
 *  The free_callback() callback function needs to be implemented by the calling application.
 *  The user parameter is used to associate the Queue (or StreamSet) receiver with the called function context.
 *  The function should return RUM_SUCCESS on success or RUM_FAILURE on failure.
*/
typedef int (*rum_free_callback_t)        (void *callback_user) ;

/** RUM connection event function signature
 *  A callback function to inform the application about events related to a connection.
  * The connection_event() callback function needs to be implemented by the calling application.
 *  The user parameter is used to associate the Queue (or StreamSet) receiver with the called function context.
*/
typedef int (*rum_on_connection_event_t)  (rumConnectionEvent *connection_event, void *user) ;


/**
 * \brief Transmitter Queue parameters used when creating a new Queue.
 *
 * rumTxQueueParameters must be initialized (using rumInitStructureParameters() with id RUM_SID_TxQUEUEPARAMETERS) before it is first used
 */
typedef struct
{
        int                    rum_length ;                          /**< For RUM usage, length of this structure                           */
        int                    rum_reserved ;                        /**< For RUM usage, must not be changed                                */
        rumConnection          rum_connection ;                      /**< connection to use for the new queue                               */
        RUM_Tx_RELIABILITY_t   reliability ;                         /**< reliability for the queue. Default value RUM_RELIABLE_NO_HISTORY  */
        int                    pad1 ;                                /**< Reserved - Alignment pad                                       */
        int                    enable_msg_properties ;               /**< If set (not zero) message properties can be attached to
                                                                      *   messages sent on this queue. Disabled (zero) by default           */
        int                    is_late_join ;                        /**< set zero to create non-Late-Join queue. Default value 0           */
        rum_on_event_t         on_event ;                            /**< application callback to process events                            */
        void                  *event_user ;                          /**< user information associated with on_event                         */
        rumStreamID_t         *stream_id ;                           /**< output parameter; will hold the stream ID of the queue            */
        char                   queue_name[RUM_MAX_QUEUE_NAME] ;      /**< Queue Name                                                        */
        int                    app_controlled_batch ;                /**< If set, RUM does not attempt to send partial packets.
                                                                      *   In this case the application is responsible for sending
                                                                      *   partial packets using either the dont_batch flag in
                                                                      *   rumTxMessage ; if the
                                                                      *   application does not force partial packets to be sent RUM
                                                                      *   will only send packets when they fill up. Default 0               */
        char                   pad[sizeof(void *)+sizeof(int)];      /**< Reserved - Alignment pad                                          */
        char                   rsrv[248] ;                           /**< Reserved                                                          */
}rumTxQueueParameters ;

/**
 * \brief Receiver Queue parameters used when creating a new Queue or StreamSet.
 *
 * rumRxQueueParameters must be initialized (using rumInitStructureParameters() with id RUM_SID_RxQUEUEPARAMETERS) before it is first used
 */
typedef struct
{
        int                    rum_length ;                          /**< For RUM usage, length of this structure                           */
        int                    rum_reserved ;                        /**< For RUM usage, must not be changed                                */
        RUM_Rx_RELIABILITY_t   reliability ;                         /**< reliability for the Queue. Default value RUM_RELIABLE_RCV         */
        int                    pad1 ;                                /**< Reserved - Alignment pad                                          */
        char                   queue_name[RUM_MAX_QUEUE_NAME] ;      /**< Queue name to accept. Ignored if accept_stream is provided        */
        rum_accept_stream_t    accept_stream ;                       /**< accept stream callback. If NULL queue_name is used                */
        void                  *accept_user ;                         /**< user to associate with accept_stream callbacks                    */
        rum_on_event_t         on_event ;                            /**< callback to process events                                        */
        void                  *event_user ;                          /**< user to associate with on_event callbacks                         */
        rum_on_message_t       on_message ;                          /**< If not NULL Queue will be created in push/receive mode            */
        void                  *user ;                                /**< user to associate with on_message, on_packet or on_data callbacks */
        rumMsgSelector        *msg_selector ;                        /**< Message selector to be used for message filtering. Can be NULL to
                                                                      *   indicate no filtering should be performed. If a message selector
                                                                      *   is provided the enable_msg_properties flag must be set            */
        int                    enable_msg_properties ;               /**< Must be set (not zero) in order to receive streams with message
                                                                      *   properties. Disabled (zero) by default                            */
        int                    goback_time_milli ;                   /**< if >= 0 will set the time for go-back replay mode. Default 0      */
        rum_on_packet_t        on_packet ;                           /**< If not NULL Queue will be created in push/receive mode            */
        rum_on_data_t          on_data ;                             /**< If not NULL Queue will be created in pull/receive mode            */
        int                    notify_per_packet ;                   /**< If on_data is used, sets the notification policy. Default 0       */
        int                    max_pending_packets;                  /**< Max packets to hold in ready packet Q in pull (on_data) mode.
                                                                      *   If set (>0) forces queue cleaning. Default 0                      */
        int                    max_queueing_time_milli;              /**< Max time a packet is retained in ready packet Q in pull (on_data)
                                                                      *   mode. If set (>0) forces queue cleaning. Default 0                */
        int                    stream_join_backtrack ;               /**< Number of packets that the receiver will attempt to request
                                                                      *   from the transmitter's history when a new stream is accepted
                                                                      *   Default (value=0) is to join on the first data packet and not
                                                                      *   request any packets                                               */
        int                    pad2 ;                                /**< Reserved - Alignment pad                                          */
        char                   rsrv[64-5*4-sizeof(void *)] ;         /**< Reserved                                                          */
}rumRxQueueParameters ;

/**
 * \brief Connection parameters used when establishing a new connection.
 *
 * rumEstablishConnectionParams must be initialized (using rumInitStructureParameters() with id RUM_SID_ESTABLISHCONNPARAMS) before it is first used
 */
typedef struct
{
        int                    rum_length ;                          /**< For RUM usage, length of this structure                     */
        int                    rum_reserved ;                        /**< For RUM usage, must not be changed                          */
        char                   address[RUM_HOSTNAME_STR_LEN] ;       /**< Destination IP address or host name                         */
        unsigned char         *connect_msg ;                         /**< Application message that will be delivered at the
                                                                      *   destination when the connection is first accepted. Can be
                                                                      *   used for example as a means of authentication. Can be NULL  */
        int                    connect_msg_len ;                     /**< Length in bytes of connect_msg. Range 0 - 64512 for RUM_TCP,
                                                                      *   and (PacketSizeBytes-128) for RUM_IB                        */
        int                    port ;                                /**< Destination port. Range 0 - 65535. Default 0                */
   RUM_ESTABLISH_METHOD_t      connect_method ;                      /**< Describes the method of using existing connections.
                                                                      *   Default RUM_CREATE_NEW                                    */
        int                    heartbeat_timeout_milli ;             /**< Heartbeat timeout to be used for this connection. A value of
                                                                      *   zero indicates that heartbeat timeout will not be reported.
                                                                      *   Value must be >= 0. Default 10000                           */
        int                    heartbeat_interval_milli ;            /**< Interval between heartbeat packets sent on the connection
                                                                      *   Value must be >= 0 and smaller than heartbeat_timeout_milli.
                                                                      *   Default 1000                                                */
        int                    establish_timeout_milli ;             /**< Maximal time to attempt to establish the connection
                                                                      *   Value must be >= 0. Default 10000                           */
  rum_on_connection_event_t    on_connection_event ;                 /**< Connection event handling callback. Must be provided by the
                                                                      *   application                                                 */
        void                  *on_connection_user ;                  /**< User associated with the on_connection_event callback.
                                                                      *   Can be NULL                                                 */
        int                    one_way_heartbeat;                    /**< By default connection heartbeats are sent from both side of
                                                                      *   the connection. Set this flag (value = 1) to instruct RUM to
                                                                      *   send heartbeats only from the originating side. Default 0   */
        int                    use_shared_memory;                    /**< If set (not zero) RUM will use shared memory for data
                                                                      *   transfer. Note that the TCP connection must be established
                                                                      *   even when use_shared_memory is set, hence valid connection
                                                                      *   parameters must be provided. Default 0                      */
        char                   rsrv[64] ;                            /**< Reserved                                                    */
}rumEstablishConnectionParams ;

/**
 * \brief Port and address information used in RUM configuration
 *
 * The application can use the rumPort structure to define a reception port and address that the RUM will use to listen for
 * incoming connections. The application may provide the port number or allow
 * RUM to select a free port from a certain range.
 */
typedef struct
{
        int                    port_number ;                         /**< The number of the port to listen to. If the port_number
                                                                      *   is set to zero, a port number will be dynamically chosen
                                                                      *   from the range given by port_range_low to port_range_high.
                                                                      *   Range 0 - 65535                                            */
        int                    port_range_low ;                      /**< Low boundary of range used for dynamically assigned port.
                                                                      *   Used only if port_number is set to zero. Range 0 - 65535    */
        int                    port_range_high ;                     /**< Upper boundary of range used for dynamically assigned port.
                                                                      *   Used only if port_number is set to zero. If both port_number
                                                                      *   and port_range_high are zero then RUM will assign a dynamic
                                                                      *   port from a default range. Range 0 - 65535                  */
        int                    assigned_port ;                       /**< output parameter - the assigned port number                 */
        char                   networkInterface[RUM_ADDR_STR_LEN] ;  /**< If a valid IP address is provided RUM will use this address
                                                                      *   when it performs bind on the socket (port and address).
                                                                      *   A value of "NONE" directs RUM to use the Instance Interface,
                                                                      *   while a value of "ALL" or an empty string indicates that
                                                                      *   RUM should not perform bind on a specific address. Typically
                                                                      *   an application would provide a network interface only if it
                                                                      *   wants to limit data reception on this port to a specific
                                                                      *   network interface; in all other cases setting the value to
                                                                      *   "NONE" is recommended                                       */
        char                   rsrv[128] ;                           /**< Reserved                                                    */
}rumPort ;

/**
 * \brief Basic RUM instance configuration parameters.
 *
 * rumConfig must be initialized (using rumInitStructureParameters() with id RUM_SID_CONFIG) before it is first used
 */
typedef struct
{
    int                      rum_length ;                            /**< For RUM usage, length of this structure                     */
    int                      rum_reserved ;                          /**< For RUM usage, must not be changed                          */
    RUM_PROTOCOL_t           Protocol ;                              /**< The packet protocol to use. Default value RUM_TCP           */
    RUM_IPVERSION_t          IPVersion ;                             /**< The IP (Internet Protocol) version to use.
                                                                      *   Default value RUM_IPver_IPv4                                */
    int                      ServerSocketPort ;                      /**< Server port used by this instance to accept new connections
                                                                      *   Range 0 - 65535. Default value 35353                        */
    RUM_LOGLEV_t             LogLevel ;                              /**< Instance logging level. Default value RUM_LOGLEV_INFO       */
    RUM_RATE_LIMIT_t         LimitTransRate ;                        /**< Transmission rate control mechanism. When not disabled, the
                                                                      *   transmission rate is limited to TransRateLimitKbps.
                                                                      *    Default value RUM_DISABLED_RATE_LIMIT                      */
    int                      TransRateLimitKbps ;                    /**< Rate limit in kbps. Must be at least 8.
                                                                      *   Default value 100000                                        */
    int                      PacketSizeBytes ;                       /**< Size of packets in bytes. Range 300 - 65000
                                                                      *   Default value 8000                                          */
    RUM_TRANS_DIRECTION_t    TransportDirection ;                    /**< Transport direction send, receive or both
                                                                      *   Default value RUM_Tx_Rx (both)                              */
    int                      MaxMemoryAllowedMBytes ;                /**< The maximum amount of memory in MBytes that the instance
                                                                      *   is allowed to consume. MaxMemoryAllowedKBytes must be
                                                                      *   larger than 20. Default value 200.
                                                                      *   Maximal value 4000 for 32 bit and 1000000 for 64 bit        */
    int                      MinimalHistoryMBytes ;                  /**< The minimum amount of memory in MBytes that the instance
                                                                      *   must reserve for storing History packets. Typically set to
                                                                      *   zero unless the high availability features are used.
                                                                      *   Default value 0.
                                                                      *   Maximal value 4000 for 32 bit and 1000000 for 64 bit        */
    int                      SocketReceiveBufferSizeKBytes ;         /**< Socket buffer size for receive sockets. Default value 1024  */
    int                      SocketSendBufferSizeKBytes ;            /**< Socket buffer size for send sockets. Default value 64       */
    rum_on_event_t           on_event ;                              /**< callback to process events. Default value NULL              */
    void                    *event_user ;                            /**< user to associate with on_event callbacks.
                                                                      *   Default value NULL                                          */
    rum_free_callback_t      free_callback ;                         /**< Callback function to inform the application that memory
                                                                      *   associated with a connection listener can be freed.
                                                                      *   Default value NULL                                          */
    char                   **AncillaryParams ;                       /**< A list of advanced configuration parameters. Each entry in
                                                                      *   AncillaryParams is in the form "config_param=config_value".
                                                                      *   AncillaryParams can be used to set advanced configuration
                                                                      *   parameters. Default value NULL                              */
    int                      Nparams ;                               /**< Number of ancillary parameters. Default value 0             */
    char                     RxNetworkInterface[RUM_ADDR_STR_LEN] ;  /**< The IP address of the interface on which the RUM instance
                                                                      *   will receive data. If set to "None" the system default
                                                                      *   interface is used. If set to "All" will accept data on all
                                                                      *   interfaces. Default value "None"                            */
    char                     TxNetworkInterface[RUM_ADDR_STR_LEN] ;  /**< The IP address of the interface which the RUM instance will
                                                                      *   prefer for establishing outgoing connections. If set to
                                                                      *   "None" or "ALL" the system default interface is used.
                                                                      *   Default value "None"                                        */
    char                     AdvanceConfigFile[RUM_FILE_NAME_LEN] ;  /**< The name of an advance transmitter configuration file
                                                                      *   or "None". Note that if an advanced configuration file named
                                                                      *   rumAdvanced.cfg is found in the running directory that file
                                                                      *   will be used to set the advanced configuration parameters
                                                                      *   even if the AdvanceConfigFiles parameter is set to "None".
                                                                      *   Default value "None"                                        */
    int                     SupplementalPortsLength ;                /**< Input parameter. Number of elements in the SupplementalPorts
                                                                      *   array that contain a valid port definition that RUM should
                                                                      *   use. Default value is 0                                     */
    rumPort                 *SupplementalPorts ;                     /**< Input parameter. A pointer to an array of rumPort elements
                                                                      *   where each elements represents an additional reception
                                                                      *   (Server) port.
                                                                      *   The given array must be allocated by the application.
                                                                      *   Default value NULL                                          */
    const char*             instanceName;                            /**< A unique instance ID. (If not set it will be assigned by the RUM).  */
    char                     rsrv[256-sizeof(void *)-sizeof(char*)] ; /**< Reserved                                                    */
}rumConfig ;


/*----    Statistics related data structures    ----*/

/**
 * \brief Transmitter Stream statistics.
 *
 * All parameters are output parameters that are set by RUM when one of the statistics API that use this structure is invoked.
 */
typedef struct
{
        rum_uint64        timestamp ;                       /**< Relative time in milliseconds                          */
        rumStreamID_t     stream_id ;                       /**< ID of the stream                                       */
        rumConnectionID_t connection_id ;                   /**< ID of the connection used by the stream                */
        rum_uint64        messages_sent ;                   /**< Number of messages sent since the stream was created  
                                                             *   or the stream statistics were reset                    */
        rum_uint64        bytes_sent ;                      /**< Number of bytes sent since the stream was created or 
                                                             *   the stream statistics were reset                       */
        rum_uint64        repair_bytes ;                    /**< Number of repair data bytes sent since the stream was  
                                                             *   created or the stream statistics were reset            */
        rum_uint64        msg_sqn ;                         /**< Last message sequence number (HA queues)               */
        int               transport ;                       /**< Transport of the stream                                */
        int               reliability ;                     /**< Reliability level                                      */
        int               pad1 ; 
        unsigned int      txw_trail ;                       /**< Transmission window trail                              */
        unsigned int      txw_lead ;                        /**< Transmission window lead                               */
        unsigned int      late_join_mark ;                  /**< Current position of late join mark                     */
        int               naks_received ;                   /**< Number of NAKs received since the stream was created 
                                                             *   or the stream statistics were reset                    */
        unsigned int      repair_packets ;                  /**< Number of repair packets sent since the stream was 
                                                             *   created or the stream statistics were reset            */
        int               history_packets ;                 /**< Number of packets in History queue                     */
        int               pending_packets ;                 /**< Number of packets in pending queue                     */
        int               packet_rate ;                     /**< Current rate in packets/second                         */
        int               rate_kbps ;                       /**< Current rate in kbps                                   */
        int               batch_time_micro ;                /**< Current batch time in microseconds                     */
        int               heartbeat_timeout_milli ;         /**< Heartbeat timeout of the stream                        */
        int               dest_data_port ;                  /**< Destination port for data                              */
        char              dest_addr[RUM_ADDR_STR_LEN] ;     /**< Destination address of stream                          */
        char              stream_id_str[24] ;               /**< String representation of stream ID                     */
        char              queue_name[RUM_MAX_QUEUE_NAME] ;  /**< The queue Name of the stream                           */
        char              pad[sizeof(int)] ;                /**< Reserved - Alignment pad                               */
        rum_uint64        packets_sent ;                    /**< Number of data packets sent since the stream was 
                                                             *   created or the stream statistics were reset            */
        rum_uint64        total_packets_sent ;              /**< Total number of data packets sent since the stream was 
                                                             *   created                                                */
        rum_uint64        total_messages_sent ;             /**< Total number of messages sent since the stream was 
                                                             *   created                                                */
        rum_uint64        total_bytes_sent ;                /**< Total number of bytes sent since the stream was 
                                                             *   created                                                */ 
        char              rsrv[224] ;                       /**< Reserved                                               */
}rumTStreamStats ;

/**
 * \brief Receiver Stream statistics.
 *
 * All parameters are output parameters that are set by RUM when one of the statistics API that use this structure is invoked.
 */
typedef struct
{
        rum_uint64        timestamp ;                       /**< Relative time in milliseconds                          */
        rumStreamID_t     stream_id ;                       /**< ID of the stream                                       */
        rumConnectionID_t connection_id ;                   /**< ID of the connection used by the stream                */
        rum_uint64        messages_delivered ;              /**< Number of messages delivered to the application from 
                                                             *   this stream since the stream was accepted or the  
                                                             *   stream statistics were reset                           */
        rum_uint64        tf_msgs_filtered ;                /**< Number of messages filtered (by message filtering tools 
                                                             *   such as TurboFlow or message selector) from this stream  
                                                             *   since the stream was accepted or the stream statistics  
                                                             *   were reset                                             */
        rum_uint64        messages_received ;               /**< Number of messages received from this stream since the 
                                                             *   stream was accepted or the stream statistics were 
                                                             *   reset                                                  */
        rum_uint64        msgs_bytes_received ;             /**< Number of bytes received from this stream since the 
                                                             *   stream was accepted or the stream statistics were 
                                                             *   reset                                                  */
        int               reliability ;                     /**< Reliability level                                      */
        int               pad1 ; 
        unsigned int      txw_trail ;                       /**< Transmission window trail                              */
        unsigned int      txw_lead ;                        /**< Transmission window lead                               */
        unsigned int      rxw_tail ;                        /**< Receive window tail                                    */
        unsigned int      rxw_head ;                        /**< Receive window front                                   */
        unsigned int      packets_processed ;               /**< Number of packets processed by RUM from this stream 
                                                             *   since the stream was accepted or the stream statistics
                                                             *   were reset                                             */
        unsigned int      packets_delivered ;               /**< Number of packets delivered to application from this 
                                                             *   stream since the stream was accepted or the stream 
                                                             *   statistics were reset.                                 */
        unsigned int      packets_lost ;                    /**< Number of unrecoverable lost packets for this stream
                                                             *   since the stream was accepted or the stream statistics
                                                             *   were reset                                             */
        unsigned int      packets_duplicate ;               /**< Number of duplicate packets from this stream since the 
                                                             *   stream was accepted or the stream statistics were 
                                                             *   reset                                                  */
        int               pad2 ; 
        unsigned int      pad3 ; 
        char              stream_id_str[24] ;               /**< String representation of stream ID                     */
        char              queue_name[RUM_MAX_QUEUE_NAME] ;  /**< The queue Name of the stream                           */
        rum_uint64        total_messages_received ;         /**< Total number of messages received from the stream since
                                                             *   the stream was accepted                                */
        rum_uint64        total_messages_delivered ;        /**< Total number of messages delivered to the application
                                                             *   from the stream since the stream was accepted          */
        rum_uint64        total_msgs_bytes_received ;       /**< Total number of bytes received in messages from the 
                                                             *   stream since the stream was accepted                   */
        char              rsrv[236] ;                       /**< Reserved                                               */
}rumRStreamStats ;

/**
 * \brief Receiver Queue/StreamSet statistics
 *
 * All parameters except for stream_stats and stream_stats_length are output parameters that are set by RUM
 * when one of the statistics API that use this structure is invoked.
 */
typedef struct
{
        rum_uint64       timestamp ;                       /**< Relative time in milliseconds                          */
        rumMessageID_t   msg_sqn ;                         /**< Next message sequence number (HA queues)               */
        int              instance_id ;                     /**< ID of the RUM instance                                 */
        int              reliability ;                     /**< Reliability level                                      */
        int              pad1 ; 
        int              tf_labels_accepted ;              /**< Number of TurboFlow labels accepted                    */
        unsigned int     pad2 ; 
        int              num_streams ;                     /**< Number of streams actual                               */
        int              num_streams_recorded ;            /**< Number of streams recorded in stream_stats             */
        int              stream_stats_length ;             /**< Number of elements in stream_stats array, or the
                                                            *   maximal number of stream statistics the application
                                                            *   wants to record (whichever is smaller)                 */
        rumRStreamStats *stream_stats ;                    /**< Input parameter. A pointer to an array Array of
                                                            *   rumRStreamStats elements where statistics of the
                                                            *   streams which are accepted by this QueueR are to be
                                                            *   recorded. The stream_stats array must be allocated by
                                                            *   the application                                        */
        char             queue_name[RUM_MAX_QUEUE_NAME] ;  /**< Queue Name (zero length for StreamSet)                 */
        unsigned int     ready_packets ;                   /**< Number of packets in recvQ ("on_data" mode)            */
        unsigned int     trimmed_packets ;                 /**< Number of packets trimmed from the receive queue of the 
                                                            *   RUM queue (for queues working in "on_data" mode) since  
                                                            *   the queue was created or the receiver queue or  
                                                            *   instance statistics were reset                         */
        char             pad[sizeof(void *)+sizeof(int)] ; /**< Reserved - Alignment pad                               */
        unsigned int     packets_lost ;                    /**< Number of unrecoverable packets on the currently active
                                                            *   streams of the queue since the queue was created or 
                                                            *   the receiver queue or instance statistics were reset   */
        rum_uint64       messages_received ;               /**< Number of messages received on the queue since the
                                                            *   queue was created or the receiver queue or instance
                                                            *   statistics were reset                                  */
        rum_uint64       messages_delivered ;              /**< Number of messages delivered to the application on this
                                                            *   queue since the queue was created or the receiver queue
                                                            *   or instance statistics were reset                      */
        char             rsrv[224] ;                       /**< Reserved                                               */
}rumRQueueStats ;

/**
 * \brief RUM Instance statistics
 *
 * Provides general information about the RUM instance and information about transmitter streams, receiver streams,
 * and connections which are handled by this instance
 * All parameters are output parameters except those which are indicated as input parameters.
 */
typedef struct
{
        rum_uint64       timestamp ;                       /**< Relative time in milliseconds                          */
        rum_uint64       msgs_sent ;                       /**< Total number of messages sent, on the currently active 
                                                            *   streams, since the RUM instance was created or the   
                                                            *   instance statistics were reset                         */
        rum_uint64       msgs_received ;                   /**< Total number of messages received on the currently  
                                                            *   active streams, since the RUM instance was created or    
                                                            *   the instance statistics were reset                     */
        rum_uint64       bytes_sent ;                      /**< Total number of bytes sent on the currently active 
                                                            *   streams, since the RUM instance was created or the   
                                                            *   instance statistics were reset                         */
        rum_uint64       bytes_received ;                  /**< Total number of bytes received on the currently active 
                                                            *   streams, since the RUM instance was created or the   
                                                            *   instance statistics were reset                         */
        int              instance_id ;                     /**< ID of the RUM instance                                 */
        int              server_port ;                     /**< Server port used                                       */
        int              num_streams_tx ;                  /**< Number of active transmitter Queues/Streams            */
        int              num_streams_rx ;                  /**< Number of active receiver streams                      */
        int              num_rejected ;                    /**< number of receiver rejected streams                    */
        int              num_queues_rx ;                   /**< Number of active receiver Queues                       */
        int              packet_rate_tx ;                  /**< Current transmit rate in packets/sec                   */
        int              packet_rate_rx ;                  /**< Current receive rate in packets/sec                    */
        int              rate_kbps_tx ;                    /**< Current transmit rate in kbps                          */
        int              rate_kbps_rx ;                    /**< Current receive rate in kbps                           */
        int              token_bucket_rate ;               /**< Current rate of token_bucket                           */
        unsigned int     packets_sent ;                    /**< Number of data packets sent on the currently active 
                                                            *   streams since the RUM instance was created or the 
                                                            *   instance statistics were reset                         */
        unsigned int     packets_received ;                /**< Number of packets received on the currently active 
                                                            *   streams since the RUM instance was created or the
                                                            *   instance statistics were reset                         */
        int              num_buffers_tx ;                  /**< Total number of transmitter buffers allocated          */
        int              num_buffers_rx ;                  /**< Total number of receiver buffers allocated             */
        int              buffers_in_use_tx ;               /**< Number of transmitter buffers currently used           */
        int              buffers_in_use_rx ;               /**< Number of receiver buffers currently used              */
        int              streamT_stats_length ;            /**< Input parameter. Number of elements in streamT_stats
                                                            *   array, or the maximal number of transmitter stream
                                                            *   statistics the application wants to record (whichever
                                                            *   is smaller)                                            */
        int              num_streamsT_recorded ;           /**< Number of streams that were recorded in streamT_stats
                                                            *   (at most streamT_stats_length can be recorded)         */
        rumTStreamStats *streamT_stats ;                   /**< Input parameter. A pointer to an array of
                                                            *   rumTStreamStats elements where statistics of the
                                                            *   streams which are transmitted by this RUM instance are
                                                            *   to be recorded. The streamT_stats array must be
                                                            *   allocated by the application                           */
        int              streamR_stats_length ;            /**< Input parameter. Number of elements in streamR_stats
                                                            *   array, or the maximal number of receiver stream
                                                            *   statistics the application wants to record (whichever
                                                            *   is smaller)                                            */
        int              num_streamsR_recorded ;           /**< Number of streams that were recorded in streamR_stats
                                                            *   (at most streamR_stats_length can be recorded)         */
        rumRStreamStats *streamR_stats ;                   /**< Input parameter. A pointer to an array of
                                                            *   rumRStreamStats elements where statistics of the
                                                            *   streams which are accepted by this RUM instance are to
                                                            *   be recorded. The streamR_stats array must be allocated
                                                            *   by the application                                     */
        int              connection_stats_length ;         /**< Input parameter. Number of elements in the
                                                            *   connection_stats array, or the maximal number of
                                                            *   connection statistics the application wants to record
                                                            *   (whichever is smaller)                                 */
        int              num_conn_recorded ;               /**< Number of connections that were recorded in
                                                            *   connection_stats (at most connection_stats_length can
                                                            *   be recorded)                                           */
        rumConnection   *connection_stats ;                /**< Input parameter. A pointer to an array of
                                                            *   rumConnection elements where statistics of the
                                                            *   connections which are handled by this RUM instance are
                                                            *   to be recorded. The connection_stats array must be
                                                            *   allocated by the application                           */
        char             local_address[RUM_ADDR_STR_LEN] ; /**< Local address used by the RUM instance                 */
        const  char*     instanceName ;                    /**< Name of the rum instance                               */
        char             pad[sizeof(void *)] ;             /**< Reserved - Alignment pad                               */
        rum_uint64       total_msgs_sent ;                 /**< Total number of messages sent on all streams since the 
                                                            *   RUM instance was created                               */
        rum_uint64       total_msgs_received ;             /**< Total number of messages received on all streams since  
                                                            *   the RUM instance was created                           */
        rum_uint64       total_bytes_sent ;                /**< Total number of bytes sent on all streams since the 
                                                            *   RUM instance was created                               */
        rum_uint64       total_bytes_received ;            /**< Total number of bytes received on all streams since the 
                                                            *   RUM instance was created                               */
        rum_uint64       total_packets_sent ;              /**< Number of data packets sent on all streams since the 
                                                            *   RUM instance was created                               */
        rum_uint64       total_packets_received ;          /**< Number of packets received on all streams since the
                                                            *   RUM instance was created                               */
        char             rsrv[212-sizeof(void *)] ;        /**< Reserved                                               */
}rumStats ;

/** \brief RUM Instance configuration parameters  */
typedef struct
{
        int    instance_id ;                               /**< ID of the RUM instance                                 */
        int    Protocol ;                                  /**< Packet transport protocol                              */
        int    IPVersion ;                                 /**< The IP (Internet Protocol) version to use              */
        int    ServerPort ;                                /**< Server port in use                                     */
        int    LogLevel ;                                  /**< Current log level                                      */
        int    LimitTransRate ;                            /**< Rate limit mode                                        */
        int    TransRateLimitKbps ;                        /**< Rate limit (if LimitTransRate is not disabled)         */
        int    PacketSizeBytes ;                           /**< Packet size                                            */
        int    TransportDirection ;                        /**< TransportDirection (send/receive or both)              */
        int    MaxMemoryAllowedMBytes ;                    /**< Maximum amount of memory in MBytes for the instance    */
        int    MinimalHistoryMBytes ;                      /**< Memory in MBytes reserved for transmission history     */
        int    SocketReceiveBufferSizeKBytes ;             /**< Socket buffer size for receive sockets                 */
        int    SocketSendBufferSizeKBytes ;                /**< Socket buffer size for send sockets                    */
        int    MaxPendingQueueKBytes ;                     /**< Memory allocated for pending packets                   */
        int    MaxStreamsPerTransmitter ;                  /**< Maximal number of streams                              */
        int    PacketsPerRound ;                           /**< Max number of packets sent in each round               */
        int    PacketsPerRoundWhenCleaning ;               /**< Packets per round when cleaning history                */
        int    RdataSendPercent ;                          /**< Percent of Repair to data to send                      */
        int    CleaningMarkPercent ;                       /**< Point at which history is cleaned                      */
        int    MinTrimSize ;                               /**< Minimal number of packets to trim                      */
        int    MaintenanceLoop ;                           /**< Maintenance loop cycle                                 */
        int    BatchingMode ;                              /**< Batching mode                                          */
        int    MinBatchingMicro ;                          /**< Minimal batch time                                     */
        int    MaxBatchingMicro ;                          /**< Maximal batch time                                     */
        int    BatchYield ;                                /**< Batch using yield                                      */
        int    StreamReportIntervalMilli ;                 /**< Interval between stream monitoring messages            */
        int    DefaultLogLevel ;                           /**< log level used for RUM direct logging                  */
        int    ReuseAddress ;                              /**< Apply reuse address to the receive server socket       */
        int    NumMAthreads ;                              /**< Number of MessageAnnouncer threads                     */
        int    RecvPacketSize ;                            /**< Packet size                                            */
        int    recvBuffsQsize ;                            /**< Deprecated. not in use                                 */
        int    nackQsize ;                                 /**< NAK queue size                                         */
        int    recvQsize ;                                 /**< Receive packets queue size                             */
        int    rsrvQsize ;                                 /**< Reserved packets queue size                            */
        int    fragQsize ;                                 /**< Fragments queue size                                   */
        int    BaseWaitMili ;                              /**< Timing parameter                                       */
        int    LongWaitMili ;                              /**< Timing parameter                                       */
        int    BaseWaitNano ;                              /**< Timing parameter                                       */
        int    LongWaitNano ;                              /**< Timing parameter                                       */
        int    NackGenerCycle ;                            /**< Interval between NAK generator invocations             */
        int    TaskTimerCycle ;                            /**< Interval between Task timer invocations                */
        int    MaxNacksPerCycle ;                          /**< limit on maximal number of NAKs per cycle              */
        int    MaxSqnPerNack ;                             /**< Maximal number of NAKs per packet                      */
        int    evntQsize ;                                 /**< Size of queue used for storing events                  */
        int    BindRetryTime ;                             /**< Time to attempt binding to receiver address and port   */
        int    SnapshotCycleMilli_rx ;                     /**< Receiver snapshots interval                            */
        int    PrintStreamInfo_rx ;                        /**< Record Receiver stream info in snapshots               */
        int    SnapshotCycleMilli_tx ;                     /**< Transmitter snapshots interval                         */
        int    PrintStreamInfo_tx ;                        /**< Record transmitter stream info in snapshots            */
        char   RxNetworkInterface[RUM_ADDR_STR_LEN] ;      /**< "None" or a valid IP address                           */
        char   TxNetworkInterface[RUM_ADDR_STR_LEN] ;      /**< "None" or a valid IP address                           */
        char   AdvanceConfigFile[RUM_FILE_NAME_LEN] ;      /**< "None" or a valid file name                            */
        char   HostInformation[RUM_MAX_QUEUE_NAME] ;       /**< Information about the host running RUM                 */
        char   OsInformation[RUM_MAX_QUEUE_NAME] ;         /**< Information about the OS used                          */
        char   RumVersion[RUM_API_VERSION_LEN] ;           /**< RUM version string                                     */
        int    ThreadPriority_tx ;                         /**< Priority set on RUM sending threads                    */
        int    ThreadPriority_rx ;                         /**< Priority set on RUM receiving threads                  */
        int    SupplementalPortsLength ;                   /**< Number of elements in SupplementalPorts array          */
   rum_uint64  ThreadAffinity_tx ;                         /**< Affinity set on RUM sending threads                    */
   rum_uint64  ThreadAffinity_rx ;                         /**< Affinity set on RUM receiving threads                  */
    rumPort   *SupplementalPorts ;                         /**< A pointer to an array of rumPort elements. Note that
                                                            *   the array is held within RUM instance and is valid
                                                            *   until rumStop is called. The application must not
                                                            *   change the content of this array                       */
        int    LatencyMonSampleRate_tx ;                   /**< Latency monitoring packet sample rate for Tx           */
        int    LatencyMonSampleRate_rx ;                   /**< Latency monitoring packet sample rate for Rx           */
        int    NumExthreads ;                              /**< Number of Extra receiver threads                       */
        int    MemoryAlertPctLow ;                         /**< Point at which memory alert on is reported             */
        int    MemoryAlertPctHigh ;                        /**< Point at which memory alert off is reported            */
        int    ThreadStackSize ;                           /**< Thread Stack Size for threads delivering messages      */
   const char *instanceName ;                              /**< Name of the rum instance                               */
        int    pad1 ; 
        char  *pad2 ; 
        char   pad[sizeof(void *)] ;                       /**< Reserved - Alignment pad                               */
        int    LimitPendingPackets ;                       /**< Indicates if Limit-Pending-Packets is used or not      */
        char   rsrv[196-4*sizeof(void *)] ;                /**< Reserved                                               */
}rumConfigStats ;


/*----    Latency Monitoring related data structures    ----*/

/** \brief define a type for histogram counter (array element) */
#define   rumHistEntry_t        unsigned int

/** RUM external clock callback function signature. Used for latency monitoring.
 *  A callback function to be called to obtain a time measurement using an application clock implementation.
 *  The time is returned as a double representing a value in seconds; for example a value of 1.234567890 means 1 second
 *  and 234567890 nanoseconds.
 *  The application may register a get_external_time() callback as part of the latency monitoring mechanism.
 *  If the callback is provided RUM will use this callback to obtain clock reads and calculate latencies.
 *  If the application does not provide a callback RUM will use its internal clock which is mapped to the OS
 *  high resolution clock. This is the recommended way to work unless the application has access to an external timing
 *  device that can provide high resolution time reads at low cost.
 *  The callback must be thread safe. It is recommended that the clock be monotonic for accurate latency measurements.
 *  It is important to note that this callback can be invoked with high frequency; hence, the executing the callback
 *  must be extremely fast (including when accessed by multiple threads) and consume minimal resources (CPU, Memory, etc).
 *  Since version 2.3, the userdata parameter has been added. Therefore, you need to update any external clock callback from 
 *  previous releases (2.2 and previous). 
 */
typedef double (*rum_get_external_time_t)(void *userdata) ;

/** \brief Latency Monitoring level */
typedef enum
{
   RUM_LATENCY_MON_DISABLED     = 0,    /**< Latency Monitoring is turned off                              */
   RUM_LATENCY_MON_LOW          = 1,    /**< Latency Monitoring is turned on with low resolution. Only
                                         *   queue sizes are monitored; latency is not measured. In this
                                         *   level there is minimal overhead associated with monitoring.   */
   RUM_LATENCY_MON_MEDIUM       = 2,    /**< Latency Monitoring is turned on with medium resolution. In
                                         *   addition to queue sizes basic latency is monitored. The
                                         *   information provided in this level includes the overall
                                         *   RUM internal latency in the transmitter and the receiver.     */
   RUM_LATENCY_MON_HIGH         = 3        /**< Latency Monitoring is turned on with high resolution. In
                                         *   addition the information provided in lower levels this
                                         *   level provides a breakdown of the internal RUM latency in
                                         *   the transmitter and the receiver.                             */
}RUM_LATENCY_MON_LEVEL_t ;

/**
 * \brief Latency Monitoring parameters used when calling rum<T|R>SetLatencyMonitoringParams.
 *
 * rumLatencyMonParams must be initialized (using rumInitStructureParameters() with id RUM_SID_LATENCYMONPARAMS) before it is first used.
 */
typedef struct
{
  int                         rum_length ;                    /**< For RUM usage, length of this structure                            */
  int                         rum_reserved ;                  /**< For RUM usage, must not be changed                                 */
  double                      latency_histogram_unit ;        /**< Time interval in seconds that each histogram entry represents.
                                                               *   For example a value of 1e-5 means that the histogram resolution is
                                                               *   ten microseconds, hence in this case the value in position P in
                                                               *   the histogram represents the number of latency measurements with
                                                               *   values in the range 10P to 10P+9 microseconds. Default 1e-6        */
  rum_get_external_time_t     get_time ;                      /**< Optional external time function. Set to NULL to use RUM internal
                                                               *   high resolution clock (recommended). Note: that an application may
                                                               *   have at most one external time function per instance. Also set this 
                                                               *   variable to NULL if the Coordinated Cluster Time is enabled. 
                                                               */
  RUM_LATENCY_MON_LEVEL_t     monitor_level ;                 /**< Required level of latency monitoring.
                                                               *   Default RUM_LATENCY_MON_DISABLED                                   */
  int                         latency_histogram_size ;        /**< Number of entries in the latency histogram array. A histogram is
                                                               *   represented as an array of integers where each array element counts
                                                               *   the number of latency measurements that fall in a certain range.
                                                               *   The values covered by a histogram are from zero to
                                                               *   (latency_histogram_unit * latency_histogram_size); larger values
                                                               *   will be counted as outliers.
                                                               *   For example, to cover the range of 0 to 200 milli with resolution
                                                               *   of 10 microseconds, set latency_histogram_unit=1e-5 and
                                                               *   latency_histogram_size=20000 (100 entries cover a millisecond).
                                                               *   Default 50000                                                      */
  int                         queue_histogram_size ;          /**< Number of entries in the queue histogram array. The unit for queue
                                                               *   histograms is always one. A queue histogram size can typically be
                                                               *   much smaller than a latency histogram size. Default 500            */
  int                         deliver_timestamp ;             /**< Instruct RUM to send a timestamp on the wire from the transmitter
                                                               *   to the receiver. This can be used to measure network latency
                                                               *   (provided that the two clocks are synchronized).
                                                               *   This flag is used only when the monitoring level is set to the
                                                               *   highest level. Default 0                                           */
  void *                    get_time_param;                     /**< The user data parameter for the optional external get_time callback*/
  char                      rsrv[64] ;                        /**< Reserved                                                           */
}rumLatencyMonParams ;

/**
 * \brief Structure to hold histogram information used for latency monitoring.
 * Histograms are used to record statistics of both Latency measurements and queue sizes.
 */
typedef struct
{
  double                      duration ;                      /**< Duration of the test in seconds                                    */
  double                      average ;                       /**< Average value for all measurements (including outliers)            */
  double                      max ;                           /**< Maximal value for all measurements                                 */
  double                      min ;                           /**< Minimal value for all measurements                                 */
  double                      p50 ;                           /**< 50th percentile (median) for values recorded in the histogram      */
  double                      p90 ;                           /**< 90th percentile for values recorded in the histogram               */
  double                      p95 ;                           /**< 95th percentile for values recorded in the histogram               */
  double                      p99 ;                           /**< 99th percentile for values recorded in the histogram               */
  double                      latency_histogram_unit ;        /**< Time interval in seconds that each histogram entry represents.
                                                               *   Same value that was given in rumLatencyMonParams when the
                                                               *   histogram was created                                              */
  rumHistEntry_t             *histogram ;                     /**< Input parameter: the histogram array itself. An array of integers
                                                               *   with a size of at least histogram_array_length. If histogram is set
                                                               *   to NULL then no histogram is provided; this can be used if the
                                                               *   application is only interested in the basic statistics that are
                                                               *   provided. When provided, the histogram array should be allocated
                                                               *   by the application                                                 */
  rumHistEntry_t              number_of_measurements ;        /**< Total number of measurements recorded                              */
  rumHistEntry_t              number_of_outliers ;            /**< Number of measurements that fall outside the histogram range       */
  rumHistEntry_t              number_of_packets ;             /**< Total number of packet out of which number_of_measurements were
                                                               *   sampled                                                            */
  int                         histogram_array_length ;        /**< Input parameter: Number of elements in the histogram array         */
  int                         histogram_size ;                /**< Number of actual elements recorded in the histogram                */
  int                         reset_values ;                  /**< A flag (0/1) indicating that the application wishes to reset the
                                                               *   latency information recorded in this histogram so far and start
                                                               *   building a new set of measurements                                 */
  char                        pad[sizeof(void *)] ;           /**< Reserved - Alignment pad                                           */
  char                        rsrv[64] ;                      /**< Reserved                                                           */
}rumHistogram ;



/*****************************************************/
/*-----  RUM C API Functions ---------*/
/*****************************************************/

/*---- General API Functions    ----*/

/**
 * Returns a string representing the version of the RUM implementation.
 * @param version  Buffer of at least 64 bytes where the version should be copied (output).
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumGetVersion(char *version, int *rc);

/**
 * Returns a description to a failure reason/return code which was set by an API call that returned
 * RUM_FAILURE.
 * @param error_code the error code to describe (input)
 * @param description the description of the error code as a NULL terminated string. Must be allocated by the application. (output)
 * @param max_length maximal number of bytes to copy into description (input)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumGetErrorDescription(int error_code, char* description, int max_length);

/**
 * Creates an RUM instance. The function initializes a new RUM stack.
 * Reads configuration parameters from rum_config structure. The application
 * should provide an implementation of the on_log_event() callback to handle log events.
 * The user parameter is used to identify the callback function and can be NULL.
 * On success the function creates a new RUM handler (unique non-negative integer) in the rum_instance structure.
 * @param rum_instance Pointer to structure where instance handle will be created (output)
 * @param rum_config Structure containing RUM configuration values (input)
 * @param on_log_event Implementation of log callback (input)
 * @param log_user  User data structure that will be passed to log event callback functions (input)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumInit(rumInstance *rum_instance, const rumConfig *rum_config, rum_on_log_event_t on_log_event, void *log_user, int *rc);


/**
 * Stops the specified RUM instance in a controlled fashion. Other instances (if any) are not affected.
 * Announces the stop to receivers, waits for linger_time_milli milliseconds to let transmitter send pending
 * packets if any, and receivers complete the reception, then stops the transmitter and frees its resources.
 * Important: the application must NOT call rumStop from an RUM thread, i.e., from one of the RUM callbacks.
 * @param rum_instance  The initialized RUM instance to stop (input)
 * @param linger_time_milli The time to allow pending data and repairs to be sent (input)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int rumStop(rumInstance *rum_instance, int linger_time_milli, int *rc);


/**
 * Requests RUM to establish a new connection.
 * The connection is created in an asynchronous fashion. <br>
 * The rumEstablishConnectionParams structure provided to theis API includes an on_connection_event() callback which is used to get
 * events related to this connection both during establishment time and later (if the connection has been
 * established successfully). The callback may return one of the following values: <ul>
 *  <li> RUM_ON_CONNECTION_REJECT - indicates the connection should be rejected.
 *  <li> RUM_ON_CONNECTION_ACCEPT - indicates the connection should be accepted.
 *  <li> RUM_ON_CONNECTION_NULL - indicates no action should be taken. </ul>
 * connect_params provides information about the connection including the destination address + port and the opening method.
 * Currently the only supported method is RUM_CREATE_NEW which means that a new connection will be created even if a
 * connection to the destination already exist.
 * @param rum_instance  An initialized RUM instance (input)
 * @param connect_params connection parameters (input)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
*/
RUMAPI_DLL int  rumEstablishConnection(const rumInstance *rum_instance, const rumEstablishConnectionParams *connect_params, int *rc);

/**
 * Closes the specified RUM connection.
 * All streams (inbound and outbound) that use this connection will be terminated.
 * @param rum_connection  The connection to close (input/output)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
*/
RUMAPI_DLL int  rumCloseConnection(rumConnection *rum_connection, int *rc);

/**
 * Adds a listener for incoming connections events.
 * When a new connection is received by RUM it will call the connection listeners one by one until the first listener returns
 * RUM_ON_CONNECTION_ACCEPT; all events related to the connection will be delivered to (and only to) the listener that accepted
 * the connection. RUM may call the connection listeners in any order.
 * @param rum_instance  An initialized RUM instance (input)
 * @param  on_conn_event a callback to handle connection events on inbound connections; the return values of the callback
 *   are as in rumEstablishConnection().
 * @param event_user used to identify the callback function and can be NULL. The user pointer is used when removing the listener.
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumAddConnectionListener(const rumInstance *rum_instance, rum_on_connection_event_t on_conn_event, void *event_user, int *rc);

/**
 * Removes a listener for incoming connections events that was added using rumAddConnectionListener().
 * The user pointer identifies the connection listener.
 * Existing connections that were received using this connection listener are not affected.
 * @param rum_instance  An initialized RUM instance (input)
 * @param event_user used to identify the connection listener.
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumRemoveConnectionListener(const rumInstance *rum_instance, void *event_user, int *rc);

/**
 * Changes the log level of all RUM instances.
 * @param new_log_level New log level (input)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumChangeLogLevel(int new_log_level, int *rc);


/**
 * Initialize a structure that is passed by the application to RUM in one of the APIs.
 * Will set appropriate initial values in the structure. The application then only needs to change the fields it is interested in.
 * @param structure_id  Identifier of the structure (input)
 * @param structure  Structure to be initialized (output)
 * @param api_version  The RUMCAPI_VERSION that is used by the application for this structure (input)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int rumInitStructureParameters(RUM_STRUCTURE_ID_t structure_id, void *structure, int api_version, int *rc);


/* Statistics related Functions */


/*----    TRANSMITTER functions    ----*/

/**
 * Creates a new RUM transmitter Queue on a specified RUM instance.
 * A transmitter queue can be created only on an active connection.
 * At most 128 transmitter queues can be created on a single connection.
 * @param rum_instance  An initialized RUM instance structure from rumInit() (input)
 * @param queue_params Queue configuration parameters (input)
 * @param queue_t  Pointer to a Queue transmitter structure where the Queue and its RUM instance will be placed (output)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumTCreateQueue(const rumInstance *rum_instance, rumTxQueueParameters *queue_params, rumQueueT *queue_t, int *rc);

/**
 * Soft shutdown of queue transmission.
 * Only the given queue_t is closed; other queues (if any) are not affected.
 * The receivers are automatically informed about stream closing. Other transmitting threads using this queue_t
 * instance will not be able to submit new messages once it is closed.
 * It is important to note that when the call to rumTCloseQueue() returns
 * the only meaning is that no more messages can be submitted on the queue. The queue, however,
 * remains valid for another linger_time_milli milliseconds. When the timeout expires the queue is
 * completely removed and its resources are freed.
 * @param queue_t A Queue transmitter structure to be closed (input)
 * @param linger_time_milli The time to allow pending data and repairs to be sent (input)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumTCloseQueue(rumQueueT *queue_t, int linger_time_milli, int *rc);

/**
 * Submit a message for transmission on a queue_t transmitter.
 * This API cannot be used with Turbo Flow queues.
 * @param queue_t An initialized Queue transmitter structure (input)
 * @param msg Pointer to the message data (input)
 * @param msg_len  Message length in bytes (must be > 0) (input)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumTSubmitMsg(const rumQueueT *queue_t, const char *msg,  int msg_len, int *rc);

/**
 * Submit a message for transmission on a queue_t transmitter.
 * @param queue_t An initialized Queue transmitter structure (input)
 * @param msg_params Message information and options (input)
 * @param rc Pointer to where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumTSubmitMessage(const rumQueueT *queue_t, const rumTxMessage *msg_params, int *rc);


/* HA Tx Functions */

/**
 * Get the stream ID associated with the queue_t.
 * @param queue_t  An initialized Queue transmitter structure (input)
 * @param stream_id  holds the stream id on return (output)
 * @param rc  Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumTGetStreamID(const rumQueueT *queue_t, rumStreamID_t *stream_id, int *rc);


/*----    RECEIVER functions    ----*/

/**
 * Creates a Queue receiver on the specified RUM instance accepting all streams that match the queue's
 * specifications.
 * Parameters in queue_params should include the following:
 *
 * accept_stream - If not NULL creates a StreamSet receiver, accepting all streams that match the rules defined in the
 * accept_stream callback. The accept_stream callback should return RUM_STREAM_REJECT (0) if the stream
 * should not be accepted. If the stream should be accepted the accept_stream callback should return RUM_STREAM_ACCEPT (1).
 * In case of multiple StreamSet receivers, in the same RUM instance, with overlapping accept_stream rules,
 * a stream will be assigned to the first matching StreamSet.
 * The accept_user parameter is used to associate the queue with the called function context.
 *
 * queue_name - If accept_stream is NULL the queue_name is used as the matching rule for accepting streams.
 * All the streams with the queue_name will be accepted by the queue. The function will return an error if
 * called twice for the same queue name on the same RUM instance.
 * Note that a single Queue receiver can receive any number of streams; for example, if 20 machines
 * create Queue transmitters with a name "ibm", the Queue receiver with the queue_name "ibm" will
 * accept messages from all of them.

 * on_event -  Registers a callback function to be called on events related to the queue_r receiver.
 * The on_event() callback function needs to be implemented by the calling application.
 * The event structure should be considered as read-only; the application must not change any of its fields.
 * The event_user parameter is used to associate the queue with the called function context.
 *
 * on_message - Registers a callback function to be called for every message received on the queue_r receiver.
 * The on_message() callback function needs to be implemented by the calling application.
 * The msg structure should be considered as read-only; the application must not change any of its fields.
 * Memory is freed after the on_message() function returns.
 * The user parameter is used to associate the Queue (or StreamSet) receiver with the called function context.

 * @param rum_instance An initialized RUM instance (input)
 * @param queue_params  Queue parameters including callbacks and queue options. (input).
 * @param queue_r  On success the function creates a new Queue handler (identifying the Queue and its RUM instance) in the queue_r structure (output)
 * @param rc Pointer to integer where failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumRCreateQueue(const rumInstance *rum_instance, const rumRxQueueParameters *queue_params, rumQueueR *queue_r, int *rc);

/**
 * Closes the queue_r receiver. After this function returns with RUM_SUCCESS data corresponding
 * to this Queue or StreamSet will not be accepted and delivered.  If the application is busy processing
 * messages of the queue_r the function will return RUM_BUSY indicating that the queue_r cannot be
 * completely removed at this point. In this case RUM marks the queue_r as closed and will not
 * accept new data on it. The application should try to call this function again to remove the queue_r.
 * @param queue_r  An initialized queue receiver structure.
 * @param rc A pointer to an integer where the failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success (queue_r removed), RUM_BUSY if the queue_r could not be removed due to messages
 * (of queue_r) being processed by the application, or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumRCloseQueue(rumQueueR *queue_r, int *rc);


/**
 * Instruct the RUM receiver to reject the stream with ID stream_id.
 * RUM will ensure that no messages are delivered on the stream once the application returns from this call.
 * @param rum_instance An initialized RUM instance (input)
 * @param stream_id Stream identifier (input)
 * @param rc A pointer to an integer where the failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumRRemoveStream(const rumInstance *rum_instance, rumStreamID_t stream_id, int *rc);

/**
 * Instruct the RUM receiver to clear the list of rejected streams.
 * @param rum_instance An initialized RUM instance (input)
 * @param rc A pointer to an integer where the failure-specific error code will be copied (output)
 * @return RUM_SUCCESS on success or RUM_FAILURE on failure.
 */
RUMAPI_DLL int  rumRClearRejectedStreams(const rumInstance *rum_instance, int *rc);

/* HA Rx Functions */


/* Turbo Flow Rx Functions */





/*-----  RUM-API ---------*/


#   ifdef __cplusplus
    }
#   endif


#endif

