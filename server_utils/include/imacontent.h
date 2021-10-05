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

#ifndef __CONTENT_DEFINED
#define __CONTENT_DEFINED

#include <ismutil.h>

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#include <ismjson.h>

/**
 * @file imacontent.h Content encoding and decoding for the ISM JMS content
 *
 * The content encoding is used by ISM JMS for encoding message content, properties,
 * and actions.  It is a concise representation which encodes together a type and length.
 * While using much of the encoding space it is also designed to have enough
 * redundancy to detect errors.
 *
 * Each value is encoded starting with a type byte which can also contain the length
 * or value.  For small names and strings, the type and length are encoded in this byte.
 * for boolean and small byte values the type and value are encoded together into this
 * byte.
 *
 * A map or properties consists of alternating names and values.  The names themselves
 * have type which is either a name (string) or ID (integer).  The values can be
 * of types boolean, byte, bytearray, char, double, float, int, long, map, short,
 * string, time, ubyte, uint, ulong, user, or ushort.  There is an encoding for array
 * but it is not currently used.  Note that Java does not use some of these types so
 * they are mapped to the nearest equivalent.

 */

/*
 * Defined numeric property identifiers
 *
 * An ID must be in the range 0 to 16777216.  The values 0 to 255 are reserved for common
 * use and other IDs can be used privately.
 * <p>
 * In the native protocols of MessageSight, MQTT throws away all properties but uses the
 * ID_Topic to return the name of the originating topic.  JMS uses several of the common
 * IDs to hold JMS system properties and discards the others when presented to the JMS
 * client application.
 */
enum CommonID_e {
    ID_Timestamp    =  2,    /* The creation timestamp of the message (milliseconds in the Unix epoch) */
    ID_Expire       =  3,    /* The expiration timestamp of the message (milliseconds in the Unix eposh) */
    ID_MsgID        =  4,    /* The message ID (commonly system generated) */
    ID_CorrID       =  5,    /* The correlation ID */
    ID_JMSType      =  6,    /* The type string */
    ID_ReplyToQ     =  7,    /* The reply to queue */
    ID_ReplyToT     =  8,    /* The reply to topic */
    ID_Topic        =  9,    /* The topic name (this is needed if the protocol allows wildcard topics) */
    ID_DeliveryTime = 10,    /* To time before which the message should not be delivered */
    ID_Queue        = 11,    /* The queue name  */
    ID_UserID       = 12,    /* The user ID of the originator of the message */
    ID_ClientID     = 13,    /* The client ID of the originator of the message */
    ID_Domain       = 14,    /* The domain of the client ID */
    ID_Token        = 15,    /* A security token authenticating this message */
    ID_DeviceID     = 16,    /* The device identifier of the originator */
    ID_AppID        = 17,    /* The application identifier of the originator */
    ID_Protocol     = 18,    /* The protocol which originated the message */
    ID_ObjectID     = 19,    /* An identifier of an object associated with the message */
    ID_GroupID      = 20,    /* An identifier of the group of a message */
    ID_GroupSeq     = 21,    /* The sequence within a group */
    ID_ServerTime   = 22,    /* The originating server timestamp (nanosecods in the Unix epoch) */
    ID_OriginServer = 23,    /* The originating server as a UID */
    ID_RealExpiry   = 24,    /* The real expiry time that should be set for this (retained) message */
    ID_ContentType  = 25,    /* MIME style content type */
	ID_Processor    = 26     /* Processor of the message */
};

/*
 * Starter bytes
 */
enum S_Val {
    S_Boolean       = 0x02,
    S_Float         = 0x04,
    S_Double        = 0x06,
    S_Char          = 0x1C,
    S_Short         = 0x50,
    S_UShort        = 0x54,
    S_Byte          = 0x30,
    S_ByteLen       = 0x10,
    S_UByteLen      = 0x11,
    S_Map           = 0x48,
    S_Xid           = 0x5E,
    S_Int           = 0x60,
    S_UInt          = 0x68,
    S_ByteArray     = 0x90,
    S_User          = 0x20,
    S_Long          = 0x70,
    S_Time          = 0x08,
    S_NameLen       = 0x58,
    S_Name          = 0xA0,
    S_String        = 0xC0,
    S_StrLen        = 0x28,
    S_NullString    = 0x01,
    S_ID            = 0x14,
    S_BooleanFalse  = 0x02,    /* For boolean, the type byte encodes the value */
    S_BooleanTrue   = 0x03
};

/**
 * The object for encoding and decoding ISM JMS content is an expanding buffer.
 * This is defined in ismutil.h .
 */
typedef struct concat_alloc_t ism_actionbuf_t;

/**
 * Get the next object from the content stream.
 *
 * All values can be considered as a type,value pair.  When the next value is read
 * the invoker needs to know the type of the next value before processing the value.
 *
 * When a keyword=value pair is encoded, the keyword itself has a type which is
 * normally either a name string or and id.  A map is required to be alternating
 * keywords and values, so an invalid sequence can be detected.
 *
 * The value is returned as a field which consists of a union of scalar types,
 * a type, and a length.  The length is only used for a few of the types including
 * byte array, user, and map.  You should not assume the type of the field without
 * checking it.
 *
 * @param action  The message buffer which is an expandng buffer
 * @param field   The returned field
 * @return A retun code: 0=good
 * <br> -1 = End of the stream
 * <br> -2 = The content of the stream is not valid
 * <br) -3 = Type conversion error
 *
 */
XAPI int ism_protocol_getObjectValue(ism_actionbuf_t * action, ism_field_t * field);

/**
 * Put a field to the map or message
 *
 * All simple data types can be written by creating a field and putting the
 * object to the map or message.  This is just the reverse of getObjectValue.
 * However, individual methods for each data type are also provided since
 * the invoker knows the type of the data to put.
 *
 * @param map  The map or message to put to
 * @param var  The field to set
 */
XAPI void ism_protocol_putObjectValue(ism_actionbuf_t * map, ism_field_t * var);

/**
 * Ensure there is room in the buffer
 *
 * Extend the buffer if required.
 *
 * All methods for scalar items assume that the invoker has ensured there is room
 * in the buffer.
 * @param map  The map buffer to check
 * @param len  The length needed
 */
XAPI void ism_protocol_ensureBuffer(ism_actionbuf_t * buf, int len);

/**
 * Reserve space in the buffer (ensuring there is room)
 *
 * Extend the buffer if required.
 *
 * @param buf The buffer to reserve space in.
 * @param len The length of space to reserve.
 *
 * @return The position of the start of the reserved buffer space
 */
int ism_protocol_reserveBuffer(ism_actionbuf_t * buf, int len);

/**
 * Put a null value.
 *
 * A null value represents an object which is null which is not the same as one
 * which has a zero length.  This is normally the same as the keyword not being present.
 *
 * @param map  The map or message to put to
 */
XAPI void ism_protocol_putNullValue(ism_actionbuf_t * map);


/**
 * Put a boolean value.
 *
 * A boolean value actually has three states, true, false, or unknown.
 * The unknown value is represented as null.  This method will only set false or
 * true based on whether the value is 0 or not zero.
 *
 * @param map  The map or message to put to
 * @param val  The value 0=false, all other values are true
 */
XAPI void ism_protocol_putBooleanValue(ism_actionbuf_t * map, int val);


/**
 * Put a byte value.
 *
 * A byte value is a signed 8 bit value.  The values 0 to 19 are the
 * most efficient to encode.
 *
 * @param map  The map or message to put to
 * @param val  The value cast to a signed byte
 */
XAPI void ism_protocol_putByteValue(ism_actionbuf_t * map, int val);

/**
 * Put an unsigned byte value.
 *
 * A unsigned byte value is an unsigned 8 bit value.
 * This type is not known to JMS so it is mapped to an integer.
 *
 * @param map  The map or message to put to
 * @param val  The value cast to an unsigned byte
 */
XAPI void ism_protocol_putUByteValue(ism_actionbuf_t * map, int val);

/**
 * Put a short value.
 *
 * A short value is a signed 16 bit value.
 *
 * @param map  The map or message to put to
 * @param val  The value cast to a signed short
 */
XAPI void ism_protocol_putShortValue(ism_actionbuf_t * map, int val);

/**
 * Put an unsigned short value.
 *
 * A short value is a signed 16 bit value.
 * This type is not known to JMS so it is mapped to an integer.
 *
 * @param map  The map or message to put to
 * @param val  The value cast to a signed short
 */
XAPI void ism_protocol_putSmallValue(ism_actionbuf_t * map, int val, int otype);

/**
 * Put an int value.
 *
 * A int value is a signed 32 bit value.
 *
 * @param map  The map or message to put to
 * @param val  The value as an integer
 */
XAPI void ism_protocol_putIntValue(ism_actionbuf_t * map, int val);

/**
 * Put a long value.
 *
 * A long value is a signed 64 bit value.
 *
 * @param map  The map or message to put to
 * @param val  The value as a long
 */
XAPI void ism_protocol_putLongValue(ism_actionbuf_t * map, int64_t val);


/**
 * Put a float value.
 *
 * A float value is a 32 bit IEEE binary floating point value.
 *
 * @param map  The map or message to put to
 * @param val  The value as an float
 */
XAPI void ism_protocol_putFloatValue(ism_actionbuf_t * map, float val);


/**
 * Put a double value.
 *
 * A double value is a 64 bit IEEE binary floating point value.
 *
 * @param map  The map or message to put to
 * @param val  The value as a double
 */
XAPI void ism_protocol_putDoubleValue(ism_actionbuf_t * map, double val);

/**
 * Put a string value.
 *
 * A string value is an array of bytes terminating with a null byte.
 *
 * @param map  The map or message to put to
 * @param str  The value as a null terminated string
 */
XAPI void ism_protocol_putStringValue(ism_actionbuf_t * map, const char * str);

/**
 * Put a string value with a length
 *
 * A string value is an array of bytes and the terminating null byte is added.
 *
 * @param map  The map or message to put to
 * @param str  The value as a null terminated string
 * @param len  The length of the buffer
 */
XAPI void ism_protocol_putStringLenValue(ism_actionbuf_t * map, const char * str, int len);

/**
 * Put a name value.
 *
 * A name value is an aray of bytes terminating with a null byte.
 *
 * @param map  The map or message to put to
 * @param str  The value as a null terminated string
 */
XAPI void ism_protocol_putNameValue(ism_actionbuf_t * map, const char * str);

/**
 * Put a name value with name which is not null terminated.
 * A name value is just like a string, but is the name of a map or property
 * @parm map   The map or message to put to
 * @param str  The value
 * @param len  The length of the value
 */
XAPI void ism_protocol_putNameLenValue(ism_actionbuf_t * map, const char * str, int len);

/**
 * Simple form of put name index.
 * This assume we have a single byte ID and we have checked for capacity.
 * @param map  The map or message to put to
 * @param id   The ID which must be in the range 0 to 255.
 */
XAPI void ism_protocol_putNameIndex(ism_actionbuf_t * buf, int id);


/**
 * Put a byte array value.
 *
 * A byte array is represented as a array a bytes and a length.
 *
 * @param map  The map or message to put to
 * @param str  The pointer to the byte array
 * @param len  The length of the byte array
 */
XAPI void ism_protocol_putByteArrayValue(ism_actionbuf_t * map, const char * str, int len);


/**
 * Put a map value.
 *
 * A map is represented as a byte array a bytes and a length.  The bytes contain
 * alternating names and values.
 *
 * @param map  The map or message to put to
 * @param str  The pointer to the byte array of the map
 * @param len  The length of the byte array of the map
 */
XAPI void ism_protocol_putMapValue(ism_actionbuf_t * map, const char * str, int len);


/**
 * Put a map value from properties.
 *
 * A map is represented as a byte array a bytes and a length.  The bytes contain
 * alternating names and values.
 *
 * @param map    The map or message to put to
 * @param props  The properties object
 */
XAPI void ism_protocol_putMapProperties(ism_actionbuf_t * map, ism_prop_t * props);

/**
 * Put a JSON map value into a JMS MapMessage or StreamMessage.
 *
 * A map is represented as a byte array a bytes and a length.  The bytes contain
 * alternating names and values.
 *
 * @param map      The map or message to put to
 * @param ent      The JSON TextObject or TextArray to convert
 * @param count    The number of elements in the TextObject or TextArray
 * @param inarray  Indicator for whether the JSON object is a TextObject (0) or
 *                 an TextArray (1)
 */
void ism_protocol_putJSONMapValue(ism_actionbuf_t * map, ism_json_entry_t * ent, int count, int inarray);

/*
 * Put a XID field into the content.
 * @param map  The map or message to put to
 * @param str  The location of the xid field data
 */
XAPI void ism_protocol_putXidValueF(ism_actionbuf_t * map, const char * str, int len);

/*
 * Put a XID structure into the content
 * @param map  The map or message to put to
 * @param xid  The xid structure
 */
XAPI void ism_protocol_putXidValue(ism_actionbuf_t * map, const ism_xid_t * xid);

/**
 * Dump the contents of the field into a buffer for problem determination.
 *
 * @param field  The field to dump
 * @param buf    A result buffer
 * @param len    The length of the result buffer.  If this is not long enough to hold
 *    the result, it is truncated.
 * @return The address of the buffer.
 */
XAPI char * ism_protocol_dumpField(ism_field_t * field, char * buf, int len);

/**
 * Find a property by name.
 *
 * Search a set of properties for the named property, and return a field containing its value.
 * If the named property is not found return a non-zero return code and set the field to
 * a null field.
 * @param action  The map or properties buffer
 * @param name    The property name to search fo
 * @param f       The field to return
 * @return A return code 0=found, 1=not found
 */
XAPI int ism_findPropertyName(ism_actionbuf_t * action, const char * name, ism_field_t * f);

/**
 * Find a property by name index.
 *
 * Search a set of properties for a system property, and return a field containing its value.
 * If the named property is not found return a non-zero return code and set the field to
 * a nnull field.
 *
 * @param action  The map or properties buffer
 * @param name    The property name to search fo
 * @param f       The field to return
 * @return A return code 0=found, 1=not found
 */
XAPI int ism_findPropertyNameIndex(ism_actionbuf_t * action, int index, ism_field_t * f);

/*
 * Convert a properties object to a serialized properties map.
 *
 * Properties are written to the buffer at the used position in the buffer which is updated.
 *
 * The properties are written to a single
 * @param props   The properties object to serialize
 * @param mapbuf  The buffer to place the map.
 * @return A return code 0=good
 */
XAPI int ism_protocol_serializeProperties(ism_prop_t * props, concat_alloc_t * mapbuf);

/*
 * Convert serialized properties to a properties object.
 *
 * Put all fields in the specified buffer (from pos to used) are put into the properties object.
 * The properties object must be created before calling this function.
 * @param  mapbuf The serialized properties
 * @param  props  The properties object
 * @return A return code: 0=good, ISMRC_PropertiesNotValid if bad
 */
int ism_protocol_deserializeProperties(concat_alloc_t * mapbuf, ism_prop_t * props);

/**
 * Get a 4 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 4 byte integer at the current location or 0 if past the end
 */
int ism_kafka_getInt4(concat_alloc_t * buf);

/**
 * Get a 1 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 1 byte integer at the current location or 0 if past the end
 */
int ism_kafka_getInt1(concat_alloc_t * buf);

/**
 * Get a 2 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 2 byte integer at the current location or 0 if past the end
 */
int ism_kafka_getInt2(concat_alloc_t * buf);

/**
 * Return the remaining bytes in the buffer
 * @param buf  The buffer
 * @returns    0 if at end of the buffer, >0 otherwise
 */
int ism_kafka_more(concat_alloc_t * buf);

/**
 * Get an 8 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 8 byte integer at the current location or 0 if past the end
 */
int64_t ism_kafka_getInt8(concat_alloc_t * buf);

/**
 * Get a string in kafka format
 * @param buf  The buffer
 * @param str  (output) The location of the string or NULL if none exists.  This is not null terminated.
 * @returns    The length of the string
 */
int ism_kafka_getString(concat_alloc_t * buf, char * * str);

/**
 * Get a byte array in kafka format
 * @param buf  The buffer
 * @param bytes (output) The location of the byte array or NULL if none exists.
 * @returns    The length of the byte array
 */
int ism_kafka_getBytes(concat_alloc_t * buf, char * * bytes);

/**
 * Put a 4 byte integer in kafka format
 * @param buf  The buffer
 * @param value The value to put
 */
void ism_kafka_putInt4(concat_alloc_t * buf, int value);

/**
 * Put a 4 byte integer in kafka format at the specified location.
 * This is used to fill in a length value unknown when it is originally created.
 * @param buf  The buffer
 * @param offset The position in the buffer.
 * @param value The value to put
 */
void ism_kafka_putInt4At(concat_alloc_t * buf, int offset, int value);

/**
 * Put a 1 byte integer in kafka format
 * @param buf  The buffe
 * @param value The value to put
 */
void ism_kafka_putInt1(concat_alloc_t * buf, int value);

/**
 * Put a 2 byte integer in kafka format
 * @param buf  The buffer
 * @param value The value to put
 */
void ism_kafka_putInt2(concat_alloc_t * buf, int value);

/**
 * Put an 8 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 4 byte integer at the current location or 0 if past the end
 */
void ism_kafka_putInt8(concat_alloc_t * buf, int64_t value);

/**
 * Put an 8 byte integer in kafka format at a specified location
 * @param buf  The buffer
 * @param offset The position in the buffer
 * @returns    The value of a 4 byte integer at the current location or 0 if past the end
 */
void ism_kafka_putInt8At(concat_alloc_t * buf, int offset, uint64_t value);

/**
 * Put a string in kafka format
 * @param buf  The buffer
 * @param value The location of the string
 * @param len  The length of the string or -1 to indicate it is null terminated
 * @returns    The value of a 4 byte integer at the current location or 0 if past the end
 */
void ism_kafka_putString(concat_alloc_t * buf, const char * value, int len);

/**
 * Put a byte array in kafka format
 * @param buf  The buffer
 * @param value The location of the byte array
 * @param len   The length of the byte array
 */
void ism_kafka_putBytes(concat_alloc_t * buf, const char * value, int len);


#ifdef __cplusplus
}
#endif
#endif
