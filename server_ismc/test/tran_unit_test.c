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
 * File: tran_unit_test.c
 * Component: server
 * SubComponent: server_ismc
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <ismutil.h>
#include <ismc_p.h>
#include <tran_unit_test.h>

/**
 * Test array for MQ bridge transaction tests.
 */
CU_TestInfo ISM_tran_tests[] = {
		{ "--- Testing create record API          ---", testCreate  },
		{ "--- Testing destroy API                ---", testDestroy },
		{ "--- Testing get records API            ---", testList    },
		CU_TEST_INFO_NULL
};

static const char * CID = "TRAN_UNIT_TEST_CLIENT_ID";

void testCreate(void) {
	ismc_connection_t * conn;
	ismc_session_t * session = NULL;
	ismc_manrec_t recid;
	char * server = getenv("ISMServer");
	char * client_port = getenv("ISMPort");
	char buf[1024];

	CU_ASSERT(ismc_createManagerRecord(NULL, "test", 4) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_NullPointer);
	CU_ASSERT(ismc_createManagerRecord((ismc_session_t *)"test1234", "test", 4) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_ObjectNotValid);

	CU_ASSERT(ismc_createXARecord(NULL, NULL, "test", 4) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_NullPointer);
	CU_ASSERT(ismc_createXARecord((ismc_session_t *)"test1234", NULL, "test", 4) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_ObjectNotValid);

	conn = ismc_createConnection();
	CU_ASSERT_FATAL(conn != NULL);

	ismc_setStringProperty(conn, "ClientID", CID);

	if (server) {
		ismc_setStringProperty(conn, "Server", server);
	}

	if (client_port) {
		ismc_setStringProperty(conn, "Port", client_port);
	}
	ismc_setStringProperty(conn, "Protocol", "@@@");

	CU_ASSERT_FATAL(ismc_connect(conn) == 0);

	session = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
	CU_ASSERT_FATAL(session != NULL);

	recid = ismc_createManagerRecord(session, "test", 4);
	CU_ASSERT_FATAL(recid != NULL);
	if (!recid->managed_record_id) {
	    recid->managed_record_id = 1;
	}
	CU_ASSERT(ismc_createXARecord(session, NULL, "test", 4) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_NullPointer);
	CU_ASSERT(ismc_createXARecord(session, recid, "test", 4) != NULL);

	ismc_disconnect(conn);
}

void testDestroy(void) {
	ismc_connection_t * conn;
	ismc_session_t * session = NULL;
	ismc_manrec_t manrecid;
	ismc_xarec_t  xarecid1, xarecid2;
	char * server = getenv("ISMServer");
	char * client_port = getenv("ISMPort");

	CU_ASSERT(ismc_deleteManagerRecord(NULL, NULL) == ISMRC_NullPointer);
	CU_ASSERT(ismc_deleteManagerRecord((ismc_session_t *)"test1234", NULL) == ISMRC_NullPointer);

	CU_ASSERT(ismc_deleteXARecord(NULL, NULL) == ISMRC_NullPointer);
	CU_ASSERT(ismc_deleteXARecord((ismc_session_t *)"test1234", NULL) == ISMRC_ObjectNotValid);

	conn = ismc_createConnection();
	CU_ASSERT_FATAL(conn != NULL);

	ismc_setStringProperty(conn, "ClientID", CID);

	if (server) {
		ismc_setStringProperty(conn, "Server", server);
	}

	if (client_port) {
		ismc_setStringProperty(conn, "Port", client_port);
	}
	ismc_setStringProperty(conn, "Protocol", "@@@");

	CU_ASSERT_FATAL(ismc_connect(conn) == 0);

	session = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
	CU_ASSERT_FATAL(session != NULL);

	manrecid = ismc_createManagerRecord(session, "test", 4);
	CU_ASSERT_FATAL(manrecid != NULL);
	if (!manrecid->managed_record_id) {
	    manrecid->managed_record_id = 1;
	}
	xarecid1 = ismc_createXARecord(session, manrecid, "test1", 5);
	CU_ASSERT(xarecid1 != NULL);
	xarecid2 = ismc_createXARecord(session, manrecid, "test2", 5);
	CU_ASSERT(xarecid2 != NULL);

	CU_ASSERT(ismc_deleteXARecord(session, NULL) == ISMRC_NullPointer);
	if (!xarecid1->xa_record_id) {
	    xarecid1->xa_record_id = 1;
	}
	CU_ASSERT(ismc_deleteXARecord(session, xarecid1) == 0);
	if (!xarecid2->xa_record_id) {
	    xarecid2->xa_record_id = 1;
	}
	CU_ASSERT(ismc_deleteXARecord(session, xarecid2) == 0);
	CU_ASSERT(ismc_deleteManagerRecord(session, NULL) == ISMRC_NullPointer);
	CU_ASSERT(ismc_deleteManagerRecord(session, manrecid) == 0);

	ismc_disconnect(conn);
}

void testList(void) {
	ismc_connection_t * conn;
	ismc_session_t * session = NULL;
	ismc_manrec_list_t * list1 = NULL;
	ismc_xarec_list_t * list2 = NULL;
	ismc_manrec_t manrecid = malloc(sizeof(struct ismc_manrec_t));
	char * server = getenv("ISMServer");
	char * client_port = getenv("ISMPort");
	int count;

	memcpy(manrecid->eyecatcher, QM_RECORD_EYECATCHER, sizeof(manrecid->eyecatcher));
    manrecid->managed_record_id = 1;

	CU_ASSERT(ismc_getManagerRecords(NULL, &list1, &count) == ISMRC_NullPointer);
	CU_ASSERT(ismc_getXARecords(NULL, manrecid, &list2, &count) == ISMRC_NullPointer);

	conn = ismc_createConnection();
	CU_ASSERT_FATAL(conn != NULL);

	ismc_setStringProperty(conn, "ClientID", CID);

	if (server) {
		ismc_setStringProperty(conn, "Server", server);
	}

	if (client_port) {
		ismc_setStringProperty(conn, "Port", client_port);
	}
	ismc_setStringProperty(conn, "Protocol", "@@@");

	CU_ASSERT(ismc_getManagerRecords(session, &list1, &count) == ISMRC_NullPointer);

	CU_ASSERT_FATAL(ismc_connect(conn) == 0);

	session = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
	CU_ASSERT_FATAL(session != NULL);

	CU_ASSERT(ismc_getManagerRecords(session, NULL, &count) == ISMRC_NullPointer);
	CU_ASSERT(ismc_getManagerRecords(session, &list1, NULL) == ISMRC_NullPointer);

	CU_ASSERT(ismc_getXARecords(session, NULL, &list2, &count) == ISMRC_NullPointer);
	CU_ASSERT(ismc_getXARecords(session, manrecid, NULL, &count) == ISMRC_NullPointer);
	CU_ASSERT(ismc_getXARecords(session, manrecid, &list2, NULL) == ISMRC_NullPointer);
	memset(manrecid->eyecatcher, '1', 4);
	CU_ASSERT(ismc_getXARecords(session, manrecid, &list2, NULL) == ISMRC_ObjectNotValid);

	free(manrecid);

	/* Manually allocate NULL-terminated array of 4 elements and try to free it */
	list1 = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,90),5 * sizeof(ismc_manrec_list_t));
	list1[0].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,91),1);
	list1[0].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,92),1);
	list1[1].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,93),1);
	list1[1].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,94),1);
	list1[2].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,95),1);
	list1[2].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,96),1);
	list1[3].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,97),1);
	list1[3].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,98),1);
	list1[4].data = NULL;
	list1[4].handle = NULL;
	ismc_freeManagerRecords(list1);

	list2 = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,99),5 * sizeof(ismc_xarec_list_t));
	list2[0].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,100),1);
	list2[0].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,101),1);
	list2[1].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,102),1);
	list2[1].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,103),1);
	list2[2].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,104),1);
	list2[2].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,105),1);
	list2[3].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,106),1);
	list2[3].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,107),1);
	list2[4].data = NULL;
	list2[4].handle = NULL;
	ismc_freeXARecords(list2);

	ismc_disconnect(conn);
}

