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

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

extern int set_configFromJSONStr(const char *JSONStr);

int tpCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Transport call is invoked\n");
    return 0;
}

/* Cunit test for QueuePolicy authorization */
void test_getProperties(void)
{
    int rc = ISMRC_OK;
    char *ibuf = NULL;

    ism_common_initUtil();
//    ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("test_getProperties: Init admin: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    ismDefaultTrace = &ismDefaultDomain.trace;

    ism_config_t * handle = ism_config_getHandle(ISM_CONFIG_COMP_TRANSPORT, NULL);
    CU_ASSERT_PTR_NOT_NULL(handle);

    /* Add MessageHub */
    ibuf = "{ \"MessageHub\": { \"TestHub\": { \"Description\":\"Test Hub\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("test_getProperties: create MessageHub: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );

    ism_prop_t *prop = ism_config_getProperties(handle, NULL, NULL);
    CU_ASSERT_PTR_NOT_NULL(prop);

    prop = ism_config_getProperties(handle, "MessageHub", NULL);
    CU_ASSERT_PTR_NOT_NULL(prop);

    prop = ism_config_getProperties(handle, "MessageHub", "TestHub");
    CU_ASSERT_PTR_NOT_NULL(prop);


     return;
}

