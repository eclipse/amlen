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
 * Test Inactive cluster member remove REST call
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <monitoring.h>
#include <cluster.h>

extern int ism_common_initServer();

int clrmtestno = 0;

ismCluster_ViewInfo_t *clView = NULL;

void init_clTest(void) {
    clView = (ismCluster_ViewInfo_t *)calloc(1, sizeof(ismCluster_ViewInfo_t));
    clView->numRemoteServers = 1;
    ismCluster_RSViewInfo_t * server = (ismCluster_RSViewInfo_t *)calloc(1, sizeof(ismCluster_RSViewInfo_t));
    server->pServerName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_monitoring_misc,1000),"GoodServer");
    server->pServerUID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_monitoring_misc,1000),"x2a6b8f6");
    server->state = ISM_CLUSTER_RS_STATE_INACTIVE;
    clView->pRemoteServers = server;
}

void term_clTest(void) {
    ismCluster_RSViewInfo_t * server = clView->pRemoteServers;
    ism_common_free(ism_memory_monitoring_misc,((void *)server->pServerName));
    ism_common_free(ism_memory_monitoring_misc,((void *)server->pServerUID));
    free((void *)server);
    free((void *)clView);
}

/* Dummy cluster function - for CUNIT tests to compile */
int ism_cluster_getView(ismCluster_ViewInfo_t **pView) {
    *pView = clView;
    return ISMRC_OK;
}

int ism_cluster_freeView(ismCluster_ViewInfo_t *pView) {
    return ISMRC_OK;
}

int ism_cluster_removeRemoteServer(const ismCluster_RemoteServerHandle_t phServerHandle) {
    return ISMRC_OK;
}

static int process_inactiveMemberRemoveRequest(char *mbuf, int exprc) {
    int rc = ISMRC_OK;

    ism_http_t http = {0};
    ism_http_content_t http_content = {0};
    char http_outbuf[2048];
    char *contentBuffer = ism_common_strdup(ISM_MEM_PROBE(ism_memory_monitoring_misc,1000),mbuf);

    http_content.content = contentBuffer;
    http_content.content_len = strlen(contentBuffer);

    http.content_count = 1;
    http.content = &http_content;
    http.outbuf.buf = http_outbuf;
    http.outbuf.len = sizeof(http_outbuf);

    /* invoke POST to configure */
    rc = ism_monitoring_removeInactiveClusterMember(&http);

    clrmtestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", clrmtestno, mbuf ? mbuf : "NULL", exprc, rc);
    }

    ism_common_free(ism_memory_monitoring_misc, contentBuffer);
    return rc;
}

void test_inactiveCMRemove(void)
{
    int rc = ISMRC_OK;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("test_inactiveCMRemove INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    init_clTest();

    char *mbuf = "{}";
    CU_ASSERT( process_inactiveMemberRemoveRequest(mbuf, ISMRC_InvalidObjectConfig) == ISMRC_InvalidObjectConfig );

    mbuf = "{ \"ServerName\":\"TestServer\"";
    CU_ASSERT( process_inactiveMemberRemoveRequest(mbuf, 6001) == 6001 );

    mbuf = "{ \"Version\":\"v1\" }";
    CU_ASSERT( process_inactiveMemberRemoveRequest(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    mbuf = "{ \"ServerName\":\"TestServer\" }";
    CU_ASSERT( process_inactiveMemberRemoveRequest(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    mbuf = "{ \"ServerUID\":\"TestServer\" }";
    CU_ASSERT( process_inactiveMemberRemoveRequest(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    mbuf = "{ \"Version\":\"v1\", \"ServerUID\":\"TestServer\" }";
    CU_ASSERT( process_inactiveMemberRemoveRequest(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    mbuf = "{ \"XXXX\":\"TestServer\" }";
    CU_ASSERT( process_inactiveMemberRemoveRequest(mbuf, ISMRC_ArgNotValid) == ISMRC_ArgNotValid );

    mbuf = "{ \"ServerName\":\"TestServer\", \"ServerUID\":\"TestUID\" }";
    CU_ASSERT( process_inactiveMemberRemoveRequest(mbuf, ISMRC_ClusterMemberNotFound) == ISMRC_ClusterMemberNotFound );

    mbuf = "{ \"ServerName\":\"GoodServer\", \"ServerUID\":\"x2a6b8f6\" }";
    CU_ASSERT( process_inactiveMemberRemoveRequest(mbuf, ISMRC_OK) == ISMRC_OK );

    term_clTest();
}


