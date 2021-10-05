/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

#ifndef __PROTOEX_DEFINED
#define __PROTOEX_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#include <ismuints.h>
#ifndef XAPI
    #define XAPI extern
#endif

/*
 * Extension item values
 */
enum ExtensionItemValue_e {
    EXIV_Null            = 0x00,
    /* Exist only */
    EXIV_SendNAK         = 0x10,
    EXIV_SendQoS0NAK     = 0x11,
    EXIV_SubscribeNAK    = 0x12,
    EXIV_ServerInfo      = 0x13,
    EXIV_ACLWait         = 0x14,
    EXIV_SendSubs        = 0x15,
    EXIV_CheckSessionUser= 0x16,
    EXIV_SendNFSubs      = 0x17,
    EXIV_CleanStart      = 0x18,
    EXIV_EndExtension    = 0x3F,
    /* Strings */
    EXIV_ReasonString    = 0x40,
    EXIV_Locale          = 0x41,
    EXIV_ServerProduct   = 0x44,  /* Eclipse Amlen aka IBM WIoTP Message Gateway aka IoT MessageSight */
    EXIV_ServerVersion   = 0x45,  /* 5.0.0.1 */
    EXIV_ServerDetails   = 0x46,  /* 20151212_2100 */
    EXIV_ServerName      = 0x47,  /* MyServer */
    EXIV_MonitorTopic    = 0x48,
    EXIV_ClientName      = 0x52,
    EXIV_ClientVersion   = 0x53,
    EXIV_ClientDetails   = 0x54,
    EXIV_ACLFilter       = 0x55,
    EXIV_Property        = 0x56,   /* Property as name=value */
    EXIV_RenameSession   = 0x57,
    /* Two byte int */
    EXIV_ServerRC        = 0x80,
    EXIV_ProxyProtLevel  = 0x81,
    EXIV_SubscribeOpt    = 0x82,
    EXIV_MonitorOpt      = 0x83,
    EXIV_ExpectedMsgRate = 0x84,
    EXIV_SubID           = 0x85,
    EXIV_DelayTakeover   = 0x86,
    /* Four byte int */
    EXIV_ExpireTTL       = 0xC1,
    EXIV_WaitTime        = 0xC2,
    EXIV_MaxActive       = 0xC3,
    /* Eight byte int */
    EXIV_TimeMillis      = 0xF0,
    /* Byte array */
    EXIV_Subscriptions   = 0xF8
};



/*
 * Extension functions (used by proxy protocol)
 */

/*
 * Callback for extension scan
 * @param exitem  The extension item ID
 * @param svalue  The point to a string or byte array
 * @param lvalue  The long value or length of a string or byte array
 * @param userdata Context data for the callback
 */
typedef void (* ism_scanCallback_t)(int exitem, const char * svalue, int64_t lvalue, void * userdata);

/*
 * Scan a protocol extension.
 * This checks the extension, and if callback is specified will call this method
 * for each extension found.
 * @param bufp  The extension buffer
 * @param len   The extension length
 * @param callback  The optional callback
 * @param userdata  Parameter to pass to the callback
 * @return  The number of items in the extension or negative to indicate an error
 */
XAPI int     ism_common_scanExtension(const char * bufp, int len, ism_scanCallback_t callback, void * userdata);
/*
 * Get an int value from a protocol extension
 * @param bufp  The extension buffer
 * @param len   The extension length
 * @param which The extension item
 * @param deflt The default value if not found
 * @return  The value of the item or the deflt value if it does not exist
 */
XAPI int32_t ism_common_getExtensionValue(const char * bufp, int len, int which, int deflt);
/*
 * Get a long value from a protocol extension
 * @param bufp  The extension buffer
 * @param len   The extension length
 * @param which The extension item
 * @param deflt The default value if not found
 * @return  The value of the item or the deflt value if it does not exist
 */
XAPI int64_t ism_common_getExtensionLong(const char * bufp, int len, int which, int64_t deflt);
/*
 * Get a string value from a protocol extension
 * @param bufp  The extension buffer
 * @param len   The extension length
 * @param which The extension item
 * @param outlen (output) The length of the string (optional)
 * @return  The value of the item or NULL if it does not exist
 */
XAPI const char * ism_common_getExtensionString(const char * bufp, int len, int which, int * outlen);
/*
 * Get a string value from a protocol extension
 * @param bufp  The extension buffer
 * @param len   The extension length
 * @param which The extension item
 * @param outlen (output) The length of the string (optional)
 * @return  The value of the item or NULL if it does not exist
 */
XAPI const char * ism_common_getExtensionByteArray(const char * bufp, int len, int which, int * outlen);

/*
 * Put a string item into an extension
 * @param bufp  The extension buffer
 * @param which The extension type
 * @param value the extension value
 * @return An ISMRC return code 0=good
 */
XAPI int ism_common_putExtensionString(concat_alloc_t * buf, int which, const char * value);

/*
 * Put a scalar item into an extension
 * @param bufp  The extension buffer
 * @param which The extension type
 * @param value the extension value
 * @return An ISMRC return code 0=good
 */
XAPI int ism_common_putExtensionValue(concat_alloc_t * buf, int which, uint64_t value);

/*
 * Put a byte array item into an extension
 * @param which The extension type
 * @param value the extension value
 * @param len   The extension value length
 * @return An ISMRC return code 0=good
 */
XAPI int ism_common_putExtensionByteArray(concat_alloc_t * buf, int which, const char * value, int len);



enum MqttPropType_e {
    MPT_Undef      = 0,
    MPT_Int1       = 1,
    MPT_Int2       = 2,
    MPT_Int4       = 3,
    MPT_String     = 4,
    MPT_Bytes      = 5,
    MPT_NamePair   = 6,
    MPT_Boolean    = 7,
    MPT_VarInt     = 8
};


#define CPOI_IS_ALT    0x10000
#define CPOI_ALT_MULTI 0xC0000
#define CPOI_MULTI     0x80000

/*
 * Define an MQTT extension field.
 * This is defined in a global table with multiple releases, and is used to
 * create the extension context which is for a single version.
 */
typedef struct mqtt_prop_field_t {
    uint16_t     id;         /* Field ID */
    uint8_t      type;       /* Data type */
    uint8_t      version;    /* Applies to this and higher versions */
    uint32_t     valid;      /* Bitmask indicating where it is valid */
    const char * name;       /* Short text description of the field */
} mqtt_prop_field_t;

/*
 * MQTT peroperty context
 */
typedef struct mqtt_prop_ctx_t {
    char         eyecatcher[8];
    uint8_t      version;         /* MQTT version */
    uint8_t      resv[3];
    uint32_t     max_id;          /* Max allowed ID */
    uint32_t     array_id;        /* Max ID in the array */
    uint32_t     more_count;      /* Count of IDs in the more section */
    mqtt_prop_field_t * * fields;  /* Array of fields */
    mqtt_prop_field_t * * more;    /* More fields */
    void *       userdata;
} mqtt_prop_ctx_t;


/*
 * Callback to check MQTT extended field
 *
 * If the checking function returns a non-zero value, it must have called ism_common_setError()
 * or ism_commmon_setErrorData() to indicate the reason.
 *
 * @param ctx      The extension context
 * @param userdata Arbitrary data based to the checker
 * @param fld      The field
 * @param ptr      The location of the data
 * @param len      The length of the data
 * @param value    The integer value of the field
 * @return 0=good, ISMRC otherwise
 */
typedef int (* ism_check_ext_f)(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value);

/**
 * Make an MQTT property context.
 *
 * Normally there would be one such context for each supported version of MQTT.  This is not needed
 * for versions of MQTT before v5 as extended fields do not exist.
 *
 * The input table is terminated with an entry with a zero id.
 *
 *
 * @param fields  The input extension table (containing multiple versions)
 * @param version The MQTT version
 */
XAPI mqtt_prop_ctx_t * ism_common_makeMqttPropCtx(mqtt_prop_field_t * fields, int version);

/*
 * Find the field in an MQTT property context
 * @param ctx   The property context
 * @param id    The ID of the field
 * @return The field or NULL to indicate it is not found
 */
mqtt_prop_field_t * ism_common_findMqttPropID(mqtt_prop_ctx_t * ctx, int id);

/*
 * Check the validity of an MQTT property section
 *
 * @param bufp    The location of the area
 * @param len     The length of the area
 * @param ctx     The property context
 * @oaram valid   The validity flags for this control packet
 * @param check   The field checking function
 * @param userdata The userdata function
 * @param server 0=client, 1=server
 * @return A return code: 0=good, otherwise an ISMRC return code
 */
XAPI int ism_common_checkMqttPropFields(const char * bufp, int len, mqtt_prop_ctx_t * ctx, int valid,
        ism_check_ext_f check, void * userdata);

/*
 * Get an MQTT property integer value
 *
 * @param bufp   The location of the area
 * @param len    The length of the area
 * @param which  The ID of the optional field
 * @param ctx    The property context
 * @param deflt  The default value if the opptional field is not present
 * @return The value of the optional field, or deflt if it is not present
 */
XAPI int ism_common_getMqttPropField(const char * bufp, int len, int which, mqtt_prop_ctx_t * ctx, int deflt);

/*
 * Get an MQTT property string value
 *
 * @param bufp   The location of the area
 * @param len    The length of the area
 * @param cmd    The MQTT command packet code
 * @param which  The ID of the optional field
 * @param ctx    The property context
 * @param outlen The optional output length.  This is primarily used when the field can contain null characters.
 */
XAPI const char * ism_common_getMqttPropString(const char * bufp, int len, int which, mqtt_prop_ctx_t * ctx, int * outlen);

/*
 * Put an MQTT property integer value
 *
 * @param buf   The buffer to put the optional field
 * @oaram which The ID of the optional field
 * @param ctx   The property context
 * @param value The value of the optional field
 * @return A return code, 0=good
 */
XAPI int ism_common_putMqttPropField(concat_alloc_t * buf, int which, mqtt_prop_ctx_t * ctx, int value);

/*
 * Put an MQTT property string value
 *
 * @param buf   The buffer to put the optional field
 * @oaram which The ID of the optional field
 * @param ctx   The property context
 * @param value The value of the optional field
 * @param len   The length of the value, or -1 to indicate strlen should be used
 * @return A return code, 0=good
 */
XAPI int ism_common_putMqttPropString(concat_alloc_t * buf, int which, mqtt_prop_ctx_t * ctx, const char * value, int len);


/*
 * Put an MQTT property name pair
 *
 * @param buf   The buffer to put the optional field
 * @oaram which The ID of the optional field
 * @param ctx   The property context
 * @param name  The name of the object
 * @param namelen The length of the name, or -1 to indicate strlen should be used
 * @param value The value of the optional field
 * @param len   The length of the value, or -1 to indicate strlen should be used
 * @return A return code, 0=good
 */
XAPI int ism_common_putMqttPropNamePair(concat_alloc_t * buf, int which, mqtt_prop_ctx_t * ctx,
        const char * name, int namelen, const char * value, int len);

/*
 * Get an MQTT variable size integer
 * @param buf   The buffer containing the length
 * @return The length value
 */
XAPI int ism_common_getMqttVarInt(concat_alloc_t * buf);

/*
 * Get an MQTT variable size integer without an alloc buffer
 * @param buf   The buffer containing the variable length string
 * @param buf   The length of the buffer
 * @param used  (output) The number of bytes in the variable length integer
 * @return The length value
 */
XAPI int ism_common_getMqttVarIntExp(const char * buf, int buflen, int * used);

/*
 * Put an MQTT variable length
 * @param buf   The buffer to put the length field
 * @param len   The length value to put
 */
XAPI void ism_common_putMqttVarInt(concat_alloc_t * buf, int len);

#ifdef __cplusplus
}
#endif

#endif
