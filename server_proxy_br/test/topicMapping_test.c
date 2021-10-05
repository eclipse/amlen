/*
 * Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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
void ism_bridge_topicMapper(concat_alloc_t * buf, const char * topic, const char * tmap, ism_forwarder_t * forwarder, mqttbrMsg_t * mmsg);

//             "Topic": "wiotp/event/+/+/MyEvent/#",
//             "TopicMap": "iot-2/type/${Topic2}/id/${Topic3}/evt/${Topic4}/fmt/json'"

void testOneMap(const char * topic, const char * tmap, const char * expected) {
    char xbuf [4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 0};
    ism_bridge_topicMapper(&buf, topic, tmap, NULL, NULL);
    xbuf[buf.used] = 0;
    int exlen = strlen(expected);
    if (exlen && expected[exlen-1]=='*') {
        if (memcmp(xbuf, expected, exlen-1)) {
            printf("is:'%s' expected_start='%s'\n", xbuf, expected);
            CU_ASSERT_STRING_EQUAL(xbuf, expected);
        }
    } else {
        if (strcmp(xbuf, expected))
            printf("is:'%s' expected='%s'\n", xbuf, expected);
        CU_ASSERT_STRING_EQUAL(xbuf, expected);
    }

}

void topicMappingTest(void) {
    testOneMap("wiotp/event/Topic2/Topic3/MyEvent/etc", "iot-2/type/${Topic2}/id/${Topic3}/evt/${Topic4}/fmt/json'", "iot-2/type/Topic2/id/Topic3/evt/MyEvent/fmt/json'");

    // UTF-8 characters in source topic
    testOneMap("wiotp/event/主题/Topic3/MyEvent/etc", "iot-2/type/${Topic2}/id/${Topic3}/evt/${Topic4}/fmt/json'", "iot-2/type/主题/id/Topic3/evt/MyEvent/fmt/json'");

    // UTF-8 characters in topic mapping
    testOneMap("wiotp/event/Topic2/Topic3/MyEvent/etc", "物联网-2/type/${Topic2}/id/${Topic3}/evt/${Topic4}/fmt/json'", "物联网-2/type/Topic2/id/Topic3/evt/MyEvent/fmt/json'");

    // two digit number
    testOneMap("0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "a${Topic12}e", "ace");

    // A non-existent topic part is empyt string
    testOneMap("0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "a${Topic20}e", "ae");

    // With *, all topic parts after than in the source topic are placed in the output
    testOneMap("0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "1${Topic10*}2", "1a/b/c/d/e/f2");

    // Multiple *s
    testOneMap("0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "${Topic2*}/1${Topic10*}2", "2/3/4/5/6/7/8/9/a/b/c/d/e/f/1a/b/c/d/e/f2");

    // Non-existent macro name
    testOneMap("0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "${abc}", "");

    // Simple JSON
    testOneMap("0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "${JSON:abc:Topic2}", "\"abc\":\"2\"");

    // Simple JSON with *
    testOneMap("0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "${JSON:abc:Topic12*}", "\"abc\":\"c/d/e/f\"");

    testOneMap("0/1", "{${JSON?:f1:Topic5}, ${JSON?:f2:Topic3},${JSON?:f3:Topic2},${JSON?:f4:Topic0}}", "{ \"f4\":\"0\"}");

    testOneMap("0/1", "[${JSON?::Topic5},${JSON?::Topic3},${JSON?::Topic2},${JSON?::Topic0}]", "[\"0\"]");

    testOneMap("0/1/2/3/4/5/6", "[${JSON?::Topic5},${JSON?::Topic3},${JSON?::Topic2},${JSON?::Topic0}]", "[\"5\",\"3\",\"2\",\"0\"]");

    testOneMap("0/1/2/3", "{${JSON?:event:Topic1},${JSON?:time:TimeISO}", "{\"event\":\"1\",\"time\":\"20*");

    testOneMap("0/1/2/3", "${Topic0*}", "0/1/2/3");


    /* TODO:  Add tests for QoS, Time, and Properties */


}


