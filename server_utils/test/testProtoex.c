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
#include <ismutil.h>
#include <protoex.h>
#include <imacontent.h>
#include <selector.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/* Include the source to test so we can see statics */
#include <filter.c>
extern int g_verbose;

extern CU_TestInfo ISM_Util_CUnit_Protoex[];

void testExtension(void) {
    char xbuf[1024];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    CU_ASSERT(ism_common_putExtensionValue(&buf, EXIV_SendNAK, 0)==0);
    CU_ASSERT(buf.used == 1);
    CU_ASSERT(ism_common_putExtensionValue(&buf, EXIV_ServerRC, 100)==0);
    CU_ASSERT(buf.used == 4);
    CU_ASSERT(ism_common_putExtensionValue(&buf, EXIV_WaitTime, 100)==0);
    CU_ASSERT(buf.used == 9);
    CU_ASSERT(ism_common_putExtensionValue(&buf, EXIV_TimeMillis, 12345678901234L)==0);
    CU_ASSERT(buf.used == 18);
    CU_ASSERT(ism_common_putExtensionString(&buf, EXIV_Locale, "en-US") == 0);
    CU_ASSERT(buf.used == 27);
    CU_ASSERT(ism_common_scanExtension(buf.buf, buf.used, NULL, NULL) == 5);
    CU_ASSERT(ism_common_putExtensionValue(&buf, EXIV_SubscribeNAK, 0)==0);
    CU_ASSERT(buf.used == 28);
    CU_ASSERT(ism_common_scanExtension(buf.buf, buf.used, NULL, NULL) == 6);
    CU_ASSERT(ism_common_putExtensionValue(&buf, EXIV_EndExtension, 0)==0);
    CU_ASSERT(ism_common_putExtensionValue(&buf, EXIV_SubscribeNAK, 0)==0);
    CU_ASSERT(ism_common_scanExtension(buf.buf, buf.used, NULL, NULL) < 0);
    buf.used--;

    CU_ASSERT(ism_common_putExtensionValue(&buf, EXIV_Locale, 0)!=0);
    CU_ASSERT(ism_common_putExtensionString(&buf, EXIV_SendNAK, 0)!=0);
    CU_ASSERT(ism_common_putExtensionString(&buf, EXIV_WaitTime, 0)!=0);

    CU_ASSERT(ism_common_getExtensionValue(buf.buf, buf.used, EXIV_SendNAK, 0)==1);
    CU_ASSERT(ism_common_getExtensionValue(buf.buf, buf.used, EXIV_SubscribeNAK, 0)==1);
    CU_ASSERT(ism_common_getExtensionValue(buf.buf, buf.used, 0x0F, 0)==0);
    CU_ASSERT(ism_common_getExtensionValue(buf.buf, buf.used, EXIV_WaitTime, 0)==100);
    CU_ASSERT(ism_common_getExtensionValue(buf.buf, buf.used, EXIV_ServerRC, 0)==100);

    CU_ASSERT(ism_common_getExtensionLong(buf.buf, buf.used, EXIV_TimeMillis, 0)==12345678901234L);
    CU_ASSERT(ism_common_getExtensionLong(buf.buf, buf.used, EXIV_SubscribeNAK, 0)==1);
    CU_ASSERT(ism_common_getExtensionLong(buf.buf, buf.used, EXIV_WaitTime, 0)==100);
    CU_ASSERT(!strcmp(ism_common_getExtensionString(buf.buf, buf.used, EXIV_Locale, NULL), "en-US"));
}

/*
 * Test kafka content primatives
 */
void testKafkaContent(void) {
    char xbuf[1024];
    char * str;
    int    val;
    concat_alloc_t bufx = {xbuf, sizeof xbuf};
    concat_alloc_t * buf = &bufx;

    ism_kafka_putInt4(buf, 1234);
    CU_ASSERT(buf->used == 4);
    ism_kafka_putInt1(buf, -12);
    CU_ASSERT(buf->used == 5);
    ism_kafka_putInt2(buf, -123);
    CU_ASSERT(buf->used == 7);
    ism_kafka_putInt4(buf, 1236);
    CU_ASSERT(buf->used == 11);
    CU_ASSERT(ism_kafka_getInt4(buf) == 1234);
    CU_ASSERT(buf->pos == 4);
    ism_kafka_putInt4At(buf, 0, 567);
    CU_ASSERT(buf->used == 11);
    ism_kafka_putString(buf, "a string", -1);
    CU_ASSERT(buf->used == 21);
    ism_kafka_putString(buf, "a string", 1);
    CU_ASSERT(buf->used == 24);
    ism_kafka_putBytes(buf, "bytearray", 9);
    ism_kafka_putInt8(buf, 1234567890123L);
    buf->pos = 0;
    CU_ASSERT(buf->used == 45);

    CU_ASSERT(ism_kafka_more(buf) == 45);
    CU_ASSERT((val =ism_kafka_getInt4(buf)) == 567);
    CU_ASSERT(ism_kafka_getInt1(buf) == -12);
    CU_ASSERT(ism_kafka_getInt2(buf) == -123);
    CU_ASSERT(ism_kafka_getInt4(buf) == 1236);
    CU_ASSERT(ism_kafka_getString(buf, &str) == 8);
    CU_ASSERT(!memcmp(str, "a string", 8));
    CU_ASSERT(ism_kafka_getString(buf, &str) == 1);
    CU_ASSERT(*str = 'a');
    CU_ASSERT(ism_kafka_getBytes(buf, &str) == 9);
    CU_ASSERT(!memcmp(str, "bytearray", 9));
    CU_ASSERT(ism_kafka_getInt8(buf) == 1234567890123L);
    CU_ASSERT(ism_kafka_more(buf) == 0);

    /* Test boundary issues */
    buf->used = 0;
    buf->pos = 0;
    ism_kafka_putInt1(buf, 1);

    CU_ASSERT(ism_kafka_getInt4(buf) == 0);
    CU_ASSERT(ism_kafka_more(buf) < 0);
    CU_ASSERT(ism_kafka_getInt4(buf) == 0);
    buf->pos = 0;
    CU_ASSERT(ism_kafka_getInt2(buf) == 0);
    CU_ASSERT(ism_kafka_more(buf) < 0);
    CU_ASSERT(ism_kafka_getInt2(buf) == 0);
    buf->pos = 0;
    CU_ASSERT(ism_kafka_getInt8(buf) == 0);
    CU_ASSERT(ism_kafka_more(buf) < 0);
    CU_ASSERT(ism_kafka_getInt8(buf) == 0);
    buf->pos = 0;
    CU_ASSERT(ism_kafka_getString(buf, &str) == 0);
    CU_ASSERT(str == NULL);
    CU_ASSERT(ism_kafka_more(buf) < 0);
    CU_ASSERT(ism_kafka_getString(buf, &str) == 0);
    CU_ASSERT(str == NULL);
    buf->pos = 0;
    CU_ASSERT(ism_kafka_getBytes(buf, &str) == 0);
    CU_ASSERT(str == NULL);
    CU_ASSERT(ism_kafka_more(buf) < 0);
    CU_ASSERT(ism_kafka_getString(buf, &str) == 0);
    CU_ASSERT(str == NULL);
    buf->pos = 0;

    ism_kafka_putInt4(buf, 0);
    CU_ASSERT(ism_kafka_getString(buf, &str) == 0);
    CU_ASSERT(str == NULL);
    buf->pos = 0;
    CU_ASSERT(ism_kafka_getBytes(buf, &str) == 0);
    CU_ASSERT(str == NULL);
}


/*
 * Test setACL, and the various runctions is calls
 */
void testACL(void) {
    const char * aclsrc;
    int   rc;

    aclsrc = "@myacl\n+type1/dev1\n+type2/dev1\n+type1/dev2\n+type2/dev2";
    rc = ism_protocol_setACL(aclsrc, -1, 0, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ism_protocol_getACLcount("myacl")==4);

    aclsrc = "@myacl\n+type1/dev1\n+type2/dev1\n+type1/dev2\n+type2/dev3";
    rc = ism_protocol_setACL(aclsrc, -1, 0, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ism_protocol_getACLcount("myacl")==5);

    aclsrc = "@myacl\n-type1/dev1\n-type2/dev1\n-type1/dev2\n-type2/dev2";
    rc = ism_protocol_setACL(aclsrc, -1, 0, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ism_protocol_getACLcount("myacl")==1);

    aclsrc = "@myacl\n+type1/dev1\n@myacl2\n+type2/dev1\n+type1/dev2\n+type2/dev2";
    rc = ism_protocol_setACL(aclsrc, -1, 0, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ism_protocol_getACLcount("myacl")==2);
    CU_ASSERT(ism_protocol_getACLcount("myacl2")==3);

    aclsrc = ":myacl\n+type3/dev1";
    rc = ism_protocol_setACL(aclsrc, -1, 0, NULL);
    CU_ASSERT(rc == 0);
    if (g_verbose)
        printf("Line=%d count=%u\n", __LINE__, ism_protocol_getACLcount("myacl"));
    CU_ASSERT(ism_protocol_getACLcount("myacl")==1);

    aclsrc = "!myacl\0!myacl2";
    rc = ism_protocol_setACL(aclsrc, 14, 0, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ism_protocol_getACLcount("myacl")==-1);
    CU_ASSERT(ism_protocol_getACLcount("myacl2")==-1);
}

enum testFields_e {
    TMXF_MsgFormat     = 1,
    TMXF_KeepAlive     = 2,
    TMXF_MsgExpire     = 3,
    TMXF_SessionExpire = 4,
    TMXF_MaxPacketSize = 23,
    TMXF_Reason        = 5,
    TMXF_ReplyTopic    = 6,
    TMXF_CorrID        = 7,
    TMXF_Property      = 10,
    TMXF_NoRetain      = 11,
    TMXF_SubID         = 15,
    TMXF_ClientID      = 33,
    TMXF_MaxPKID       = 34,
    TMXF_WillQoS       = 115,
    TMXF_WillTopic     = 116
};



enum MqttPropIDs_e {
    CPOI_CONNECT      = 0x0001,
    CPOI_CONNACK      = 0x0002,
    CPOI_PUBLISH      = 0x0004,
    CPOI_PUBACK       = 0x0008,
    CPOI_PUBCOMP      = 0x0010,
    CPOI_SUBSCRIBE    = 0x0020,
    CPOI_SUBACK       = 0x0040,
    CPOI_UNSUBSCRIBE  = 0x0080,
    CPOI_UNSUBACK     = 0x0100,
    CPOI_S_DISCONNECT = 0x0200,
    CPOI_C_DISCONNECT = 0x0400,
    CPOI_DISCONNECT   = 0x0600,
    CPOI_ACK          = 0x075A
};

/*
 * Test field table
 */
mqtt_prop_field_t test_opt_fields [] = {
    { TMXF_MsgFormat,     MPT_Int1,   5, CPOI_PUBLISH | CPOI_CONNECT,           "MessageFormat" },
    { TMXF_KeepAlive,     MPT_Int2,   5, CPOI_CONNACK | CPOI_CONNECT,           "KeepAlive"     },
    { TMXF_MaxPKID,       MPT_Int2,   5, CPOI_CONNECT,                          "MaxPacketD"    },
    { TMXF_MaxPKID,       MPT_Undef,  6, CPOI_CONNECT,                          "MaxPacketID"   },
    { TMXF_MaxPacketSize, MPT_Int4,   5, CPOI_CONNECT,                          "MaxPacketSize" },
    { TMXF_MsgExpire,     MPT_Int4,   5, CPOI_PUBLISH | CPOI_CONNECT,           "MessageExpire" },
    { TMXF_SessionExpire, MPT_Int4,   5, CPOI_CONNECT,                          "SessionExpire" },
    { TMXF_Reason,        MPT_String, 5, CPOI_ACK | CPOI_CONNECT,               "Reason"        },
    { TMXF_ReplyTopic,    MPT_String, 5, CPOI_PUBLISH,                          "ReplyTopic"    },
    { TMXF_CorrID,        MPT_Int4,   5, CPOI_PUBLISH,                          "CorrelationID" },
    { TMXF_CorrID,        MPT_String, 6, CPOI_PUBLISH,                          "CorrelationIDString" },
    { TMXF_ClientID,      MPT_String, 5, CPOI_CONNACK,                          "ClientID"      },
    { TMXF_WillQoS,       MPT_Int1,   6, CPOI_CONNECT,                          "WillQoS"       },
    { TMXF_WillTopic,     MPT_String, 6, CPOI_CONNECT,                          "WillTopic"     },
    { TMXF_Property,      MPT_NamePair, 5, CPOI_CONNECT | CPOI_MULTI,           "Property"      },
    { TMXF_NoRetain,      MPT_Boolean,  5, CPOI_CONNECT,                        "NoRetain"      },
    { TMXF_SubID,         MPT_VarInt, 5, CPOI_CONNECT,                          "SubscribeID"   },
    { 0, 0, 0, 0, NULL},   /* End of table */
};

/*
 * Validate a namepair (user property)
 * The lengths are already validated
 */
static int checkNamePair(const char * ptr, int len) {
    int namelen = BIGINT16(ptr);
    int valuelen = BIGINT16(ptr+2+namelen);
    char * outname;
    char * outval;
    if (ism_common_validUTF8Restrict(ptr+2, namelen, UR_NoNull)<0 ||
        ism_common_validUTF8(ptr+4+namelen, valuelen) < 0) {
        /* Make the error string valid */
        int shortname = namelen;
        if (shortname > 256)
            shortname = 256;
        outname = alloca(shortname+1);
        if (valuelen > 256)
            valuelen = 256;
        outval = alloca(valuelen+1);
        ism_common_validUTF8Replace(ptr+2, shortname, outname, UR_NoControl | UR_NoNonchar, '?');
        ism_common_validUTF8Replace(ptr+4+namelen, valuelen, outval, UR_NoControl | UR_NoNonchar, '?');
        ism_common_setErrorData(ISMRC_UnicodeNotValid, "%s%s", outname, outval);
        return ISMRC_UnicodeNotValid;
    }
    return 0;
}

/*
 * Check extension
 */
int checkExt(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value) {
    if (fld->type==MPT_String) {
        if (ism_common_validUTF8Restrict(ptr, len, UR_NoControl | UR_NoNonchar) < 0) {
            char * out;
            if (len > 256)
                len = 256;
            out = alloca(len+1);
            ism_common_validUTF8Replace(ptr, len, out, UR_NoControl | UR_NoNonchar, '?');
            ism_common_setErrorData(ISMRC_UnicodeNotValid, "%s%s", fld->name, out);
            return ISMRC_UnicodeNotValid;
        }
    }
    if (fld->type==MPT_NamePair) {
        return checkNamePair(ptr, len);
    }
    switch (fld->id) {
    case TMXF_MsgFormat:
        if (value>4) {
            ism_common_setErrorData(ISMRC_BadClientData, "%s%u", fld->name, value);
            return ISMRC_BadClientData;
        }
        break;
    case TMXF_WillQoS:
        if (value>2) {
            ism_common_setErrorData(ISMRC_BadClientData, "%s%u", fld->name, value);
            return ISMRC_BadClientData;
        }
        break;
    }
    return 0;
}

/*
 * Test for MQTT properties
 */
void testMqttProperties(void) {
    int  len;
    int  val;
    char xbuf[100];
    const char * sval;
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    mqtt_prop_ctx_t * ctx5;
    mqtt_prop_ctx_t * ctx6;

    ctx5 = ism_common_makeMqttPropCtx(test_opt_fields, 5);
    CU_ASSERT(ctx5 != NULL);
    if (ctx5) {
        CU_ASSERT(ctx5->version == 5);
        CU_ASSERT(ctx5->fields[TMXF_KeepAlive]->type == MPT_Int2);
        CU_ASSERT(ctx5->fields[TMXF_MaxPKID]->type == MPT_Int2);
    }

    ctx6 = ism_common_makeMqttPropCtx(test_opt_fields, 6);
    CU_ASSERT(ctx6 != NULL);
    if (ctx6) {
        CU_ASSERT(ctx6->version == 6);
        CU_ASSERT(ctx6->fields[TMXF_KeepAlive]->type == MPT_Int2);
        CU_ASSERT(ctx6->fields[TMXF_MaxPKID]->type == MPT_Undef);
    }


    ism_common_putMqttPropField(&buf, TMXF_MsgFormat, ctx5, 3);
    CU_ASSERT(buf.used == 2);
    ism_common_putMqttPropField(&buf, TMXF_MsgExpire, ctx5, 44000);
    CU_ASSERT(buf.used == 7);
    ism_common_putMqttPropField(&buf, TMXF_MaxPKID,   ctx5, 260);
    CU_ASSERT(buf.used == 10);
    ism_common_putMqttPropString(&buf, TMXF_Reason,    ctx5, "Why", -1);
    CU_ASSERT(buf.used == 16);
    ism_common_putMqttPropField(&buf, TMXF_SessionExpire, ctx5, 88888);
    CU_ASSERT(buf.used == 21);
    ism_common_putMqttPropNamePair(&buf, TMXF_Property, ctx5, "name", -1, "kwb", -1);
    CU_ASSERT(buf.used == 33);
    ism_common_putMqttPropField(&buf, TMXF_NoRetain, ctx5, 0);
    CU_ASSERT(buf.used == 33);
    ism_common_putMqttPropField(&buf, TMXF_NoRetain, ctx5, 1);
    CU_ASSERT(buf.used == 34);
    ism_common_putMqttPropField(&buf, TMXF_SubID, ctx5, 200);
    CU_ASSERT(buf.used == 37);
    ism_common_putMqttPropField(&buf, TMXF_MaxPacketSize, ctx5, 123456);
    CU_ASSERT(buf.used == 42);

    if (g_verbose)
        ism_common_setTraceLevel(6);
    ism_common_setTraceFile("stdout", 0);
    ism_common_setTraceOptions("time,thread,where");
    CU_ASSERT(ism_common_checkMqttPropFields(buf.buf, buf.used, ctx5, CPOI_CONNECT, checkExt, NULL)==0);
    CU_ASSERT(ism_common_checkMqttPropFields(buf.buf, buf.used, ctx6, CPOI_CONNECT, NULL, NULL)!=0);
    buf.buf[1] = 9;
    CU_ASSERT(ism_common_checkMqttPropFields(buf.buf, buf.used, ctx5, CPOI_CONNECT, checkExt, NULL) == ISMRC_BadClientData);
    buf.buf[1] = 3;
    ism_common_setTraceLevel(2);

    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_MsgFormat, ctx5, -1);
    CU_ASSERT(val == 3);
    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_SessionExpire, ctx5, -1);
    CU_ASSERT(val == 88888);
    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_MsgExpire, ctx5, -1);
    CU_ASSERT(val == 44000);
    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_MaxPKID, ctx5, -1);
    CU_ASSERT(val == 260);
    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_MsgFormat, ctx6, -1);
    CU_ASSERT(val == 3);
    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_MsgExpire, ctx6, -1);
    CU_ASSERT(val == 44000);
    val = ism_common_getMqttPropField(buf.buf, buf.used, 99, ctx5, -1);
    CU_ASSERT(val == -1);
    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_Reason, ctx5, -1);
    CU_ASSERT(val == -1);
    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_MaxPacketSize, ctx5, 77);
    CU_ASSERT(val == 123456);
    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_NoRetain, ctx5, 0);
    CU_ASSERT(val == 1);
    val = ism_common_getMqttPropField(buf.buf, buf.used, TMXF_SubID, ctx5, 0);
    CU_ASSERT(val = 200);



    sval = ism_common_getMqttPropString(buf.buf, buf.used, TMXF_Reason, ctx5, &len);
    CU_ASSERT(sval != NULL);
    CU_ASSERT(!memcmp(sval, "Why",3));
    CU_ASSERT(len == 3);


}


/*
 * Inner test for MQTT variable length
 */
void testMqttVarInt(int value, int expected) {
    char xbuf[100];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    int used;

    ism_common_putMqttVarInt(&buf, value);
    CU_ASSERT(buf.used == expected);
    if (buf.used != expected)
        printf("testMqttVarInt  value=%d expected=%u buf.used=%u\n", value, expected, buf.used);
    if (expected) {
        int len = ism_common_getMqttVarInt(&buf);
        CU_ASSERT(len == value);
        CU_ASSERT(buf.pos == buf.used);
        if (len != value || buf.pos !=buf.used) {
            printf("testMqttVarInt value=%d len=%d pos=%d used=%d\n", value, len, buf.pos, buf.used);
        }
        len = ism_common_getMqttVarIntExp(buf.buf, buf.used, &used);
        CU_ASSERT(len == value);
        CU_ASSERT(used =+ buf.used);
        if (len != value || used !=buf.used) {
            printf("testMqttVarIntExp value=%d len=%d pos=%d used=%d\n", value, len, used, buf.used);
        }
    }
}

/*
 * Outer test for MQTT variable length
 */
void testMqttVarLen(void) {
    char xbuf[100];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    testMqttVarInt(-1, 0);
    testMqttVarInt(0, 1);
    testMqttVarInt(1, 1);
    testMqttVarInt(127, 1);
    testMqttVarInt(128, 2);
    testMqttVarInt(16383, 2);
    testMqttVarInt(16384, 3);
    testMqttVarInt(2097151, 3);
    testMqttVarInt(2097152, 4);
    testMqttVarInt(268435455, 4);
    testMqttVarInt(268435456, 0);

    /* Check buffer too small */
    memcpy(buf.buf, "\x80\x80", 2);
    buf.used = 2;
    CU_ASSERT(ism_common_getMqttVarInt(&buf) == -1);
    CU_ASSERT(buf.pos == 0);

    /* Check length too long */
    memcpy(buf.buf, "\x80\x80\x80\x80\x80", 5);
    buf.used = 5;
    CU_ASSERT(ism_common_getMqttVarInt(&buf) == -1);

    /* Check lenght not shortest */
    memcpy(buf.buf, "\x80\x80\x00", 3);
    buf.used = 3;
    CU_ASSERT(ism_common_getMqttVarInt(&buf) == -1);
}

/*
 * test topicpart in filter.c
 */
void testPart(void) {
    int len;
    const char * part;
    const char * topic = "p0/part1/part2xx/part3abc/part4";

    len = topicpart(topic, &part, 0);
    CU_ASSERT(len==2);
    if (len==2)
        CU_ASSERT(!memcmp(part, "p0", len));

    len = topicpart(topic, &part, 1);
    CU_ASSERT(len==5);
    if (len==5)
        CU_ASSERT(!memcmp(part, "part1", len));

    len = topicpart(topic, &part, 2);
    CU_ASSERT(len==7);
    if (len==7)
        CU_ASSERT(!memcmp(part, "part2xx", len));

    len = topicpart(topic, &part, 3);
    CU_ASSERT(len==8);
    if (len==8)
        CU_ASSERT(!memcmp(part, "part3abc", len));

    len = topicpart(topic, &part, 4);
    CU_ASSERT(len==5);
    if (len==5)
        CU_ASSERT(!strcmp(part, "part4"));

    len = topicpart(topic, &part, 5);
    CU_ASSERT(len==0);
    CU_ASSERT(part==NULL);
}


CU_TestInfo ISM_Util_CUnit_Protoex[] = {
    { "Extension test", testExtension },
    { "Kafka content test", testKafkaContent },
    { "MQTT properties test", testMqttProperties },
    { "MQTT variable length test", testMqttVarLen },
    { "ACL utilities test", testACL },
    { "topicpart test", testPart },
    CU_TEST_INFO_NULL
};
