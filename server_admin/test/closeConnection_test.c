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

/*
 * Test close connection
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <transport.h>

extern int ism_common_initServer();
extern int ism_admin_closeConnection(ism_http_t *http);

int clcontestno = 0;

static int process_closeConnectionRequest(char *mbuf, int exprc) {
    int rc = ISMRC_OK;

    ism_http_t http = {0};
    ism_http_content_t http_content = {0};
    char http_outbuf[2048];
    char *contentBuffer = strdup(mbuf);

    http_content.content = contentBuffer;
    http_content.content_len = strlen(contentBuffer);

    http.content_count = 1;
    http.content = &http_content;
    http.outbuf.buf = http_outbuf;
    http.outbuf.len = sizeof(http_outbuf);

    /* invoke POST to configure */
    rc = ism_admin_closeConnection(&http);

    clcontestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", clcontestno, mbuf ? mbuf : "NULL", exprc, rc);
    }

    free(contentBuffer);
    return rc;
}

void test_closeConnection(void)
{
    int rc = ISMRC_OK;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("test_inactiveCMRemove INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    char *mbuf = "{}";
    CU_ASSERT( process_closeConnectionRequest(mbuf, ISMRC_InvalidObjectConfig) == ISMRC_InvalidObjectConfig );

    mbuf = "{ \"ClientID\":\"TestID\"";
    CU_ASSERT( process_closeConnectionRequest(mbuf, 6001) == 6001 );

    mbuf = "{ \"Version\":\"v1\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, 6204) == 6204 );

    mbuf = "{ \"XXXX\":\"TestServer\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, ISMRC_ArgNotValid) == ISMRC_ArgNotValid );

    mbuf = "{ \"ClientID\":\"TestID\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, 6136) == 6136 );

    mbuf = "{ \"UserID\":\"TestID\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, 6136) == 6136 );

    mbuf = "{ \"ClientAddress\":\"TestID\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, 6136) == 6136 );

    mbuf = "{ \"ClientID\":\"ClientA\", \"ClientAddress\":\"TestID\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, 6136) == 6136 );

    mbuf = "{ \"ClientID\":\"ClientA\", \"UserID\":\"UserA\", \"ClientAddress\":\"TestID\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, 6136) == 6136 );

    mbuf = "{ \"ClientID\":\"ClientA\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClientID\":\"Clien*\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"UserID\":\"UserA\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"UserID\":\"Use*\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClientAddress\":\"9.3.179.167\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClientAddress\":\"9.3.17*\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClientID\":\"ClientA\", \"UserID\":\"UserA\", \"ClientAddress\":\"9.3.179.167\" }";
    CU_ASSERT( process_closeConnectionRequest(mbuf, ISMRC_OK) == ISMRC_OK );

}


