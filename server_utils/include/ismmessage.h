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

/*********************************************************************/
/*                                                                   */
/* Module Name: ismmessage.h                                         */
/*                                                                   */
/* Description: ISM message header file                              */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_MESSAGE_DEFINED
#define __ISM_MESSAGE_DEFINED

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************/
/* Message Header                                                    */
/*********************************************************************/
typedef struct
{
    uint64_t OrderId;         ///< Order on queue - monotonically increasing
    uint8_t  Persistence;     ///< Persistence - ismMESSAGE_PERSISTENCE_*
    uint8_t  Reliability;     ///< Reliability of delivery - ismMESSAGE_RELIABILITY_*
    uint8_t  Priority;        ///< Priority - ismMESASGE_PRIORITY_*
    uint8_t  RedeliveryCount; ///< Redelivery counts - up to 255 attempts
    uint32_t Expiry;          ///< Expiry time - seconds since 2000-01-01T00
    uint8_t  Flags;           ///< Flags, such as retained - ismMESSAGE_FLAGS_*
    uint8_t  MessageType;     ///< Message type - MTYPE_*
} ismMessageHeader_t;


//
// Message persistence
//
#define ismMESSAGE_PERSISTENCE_NONPERSISTENT 0
#define ismMESSAGE_PERSISTENCE_PERSISTENT    1


//
// Message reliability
//
#define ismMESSAGE_RELIABILITY_AT_MOST_ONCE  0
#define ismMESSAGE_RELIABILITY_AT_LEAST_ONCE 1
#define ismMESSAGE_RELIABILITY_EXACTLY_ONCE  2

// Helpful reliability constants corresponding to the JMS delivery modes
#define ismMESSAGE_RELIABILITY_JMS_NONPERSISTENT ismMESSAGE_RELIABILITY_AT_MOST_ONCE
#define ismMESSAGE_RELIABILITY_JMS_PERSISTENT    ismMESSAGE_RELIABILITY_EXACTLY_ONCE


//
// Message priority
//
#define ismMESSAGE_PRIORITY_MINIMUM          0
#define ismMESSAGE_PRIORITY_DEFAULT          4
#define ismMESSAGE_PRIORITY_MAXIMUM          9

// Helpful priority constant corresponding to the JMS default priority
#define ismMESSAGE_PRIORITY_JMS_DEFAULT      4

// Helpful priority constant corresponding to the MQI default priority
#define ismMESSAGE_PRIORITY_MQI_DEFAULT      0


//
// Message flags
//
#define ismMESSAGE_FLAGS_NONE                 0

// Message published from a retained source
#define ismMESSAGE_FLAGS_RETAINED             1

// Message delivered to the server over a secure connection
#define ismMESSAGE_FLAGS_SECURE_DELIVERY      2

// Publisher requested message retention
#define ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN 4

// Consumer should propagate the retain flag
#define ismMESSAGE_FLAGS_PROPAGATE_RETAINED   8

//
// Message types
//
// The message types are defined globally to allow mapping as required.
// Any payload mapping is normally done in the protocol during write.
//
#ifndef MSGTYPE_DEFINED
#define MSGTYPE_DEFINED
enum msgtype_e {
    MTYPE_JMS             = 0x00,   // ISM JMS message range, unused value
    MTYPE_Message         = 0x01,   // ISM JMS message with no payload
    MTYPE_BytesMessage    = 0x02,   // ISM JMS BytesMessage
    MTYPE_MapMessage      = 0x03,   // ISM JMS MapMessage
    MTYPE_ObjectMessage   = 0x04,   // ISM JMS ObjectMessage
    MTYPE_StreamMessage   = 0x05,   // ISM JMS StreamMessage
    MTYPE_TextMessage     = 0x06,   // ISM JMS TextMessage
    MTYPE_TextMessageNull = 0x07,   // ISM JMS TextMessage with content not set
    MTYPE_MQTT            = 0x10,   // MQTT message range, and message with no payload
    MTYPE_MQTT_Binary     = 0x11,   // MQTT binary payload
    MTYPE_MQTT_Text       = 0x12,   // MQTT text string payload
    MTYPE_MQTT_Map        = 0x13,   // MQTTv5 map message
    MTYPE_MQTT_TextObject = 0x14,   // MQTT JSON text map
    MTYPE_MQTT_TextArray  = 0x15,   // MQTT JSON text stream
    MTYPE_NullRetained    = 0x20    // A NULL retained message
};
#endif


//
// Default value for the message header
//
#define ismMESSAGE_HEADER_DEFAULT         \
  {                                       \
    0,                                    \
    ismMESSAGE_PERSISTENCE_NONPERSISTENT, \
    ismMESSAGE_RELIABILITY_AT_MOST_ONCE,  \
    ismMESSAGE_PRIORITY_DEFAULT,          \
    0,                                    \
    0,                                    \
    ismMESSAGE_FLAGS_NONE,                \
    MTYPE_JMS,                            \
  }



/*********************************************************************/
/* Message areas                                                     */
/*                                                                   */
/* The areas are as follows:                                         */
/*   Properties        - System and user-defined properties          */
/*                       May be used in message selection.           */
/*   Payload           - Message body                                */
/*********************************************************************/
typedef enum
{
    ismMESSAGE_AREA_PROPERTIES = 1,
    ismMESSAGE_AREA_PAYLOAD = 2,
    ismMESSAGE_AREA_INTERNAL_HEADER = 3,
    ismMESSAGE_AREA_COUNT = 3
} ismMessageAreaType_t;

#ifdef __cplusplus
}
#endif

#endif /* __ISM_MESSAGE_DEFINED */

/*********************************************************************/
/* End of ismmessage.h                                               */
/*********************************************************************/
