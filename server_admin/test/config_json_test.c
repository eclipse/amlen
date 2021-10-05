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
 * File: config_json_test.c
 * Component: server
 * SubComponent: server_admin
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <admin.h>
#include <ismjson.h>
#include <config.h>

extern ism_json_parse_t * ism_config_json_loadJSONFromFile(const char * name);
extern int ism_config_json_parseConfig(ism_json_parse_t * parseobj);
extern int ism_config_updateClientSetStatus(char *clientID, char *retain, ismClientSetState_t status);
extern int ism_config_deleteClientSetFromList(char *clientID, char *retain);
extern int ism_validate_ClientSetStatus(char *clientID, char *retain, ismClientSetState_t status);

/*
 *
 * init
 */
void test_config_JSON_FileLoad(void) {
	ism_json_parse_t * parseobj = NULL;
    int rc = 0;
    int status = -1;

    parseobj = ism_config_json_loadJSONFromFile("test/testQueuedTask");
    /*
     * Initialize the utilities
     */
    CU_ASSERT_PTR_NOT_NULL(parseobj);

    rc = ism_config_json_parseConfig(parseobj);
    printf("ism_config_json_parseConfig: rc=%d\n", rc);
    CU_ASSERT( rc == 0 );

    status = ism_validate_ClientSetStatus("xyz", "ABC", ismCLIENTSET_WAITING);
    CU_ASSERT( status == 0);

    status = ism_validate_ClientSetStatus("llin", NULL, ismCLIENTSET_WAITING);
    CU_ASSERT( status == 0);

    status = ism_validate_ClientSetStatus("Marc", "Marc's_Message", ismCLIENTSET_WAITING);
    CU_ASSERT( status == 0);

    rc = ism_config_updateClientSetStatus("xyz", "ABC", ismCLIENTSET_DELETINGCLIENTS);
    CU_ASSERT( rc == 0 );
    status = ism_validate_ClientSetStatus("xyz", "ABC", ismCLIENTSET_DELETINGCLIENTS);
    CU_ASSERT( status == 0);

    rc = ism_config_updateClientSetStatus("llin", NULL, ismCLIENTSET_HAVECLIENTLIST);
    CU_ASSERT( rc == 0 );
    status = ism_validate_ClientSetStatus("llin", NULL, ismCLIENTSET_HAVECLIENTLIST);
    CU_ASSERT( status == 0);

    rc = ism_config_updateClientSetStatus("Marc", "Marc's_Message", ismCLIENTSET_DONE);
    CU_ASSERT( rc == 0 );
    status = ism_validate_ClientSetStatus("Marc", "Marc's_Message", ismCLIENTSET_DONE);
    CU_ASSERT( status == 0);
    /*At this point the clientSet has been removed*/
	status = ism_validate_ClientSetStatus("Marc", "Marc's_Message", ismCLIENTSET_NOTFOUND);
	CU_ASSERT( status == 0);


    rc = ism_config_deleteClientSetFromList("llin", NULL);
    CU_ASSERT( rc == 0 );
    status = ism_validate_ClientSetStatus("llin", NULL, ismCLIENTSET_NOTFOUND);
    CU_ASSERT( status == 0);

    rc = ism_config_deleteClientSetFromList("xyz", "ABC");
    CU_ASSERT( rc == 0 );
    status = ism_validate_ClientSetStatus("xyz", "ABC", ismCLIENTSET_NOTFOUND);
    CU_ASSERT( status == 0);

}

