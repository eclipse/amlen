/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>
#include <engine.h>

extern int ism_config_json_parseServiceExportClientSetPayload(ism_http_t *http, ism_rest_api_cb callback);
extern int ism_config_json_parseServiceImportClientSetPayload(ism_http_t *http, ism_rest_api_cb callback);
extern int ism_config_json_deleteClientSet(ism_http_t * http, char * object, char * subinst, int force);
extern int ism_config_json_getClientSetStatus(ism_http_t * http, char * serviceType, char * requestID);

/* dummy engine functions */
XAPI int32_t ism_engine_exportResources (
        const char *                    pClientId,
        const char *                    pTopic,
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn)
{
    printf("Calling dummy engine export function\n");
    return ISMRC_AsyncCompletion;
}

XAPI int32_t ism_engine_importResources (
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn)
{
    printf("Calling dummy engine import function\n");
    return ISMRC_AsyncCompletion;
}

static int testNumExportValidate = 0;
static int testNumImportValidate = 0;

static int32_t validate_ExportClientSet(char * mbuf, int expRC) {

    int rc = ISMRC_OK;
    ism_http_t http;
    ism_http_content_t content;
    content.content_len = strlen(mbuf) + 1;
    content.content = (char *) malloc(content.content_len);
    if (!content.content) {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto VALIDATE_END;
    }
    strcpy(content.content, mbuf);

    http.content_count = 1;
    http.content = &content;

    rc = ism_config_json_parseServiceExportClientSetPayload(&http, NULL);

VALIDATE_END:

    testNumExportValidate++;
    if (expRC != rc)
    {
        printf("Test %2d for Export ClientSet Validation Failed: buf=\"%s\" exprc=%d rc=%d\n", testNumExportValidate,
            mbuf ? mbuf : "NULL", expRC, rc);
    }

    if (content.content)
        free(content.content);
    return rc;
}

static int32_t validate_ImportClientSet(char * mbuf, int expRC) {

    int rc = ISMRC_OK;
    ism_http_t http;
    ism_http_content_t content;
    content.content_len = strlen(mbuf) + 1;
    content.content = (char *) malloc(content.content_len);
    if (!content.content) {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto VALIDATE_END;
    }
    strcpy(content.content, mbuf);

    http.content_count = 1;
    http.content = &content;

    rc = ism_config_json_parseServiceImportClientSetPayload(&http, NULL);

VALIDATE_END:

    testNumImportValidate++;
    if (expRC != rc)
    {
        printf("Test %2d for Import ClientSet Validation Failed: buf=\"%s\" exprc=%d rc=%d\n", testNumImportValidate,
            mbuf ? mbuf : "NULL", expRC, rc);
    }
    if (content.content)
        free(content.content);
    return rc;
}

void test_validate_ImportExport(void) {

    int rc = ISMRC_OK;

    ism_field_t  f;

    ism_common_initUtil();

    f.type = VT_Long;
    f.val.l = (uint64_t)(uintptr_t) ism_engine_exportResources;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_exportResources_fnptr", &f);

    f.type = VT_Long;
    f.val.l = (uint64_t)(uintptr_t) ism_engine_importResources;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_importResources_fnptr", &f);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("ImportExport INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    /* Negative condition for Export ClientSet validation tests */
    char *mbuf = NULL;

    mbuf = "{\"FileName\":2.0, \"Password\":\"CUnit!!\", \"Topic\":\"^/CUnitTopic/\", \"ClientID\":\"^CUnitID\"}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{\"FileName\":\"\", \"Password\":\"CUnit!!\", \"Topic\":\"^/CUnitTopic/\", \"ClientID\":\"^CUnitID\"}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\": 2.0, \"Topic\":\"^/CUnitTopic/\", \"ClientID\":\"^CUnitID\"}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\":\"\", \"Topic\":\"^/CUnitTopic/\", \"ClientID\":\"^CUnitID\"}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\":\"CUnit!!\", \"Topic\": 2.0, \"ClientID\":\"^CUnitID\"}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\":\"CUnit!!\", \"Topic\":\"\", \"ClientID\":\"^CUnitID\"}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\":\"CUnit!!\", \"Topic\":\"^/CUnitTopic/\", \"ClientID\": 2.0}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\":\"CUnit!!\", \"Topic\":\"^/CUnitTopic/\", \"ClientID\":\"\"}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"PasswordLength\": 5, \"Topic\":\"^/CUnitTopic/\", \"ClientID\":\"^CUnitID\"}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_ArgNotValid) == ISMRC_ArgNotValid );

    /* Negative condition for Import ClientSet validation tests */
    mbuf = "{\"FileName\":2.0, \"Password\":\"CUnit!!\"}";
    CU_ASSERT( validate_ImportClientSet(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{\"FileName\":\"\", \"Password\":\"CUnit!!\"}";
    CU_ASSERT( validate_ImportClientSet(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\": 2.0}";
    CU_ASSERT( validate_ImportClientSet(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\":\"\"}";
    CU_ASSERT( validate_ImportClientSet(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* Test positive cases */
    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\":\"CUnit!!\", \"Topic\":\"^/CUnitTopic/\", \"ClientID\":\"^CUnitID\"}";
    CU_ASSERT( validate_ExportClientSet(mbuf, ISMRC_AsyncCompletion) == ISMRC_AsyncCompletion );

    mbuf = "{\"FileName\":\"CUnitOrgMove\", \"Password\": \"CUnit!!\"}";
    CU_ASSERT( validate_ImportClientSet(mbuf, ISMRC_AsyncCompletion) == ISMRC_AsyncCompletion );

    return;
}

/* global variables for tests, we use the same file & status names given below for all tests */
const char * c_exportDir;
const char * c_importDir;

const char * c_statusPrefix = "request_";
const char * c_statusSuffix = ".status";
const char * c_statusID_strng = "12345";
const char * c_expFile = "CUnitExportFile";
const char * c_impFile = "CUnitImportFile";

int c_expFileStrngLen;
int c_impFileStrngLen;

int c_expStatusStrngLen;
int c_impStatusStrngLen;

void test_RESTSetupImportExport(void) {

    /* Create a faux import/export file and status file to do tests */

    int rc = ISMRC_OK;
    const char * cmd = "touch";

    ism_field_t f;

    ism_common_initUtil();
    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("ImportExport INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    char *mbuf = NULL;
    int wrtcnt = 0;

    c_exportDir = ism_common_getStringConfig("ExportDir");
    c_importDir = ism_common_getStringConfig("ImportDir");

    if (!c_exportDir || !c_importDir) {
        c_exportDir = "/tmp/export";
        c_importDir = "/tmp/import";

        printf("Config directory items not initialized for import/export, using %s and %s for import and export respectively\n", c_importDir, c_exportDir);

        f.type = VT_String;
        f.val.s = (char *)c_exportDir;
        ism_common_setProperty(ism_common_getConfigProperties(),"ExportDir", &f);

        f.type = VT_String;
        f.val.s = (char *)c_importDir;
        ism_common_setProperty(ism_common_getConfigProperties(),"ImportDir", &f);

    }

    int expFileCmdLen = 0;
    int impFileCmdLen = 0;
    char * systemcmd = alloca(128);
    sprintf(systemcmd, "mkdir -p %s", c_exportDir);
    rc = system(systemcmd);
    if (rc == ISMRC_OK) {
        memset(systemcmd, 0, 128);
        sprintf(systemcmd, "mkdir -p %s", c_importDir);
        rc = system(systemcmd);
    }

    CU_ASSERT_FATAL(rc == ISMRC_OK);

    c_expFileStrngLen = strlen(c_exportDir) + strlen(c_expFile) + 2; // add the '/' and '\0'
    c_impFileStrngLen = strlen(c_importDir) + strlen(c_impFile) + 2; // add the '/' and '\0'


    c_expStatusStrngLen = strlen(c_exportDir) + strlen(c_statusPrefix) + strlen(c_statusID_strng) + strlen(c_statusSuffix) + 2; // add the '/' and '\0'
    c_impStatusStrngLen = strlen(c_importDir) + strlen(c_statusPrefix) + strlen(c_statusID_strng) + strlen(c_statusSuffix) + 2; // add the '/' and '\0'

    expFileCmdLen = c_expFileStrngLen + strlen(cmd) + 1; // add space between cmd and args
    impFileCmdLen = c_impFileStrngLen + strlen(cmd) + 1; // add space between cmd and args
    char expFileStrngCmd[expFileCmdLen];
    char impFileStrngCmd[impFileCmdLen];

    char expStatus[c_expStatusStrngLen];
    char impStatus[c_impStatusStrngLen];

    sprintf(expFileStrngCmd,"%s %s/%s", cmd, c_exportDir, c_expFile);
    expFileStrngCmd[expFileCmdLen-1] = '\0';
    sprintf(impFileStrngCmd, "%s %s/%s", cmd, c_importDir, c_impFile);
    impFileStrngCmd[impFileCmdLen-1] = '\0';

    rc = system(expFileStrngCmd);
    CU_ASSERT_FATAL(rc == ISMRC_OK);
    rc = system(impFileStrngCmd);
    CU_ASSERT_FATAL(rc == ISMRC_OK);

    sprintf(expStatus,"%s/%s%s%s", c_exportDir, c_statusPrefix, c_statusID_strng, c_statusSuffix);
    expStatus[c_expStatusStrngLen-1] = '\0';
    sprintf(impStatus, "%s/%s%s%s", c_importDir, c_statusPrefix, c_statusID_strng, c_statusSuffix);
    impStatus[c_impStatusStrngLen-1] = '\0';

    FILE *f_exportStatus = fopen(expStatus, "w+");
    FILE *f_importStatus = fopen(impStatus, "w+");
    CU_ASSERT_FATAL((f_exportStatus != NULL) && (f_importStatus != NULL));

    mbuf = "{\"RequestID\":12345,\"FilePath\":\"/tmp/export/CUnitExportFile\",\"Status\":0,\"RetCode\":0}";
    wrtcnt = fwrite(mbuf, strlen(mbuf), 1, f_exportStatus);
    CU_ASSERT(wrtcnt == 1);

    mbuf = "{\"RequestID\":12345,\"FilePath\":\"/tmp/import/CUnitImportFile\",\"Status\":0,\"RetCode\":0}";
    wrtcnt = fwrite(mbuf, strlen(mbuf), 1, f_importStatus);
    CU_ASSERT(wrtcnt == 1);

    fclose(f_exportStatus);
    fclose(f_importStatus);
}

static int get_ImportExportStatus(const char * serviceType, const char * requestID) {

    int rc = ISMRC_OK;
    ism_http_t http;

    memset(&http, 0, sizeof(http));
    http.outbuf.buf = (char *) malloc(sizeof(char) * 4096);
    http.outbuf.len = 4096;
    http.outbuf.inheap = 1;
    if (!http.outbuf.buf) {
        rc = ISMRC_AllocateError;
        goto TEST_END;
    }
    memset(http.outbuf.buf, 0, http.outbuf.len);

    rc = ism_config_json_getClientSetStatus(&http, (char *) serviceType, (char *) requestID);
    if (rc == ISMRC_OK) {
        /* Check the msgcode returned is an expected value */
        json_t * root = NULL;
        json_error_t error;
        root = json_loads(http.outbuf.buf, 0, &error);
        if (!root) {
            rc = ISMRC_Error;
        } else {
            json_t * retcode_objval = json_object_get(root, ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETCODE);
            json_t * statusCode_objval = json_object_get(root, ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS);
            int retcode = -1;
            int statusCode = -1;
            const char * code = NULL;
            if (retcode_objval && statusCode_objval) {
                retcode = json_integer_value(retcode_objval);
                statusCode = json_integer_value(statusCode_objval);
                code = json_string_value(json_object_get(root, "Code"));
                if (retcode == 0) {
                    if (statusCode == 0) {
                        CU_ASSERT(strcmp(code, "CWLNA6176") == 0);
                    } else if (statusCode == 1) {
                        CU_ASSERT(strcmp(code, "CWLNA6177") == 0);
                    }
                } else {
                    CU_ASSERT(strcmp(code, "CWLNA6174") == 0);
                }
            } else {
                rc = ISMRC_NotFound;
            }

            json_decref(root);
            root = NULL;

        }
    }
TEST_END:
    if (http.outbuf.inheap)
        free(http.outbuf.buf);
    return rc;
}

void test_RESTGetImportExport(void) {

    FILE * f_exportStatus = NULL;
    FILE * f_importStatus = NULL;
    int wrtcnt = 0;
    const char * BAD_ID_STRNG = "9999";
    char * mbuf = NULL;

    char expStatus[c_expStatusStrngLen];
    char impStatus[c_impStatusStrngLen];

    /* EXPORT Tests - Setup should have already been done from 1st test run so these globals should be initialized */
    CU_ASSERT( get_ImportExportStatus("export", c_statusID_strng) == ISMRC_OK ); /* setup has already given case for 1st test i.e. RetCode is 0 */

    sprintf(expStatus,"%s/%s%s%s", c_exportDir, c_statusPrefix, c_statusID_strng, c_statusSuffix);
    expStatus[c_expStatusStrngLen-1] = '\0';
    f_exportStatus = fopen(expStatus, "w+");
    CU_ASSERT(f_exportStatus != NULL);
    mbuf = "{\"RequestID\":12345,\"FilePath\":\"/tmp/export/CUnitExportFile\",\"Status\":0,\"RetCode\":113}";
    wrtcnt = fwrite(mbuf, strlen(mbuf), 1, f_exportStatus);
    CU_ASSERT(wrtcnt == 1);
    fclose(f_exportStatus);
    CU_ASSERT( get_ImportExportStatus("export", c_statusID_strng) == ISMRC_OK );

    CU_ASSERT( get_ImportExportStatus("export", BAD_ID_STRNG) == 6178);

    /* IMPORT Tests - Setup should have already been done from 1st test run so these globals should be initialized */
    CU_ASSERT( get_ImportExportStatus("import", c_statusID_strng) == ISMRC_OK ); /* setup has already given case for 1st test i.e. RetCode is 0 */

    sprintf(impStatus, "%s/%s%s%s", c_importDir, c_statusPrefix, c_statusID_strng, c_statusSuffix);
    impStatus[c_impStatusStrngLen-1] = '\0';
    f_importStatus = fopen(impStatus, "w+");
    CU_ASSERT(f_importStatus != NULL);
    mbuf = "{\"RequestID\":12345,\"FilePath\":\"/tmp/import/CUnitImportFile\",\"Status\":0,\"RetCode\":113}";
    wrtcnt = fwrite(mbuf, strlen(mbuf), 1, f_importStatus);
    CU_ASSERT(wrtcnt == 1);
    fclose(f_importStatus);
    CU_ASSERT( get_ImportExportStatus("import", c_statusID_strng) == ISMRC_OK );

    CU_ASSERT( get_ImportExportStatus("import", BAD_ID_STRNG) == 6178);

    return;
}


static int delete_ImportExportStatus(const char * serviceType, const char * requestID) {
    int rc = ISMRC_OK;
    ism_http_t http;

    memset(&http, 0, sizeof(http));
    http.outbuf.buf = (char *) malloc(sizeof(char) * 4096);
    http.outbuf.len = 4096;
    http.outbuf.inheap = 1;
    if (!http.outbuf.buf) {
        rc = ISMRC_AllocateError;
        goto TEST_END;
    }
    memset(http.outbuf.buf, 0, http.outbuf.len);

    rc = ism_config_json_deleteClientSet(&http, (char *) serviceType, (char *) requestID, 0);
TEST_END:
    if (http.outbuf.inheap)
        free(http.outbuf.buf);
    return rc;
}

void test_RESTDeleteImportExport(void) {
    FILE * f_exportStatus = NULL;
    FILE * f_importStatus = NULL;
    int wrtcnt = 0;
    char * mbuf = NULL;
    const char * BAD_ID_STRNG = "9999";

    char expStatus[c_expStatusStrngLen];
    char impStatus[c_impStatusStrngLen];

    char expFileStrng[c_expFileStrngLen];
    char impFileStrng[c_impFileStrngLen];

    /* EXPORT Tests - Setup should have already been done from 1st test run so these globals should be initialized */
    CU_ASSERT( delete_ImportExportStatus("export", c_statusID_strng) == 6176 );

    sprintf(expStatus,"%s/%s%s%s", c_exportDir, c_statusPrefix, c_statusID_strng, c_statusSuffix);
    expStatus[c_expStatusStrngLen-1] = '\0';
    f_exportStatus = fopen(expStatus, "w+");
    mbuf = "{\"RequestID\":12345,\"FilePath\":\"/tmp/export/CUnitExportFile\",\"Status\":1,\"RetCode\":113}";
    wrtcnt = fwrite(mbuf, strlen(mbuf), 1, f_exportStatus);
    CU_ASSERT(wrtcnt == 1);
    fclose(f_exportStatus);
    CU_ASSERT( delete_ImportExportStatus("export", c_statusID_strng) == ISMRC_OK );

    /* faux write status file again but no export file (assert previously deleted) - should still work */
    sprintf(expFileStrng,"%s/%s", c_exportDir, c_expFile);
    expFileStrng[c_expFileStrngLen-1] = '\0';
    unlink(expFileStrng);
    CU_ASSERT(errno == ENOENT);
    f_exportStatus = fopen(expStatus, "w+");
    mbuf = "{\"RequestID\":12345,\"FilePath\":\"/tmp/export/CUnitExportFile\",\"Status\":1,\"RetCode\":113}";
    wrtcnt = fwrite(mbuf, strlen(mbuf), 1, f_exportStatus);
    CU_ASSERT(wrtcnt == 1);
    fclose(f_exportStatus);
    CU_ASSERT( delete_ImportExportStatus("export", c_statusID_strng) == ISMRC_OK );

    CU_ASSERT( delete_ImportExportStatus("export", BAD_ID_STRNG) == 6178);

    /* IMPORT Tests - Setup should have already been done from 1st test run so these globals should be initialized */
    CU_ASSERT( delete_ImportExportStatus("import", c_statusID_strng) == 6176 );

    sprintf(impStatus, "%s/%s%s%s", c_importDir, c_statusPrefix, c_statusID_strng, c_statusSuffix);
    impStatus[c_impStatusStrngLen-1] = '\0';
    f_importStatus = fopen(impStatus, "w+");
    CU_ASSERT(f_importStatus != NULL);
    mbuf = "{\"RequestID\":12345,\"FilePath\":\"/tmp/import/CUnitImportFile\",\"Status\":1,\"RetCode\":113}";
    wrtcnt = fwrite(mbuf, strlen(mbuf), 1, f_importStatus);
    CU_ASSERT(wrtcnt == 1);
    fclose(f_importStatus);
    CU_ASSERT( delete_ImportExportStatus("import", c_statusID_strng) == ISMRC_OK );

    /* faux write status file again but no import file (assert previously deleted) - should still work */
    sprintf(impFileStrng, "%s/%s", c_importDir, c_impFile);
    impFileStrng[c_impFileStrngLen-1] = '\0';
    unlink(impFileStrng);
    CU_ASSERT(errno == ENOENT);
    f_importStatus = fopen(impStatus, "w+");
    mbuf = "{\"RequestID\":12345,\"FilePath\":\"/tmp/import/CUnitImportFile\",\"Status\":1,\"RetCode\":113}";
    wrtcnt = fwrite(mbuf, strlen(mbuf), 1, f_importStatus);
    CU_ASSERT(wrtcnt == 1);
    fclose(f_importStatus);
    CU_ASSERT( delete_ImportExportStatus("import", c_statusID_strng) == ISMRC_OK );

    CU_ASSERT( delete_ImportExportStatus("import", BAD_ID_STRNG) == 6178);

    return;
}

void test_RESTCleanupImportExport(void) {
    int rc = system("rm -rf /tmp/import /tmp/export");
    printf("rc = %d\n", rc);
    return;
}
