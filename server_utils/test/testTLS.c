/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
#include <testTLS.h>
#include <log.h>

#ifdef TRACE_COMP
#undef TRACE_COMP
#endif
#include <ssl.c>
#include <tls.c>
extern int g_verbose;
void snitest(void);

CU_TestInfo ISM_Util_CUnit_TLS[] = {
    { "csvTest", csvtest },
    { "pskTest", psktest },
    { "timeTest", timetest },
    { "crlTest", crltest },
    { "sniTest", snitest },
    CU_TEST_INFO_NULL
};

int csvfld(const char * str, int exp, const char * f1, const char * f2) {
    char * res;
    int    good;
    char * xx = alloca(strlen(str)+1);
    strcpy(xx, str);
    res = csvfield(xx, exp);
    good = !strcmp(xx, f1);
    if (good) {
        if (f2 == NULL)
            good = res == NULL;
        else
            good = !strcmp(res, f2);
    }
    if (!good) {
        printf("str=[%s] f1=[%s] res=[%s] f2=[%s]\n", xx, f1, res, f2);
    }
    return good;
}


void csvtest(void) {
    CU_ASSERT(csvfld("abc,def", 1,       "abc",   "def"));
    CU_ASSERT(csvfld("\"abc\",def", 1,   "abc",   "def"));
    CU_ASSERT(csvfld("\"abc\"\"d\",def", 1,"abc\"d", "def"));
    CU_ASSERT(csvfld("\"abc\"d,efg", 1,  "abc",   NULL));
    CU_ASSERT(csvfld("\"abc\"d,efg", 0,  "abc",   "d,efg"));
    CU_ASSERT(csvfld("abc,def,", 0,      "abc",   NULL));
    CU_ASSERT(csvfld("\"abc\",def", 0,   "abc",   NULL));
    CU_ASSERT(csvfld("abc,\"def\"", 0,   "abc",   NULL));
}

void psktest(void) {
    /* TODO */
}

/* A CRL */
char * crlsrc =
"-----BEGIN X509 CRL-----\n"
"MIIEqDCCApACAQEwDQYJKoZIhvcNAQELBQAwTzETMBEGA1UEChMKRWxlY3Ryb2x1\n"
"eDEdMBsGA1UECxMUQ29ubmVjdGVkIEFwcGxpYW5jZXMxGTAXBgNVBAMTEFFBIEFw\n"
"cGxpYW5jZXMgQ0EXDTE3MDIyODE1MDgwNVoXDTE3MDQyNTE1MjgwNVowggGqMDIC\n"
"E1IAAADq+b/xxzJ9bYgAAAAAAOoXDTE3MDEyNTEyMDMwMFowDDAKBgNVHRUEAwoB\n"
"BDAyAhNSAAAA6XNOZcXvO3b8AAAAAADpFw0xNzAxMjUxMjAzMDBaMAwwCgYDVR0V\n"
"BAMKAQQwMgITUgAAAOhxQa6tjFuxpQAAAAAA6BcNMTcwMTI1MTIwMzAwWjAMMAoG\n"
"A1UdFQQDCgEEMDICE1IAAADrStj5ENDjbaUAAAAAAOsXDTE3MDEyNTEyMDIwMFow\n"
"DDAKBgNVHRUEAwoBBDAkAhNSAAAAZcOBWMq+sjynAAAAAABlFw0xNjExMDMxNDQw\n"
"MDBaMDICE1IAAACYPCpxL9ihvBYAAAAAAJgXDTE2MTAyNTEyMzcwMFowDDAKBgNV\n"
"HRUEAwoBBjAkAhNSAAAAmiNKZKyG/fp/AAAAAACaFw0xNjEwMjUwNzM2MDBaMDIC\n"
"E1IAAABmGvTH7Xn48o0AAAAAAGYXDTE2MTAxODE0MTcwMFowDDAKBgNVHRUEAwoB\n"
"ATAkAhNSAAAAUQDRiIzs+EcnAAAAAABRFw0xNjEwMTMxNDU1MDBaoF8wXTAfBgNV\n"
"HSMEGDAWgBSrSw3pjKQznXjOPoEt7oRhJnLeYDAQBgkrBgEEAYI3FQEEAwIBADAK\n"
"BgNVHRQEAwIBRjAcBgkrBgEEAYI3FQQEDxcNMTcwMzI4MTUxODA1WjANBgkqhkiG\n"
"9w0BAQsFAAOCAgEAVmuaVICE3JV2f/1JR6jnaYyBTmpVFsojXa9J96io/HxR848X\n"
"9lqr/SlpKknxzgmi/8IezWgZ9Kdwq1+00V72iqPzNs0hPGTXUpmROVZ4Z27WWZuV\n"
"7R2+BbZ4MSuR2wJxSHmvb1YbXKIMr/nEGK5ZwiZXCfpPa1gtYUQbFWXF4CKihSZz\n"
"ijtDPhsWQ9cusDcP+f1K3MKbJBslu2B2T1LOlFDfodIBPodqP9ixRSAATbyjMtDD\n"
"i87iG3SrSaWXriSLMq/0gVrGdx/w7YLGSLlC4jOCOXrzX9xjB8gu4BN5BQuQ3xmF\n"
"oXlAhrZYDUJuGwNZU3c2+5o+EuLQY0yDPUAG6YY8uf1FDFg0W0BgPnod5n5Mvs5z\n"
"asW4dVqIjSonaUcUNs0RvbIpCbo9dnLT8SgaOvUmCKRtZyGUw+8sb9YC4dmgJLIu\n"
"OBL02JBFm26GWQeyjY/iMoShbnAP3rMJdG2A19rrFlby4NvXNpHzv6T6BH8Wn0we\n"
"bUCopa0IxYg7LBj/dKntV1A2r+XdOugDTQC/e4bvbHG4NpovRJqLyEyEHIUGWSpC\n"
"URBJ1HdVbhLW3L0cKE+UEoessIyX6PgEKywL25BQLZIiKJ6+npBwOGCbQwXld6AO\n"
"9+nD3O1HfLY94sKRZHe5j1g2TTqAiNq57etPEvk8+Zhr/l5t/bcxDOzx/Yw=\n"
"-----END X509 CRL-----\n";

/*
 * Test the ism_ssl_convertTime() method
 */
void timetest(void) {
    ASN1_TIME * gentime = (ASN1_TIME *)ASN1_STRING_type_new(V_ASN1_GENERALIZEDTIME);
    ASN1_TIME * utctime = ASN1_TIME_new();
    ism_ts_t * ts;
    char timest[32];

    CU_ASSERT(ASN1_TIME_set_string(gentime, "20170102030405Z") == 1);
    CU_ASSERT(ASN1_TIME_set_string(utctime, "170102030406Z") == 1);
    ts = ism_common_openTimestamp(ISM_TZF_UNDEF);

    ism_common_setTimestamp(ts, ism_ssl_convertTime(gentime));
    ism_common_formatTimestamp(ts, timest, sizeof timest, 6, ISM_TFF_ISO8601|ISM_TFF_FORCETZ);
    // printf("gentime=%s\n", timest);
    CU_ASSERT(!strcmp(timest, "2017-01-02T03:04:05Z"));

    ism_common_setTimestamp(ts, ism_ssl_convertTime(utctime));
    ism_common_formatTimestamp(ts, timest, sizeof timest, 6, ISM_TFF_ISO8601|ISM_TFF_FORCETZ);
    // printf("utctime=%s\n", timest);
    CU_ASSERT(!strcmp(timest, "2017-01-02T03:04:06Z"));
}

extern ism_logWriter_t * g_logwriter[LOGGER_Max+1];
extern int ism_log_init(void);
extern int ism_log_term(void);
extern int ism_log_setWriterCallback(ism_logWriter_t * lw, void (*callback)(const char *));

const char * g_logmsg;
void savelog(const char * msg) {
    if (g_logmsg)
        ism_common_free(ism_memory_utils_misc,(char *)g_logmsg);
    if (msg)
        g_logmsg = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),msg);
    else
        g_logmsg = NULL;
    //if (g_logmsg)
    //    printf(">>>> %s\n", g_logmsg);
}

void waitlog(int millis) {
    int i;
    for (i=0; i<millis; i += 50) {
        if (g_logmsg)
            break;
        usleep(50000);
    }
}

/*
 * Test some CRL methods
 */
void crltest(void) {
    tlsCrl_t * crlobj;
    X509_CRL * crl = readCRL(crlsrc, strlen(crlsrc));
    CU_ASSERT(crl != NULL);
    if (g_verbose)
        ism_common_setTraceLevel(8);

    ism_log_init();
    ism_log_setWriterCallback(g_logwriter[0], savelog);
    savelog(NULL);

    crlobj = newCrlObj("file:///test.crl.pem");
    CU_ASSERT(crlobj != NULL);
    if (crlobj) {
        CU_ASSERT(!strcmp(crlobj->name, "file:///test.crl.pem"));
        crlobj->last_update_ts = ism_common_currentTimeNanos();
    }
    if (crl) {
        enableCRL(crl, "myorg", crlobj);
        waitlog(1000);
        CU_ASSERT(g_logmsg && strstr(g_logmsg, "CWLNA0985"));
        CU_ASSERT(g_logmsg && strstr(g_logmsg, "Number=70"));
        CU_ASSERT(g_logmsg && strstr(g_logmsg, "Issuer=\"QA Appliances CA\""));
    }
    usleep(50000);
    ism_log_term();
    if (g_verbose)
        ism_common_setTraceLevel(2);
}

const char * calist =
    "-----BEGIN CERTIFICATE-----\n"
    "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\n"
    "fO3ES//9R80pATfr1WDBJXPBGScw6QiQu+Vi8B8=\n"
    "-----END CERTIFICATE-----\n";

const char * cert =
    "\n-----BEGIN CERTIFICATE-----\n"
    "MIIB+zCCAVygAwIBAgIUEefFEwf/UsBmpPaiEqRuvMMgFW0wCgYIKoZIzj0EAwIw\n"
    "DzENMAsGA1UEAwwEeG9yZzAeFw0xOTA1MTYxMzIzNTlaFw0yOTA1MTMxMzIzNTla\n"
    "MA8xDTALBgNVBAMMBHhvcmcwgZswEAYHKoZIzj0CAQYFK4EEACMDgYYABAFk5uCG\n"
    "GdrsXo4AKpz4ckRDfwdpe70SJvEzE5trfqPaufWdhulpUoq2Kehj21r9+ECAklC6\n"
    "Q/y5NAPRKP5sa8ntmgDE3FCF6aaE5uG7EBJk8YHyHeAxYrZsteC/42fe0D9I1XqD\n"
    "xFP3oB7zxdLYtUJqFeI2s+tZBYeao52yO5/ixhyuO6NTMFEwHQYDVR0OBBYEFHLh\n"
    "eJRoSIMnc/Zhwmt6TECs0Uu3MB8GA1UdIwQYMBaAFHLheJRoSIMnc/Zhwmt6TECs\n"
    "0Uu3MA8GA1UdEwEB/wQFMAMBAf8wCgYIKoZIzj0EAwIDgYwAMIGIAkIBb0Wzo15O\n"
    "6VPmou0chb1jhgal72ykouutJxmLfLhKo6Da5zv4MWtp2gknChwn2TLE8DvYeXZt\n"
    "HEVvGicpGaQNB8sCQgFrembPtZodJ33K/RrBB6Jy4eY89Vw1JEjIF0vVaVBy4wMn\n"
    "qPgxIRNmntwdZl7PDL0Jd0R8qMVjUL5ufV6HZbiqlg==\n"
    "-----END CERTIFICATE-----\n";

const char * key =
    "-----BEGIN EC PARAMETERS-----\n"
    "BgUrgQQAIw==\n"
    "-----END EC PARAMETERS-----\n"
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MIHcAgEBBEIAPAnb4R6DnM26Q1K9+t0Pepq6b64ldyDAR+G2/lSgIj45uzvmNQlC\n"
    "0Zesq+nBLw/oFlahb2GMdULjpHW4LhqWaA2gBwYFK4EEACOhgYkDgYYABAFk5uCG\n"
    "GdrsXo4AKpz4ckRDfwdpe70SJvEzE5trfqPaufWdhulpUoq2Kehj21r9+ECAklC6\n"
    "Q/y5NAPRKP5sa8ntmgDE3FCF6aaE5uG7EBJk8YHyHeAxYrZsteC/42fe0D9I1XqD\n"
    "xFP3oB7zxdLYtUJqFeI2s+tZBYeao52yO5/ixhyuOw==\n"
    "-----END EC PRIVATE KEY-----\n";

extern int g_cunitCRLcount;

extern int g_traceInited;
extern void ism_common_initTrace(void);

xUNUSED void snitest(void) {
    int rc;
    g_disableCRL = 1;
    g_keystore = "keystore";
    g_truststore = "truststore";

#if OPENSSL_VERSION_NUMBER < 0x10100000L 
    printf("Skipping sni test due to openssl <1.1\n");
    return;
#endif 

    if (g_verbose)
        ism_common_setTraceLevel(8);
    ism_common_setTraceFile("stdout", 0);
       
    ism_log_init();
    ism_ssl_SNI_init();
    //Test NULL Key
    rc = ism_ssl_setSNIConfig("xorg", cert, NULL, NULL, calist, 1, NULL);
    CU_ASSERT(rc == ISMRC_CreateSSLContext);

    rc = ism_ssl_setSNIConfig("xorg", cert, key, NULL, calist, 1, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(g_cunitCRLcount == 0);
    tlsOrgConfig_t * orgConfig = ism_common_getHashMapElement(orgConfigMap, "xorg", 0);
    CU_ASSERT(orgConfig != NULL);
    if (orgConfig) {
        tlsCrl_t * crl1 = newCrlObj("cunit://crl/1");
        tlsCrl_t * crl2 = newCrlObj("cunit://crl/2");
        tlsCrl_t * crl3 = newCrlObj("cunit://crl/3");
        CU_ASSERT(crl3 != NULL);
        CU_ASSERT(orgConfig->useCount == 1);
        crl1->next = crl2;
        crl2->next = crl3;
        crl1->inprocess = 0;
        crl2->inprocess = 0;
        crl3->inprocess = 0;
        orgConfig->crl = crl1;
        ism_time_t now = ism_common_currentTimeNanos();
        crl2->last_update_ts = now;
        crl2->next_update_ts = now;
        crl2->valid_ts = now;
        crl2->state = 2;
        rc = ism_ssl_setSNIConfig("xorg", cert+1, key, NULL, calist, 1, NULL);   /* cert+1 to force difference */

        orgConfig = ism_common_getHashMapElement(orgConfigMap, "xorg", 0);
        CU_ASSERT(orgConfig != NULL);
        CU_ASSERT(orgConfig->useCount == 1);
        CU_ASSERT(rc == 0);
        CU_ASSERT(g_cunitCRLcount == 3);
        CU_ASSERT(crl2->last_update_ts == 0);
        CU_ASSERT(crl2->last_update_ts == 0);
        CU_ASSERT(crl2->valid_ts == 0);
        CU_ASSERT(crl2->state == 0);
        freeOrgConfig("xorg");
    }
    usleep(50000);
    ism_log_term();
     if (g_verbose)
        ism_common_setTraceLevel(2);
}

