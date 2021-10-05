/*
 * Copyright (c) 2009-2021 Contributors to the Eclipse Foundation
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


/** \file llmCommonApi.h
 *  \brief A common interface for RMM and RUM components.
 *
 */

#ifndef LLM_COMMON_API_H
#define LLM_COMMON_API_H
#include <stdarg.h>

#   ifdef __cplusplus
    extern "C" {
#   endif


#include <inttypes.h>


#ifndef LLM_API
    #ifdef _WIN32
        #define LLM_API extern __declspec(dllexport)
    #else
        #define LLM_API extern
    #endif
#endif

/** \brief Indicates a successful operation           */
#define  LLM_SUCCESS               0
/** \brief Indicates a failed operation               */
#define  LLM_FAILURE              -1
/** \brief Indicates an operation that currently cannot be completed, the caller should try again */
#define  LLM_BUSY                 -11

/** \brief maximal length of IP address string including NULL terminator                           */
#define LLM_ADDR_STR_LEN               64

/** \brief maximal length of host name string including NULL terminator                            */
#define LLM_HOSTNAME_STR_LEN           256


 /** \brief Log Levels     */
typedef enum LLM_LOGLEV {
    LLM_LOGLEV_NONE   = 0,     /**<  N = none            */
    LLM_LOGLEV_STATUS = 1,     /**< S = Status           */
    LLM_LOGLEV_FATAL  = 2,     /**< F = Fatal            */
    LLM_LOGLEV_ERROR  = 3,     /**< E = Error            */
    LLM_LOGLEV_WARN   = 4,     /**< W = Warning          */
    LLM_LOGLEV_INFO   = 5,     /**< I = Informational    */
    LLM_LOGLEV_EVENT  = 6,     /**< V = eVent            */
    LLM_LOGLEV_DEBUG  = 7,     /**< D = Debug            */
    LLM_LOGLEV_TRACE  = 8,     /**< T = Trace            */
    LLM_LOGLEV_XTRACE = 9      /**< X = eXtended trace   */
} LLM_LOGLEV_t;

 /** \brief Log event information
  *
  * This structure contains information which is passed to the application for each
  * log entry.
  */
typedef struct llmLogEvent_t
{
    int             msg_key ;                /**< Message key                        */
    int             log_level ;              /**< Log level of msg (enum LLM_LOGLEV) */
    void            **event_params ;         /**< List of event parameters           */
    int             nparams ;                /**< Number of event parameters         */
    char            *module ;                /**< Originating component              */
    uintptr_t       threadId;                /**< Originating thread id              */
    uint64_t        timestamp;               /**< Timestamp when event was created   */
    const char      *instanceName;           /**< Name of the logging instance       */
    char            pad[sizeof(int)] ;       /**< Reserved - Alignment pad           */
    char            rsrv[32-sizeof(uintptr_t)-sizeof(uint64_t)-sizeof(char*)] ; /**< Reserved                   */
} llmLogEvent_t;

/** \brief Log event callback
 *
 * This function is called to deliver a log event to the application.
 * @param log_event  A log event structure
 * @param user       User data sent to the callback
 */
typedef void (*llm_on_log_event_t) (const llmLogEvent_t *log_event, void *user) ;

 /** \brief IPVersion      */
typedef enum
{
   LLM_IPver_IPv4             = 1,    /**< Communicate using IPv4 only                                                       */
   LLM_IPver_IPv6             = 2,    /**< Communicate using IPv6 only                                                       */
   LLM_IPver_ANY              = 3,    /**< Can communicate using IPv4, IPv6 or both depending on which protocol is available */
   LLM_IPver_BOTH             = 4     /**< Must support communication using both IPv4 and IPv6.                              */
}LLM_IPVERSION_t ;

/** \brief Topic Mapping resolution Level */
typedef enum
{
   LLMCM_TMAP_DISABLED     = 0,       /**< Topic mapping is turned off                                 */
   LLMCM_TMAP_BASIC        = 1,       /**< Topic mapping is turned on with basic resolution.
                                       *   Only destination fields are mapped.                         */
   LLMCM_TMAP_FULL         = 2        /**< Topic mapping is turned on with resolution of all topic fields. */
}LLMCM_TMAP_LEVEL_t ;

/**
 * \brief LLM Coordination Manager (CM) destination address
 *
 * llmCMAddress must be initialized (using rmmInitStructureParameters() with id RMM_SID_LLMCM_ADDRESS)
 * before it is first used
 */
typedef struct llmCMAddress
{
   int                  llm_length ;                            /**< For LLM usage, length of this structure                     */
   int                  llm_reserved ;                          /**< For LLM usage, must not be changed                          */
   char                 ip_address[LLM_HOSTNAME_STR_LEN] ;      /**< Destination IP address or host name                         */
   int                  port_number ;                           /**< Destination port number                                     */
   char                 pad[sizeof(int)] ;                      /**< Reserved - Alignment pad                                    */
   char                 rsrv[256] ;                             /**< Reserved                                                    */
}llmCMAddress ;

/**
 * \brief LLM Coordination Manager (CM) receiver port information
 *
 * llmCMPort must be initialized (using rmmInitStructureParameters() with id RMM_SID_LLMCM_PORT)
 * before it is first used
 */
typedef struct llmCMPort
{
   int                  llm_length ;                            /**< For LLM usage, length of this structure                     */
   int                  llm_reserved ;                          /**< For LLM usage, must not be changed                          */
   int                  port_number ;                           /**< The number of the port to listen to. If the port_number
                                                                 *   is set to zero, a port number will be dynamically chosen
                                                                 *   from the range given by port_range_low to port_range_high.
                                                                 *   Range 0 - 65535. Default 0.                                 */
   int                  port_range_low ;                        /**< Low boundary of range used for dynamically assigned port.
                                                                 *   Used only if port_number is set to zero. Range 0 - 65535.
                                                                 *   Default 0                                                   */
   int                  port_range_high ;                       /**< Upper boundary of range used for dynamically assigned port.
                                                                 *   Used only if port_number is set to zero. If both port_number
                                                                 *   and port_range_high are zero then LLMCM will assign a dynamic
                                                                 *   port from a default range. Range 0 - 65535.
                                                                 *   Default 0                                                   */
   int                  assigned_port ;                         /**< output parameter - the assigned port number.
                                                                 *   Default 0                                                   */
   char                 rsrv[128] ;                             /**< Reserved                                                    */
}llmCMPort ;

/**
 * \brief LLM Coordination Manager parameters.
 *
 * llmCMParameters must be initialized (using rmmInitStructureParameters() with id RMM_SID_LLMCM_PARAMETERS)
 * before it is first used
 */
typedef struct llmCMParameters
{
   int                    llm_length ;                                /**< For LLM usage, length of this structure                     */
   int                    llm_reserved ;                              /**< For LLM usage, must not be changed                          */
   LLM_IPVERSION_t        ip_version ;                                /**< The IP (Internet Protocol) version to use.
                                                                       *   Default value LLM_IPver_IPv4                                */
   int                    action_timeout_milli ;                      /**< Timeout in millisecond to wait for an action, such as
                                                                       *   resolve or remove a topic mapping. If the action is not
                                                                       *   completed within the given timeout the action fails.
                                                                       *   A value of zero or less indicates infinite timeout.
                                                                       *   Small values (below 50) may not be fully enforced.
                                                                       *   Default value 2000                                          */
   int                    heartbeat_timeout_milli ;                   /**< Heartbeat timeout in milliseconds. The minimum allowed
                                                                       *   value is 100. Default value 30000                          */
   int                    heartbeat_interval_milli ;                  /**< Heartbeat interval in milliseconds. The minimum allowed
                                                                       *   value is 25. Default value 5000                            */
   char                   application_id[LLM_ADDR_STR_LEN] ;          /**< Application identifier as a NULL terminated string. Can be
                                                                       *   used for tagging to identify the process or the application.
                                                                       *   Default empty string                                        */
   char                   client_id[LLM_ADDR_STR_LEN] ;               /**< Client or user identifier as a NULL terminated string.
                                                                       *   Can be used to identify the LLM-CM client instance.
                                                                       *   Default empty string                                        */
   char                   local_unicast_addr[LLM_HOSTNAME_STR_LEN] ;  /**< A unicast address or host name to be used as the unicast
                                                                       *   address that the LLM-CM client reports to the LLM-CM server.
                                                                       *   If no address is provided (default) the client will
                                                                       *   automatically select one of the addresses used by the LLM
                                                                       *   receiver. This field is used only for receiver instances.
                                                                       *   The address should be provided as a NULL terminated string.
                                                                       *   Default empty string (no address).                          */
   llmCMAddress          *llmcm_addrs ;                               /**< A pointer to an ordered list of coordination manager
                                                                       *   destinations. The first entry in the list indicates the
                                                                       *   primary manager, the second entry indicates the secondary
                                                                       *   manager, and so on. Default NULL                            */
   int                    llmcm_addrs_length ;                        /*   Number of destinations in the list. Default 0               */
   char                   pad[sizeof(int)] ;                          /**< Reserved - Alignment pad                                    */
   llmCMPort             *local_unicast_port ;                        /**< Pointer to an LLMCM port which is used by the client to
                                                                       *   receive data from the LLM-CM server.
                                                                       *   Default value is NULL indicating that the port number
                                                                       *   will be dynamically chosen by LLM from the default range.   */
   char                   rsrv[256-sizeof(void*)];                    /**< Reserved                                                    */
}llmCMParameters ;


/**
 * Message store location structure.
 *
 * This structure is filled in by APIs which find the location of a message store.
 */
typedef struct llmStoreLocation_t {
    int    llm_length;          /**< For LLM usage, length of this structure                 */
    int    llm_reserved;        /**< Reserved for use by LLM                           */
    char   policy[256];         /**< The name of the policy within the message store   */
    char   domain[256];         /**< The domain of the policy within the message store */
    char   location[4096];      /**< The location string of the message store          */
} llmStoreLocation_t;


/**
 * The type of start position for a message store retrieve.
 *
 * When a message or packet sequence number is used, the stream ID is also required.
 *
 * The start timestamp is always used, but can be any time if the sequence number range
 * does not repeat over the life of the store.
 *
 */
typedef enum {
    LLM_PTYPE_TIMESTAMP       = 0x00,  /**< Timestamp only */
    LLM_PTYPE_MESSAGESQN      = 0x01,  /**< Message sequence number with timestamp and SID */
    LLM_PTYPE_POLICYSQN       = 0x02,  /**< Policy sequence number with timestamp          */
    LLM_PTYPE_PACKETSQN       = 0x03,  /**< Packet sequence number with timestamp and SID */
    LLM_PTYPE_MESSAGESQN_HA   = 0x04   /**< Message sequence number with timestamp (SID is ignored) */
} llmPosType_t;


/**
 * Specify a position for a message store retrieve.
 */
typedef struct llmPosition_t {
    llmPosType_t   start_position;  /**< Position type                           */
    int            reserved;        /**< Must be zero                            */
    int64_t        starttime;       /**< The LLMI start timestamp (nanoseconds from 1970, etc.)               */
    int64_t        endtime;         /**< The LLMI end timestamp (nanoseconds from 1970, etc.)                 */
    uint64_t       stream_id;       /**< The streamID of the sequence number (ignored if start_position is not LLM_PTYPE_MESSAGESQN or LLM_PTYPE_PACKETSQN)    */
    uint64_t       sequence;        /**< The sequence number:
                                            Packet sequence number for LLM_PTYPE_PACKETSQN
                                            Message sequence number for  LLM_PTYPE_MESSAGESQN
                                            Policy sequence number for  LLM_PTYPE_POLICYSQN
                                    */
    char           resv[24];        /**< Reserved                               */
} llmPosition_t;

/**
 * The property kind gives the interpretation of a message property value.
 *
 * This is separate from the type which specifies how the value is stored.
 * The kind can be used to keep a higher level of type information.
 *
 * A message property of an undefined kind cannot be used, but all undefined and
 * user kinds should be treated the same as unspecified since additional kinds may
 * be added in the future.  A message property value is not required to conform
 * to the limitations of the kind.  The user kinds are to allow consenting
 * applications to pass additional types, and are treated the same as unspecified
 * by LLM.
 *
 * When LLM uses a property value for filtering or converts it for processing
 * it uses the kind to indicate how to interpret the value.  Applications
 * using LLM can also use this information when converting LLM message property
 * values.
 */
typedef enum LLM_PROP_KINDS {
    LLM_PROP_KIND_Unspecified  = 0,   /**< The type of the property is used (default) */
    LLM_PROP_KIND_ByteArray    = 1,   /**< The bytes should not be interpreted as characters */
    LLM_PROP_KIND_String       = 2,   /**< The bytes are characters */
    LLM_PROP_KIND_Boolean      = 3,   /**< A boolean */
    LLM_PROP_KIND_NoValue      = 4,   /**< The value can be ignored */
    LLM_PROP_KIND_Int8         = 5,   /**< A signed 8 bit integer */
    LLM_PROP_KIND_Int16        = 6,   /**< A signed 16 bit integer */
    LLM_PROP_KIND_Int32        = 7,   /**< A signed 32 bit integer */
    LLM_PROP_KIND_Int64        = 8,   /**< A signed 64 bit integer */
    LLM_PROP_KIND_Float32      = 9,   /**< A 32 bit IEEE float value */
    LLM_PROP_KIND_Float64      = 10,  /**< A 64 bit IEEE float value */
    LLM_PROP_KIND_Timestamp    = 11,  /**< A timestamp in nanoseconds */
    LLM_PROP_KIND_User1        = 12,  /**< A user defined type */
    LLM_PROP_KIND_User2        = 13,  /**< A user defined type */
    LLM_PROP_KIND_User3        = 14,  /**< A user defined type */
    LLM_PROP_KIND_User4        = 15   /**< A user defined type */
} LLM_PROP_KINDS_t;


#define LLM_PROP_KIND_MAX 15


/**
 * Negative message property codes are reserved for assigned usage and must
 * not be used except as assigned.  Positive message property codes can be
 * used by any application.
 */
#define LLM_PROP_COMMON_MAX         -100    /**< Start of common message property codes       */
#define LLM_PROP_ALT_MSN            -100    /**< Alternate message sequence number            */
#define LLM_PROP_JMS_TYPE           -101    /**< JMS Message Type                             */
#define LLM_PROP_JMS_CORRELATION_ID -102    /**< JMS Correlation ID                           */
#define LLM_PROP_JMS_DELIVERY_MODE  -103    /**< JMS Delivery mode                            */
#define LLM_PROP_JMS_REDELIVERED    -104    /**< JMS Redelivered                              */
#define LLM_PROP_JMS_EXPIRATION     -105    /**< JMS Expiration time                          */
#define LLM_PROP_JMS_PRIORITY       -106    /**< JMS Priority                                 */
#define LLM_PROP_JMS_REPLAYTO       -107    /**< JMS ReplayTo                                 */
#define LLM_PROP_JMS_TIMESTAMP      -108    /**< JMS Timestamp                                */
#define LLM_PROP_SRC_STREAM_ID      -109    /**< Source stream ID                             */
#define LLM_PROP_SRC_IP_ADDRESS     -110    /**< Source IP address                            */
#define LLM_PROP_SRC_MSN            -111    /**< Message sequence number set by transmitter   */
#define LLM_PROP_SRC_MSG_SUBMIT_TS  -112    /**< Message submit time (i.e. sender_time_stamp) */
#define LLM_PROP_COMMON_MIN         -149    /**< End of common message property codes         */

#define LLM_PROP_MQ_MAX            -1000    /**< Start of MQ reserved message property codes  */
#define LLM_PROP_MQ_MIN            -1499    /**< End of MQ reserved message property codes    */


/*
 * Convert a 64 bit integer stream ID to a string stream ID.
 *
 * The 64 bit integer stream ID is interpreted as big-endian regardless of the
 * endian of the system.
 *
 * @param sid  The 64 bit integer form of the stream ID
 * @param buf  The buffer to return the stream ID
 * @param len  The length of the buffer.  This must be at least 17 bytes to include the stream ID and the trailing null.
 * @return The address of the string stream ID, or NULL to indicate an error.
 */
LLM_API char * llm_formatStreamID(uint64_t sid, char * buf, int len);

/**
 * Convert a string stream ID to a 64 bit integer stream ID.
 *
 * The string form of the stream ID consists of 16 hex characters.
 *
 * @param str   The string form the the stream ID
 * @param sidp  The location to return the 64 bit integer form of the stream ID
 * @return A return code, 0=good, 1=string not a valid stream ID
 */
LLM_API int llm_parseStreamID(const char *str, uint64_t * sidp);

#   ifdef __cplusplus
    }
#   endif

#endif
