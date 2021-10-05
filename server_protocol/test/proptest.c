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
#include <imacontent.h>
#include <selector.h>
#include <mqtt.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void testProps(void);
void testSelect(void);
void testACLCheck(void);
void testIDTopic(void);

CU_TestInfo ISM_Protocol_CUnit_Props[] = {
    {"PropTest           ", testProps },
    {"SelectTest         ", testSelect },
    {"ACLCheckTest       ", testACLCheck },
    {"TestID             ", testIDTopic },
    CU_TEST_INFO_NULL
};

extern int g_verbose;

/*
 * Xid protocol test
 */
void testProps(void) {
    ism_prop_t * props;
    ism_prop_t * props2;
    ism_field_t  field;
    ism_field_t  topic;
    concat_alloc_t map;
    char  xbuf[1000];
    int   rc;

    props = ism_common_newProperties(100);
    CU_ASSERT(props != NULL);

    field.val.s = "This is a string";
    field.type = VT_String;
    ism_common_setProperty(props, "string", &field);

    field.val.i = 33;
    field.type = VT_Integer;
    ism_common_setProperty(props, "int", &field);

    field.val.d = 3.14;
    field.type = VT_Double;
    ism_common_setProperty(props, "pi", &field);

    field.val.s = "sam";
    field.type = VT_String;
    ism_common_setProperty(props, "fred", &field);

    ism_common_getProperty(props, "int", &field);
    CU_ASSERT(field.type = VT_Integer);
    CU_ASSERT(field.val.i = 33);

    ism_common_getProperty(props, "pi", &field);
    CU_ASSERT(field.type = VT_Double);
    CU_ASSERT(field.val.d = 3.14);

    ism_common_getProperty(props, "fred", &field);
    CU_ASSERT(field.type = VT_String);
    CU_ASSERT(!strcmp(field.val.s, "sam"));

    ism_common_getProperty(props, "string", &field);
    CU_ASSERT(field.type = VT_String);
    CU_ASSERT(!strcmp(field.val.s, "This is a string"));

    memset(&map, 0, sizeof(map));
    map.buf = xbuf;
    map.len = sizeof(xbuf);
    rc = ism_common_serializeProperties(props, &map);
    CU_ASSERT(rc == 0);
    CU_ASSERT(map.used > 0);

    ism_common_findPropertyName(&map, "int", &field);
    CU_ASSERT(field.type = VT_Integer);
    CU_ASSERT(field.val.i = 33);

    ism_common_findPropertyName(&map, "pi", &field);
    CU_ASSERT(field.type = VT_Double);
    CU_ASSERT(field.val.d = 3.14);

    ism_common_findPropertyName(&map, "fred", &field);
    CU_ASSERT(field.type = VT_String);
    CU_ASSERT(!strcmp(field.val.s, "sam"));

    ism_common_findPropertyName(&map, "string", &field);
    CU_ASSERT(field.type = VT_String);
    CU_ASSERT(!strcmp(field.val.s, "This is a string"));


    props2 = ism_common_newProperties(4);
    rc = ism_common_deserializeProperties(&map, props2);
    CU_ASSERT(rc == 0);

    ism_common_getProperty(props2, "int", &field);
    CU_ASSERT(field.type = VT_Integer);
    CU_ASSERT(field.val.i = 33);

    ism_common_getProperty(props2, "pi", &field);
    CU_ASSERT(field.type = VT_Double);
    CU_ASSERT(field.val.d = 3.14);

    ism_common_getProperty(props2, "fred", &field);
    CU_ASSERT(field.type = VT_String);
    CU_ASSERT(!strcmp(field.val.s, "sam"));

    ism_common_getProperty(props2, "string", &field);
    CU_ASSERT(field.type = VT_String);
    CU_ASSERT(!strcmp(field.val.s, "This is a string"));

    map.used = 0;
    field.type = VT_String;
    field.val.s = "this/is/a/topic";
    ism_common_putPropertyID(&map, IDX_Topic, &field);
    CU_ASSERT(map.used == 19);
    CU_ASSERT(ism_common_findPropertyID(&map, IDX_Topic, &topic) == 0);
    CU_ASSERT(topic.type == VT_String);
    if (topic.type == VT_String)
        CU_ASSERT(!strcmp(topic.val.s, field.val.s));

}

uint64_t OrderId;         ///< Order on queue - monotonically increasing
uint8_t  Persistence;     ///< Persistence - ismMESSAGE_PERSISTENCE_*
uint8_t  Reliability;     ///< Reliability of delivery - ismMESSAGE_RELIABILITY_*
uint8_t  Priority;        ///< Priority - ismMESASGE_PRIORITY_*
uint8_t  RedeliveryCount; ///< Redelivery counts - up to 255 attempts
uint32_t Expiry;          ///< Expiry time - seconds since 2000-01-01T00
uint8_t  Flags;           ///< Flags, such as retained - ismMESSAGE_FLAGS_*
uint8_t  MessageType;

extern int ism_protocol_selectMessage(
        ismMessageHeader_t *       hdr,
        uint8_t                    areas,
        ismMessageAreaType_t       areatype[areas],
        size_t                     areasize[areas],
        void *                     areaptr[areas],
        const char *               topic,
        const void *               rule,
        size_t                     rulelen);

/*
 * Selection test
 */
void testSelect(void) {
    ismMessageHeader_t         hdr = {0L, 0, 0, 4, 0, 0, 0, 0};
    ismMessageAreaType_t       areatype[2] = { ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t                     areasize[2] = {0, 0};
    void *                     areaptr[2] = {NULL, NULL};
    ismRule_t *                rule;
    int                        rulelen;
    int                        selected;
    char                       xbuf [1024];
    concat_alloc_t             props = {xbuf, sizeof(xbuf)};
    int                        rc;
    char                       dbuf [4096];
    ism_acl_t *                acl;

    ism_protocol_putNameValue(&props, "string");
    ism_protocol_putStringValue(&props, "This is a string");
    ism_protocol_putNameValue(&props, "int");
    ism_protocol_putIntValue(&props, 33);
    ism_protocol_putNameValue(&props, "pi");
    ism_protocol_putDoubleValue(&props, 3.14);
    ism_protocol_putNameValue(&props, "sam");
    ism_protocol_putStringValue(&props, "fred");
    ism_protocol_putNameIndex(&props, ID_Topic);
    ism_protocol_putStringValue(&props, "part0/p1/p2");
    areasize[0] = props.used;
    areaptr[0] = props.buf;
    acl = ism_protocol_findACL("_3", 1);
    ism_protocol_addACLitem(acl, "part1");
    ism_protocol_unlockACL(acl);

    rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, "#", 0);
    CU_ASSERT(rc != 0);

    rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, "!!Topic1@1&!Topic3@2", 3);
    if (rc == 0 && g_verbose) {
        ism_common_dumpSelectRule(rule, dbuf, sizeof xbuf);
        printf("\nrule 0 = %s\n", dbuf);
    }
    CU_ASSERT(rc == 0);

    rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, "!!Org@1&!Type@2&!Fmt@3&!Event@4&!ID@5", 3);
    if (rc == 0 && g_verbose) {
        ism_common_dumpSelectRule(rule, dbuf, sizeof xbuf);
        printf("\nrule 1 = %s\n", dbuf);
    }
    CU_ASSERT(rc == 0);

    rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, "QoS>0&Topic1@3", 1);
    if (rc == 0) {
        ism_common_dumpSelectRule(rule, dbuf, sizeof xbuf);
        if (g_verbose)
            printf("\nrule buscon =\n%s\n", dbuf);
        selected = ism_common_selectMessage(&hdr, 2, areatype, areasize, areaptr, "part0/part1/part2", rule, 0);
        CU_ASSERT(selected == SELECT_FALSE);
    }
    CU_ASSERT(rc == 0);

    rc = ism_common_compileSelectRule(&rule, &rulelen, "string not like 'h%' and sam not like 'mary'");
    ism_common_dumpSelectRule(rule, dbuf, sizeof dbuf);
    if (g_verbose)
        printf("\nrule 1 =%s\n", dbuf);
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    rc = ism_common_compileSelectRule(&rule, &rulelen, "sam = 'fred'");
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    rc = ism_common_compileSelectRule(&rule, &rulelen, "int = 33");
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    rc = ism_common_compileSelectRule(&rule, &rulelen, "JMS_IBM_Retain = 0");
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    rc = ism_common_compileSelectRule(&rule, &rulelen, "garbage = 999");
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_UNKNOWN);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    rc = ism_common_compileSelectRule(&rule, &rulelen, "garbage = junk");
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_UNKNOWN);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, "Topic = 'part0/p1/p2'", 1);
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, "Topic = 'p0/part1/part2'", 1);
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, "p0/part1/part2", rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, "QoS = 0", 1);
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        // ism_common_dumpSelectRule(rule, dbuf, sizeof xbuf);
        // printf("qos=%s\n", dbuf);
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, "Topic1 = 'part1'", 1);
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        // ism_common_dumpSelectRule(rule, dbuf, sizeof xbuf);
        // printf("topicpart=%s\n", dbuf);
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, "p0/part1/part2", rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
    if (rule)
        ism_common_freeSelectRule(rule);

    hdr.Flags =  ismMESSAGE_FLAGS_RETAINED;
    rc = ism_common_compileSelectRule(&rule, &rulelen, "JMS_IBM_Retain = 1");
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
    if (rule)
        ism_common_freeSelectRule(rule);
}

XAPI int ism_common_printSelectRule(ismRule_t * rule);

/*
 * ACL check test
 */
void testACLCheck(void) {
    ismMessageHeader_t         hdr = {0L, 0, 0, 4, 0, 0, 0, 0};
    ismMessageAreaType_t       areatype[2] = { ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t                     areasize[2] = {0, 0};
    void *                     areaptr[2] = {NULL, NULL};
    ismRule_t *                rule;
    int                        rulelen;
    int                        selected;
    char                       xbuf [512];
    xUNUSED  char              zbuf [1024];
    concat_alloc_t             props = {xbuf, sizeof(xbuf)};
    int                        rc;

    ism_protocol_putNameIndex(&props, ID_Topic);
    ism_protocol_putStringValue(&props, "iot-2/myorg/type/mytype/id/myid/evt/myevt");
    areasize[0] = props.used;
    areaptr[0] = props.buf;

    rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, "JMS_Topic aclcheck('fred', 3, 5) is not false", SELOPT_Internal);
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        ism_common_dumpSelectRule(rule, zbuf, sizeof zbuf);
        if (g_verbose)
            printf("\n%s\n", zbuf);
        selected = ism_protocol_selectMessage(&hdr, 2, areatype, areasize, areaptr, NULL, rule, 0);
        CU_ASSERT(selected == SELECT_TRUE);
    }
}

/*
 * Create the properties with the topic when it is known to be small.
 *
 * @param topic   The topic name
 * @param outbuf  The output buffer.  This must be at least 5 bytes longer than the topic string.
 * @return The output buffer or NULL for an error
 */
int topicProperty(const char * topic, char * outbuf) {
    int  topiclen = (int)strlen(topic);

    if (topiclen > 248 || outbuf == NULL) {
        return 0;
    }
    outbuf[0] = 0x15;             /* S_ID+1  */
    outbuf[1] = 0x09;             /* ID_Topic */
    outbuf[2] = 0x29;             /* S_StrLen+1 */
    outbuf[3] = topiclen + 1;     /* Length of the topic string (including the null) */
    strcpy(outbuf+4, topic);      /* The topic string (including the null) */
    return topiclen+5;
}

/*
 * Selection test
 */
void testIDTopic(void) {
    char  buf[1000];
    concat_alloc_t props = {buf, sizeof(buf)};
    ism_field_t f;
    int   rc;

    props.used = topicProperty("my/topic", props.buf);
    rc = ism_findPropertyNameIndex(&props, 9, &f);
    CU_ASSERT(rc == 0);
    CU_ASSERT(f.type = VT_String);
    CU_ASSERT(!strcmp(f.val.s, "my/topic"));

    props.buf[1] = 8;
    props.used = topicProperty("another/topic", props.buf);
    rc = ism_findPropertyNameIndex(&props, 9, &f);
    CU_ASSERT(rc == 0);
    CU_ASSERT(f.type = VT_String);
    CU_ASSERT(!strcmp(f.val.s, "another/topic"));

}

