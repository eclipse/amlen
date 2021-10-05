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

/*
 * File: test_json.c
 * Component: server
 * SubComponent: server_protocol
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <ismutil.h>
#include <ismregex.h>
#include <ismjson.h>
#include <assert.h>
#include <float.h>
#include <transport.h>

#ifdef TRACE_COMP
#undef TRACE_COMP
#endif
#include <json.c>

extern int g_verbose;
int g_comment;


/* Test ism_json_put() */
char* jsonput(char * name, ism_field_t value, int notfirst, int end_pos) {
    concat_alloc_t buf = {0};
    ism_json_put(&buf, name, &value, notfirst);
    buf.buf[end_pos] = '\0';
    if (g_verbose)
        printf("jsonput returning %s\n", buf.buf);
    return buf.buf;
}

/* Test ism_json_putString() */
char* jsonputstring(char * instring) {
    concat_alloc_t buf = {0};
    ism_json_putString(&buf, instring);
    if (g_verbose)
        printf("jsonputstring returning %s\n", buf.buf);
    return buf.buf;
}

/* Test ism_json_putBytes() */
char* jsonputbytes(const char * inbytes, int end_pos) {
    concat_alloc_t buf = {0};
    ism_json_putBytes(&buf, inbytes);
    if (end_pos)
        buf.buf[end_pos] = '\0';
    if (g_verbose)
        printf("jsonputbytes returning %s\n", buf.buf);
    return buf.buf;
}

/* Test ism_json_putEscapeBtyes() */
char* jsonputescapebytes(const char * inbytes, int len, int end_pos) {
    concat_alloc_t buf = {0};
    ism_json_putEscapeBytes(&buf, inbytes, len);
    if (end_pos)
        buf.buf[end_pos] = '\0';
    if (g_verbose)
        printf("jsonputescapebytes returning %s\n", buf.buf);
    return buf.buf;
}

/* Test ism_json_putInteger() */
char* jsonputint(int inint) {
    concat_alloc_t buf = {0};
    ism_json_putInteger(&buf, inint);
    if(g_verbose)
        printf("jsonputint returning %s\n", buf.buf);
    return buf.buf;
}

/* Test ism_json_get() */
int jsonget(const char * json, int ent, char * entname) {
    ism_json_parse_t pobj = {0};
    ism_json_entry_t ents[100];
    int   len;
    int   rc;

    pobj.ent_alloc = 100;
    pobj.ent       = ents;
    if (json) {
        len            = (int)strlen(json);
        pobj.source    = malloc(len);
        pobj.src_len   = len;
        memset(pobj.source, 0, len);
        memcpy(pobj.source, json, len);
    }
    rc = ism_json_parse(&pobj);
    rc = ism_json_get(&pobj, ent, entname);
    if (g_verbose)
        printf("jsonget returning %d\n", rc);
    return rc;
}

/* Test ism_json_getString() */
const char * jsongetstring(const char * json, char * entname) {
    ism_json_parse_t pobj = {0};
    ism_json_entry_t ents[100];
    int   len;
    int   rc;
    const char * value;

    pobj.ent_alloc = 100;
    pobj.ent       = ents;
    if (json) {
        len            = (int)strlen(json);
        pobj.source    = malloc(len);
        pobj.src_len   = len;
        memset(pobj.source, 0, len);
        memcpy(pobj.source, json, len);
    }
    rc = ism_json_parse(&pobj);
    value = ism_json_getString(&pobj, entname);
    if (g_verbose) {
        printf("jsongetstring got %d from ism_json_parse\n", rc);
        printf("jsongetstring returning %s\n", value);
    }
    return value;
}

/* Test ism_json_getInt() */
int jsongetint(const char * json, char * entname, int dfault) {
    ism_json_parse_t pobj = {0};
    ism_json_entry_t ents[100];
    int   len;
    int   rc;
    int   value;

    pobj.ent_alloc = 100;
    pobj.ent       = ents;
    if (json) {
        len            = (int)strlen(json);
        pobj.source    = malloc(len);
        pobj.src_len   = len;
        memset(pobj.source, 0, len);
        memcpy(pobj.source, json, len);
    }
    rc = ism_json_parse(&pobj);
    value = ism_json_getInt(&pobj, entname, dfault);
    if (g_verbose) {
        printf("jsongetint got %d from ism_json_parse\n", rc);
        printf("jsongetint returning %d\n", value);
    }
    return value;
}
/* Test ism_json_getInt() */
int jsongetintStrict(const char * json, char * entname, int dfault) {
    ism_json_parse_t pobj = {0};
    ism_json_entry_t ents[100];
    int   len;
    int   rc;
    int   value;

    pobj.ent_alloc = 100;
    pobj.ent       = ents;
    if (json) {
        len            = (int)strlen(json);
        pobj.source    = malloc(len);
        pobj.src_len   = len;
        memset(pobj.source, 0, len);
        memcpy(pobj.source, json, len);
    }
    rc = ism_json_parse(&pobj);
    value = ism_json_getInteger(&pobj, entname, dfault);
    if (g_verbose) {
        printf("jsongetintStrict got %d from ism_json_parse\n", rc);
        printf("jsongetintStrict returning %d\n", value);
    }
    return value;
}

/**
 * Set up for and call ism_json_parse.
 * Returns the number of elements found in the input string.
 */
int jsonparse(const char * json, int expected) {
    ism_json_parse_t pobj = {0};
    ism_json_entry_t ents[100];
    int   len;
    int   rc;
    char  xbuf[1000];
    concat_alloc_t buf = {0};

    pobj.ent_alloc = 100;
    pobj.ent       = ents;
    len            = (int)strlen(json);
    pobj.source    = malloc(len);
    pobj.src_len   = len;
    if (g_comment) {
        pobj.options = JSON_OPTION_COMMENT;
    }
    memset(pobj.source, 0, len);
    memcpy(pobj.source, json, len);
    rc = ism_json_parse(&pobj);

    if (pobj.ent_count != expected || g_verbose) {
#ifdef DEBUG_CUNIT
        int i;
        for(i = 0; i < pobj.ent_count; i++) {
            printf("pobj.ent[%d] = %s %s\n",i,pobj.ent[i].name, pobj.ent[i].value);
        }
#endif
        printf("jsonparse got %d from ism_json_parse\n", rc);
        printf("\nparse: %s  rc=%d count=%d\n", json, pobj.rc, pobj.ent_count);
    }

    buf.buf = xbuf;
    buf.len = 1000;

    ism_json_toString(&pobj, &buf, 0, 0, 0);
    if (pobj.ent_count != expected || g_verbose) {
        printf("tostr: %s\n", buf.buf);
    }
    return pobj.ent_count;
}

/**
 * Set up for and call ism_json_parse.
 * Returns the number of elements found in the input string.
 */
int jsontostring(concat_alloc_t * buf, int compact, const char * json, const char * expected) {
    ism_json_parse_t pobj = {0};
    ism_json_entry_t ents[100];
    int   len;
    xUNUSED int   rc;

    buf->used = 0;
    pobj.ent_alloc = 100;
    pobj.ent       = ents;
    len            = (int)strlen(json);
    pobj.source    = malloc(len);
    pobj.src_len   = len;
    memset(pobj.source, 0, len);
    memcpy(pobj.source, json, len);
    ism_json_parse(&pobj);

    rc = ism_json_toString(&pobj, buf, 0, 0, compact);
    int cmp = strcmp(expected, buf->buf);
    if (rc || cmp || g_verbose) {
        if (rc)
            printf("jsontostring rc=%d\n", rc);
        else
            printf("jsontostring returning |%s|\n", buf->buf);
        if (cmp)
            printf("javatostring expecting |%s|\n", expected);
        return 1;
    }
    return 0;
}

/**
 * Set up for and call ism_json_parse.
 * Returns the return code from ism_json_parse().
 */
int jsonparse2(const char * json) {
    ism_json_parse_t pobj = {0};
    ism_json_entry_t ents[100];
    int   len;
    int   rc;
    char  xbuf[1000];
    concat_alloc_t buf = {0};

    pobj.ent_alloc = 100;
    pobj.ent       = ents;
    len            = (int)strlen(json);
    pobj.source    = malloc(len);
    pobj.src_len   = len;
    if (g_comment) {
        pobj.options = JSON_OPTION_COMMENT;
    }
    memset(pobj.source, 0, len);
    memcpy(pobj.source, json, len);
    rc = ism_json_parse(&pobj);

    if (g_verbose) {
        printf("\nparse: %s  rc=%d count=%d\n", json, pobj.rc, pobj.ent_count);

        buf.buf = xbuf;
        buf.len = 1000;

        ism_json_toString(&pobj, &buf, 0, 0, 0);
        printf("tostr: %s\n", buf.buf);
    }
    return rc;
}


/**
 * Test JSON parsing with calls to the jsonparse() test function.
 */
void jsonparsetest(void) {
    char  xbuf[1000];
    concat_alloc_t buf = {0};
    buf.buf = xbuf;

    CU_ASSERT(jsonparse("{}", 1) == 1);
    CU_ASSERT(jsonparse("[0,1,2, 3 ,4]", 6) == 6);
    /* Negative int */
    CU_ASSERT(jsonparse("[0,-1,-2, -3 ,-4]", 6) == 6);
    CU_ASSERT(jsonparse("[\"abc\"  ,\"def\", 0.31415E+1, true, false, null]", 7) == 7);
    /* Negative float */
    CU_ASSERT(jsonparse("[\"abc\"  ,\"def\", -0.31415E+1, true, false, null]", 7) == 7);
    CU_ASSERT(jsonparse("{\"abc\":3, \"x\":\"\\b\\r\\n\\tz\", \"obj\": { \"g\":3, \"h\":4.1, \"i\":\"5\" }\n }", 7) == 7);
    CU_ASSERT(jsonparse("{\"type\":\"PUBLISH\",\"topic\":\"RequestT\",\"payload\":{\"id\":\"13981690\",\"name\":\"myName\",\"array\":[\"1\",\"2\"]},\"replyto\":\"ResponseT\",\"corrid\":\"<corrid>\"}", 11) == 11);

    CU_ASSERT(jsonparse("{\"a\":{\"array\":[\"the\", \"array\"]}, \"embed1\":{\"embeddedarrays\":{\"array1\":[1], \"array2\":[1, 2], \"array3\":[1, 2, 3]}}}", 16) == 16);

    CU_ASSERT(jsonparse("{\"a\":{\"array\":[\"the\", \"array\"]}, \"embed1\":{\"embeddedarrays\":{\"array1\":[1], \"array2\":[1, 2], \"array3\":[1, 2, 3], \"embed2\":{\"one\":1, \"two\":[1, 2], \"embed3\":[1, 2, 3]}}}}", 25) == 25);

    /* Error: Unexpected end of JSON message */
    CU_ASSERT(jsonparse2("{") == 1);
    CU_ASSERT(jsonparse2("[ \"abc\", \"d\xffg\" ]") == 2);
    CU_ASSERT(jsonparse2("[ \"abc\", \"\1gh\" ]") == 2);

    g_comment = 1;
    CU_ASSERT(jsonparse("/* */{}", 1) == 1);
    CU_ASSERT(jsonparse("{/* */}", 1) == 1);
    CU_ASSERT(jsonparse("{}/* */", 1) == 1);
    CU_ASSERT(jsonparse("//\n{}", 1) == 1);
    CU_ASSERT(jsonparse("{//  \n }", 1) == 1);
    CU_ASSERT(jsonparse("{}//", 1) == 1);
    CU_ASSERT(jsonparse2("/*") == 2);
    CU_ASSERT(jsonparse2("/*  ") == 2);
    CU_ASSERT(jsonparse2("/*  *") == 2);
    CU_ASSERT(jsonparse2("/abc") == 2);
    g_comment = 0;

//TODO: Follow up with Ken.  Should rc just be 1 instead of 3?
//      Should rc=1 mean more than just unexpected end of message
//      which is currently misleading in some cases.
    /* Error: Invalid initial entry */
    CU_ASSERT(jsonparse2("}") == 2);

    /* Error: Invalid array (missing , between 3 and 4 */
    CU_ASSERT(jsonparse2("[0,1,2, 3 4]") == 1);

    /* Error: Inalid end of array (missing ] ) */
    CU_ASSERT(jsonparse2("[0,1,2, 3 ,4") == 1);

    /* Error: Inalid end of array (wrong closing element } ) */
    CU_ASSERT(jsonparse2("[0,1,2, 3 ,4}") == 1);

//TODO: Follow up with Ken.  Should rc just be 1 instead of 3?
//      Should rc=1 mean more than just unexpected end of message
//      which is currently misleading in some cases.
    /* Error: Inalid start of array (number first - missing [ ) */
    CU_ASSERT(jsonparse2("0,1,2, 3 ,4]") == 2);

    /* Error: Inalid start of array (string first - missing [ ) */
    CU_ASSERT(jsonparse2("\"zero\",1,2, 3 ,4]") == 2);

    /* Error: Inalid start of array (boolean true token first - missing [ ) */
    CU_ASSERT(jsonparse2("true,1,2, 3 ,4]") == 2);

    /* Error: Inalid start of array (null token first - missing [ ) */
    CU_ASSERT(jsonparse2("null,1,2, 3 ,4]") == 2);

    /* Error: Boolean token as name in name/value pair */
    CU_ASSERT(jsonparse2("{false:\"false\"}") == 1);

    /* Error: Number token as name in name/value pair */
    CU_ASSERT(jsonparse2("{1:\"one\"}") == 1);

    /* Error: Missing value in name/value pair */
    CU_ASSERT(jsonparse2("{\"emptyvalue\":}") == 1);

    /* Error: Missing value in name/value pair */
    CU_ASSERT(jsonparse2("{\"emptyvalue\": \"one\":1}") == 1);

    /* Error: Missing value in name/value pair */
    CU_ASSERT(jsonparse2("{\"emptyvalue\": \"emptyvalue2\":}") == 1);

    /* Error: Missing name in name/value pair */
    CU_ASSERT(jsonparse2("{:\"emptyname\"}") == 1);

    /* Error: Missing names in name/value pairs */
    CU_ASSERT(jsonparse2("{:\"emptyname\" :1}") == 1);

    /* Error: Missing colon in name/value pair */
    CU_ASSERT(jsonparse2("{\"one\" 1}") == 2);

    /* Error: Unquoted char string */
    CU_ASSERT(jsonparse2("[""abc""  ,\"def\", 0.31415E+1, true, false, null]") == 1);

    /* Error: Partially quoted char string */
    CU_ASSERT(jsonparse2("[""abc\"  ,\"def\", 0.31415E+1, true, false, null]") == 1);

    /* Error: Partially quoted char string */
    CU_ASSERT(jsonparse2("[\"abc\"  ,\"def"", 0.31415E+1, true, false, null]") == 1);

    /* Error: Semicolon where comma is expected*/
    CU_ASSERT(jsonparse2("{\"abc\":3; \"x\":\"\\b\\r\\n\\tz\", \"obj\": { \"g\":3, \"h\":4.1, \"i\":\"5\" }\n }") == 1);

    /* Error: Case mismatch for true value */
    CU_ASSERT(jsonparse2("[\"abc\"  ,\"def\", -0.31415E+1, True, false, null]") == 1);

    /* Error: Case mismatch for false  value*/
    CU_ASSERT(jsonparse2("[\"abc\"  ,\"def\", -0.31415E+1, true, FALSE, null]") == 1);

    /* Error: Case mismatch for null value */
    CU_ASSERT(jsonparse2("[\"abc\"  ,\"def\", -0.31415E+1, true, false, Null]") == 1);

   /* Tests for ism_json_toString */
    CU_ASSERT(jsontostring(&buf, 0, "{}", "{}") == 0);
    CU_ASSERT(jsontostring(&buf, 0, "[0,1,2, 3 ,4]", "[0, 1, 2, 3, 4]") == 0);
    CU_ASSERT(jsontostring(&buf, 0, "[0,-1,-2, -3 ,-4]", "[0, -1, -2, -3, -4]") == 0);
    CU_ASSERT(jsontostring(&buf, 0, "[\"abc\"  ,\"def\", 0.31415E+1, true, false, null]", "[\"abc\", \"def\", 0.31415E+1, true, false, null]") == 0);
    CU_ASSERT(jsontostring(&buf, 0, "[\"abc\"  ,\"def\", -0.31415E+1, true, false, null]", "[\"abc\", \"def\", -0.31415E+1, true, false, null]") == 0);
    CU_ASSERT(jsontostring(&buf, 1, "{\"abc\":3, \"x\":\"\\b\\r\\n\\tz\", \"obj\": { \"g\":3,\"h\":4.1,\"i\":\"5\" }\n }","{\"abc\":3,\"x\":\"\\b\\r\\n\\tz\",\"obj\":{\"g\":3,\"h\":4.1,\"i\":\"5\"}}") == 0);
    CU_ASSERT(jsontostring(&buf, 1, "{\"type\":\"PUBLISH\",\"topic\":\"RequestT\",\"payload\":{\"id\":\"13981690\",\"name\":\"myName\",\"array\":[\"1\",\"2\"]},\"replyto\":\"ResponseT\",\"corrid\":\"<corrid>\"}", "{\"type\":\"PUBLISH\",\"topic\":\"RequestT\",\"payload\":{\"id\":\"13981690\",\"name\":\"myName\",\"array\":[\"1\",\"2\"]},\"replyto\":\"ResponseT\",\"corrid\":\"<corrid>\"}") == 0);
    CU_ASSERT(jsontostring(&buf, 1, "{\"a\":{\"array\":[\"the\", \"array\"]}, \"embed1\":{\"embeddedarrays\":{\"array1\":[1], \"array2\":[1, 2], \"array3\":[1, 2, 3]}}}", "{\"a\":{\"array\":[\"the\",\"array\"]},\"embed1\":{\"embeddedarrays\":{\"array1\":[1],\"array2\":[1,2],\"array3\":[1,2,3]}}}") == 0);
    CU_ASSERT(jsontostring(&buf, 1, "{\"a\":{\"array\":[\"the\", \"array\"]}, \"embed1\":{\"embeddedarrays\":{\"array1\":[1], \"array2\":[1, 2], \"array3\":[1, 2, 3], \"embed2\":{\"one\":1, \"two\":[1, 2], \"embed3\":[1, 2, 3]}}}}", "{\"a\":{\"array\":[\"the\",\"array\"]},\"embed1\":{\"embeddedarrays\":{\"array1\":[1],\"array2\":[1,2],\"array3\":[1,2,3],\"embed2\":{\"one\":1,\"two\":[1,2],\"embed3\":[1,2,3]}}}}") == 0);

    CU_ASSERT(jsongetint("{ \"abc\": 5}", "abc", -1) == 5);
    CU_ASSERT(jsongetint("{ \"abc\": 5.1}", "abc", -1) == 5);
    CU_ASSERT(jsongetint("{ \"abc\": \"5\"}", "abc", -1) == 5);
    CU_ASSERT(jsongetint("{ \"abc\": \"5.31435\"}", "abc", -1) == 5);

    CU_ASSERT(jsongetintStrict("{ \"abc\": 5}", "abc", -1) == 5);
    CU_ASSERT(jsongetintStrict("{ \"abc\": 5.1}", "abc", -1) == -1);
    CU_ASSERT(jsongetintStrict("{ \"abc\": \"5\"}", "abc", -1) == -1);
    CU_ASSERT(jsongetintStrict("{ \"abc\": 5.000}", "abc", -1) == 5);
}

/*
 * Test UTF8 handling with calls to the doUTF8() test function.
 */
void utf8test(void) {
    CU_ASSERT(ism_common_validUTF8("abc", 3) == 3);
    /* "abc" in hex */
    CU_ASSERT(ism_common_validUTF8("\x61\x62\x63", 3) == 3);
    CU_ASSERT(ism_common_validUTF8("a\xc2\xa3\nx", 5) == 4);
    CU_ASSERT(ism_common_validUTF8("a\xf0\xa0\xa0\x80", 5) == 2);
    CU_ASSERT(ism_common_validUTF8("\x00\x41\xf0\xa0\xa0\x80", 6) == 3);
    CU_ASSERT(ism_common_validUTF8("\xe4\xa0\x80", 3) == 1);
    CU_ASSERT(ism_common_validUTF8("\xc2\x80", 2) == 1);
    CU_ASSERT(ism_common_validUTF8("\xc2\x80\x61", 3) == 2);
    CU_ASSERT(ism_common_validUTF8("\xc2\x80\na", 4) == 3);
    CU_ASSERT(ism_common_validUTF8("a\xf1\x80\x80\x80", 5) == 2);
    CU_ASSERT(ism_common_validUTF8("\xf4\x80\x80\x80", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("a\xe0\xa0\x80", 4) == 2);
    CU_ASSERT(ism_common_validUTF8("a\xf1\x80\x80\x80", 5) == 2);

    CU_ASSERT(ism_common_validUTF8("a\xe0\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xed\xa1\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("a\xf0\x80\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\xc2\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\x80\xc3\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\x80\x80\xc4", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x90\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("a\xc0\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("a\xa3\xc2\nx", 5) < 0);

    /* 0xc0 & 0xc1 effectively not allowed because they will never provide a
     * minimal sequence.
     */
    CU_ASSERT(ism_common_validUTF8("\xc0\x80", 2) < 0);
    CU_ASSERT(ism_common_validUTF8("\xc1\x80", 2) < 0);

    /* Extra continuation character not allowed */
    CU_ASSERT(ism_common_validUTF8("\xc2\x80\x80", 3) < 0);

    /* 0xf5 - 0xff not allowed */
    CU_ASSERT(ism_common_validUTF8("\xf5\x80\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xff\x80\x80\x80", 4) < 0);

    /* Check for minimal-sequence - test with 0x0a - new line */
    CU_ASSERT(ism_common_validUTF8("\x0a", 1) == 1);
    CU_ASSERT(ism_common_validUTF8("\xc0\x8a", 2) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe0\x80\x8a", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\x80\x80\x8a", 4) < 0);

    /* Check for minimal-sequence - test with 0x2f - slash */
    CU_ASSERT(ism_common_validUTF8("\x2f", 1) == 1);
    CU_ASSERT(ism_common_validUTF8("\xc0\x2f", 2) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe0\x80\x2f", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\x80\x80\x2f", 4) < 0);

    /* Check for minimal-sequence - test with 0x00 - nul */
    CU_ASSERT(ism_common_validUTF8("\x00", 1) == 1);
    CU_ASSERT(ism_common_validUTF8("\xc0\x80", 2) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe0\x80\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\x80\x80\x80", 4) < 0);

    /* Boundry tests -- well formed sequences */
    CU_ASSERT(ism_common_validUTF8("\xc2\x80", 2) == 1);
    CU_ASSERT(ism_common_validUTF8("\xc2\xbf", 2) == 1);
    CU_ASSERT(ism_common_validUTF8("\xdf\x80", 2) == 1);
    CU_ASSERT(ism_common_validUTF8("\xdf\xbf", 2) == 1);
    CU_ASSERT(ism_common_validUTF8("\xe0\xa0\x80", 3) == 1);
    CU_ASSERT(ism_common_validUTF8("\xe0\xbf\xbf", 3) == 1);
    CU_ASSERT(ism_common_validUTF8("\xe1\x80\x80", 3) == 1);
    CU_ASSERT(ism_common_validUTF8("\xec\xbf\xbf", 3) == 1);
    CU_ASSERT(ism_common_validUTF8("\xed\x80\x80", 3) == 1);
    CU_ASSERT(ism_common_validUTF8("\xed\x9f\xbf", 3) == 1);
    CU_ASSERT(ism_common_validUTF8("\xee\x80\x80", 3) == 1);
    CU_ASSERT(ism_common_validUTF8("\xef\xbf\xbf", 3) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf0\x90\x80\x80", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf0\x90\xbf\xbf", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf0\xbf\x80\x80", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf0\xbf\xbf\xbf", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf1\x80\x80\x80", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf1\x80\xbf\xbf", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf1\xbf\x80\x80", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf1\xbf\xbf\xbf", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf3\x80\x80\x80", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf3\x80\xbf\xbf", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf3\xbf\x80\x80", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf3\xbf\xbf\xbf", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf4\x80\x80\x80", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf4\x80\xbf\xbf", 4) == 1);
    CU_ASSERT(ism_common_validUTF8("\xf4\x8f\x80\x80", 4) == 1);

    /* Boundry tests -- invalid sequences */
    CU_ASSERT(ism_common_validUTF8("\xc2\x7f", 2) < 0);
    CU_ASSERT(ism_common_validUTF8("\xc2\xc2", 2) < 0);
    CU_ASSERT(ism_common_validUTF8("\xdf\x7f", 2) < 0);
    CU_ASSERT(ism_common_validUTF8("\xdf\xc2", 2) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe0\x9f\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe0\xc2\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe0\xa0\xd0", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe0\xbf\xd0", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe1\x7f\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe1\xc2\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe1\x80\x7f", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xe1\x80\xc2", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xec\x7f\xbf", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xec\xc2\xbf", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xec\xbf\x7f", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xec\xbf\xc2", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xed\x7f\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xed\xa0\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xed\x80\x7f", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xed\x80\xc2", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xed\x9f\x7f", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xed\x9f\xc2", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xee\x7f\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xee\xc2\x80", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xee\x80\x7f", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xee\x80\xc2", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xef\x7f\xbf", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xef\xc2\xbf", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xef\xbf\x7f", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xef\xbf\xc2", 3) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\x8f\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\x90\x7f\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\x90\xc2\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\x90\x80\x7f", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\x90\x80\xc2", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\xc2\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\xbf\x7f\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\xbf\xc2\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\xbf\x80\x7f", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf0\xbf\x80\xc2", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\x7f\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\x80\x7f\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\x80\x80\x7f", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\x80\xc2\xbf", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\x80\xbf\xc2", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\xc2\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\xbf\x7f\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\xbf\x80\x7f", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\xbf\xc2\xbf", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf1\xbf\xbf\xc2", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\x7f\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\x80\x7f\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\x80\x80\x7f", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\x80\xc2\xbf", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\x80\xbf\xc2", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\xc2\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\xbf\x7f\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\xbf\x80\x7f", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\xc2\xbf\xbf", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\xbf\xc2\xbf", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf3\xbf\xbf\xc2", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x7f\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x80\x7f\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x80\xc2\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x80\x80\x7f", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x80\x80\xc2", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x90\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x8f\x7f\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x8f\xc2\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x8f\x80\x7f", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x8f\x80\xc2", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf4\x8f\x80\xc2", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xf5\x80\x80\x80", 4) < 0);
    CU_ASSERT(ism_common_validUTF8("\xff\xbf\xbf\xbf", 4) < 0);
}
/**
 * Test UTF8 handling with calls to the doUTF8() test function.
 */
static int showReplace = 0;

int doValidUTF8Replace(int line, const char * str, int len, const char * out, int nal, int rep) {
    char outbuf[256];
    int  rc = ism_common_validUTF8Replace(str, len, NULL, nal, rep);
    int  rc2 = ism_common_validUTF8Replace(str, len, outbuf, nal, rep);
    if (out == NULL)
        out = str;
    if (showReplace || rc != rc2) {
        int i;
        len = strlen(outbuf);
        printf("line=%d len=%d len2=%d out=", line, rc, rc2);
        for (i=0; i<rc; i++) {
            printf("%02x", outbuf[i]&0xff);
        }
        printf("\n");
    }
    if (rc != rc2)
        return 0;
    return !strcmp(outbuf, out);
}
#define validUTF8Replace(str, len, out, nal, rep) doValidUTF8Replace(__LINE__, str, len, out, nal, rep)
void utf8Replace_test(void) {
    CU_ASSERT(validUTF8Replace("abc",              3, NULL,           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\x61\x62\x63",     3, "abc",          UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("a\xc2\xa3\nx",     5, "a\xc2\xa3?x",  UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("a\xf0\xa0\xa0\x80",5, NULL,           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\x00\x41\xf0\xa0\xa0\x80", 6, "?\x41\xf0\xa0\xa0\x80", UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xe4\xa0\x80",     3, NULL,           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xc2\x80",         2, "?",            UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xc2\x80\x61",     3, "?a",           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xc2\x80\na",      4, "??a",          UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("a\xf1\x80\x80\x80",5, NULL,           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xf4\x80\x80\x80", 4, NULL,           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("a\xe0\xa0\x80",    4, NULL,           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("a\xf1\x80\x80\x80",5, NULL,           UR_NoControl, '?'));

    CU_ASSERT(validUTF8Replace("a\xe0\x80\x80",    4, "a??",          UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xed\xa1\x80",     3, "??",           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("a\xf0\x80\x80\x80",4, "a??",          UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xf1\xc2\x80\x80", 4, "???",          UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xf1\x80\xc3\x80", 4, "??",           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xf1\x80\x80\xc4", 4, "?",            UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xf4\x90\x80\x80", 4, "???",          UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("a\xc0\x80",        3, "a",            UR_NoControl,   0));
    CU_ASSERT(validUTF8Replace("a\xa3\xc2\nx",     5, "a??x",         UR_NoControl, '?'));

    CU_ASSERT(validUTF8Replace("\xf5\x80\x80\x80",  4, "???",         UR_None,      '?'));
    CU_ASSERT(validUTF8Replace("\x0a",              1, NULL,          UR_None,      '?'));
    CU_ASSERT(validUTF8Replace("\xc0\x8a",          2, "?",           UR_NoControl, '?'));
    CU_ASSERT(validUTF8Replace("\xc0\x8a",          2, "?",           UR_NoControl,   0));
    CU_ASSERT(validUTF8Replace("#",                 1, "?",           UR_NoWildcard,  0));

    CU_ASSERT(validUTF8Replace("a/#z",              4, "a/\xc2\x9fz",      UR_NoWildcard,  0x9f));
    CU_ASSERT(validUTF8Replace("a/#z",              4, "a/\xef\xbf\xbdz",  UR_NoWildcard,  0xfffd));
    CU_ASSERT(validUTF8Replace("\1z",               2, "\xc2\x9fz",        UR_NoControl,   0x9f));
    CU_ASSERT(validUTF8Replace("\2z",               2, "\xef\xbf\xbdz",    UR_NoControl,   0xfffd));
    CU_ASSERT(validUTF8Replace("\nz",               2, "\xc2\x9fz",        UR_NoC0Line,    0x9f));
    CU_ASSERT(validUTF8Replace("\nz",               2, "\xef\xbf\xbdz",    UR_NoC0Line,    0xfffd));
    CU_ASSERT(validUTF8Replace("z\xe0",             2, "z\xc2\x9f",        UR_None,        0x9f));
    CU_ASSERT(validUTF8Replace("z\xe0",             2, "z\xef\xbf\xbd",    UR_None,        0xfffd));
    CU_ASSERT(validUTF8Replace("z\xef\xbf\xbe",     4, "z\xc2\x9f",        UR_NoNonchar,   0x9f));
    CU_ASSERT(validUTF8Replace("z\xef\xbf\xbf",     4, "z\xef\xbf\xbd",    UR_NoNonchar,   0xfffd));
    CU_ASSERT(validUTF8Replace("z\xef\xb7\x90",     4, "z\xc2\x9f",        UR_NoNonchar,   0x9f));
    CU_ASSERT(validUTF8Replace("z\xef\xb7\x91",     4, "z\xef\xbf\xbd",    UR_NoNonchar,   0xfffd));
    CU_ASSERT(validUTF8Replace("z\xc2\x80",         3, "z\xc2\x9f",        UR_NoC1,        0x9f));
    CU_ASSERT(validUTF8Replace("z\xe2\x81",         3, "z\xef\xbf\xbd",    UR_NoC1,        0xfffd));
    CU_ASSERT(validUTF8Replace("z\xff\x80",         3, "z\xc2\x9f\xc2\x9f",          UR_None,        0x9f));
    CU_ASSERT(validUTF8Replace("z\xff\x81",         3, "z\xef\xbf\xbd\xef\xbf\xbd",  UR_None,        0xfffd));
    showReplace = 1;
}

/**
 * Test UTF8 handling with calls to the doUTF8() test function.
 */
void utf8Restrict_test(void) {
    CU_ASSERT(ism_common_validUTF8Restrict("abc", 3, UR_NoControl) == 3);
    /* "abc" in hex */
    CU_ASSERT(ism_common_validUTF8Restrict("\x61\x62\x63", 3, UR_NoControl) == 3);
    CU_ASSERT(ism_common_validUTF8Restrict("a\xc2\xa3\nx", 5, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("a\xf0\xa0\xa0\x80", 5, UR_NoControl) == 2);
    CU_ASSERT(ism_common_validUTF8Restrict("\x00\x41\xf0\xa0\xa0\x80", 6, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe4\xa0\x80", 3, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xc2\x80", 2, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xc2\x80\x61", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xc2\x80\na", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("a\xf1\x80\x80\x80", 5, UR_NoControl) == 2);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x80\x80\x80", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("a\xe0\xa0\x80", 4, UR_NoControl) == 2);
    CU_ASSERT(ism_common_validUTF8Restrict("a\xf1\x80\x80\x80", 5, UR_NoControl) == 2);

    CU_ASSERT(ism_common_validUTF8Restrict("a\xe0\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xed\xa1\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("a\xf0\x80\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\xc2\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\x80\xc3\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\x80\x80\xc4", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x90\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("a\xc0\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("a\xa3\xc2\nx",  5, UR_NoControl) < 0);

    /* 0xc0 & 0xc1 effectively not allowed because they will never provide a
     * minimal sequence.
     */
    CU_ASSERT(ism_common_validUTF8Restrict("\xc0\x80", 2, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xc1\x80", 2, UR_NoControl) < 0);

    /* Extra continuation character not allowed */
    CU_ASSERT(ism_common_validUTF8Restrict("\xc2\x80\x80", 3, UR_NoControl) < 0);

    /* 0xf5 - 0xff not allowed */
    CU_ASSERT(ism_common_validUTF8Restrict("\xf5\x80\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xff\x80\x80\x80", 4, UR_NoControl) < 0);

    /* Check for minimal-sequence - test with 0x0a - new line */
    CU_ASSERT(ism_common_validUTF8Restrict("\x0a", 1, UR_NoC0Other) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xc0\x8a", 2, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe0\x80\x8a", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x80\x80\x8a", 4, UR_NoControl) < 0);

    /* Check for minimal-sequence - test with 0x2f - slash */
    CU_ASSERT(ism_common_validUTF8Restrict("\x2f", 1, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xc0\x2f", 2, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe0\x80\x2f", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x80\x80\x2f", 4, UR_NoControl) < 0);

    /* Check for minimal-sequence - test with 0x00 - nul */
    CU_ASSERT(ism_common_validUTF8Restrict("\x00", 1, UR_None) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xc0\x80", 2, UR_None) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe0\x80\x80", 3, UR_None) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x80\x80\x80", 4, UR_None) < 0);

    /* Boundry tests -- well formed sequences */
    CU_ASSERT(ism_common_validUTF8Restrict("\xc2\x80", 2, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xc2\xbf", 2, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xdf\x80", 2, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xdf\xbf", 2, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe0\xa0\x80", 3, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe0\xbf\xbf", 3, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe1\x80\x80", 3, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xec\xbf\xbf", 3, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xed\x80\x80", 3, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xed\x9f\xbf", 3, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xee\x80\x80", 3, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xef\xbf\xbf", 3, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x90\x80\x80", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x90\xbf\xbf", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\xbf\x80\x80", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\xbf\xbf\xbf", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\x80\x80\x80", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\x80\xbf\xbf", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\xbf\x80\x80", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\xbf\xbf\xbf", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\x80\x80\x80", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\x80\xbf\xbf", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\xbf\x80\x80", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\xbf\xbf\xbf", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x80\x80\x80", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x80\xbf\xbf", 4, UR_NoControl) == 1);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x8f\x80\x80", 4, UR_NoControl) == 1);

    /* Boundry tests -- invalid sequences */
    CU_ASSERT(ism_common_validUTF8Restrict("\xc2\x7f", 2, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xc2\xc2", 2, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xdf\x7f", 2, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xdf\xc2", 2, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe0\x9f\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe0\xc2\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe0\xa0\xd0", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe0\xbf\xd0", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe1\x7f\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe1\xc2\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe1\x80\x7f", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xe1\x80\xc2", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xec\x7f\xbf", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xec\xc2\xbf", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xec\xbf\x7f", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xec\xbf\xc2", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xed\x7f\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xed\xa0\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xed\x80\x7f", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xed\x80\xc2", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xed\x9f\x7f", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xed\x9f\xc2", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xee\x7f\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xee\xc2\x80", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xee\x80\x7f", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xee\x80\xc2", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xef\x7f\xbf", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xef\xc2\xbf", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xef\xbf\x7f", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xef\xbf\xc2", 3, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x8f\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x90\x7f\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x90\xc2\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x90\x80\x7f", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\x90\x80\xc2", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\xc2\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\xbf\x7f\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\xbf\xc2\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\xbf\x80\x7f", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf0\xbf\x80\xc2", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\x7f\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\x80\x7f\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\x80\x80\x7f", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\x80\xc2\xbf", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\x80\xbf\xc2", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\xc2\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\xbf\x7f\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\xbf\x80\x7f", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\xbf\xc2\xbf", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf1\xbf\xbf\xc2", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\x7f\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\x80\x7f\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\x80\x80\x7f", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\x80\xc2\xbf", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\x80\xbf\xc2", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\xc2\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\xbf\x7f\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\xbf\x80\x7f", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\xc2\xbf\xbf", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\xbf\xc2\xbf", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf3\xbf\xbf\xc2", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x7f\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x80\x7f\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x80\xc2\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x80\x80\x7f", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x80\x80\xc2", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x90\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x8f\x7f\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x8f\xc2\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x8f\x80\x7f", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x8f\x80\xc2", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf4\x8f\x80\xc2", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xf5\x80\x80\x80", 4, UR_NoControl) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("\xff\xbf\xbf\xbf", 4, UR_NoControl) < 0);

    CU_ASSERT(ism_common_validUTF8Restrict("z\xef\xbf\xbe",     4, UR_NoNonchar) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("z\xef\xbf\xbf",     4, UR_NoNonchar) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("z\xef\xb7\x90",     4, UR_NoNonchar) < 0);
    CU_ASSERT(ism_common_validUTF8Restrict("z\xef\xb7\x91",     4, UR_NoNonchar) < 0);

    /* TODO: add more restrictions */
}



/*
 * Stub for debugging
 */
static int mwv(const char * str, const char * match, ima_transport_info_t * transport) {
    int rc = ism_common_matchWithVars(str, match, transport, NULL, 0, 0);
    // printf("[%s] [%s] rc=%d\n", str, match, rc);
    return rc;
}


/*
 * Stub for debugging with groupID
 */
int mwvGrp(const char * str, const char * match, ima_transport_info_t * transport, char *groupID[], int groups) {
    int rc = ism_common_matchWithVars(str, match, transport, groupID, groups, 0);
    return rc;
}

/*
 * Stub for debugging with groupID - case insensitive
 */
int mwvGrpNoCaseCheck(const char * str, const char * match, ima_transport_info_t * transport, char *groupID[], int groups) {
    int rc = ism_common_matchWithVars(str, match, transport, groupID, groups, 1);
    return rc;
}

/*
 * Test match functions
 */
void testMatch(void) {
    char transportx[1024] = {0};
    ima_transport_info_t * transport = (ima_transport_info_t *)transportx;

    CU_ASSERT(ism_common_match("abc", "abc") == 1);
    CU_ASSERT(ism_common_match("abababc", "*abc") == 1);
    CU_ASSERT(ism_common_match("abababc", "a*abc") == 1);
    CU_ASSERT(ism_common_match("abababc", "a*a*a*") == 1);
    CU_ASSERT(ism_common_match("*", "*\xff") == 1);
    CU_ASSERT(ism_common_match("abc*", "*\xff") == 1);
    CU_ASSERT(ism_common_match("$", "*${*}") == 0);
    CU_ASSERT(ism_common_match("ab", "abc") == 0);
    CU_ASSERT(ism_common_match("", "") == 1);
    CU_ASSERT(ism_common_match("abc", "ab") == 0);
    CU_ASSERT(ism_common_match("ab*c", "ab\xff" "c") == 1);
    CU_ASSERT(ism_common_match("ab*c", "ab*c") == 1);

    transport->clientID = "MyClientID";
    transport->userid = "MyUserID";
    transport->cert_name = "MyCertName";

    /* Try a bunch of good cases */
    CU_ASSERT(mwv("abcdef", "abcdef", transport) == 0);
    CU_ASSERT(mwv("abcdef", "abc*", transport) == 0);
    CU_ASSERT(mwv("abcdef", "*c*", transport) == 0);
    CU_ASSERT(mwv("abc*def", "abc${*}def", transport) == 0);
    CU_ASSERT(mwv("abc${*}def", "abc${$}{*}def", transport) == 0);
    CU_ASSERT(mwv("MyClientID", "${ClientID}", transport) == 0);
    CU_ASSERT(mwv("xxMyClientID", "xx${ClientID}", transport) == 0);
    CU_ASSERT(mwv("MyClientIDyy", "${ClientID}yy", transport) == 0);
    CU_ASSERT(mwv("MyUserID", "${UserID}", transport) == 0);
    CU_ASSERT(mwv("xxMyUserID", "xx${UserID}", transport) == 0);
    CU_ASSERT(mwv("MyUserIDyy", "${UserID}yy", transport) == 0);
    CU_ASSERT(mwv("MyUserID", "${UserID}", transport) == 0);
    CU_ASSERT(mwv("xxMyUserID", "xx${UserID}", transport) == 0);
    CU_ASSERT(mwv("MyCertName", "${CommonName}", transport) == 0);
    CU_ASSERT(mwv("xxMyCertName", "xx${CommonName}", transport) == 0);
    CU_ASSERT(mwv("MyCertNameyy", "${CommonName}yy", transport) == 0);
    CU_ASSERT(mwv("MyCertName_MyClientID", "${CommonName}_${ClientID}", transport) == 0);
    CU_ASSERT(mwv("MyCertName_MyCertName", "${CommonName}_${CommonName}", transport) == 0);
    CU_ASSERT(mwv("xxMyCertName_MyCertNameyy", "xx${CommonName}_${CommonName}yy", transport) == 0);

    CU_ASSERT(mwv("*", "*${*}", NULL) == 0);
    CU_ASSERT(mwv("abc*", "*${*}", NULL) == 0);
    CU_ASSERT(mwv("$", "*${*}", NULL) == MWVRC_WildcardMismatch);

    /* Mismatch in leading text */
    CU_ASSERT(mwv("abcdef", "bcdef", transport) == MWVRC_LiteralMismatch);
    CU_ASSERT(mwv("abcdef", "ab", transport) == MWVRC_LiteralMismatch);
    CU_ASSERT(mwv("ab", "abcdef", transport) == MWVRC_LiteralMismatch);
    CU_ASSERT(mwv("xxMyUserID", "yy${UserID}", transport) == MWVRC_LiteralMismatch);

    /* Non terminated variable */
    CU_ASSERT(mwv("xxMyUserID", "xx${UserID", transport) == MWVRC_MalformedVariable);

    /* Userid not set */
    CU_ASSERT(mwv("MyUserID", "${UserID}", NULL) == MWVRC_NoUserID);
    transport->userid = NULL;
    CU_ASSERT(mwv("MyUserID", "${UserID}", transport) == MWVRC_NoUserID);
    transport->userid = "MyUserID";

    /* ClientID not set */
    CU_ASSERT(mwv("MyClientID", "${ClientID}", NULL) == MWVRC_NoClientID);
    transport->clientID = NULL;
    CU_ASSERT(mwv("MyClientID", "${ClientID}", transport) == MWVRC_NoClientID);
    transport->clientID = "MyClientID";

    /* CommonName not set */
    CU_ASSERT(mwv("MyCertName", "${CommonName}", NULL) == MWVRC_NoCommonName);
    transport->cert_name = NULL;
    CU_ASSERT(mwv("MyCertName", "${CommonName}", transport) == MWVRC_NoCommonName);
    transport->cert_name = "MyCertName";

    /* Unknown variable */
    CU_ASSERT(mwv("MyCertName", "${fred}", transport) == MWVRC_UnknownVariable);

    /* Replacement longer than string */
    transport->clientID = "This is a very very long clientID which is a lot longer than the destination string to match with it";
    CU_ASSERT(mwv("My", "${ClientID}", transport) == MWVRC_ClientIDMismatch);
    CU_ASSERT(mwv("My", "${ClientID}xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", transport) == MWVRC_ClientIDMismatch);
    CU_ASSERT(mwv("My", "${UserID}", transport) == MWVRC_UserIDMismatch);
    CU_ASSERT(mwv("Myxx", "My${UserID}", transport) == MWVRC_UserIDMismatch);
    CU_ASSERT(mwv("My", "${CommonName}", transport) == MWVRC_CommonNameMismatch);
    CU_ASSERT(mwv("Myxx", "My${CommonName}", transport) == MWVRC_CommonNameMismatch);
    CU_ASSERT(mwv("My", "*${ClientID}", transport) == MWVRC_WildcardMismatch);
    CU_ASSERT(mwv("My", "*${ClientID}xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", transport) == MWVRC_WildcardMismatch);

    /* Group name matching test */
    char *groupNames[3];
    char *gra = "GroupA";
    char *grb = "GroupB";
    char *grc = "GroupC";
    groupNames[0] = gra;
    groupNames[1] = grb;
    groupNames[2] = grc;
    CU_ASSERT(mwvGrp("GroupA", "${GroupID0}", transport, groupNames, 1) == 0);
    CU_ASSERT(mwvGrp("GroupA", "${GroupID0}", transport, NULL, 0) == MWVRC_NoGroupID);
    CU_ASSERT(mwvGrp("GroupB", "${GroupID0}", transport, groupNames, 1) == MWVRC_GroupIDMismatch);
    CU_ASSERT(mwvGrp("GroupA/GroupB", "${GroupID0}/${GroupID1}", transport, groupNames, 2) == 0);
    CU_ASSERT(mwvGrp("GroupA/GroupA", "${GroupID0}/${GroupID0}", transport, groupNames, 1) == 0);
    CU_ASSERT(mwvGrp("GroupA/GroupB", "${GroupID0}/${GroupID0}", transport, groupNames, 1) == MWVRC_GroupIDMismatch);
    CU_ASSERT(mwvGrp("GroupB/GroupA", "${GroupID1}/${GroupID1}", transport, groupNames, 2) == MWVRC_GroupIDMismatch);
    CU_ASSERT(mwvGrp("GroupB/GroupA", "${GroupID1}/${GroupID2}/${GroupID3}", transport, groupNames, 2) == MWVRC_MaxGroupID);

    transport->userid = "MyUserID";
    CU_ASSERT(mwvGrp("MyUserID/GroupA", "${UserID}/${GroupID0}", transport, groupNames, 1) == 0);
    CU_ASSERT(mwvGrp("MyUserID/GroupA", "${UserID}/${GroupID0}", transport, NULL, 0) == MWVRC_NoGroupID);
    CU_ASSERT(mwvGrp("MyUserID/GroupA", "${UserID}/${GroupID2}", transport, groupNames, 1) == MWVRC_MaxGroupID);
    CU_ASSERT(mwvGrp("MyUserID/GroupA/GroupB", "${UserID}/${GroupID0}/${GroupID1}", transport, groupNames, 2) == 0);
    CU_ASSERT(mwvGrp("MyUserID/GroupA/test/GroupB/sales/GroupC", "${UserID}/${GroupID0}/test/${GroupID1}/sales/${GroupID2}", transport, groupNames, 3) == 0);

    /* test for ${GroupID} */
    groupNames[0] = "GroupA";
    groupNames[1] = "Group_B";
    groupNames[2] = NULL;
    CU_ASSERT(mwvGrp("GroupA/Group_B", "${GroupID}/${GroupID}", transport, groupNames, 2) == 0);
    CU_ASSERT(mwvGrp("GroupA/Group_B/MyUserID", "${GroupID}/${GroupID}/${GroupID}", transport, groupNames, 2) == MWVRC_MaxGroupID);

    /* Test mix case - ${GroupID1} and ${GroupID} - test should fail */
    CU_ASSERT(mwvGrp("GroupA/Group_B", "${GroupID}/${GroupID0}", transport, groupNames, 2) == MWVRC_MalformedVariable);

    /* Case Insensitive Tests for ${GroupID} */
    CU_ASSERT(mwvGrpNoCaseCheck("groupa/group_b", "${GroupID}/${GroupID}", transport, groupNames, 2) == 0);
    CU_ASSERT(mwvGrpNoCaseCheck("GROUPA/groupa", "${GroupID0}/${GroupID0}", transport, groupNames, 1) == 0);
    CU_ASSERT(mwvGrpNoCaseCheck("GROUPA/GROUP_B", "${GroupID0}/${GroupID1}", transport, groupNames, 2) == 0);
    CU_ASSERT(mwvGrpNoCaseCheck("GroUPA/GrOuP_B", "${GroupID0}/${GroupID1}", transport, groupNames, 2) == 0);
    CU_ASSERT(mwvGrpNoCaseCheck("abc/GroupA/Group_B", "*${GroupID}/${GroupID}", transport, groupNames, 2) == 0);

    groupNames[0] = "assets";
    groupNames[1] = "cpc";
    transport->userid = "pdt";
    CU_ASSERT(mwvGrpNoCaseCheck("toc/XXXXX/pdt/hbs", "toc/${GroupID}/${UserID}/*", transport, groupNames, 1) == MWVRC_GroupIDMismatch);
    CU_ASSERT(mwvGrpNoCaseCheck("toc/asset/pdt/hbs", "toc/${GroupID}/${UserID}/*", transport, groupNames, 1) == MWVRC_GroupIDMismatch);
    CU_ASSERT(mwvGrpNoCaseCheck("toc/assets/pdt/hbs", "toc/${GroupID}/${UserID}/*", transport, groupNames, 1) == 0);
    CU_ASSERT(mwvGrpNoCaseCheck("toc/ASSETS/pdt/hbs", "toc/${GroupID}/${UserID}/*", transport, groupNames, 1) == 0);
    CU_ASSERT(mwvGrpNoCaseCheck("toc/assetX/pdt/hbs", "toc/${GroupID}/${UserID}/*", transport, groupNames, 1) == MWVRC_GroupIDMismatch);
    CU_ASSERT(mwvGrpNoCaseCheck("toc/assetXY/pdt/hbs", "toc/${GroupID}/${UserID}/*", transport, groupNames, 1) == MWVRC_GroupIDMismatch);


}

/*
 * Stub for debugging with groupID
 */
int regexm(const char * regex_str, const char * str, int expect_rc) {
    ism_regex_t regex;
    char xbuf [256];
    int  rc;

    rc = ism_regex_compile(&regex, regex_str);
    if (rc != expect_rc) {
        printf("regex compile error: regex=%s rc=%d error=%s\n", regex_str, rc,
                ism_regex_getError(rc, xbuf, sizeof xbuf, regex));
    }
    if (!rc) {
        rc = !!ism_regex_match(regex, str);
        if (rc != expect_rc) {
            printf("regex match error: regex=%s str=%s rc=%d\n", regex_str, str, rc);
        }
        ism_regex_free(regex);
    }
    return rc;
}

int regexmsubexpr(const char *regex_str, const char *str, int expect_subexprcnt, int use_nummatches,
                  int expect_rc, const char **expect_subexpr) {
    ism_regex_t regex;
    char xbuf [256];
    int  rc;
    int subexprcnt;

    rc = ism_regex_compile_subexpr(&regex, &subexprcnt, regex_str);
    if (rc != expect_rc) {
        printf("regex subexpr compile error: regex=%s rc=%d error=%s\n", regex_str, rc,
                ism_regex_getError(rc, xbuf, sizeof xbuf, regex));
    } else if (subexprcnt != expect_subexprcnt) {
        printf("regex subexpr compile unexpected subexprcnt: %d (expected %d)\n", subexprcnt, expect_subexprcnt);
        rc = ISMRC_Error;
    }
    if (!rc) {
        ism_regex_matches_t matches[use_nummatches];
        rc = !!ism_regex_match_subexpr(regex, str, use_nummatches, matches);
        if (rc != expect_rc) {
            printf("regex subexpr match error: regex=%s str=%s use_nummatches=%d rc=%d\n", regex_str, str, use_nummatches, rc);
        }
        if (!rc && expect_subexpr) {
            int subexprno = 0;
            const char *this_subexpr = expect_subexpr[subexprno];
            while(this_subexpr) {
                size_t length = matches[subexprno].endOffset-matches[subexprno].startOffset;
                if (strlen(this_subexpr) != length || memcmp(this_subexpr, &str[matches[subexprno].startOffset], length)) {
                    printf("subexpr %d (%.*s) doesn't match expected subexpr %s\n",
                           subexprno, (int)length, &str[matches[subexprno].startOffset], this_subexpr);
                    rc = ISMRC_Error;
                    break;
                }
                subexprno++;
                this_subexpr = expect_subexpr[subexprno];
            }
        }
        ism_regex_free(regex);
    }
    return rc;
}

/*
 * Test match functions
 */
void testRegex(void) {
    CU_ASSERT(regexm("^abc", "abc", 0) == 0);
    CU_ASSERT(regexm("abc", "xxxabc", 0) == 0);
    CU_ASSERT(regexm("^[da]:abcdef:", "a:abcdef:myapp", 0) == 0);
    CU_ASSERT(regexm("^[da]:abcdef:", "d:abcdef:type:dev", 0) == 0);
    CU_ASSERT(regexm("^[abc", "abd", 7) == 7);
    /* test subexpressions */
    CU_ASSERT(regexmsubexpr("(abc)", "abc", 1, 2, 0, (const char *[]){"abc", "abc", NULL}) == 0);
    CU_ASSERT(regexmsubexpr("(a)(bc)", "abcdef", 2, 3, 0, (const char *[]){"abc", "a", "bc", NULL}) == 0);
    CU_ASSERT(regexmsubexpr("^[^:]+:([^:]+):", "a:abcdef:myapp", 1, 1, 0,
                            (const char *[]){"a:abcdef:", NULL}) == 0);
    CU_ASSERT(regexmsubexpr("^[^:]+:([^:]+):", "a:abcdef:myapp", 1, 2, 0,
                            (const char *[]){"a:abcdef:", "abcdef", NULL}) == 0);
    CU_ASSERT(regexmsubexpr("^[^:]+:([^:]+):(myapp)", "a:abcdef:myapp", 2, 2, 0,
                            (const char *[]){"a:abcdef:myapp", "abcdef", NULL}) == 0);
}

void testValidNumber(void) {
    CU_ASSERT(ism_json_isValidNumber("123") == 1);
    CU_ASSERT(ism_json_isValidNumber("123.45") == 2);
    CU_ASSERT(ism_json_isValidNumber("123.45e02") == 2);
    CU_ASSERT(ism_json_isValidNumber("1.45E-02") == 2);
    CU_ASSERT(ism_json_isValidNumber("-123") == 1);
    CU_ASSERT(ism_json_isValidNumber("-1.45") == 2);
    CU_ASSERT(ism_json_isValidNumber("123A") == 0);
    CU_ASSERT(ism_json_isValidNumber("A123") == 0);
    CU_ASSERT(ism_json_isValidNumber("E-10") == 2);
    CU_ASSERT(ism_json_isValidNumber("1.e+123") == 2);
    CU_ASSERT(ism_json_isValidNumber("1.E-10") == 2);
    CU_ASSERT(ism_json_isValidNumber("10.10.0.5") == 0);
    CU_ASSERT(ism_json_isValidNumber(" 123") == 0);
    CU_ASSERT(ism_json_isValidNumber("+123") == 0);
}
