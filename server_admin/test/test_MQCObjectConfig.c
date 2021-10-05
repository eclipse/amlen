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

/******************************************************************************************************
 * ----------   CUNIT TES FUNCTIONS FOR MQC Object Configuration   ----------------------
 ******************************************************************************************************/

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

extern int ism_common_initServer();
extern int ism_config_json_processMQCRequest(const char *inpbuf, int inpbuflen, const char * locale, concat_alloc_t *output_buffer, int rc1);

int mqctestno = 0;

/*
 * Fake callback for MQC
 */
int mqcCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("MQC callback is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING MQC Configuration Object   ---------------
 *
 ******************************************************************************************************/

static int32_t mqc_config( char *inpbuf, int buflen, int exprc )
{
    int rc = ism_config_json_processMQCRequest(inpbuf, buflen, NULL, NULL, 0);
    mqctestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", mqctestno, inpbuf ? inpbuf : "NULL", exprc, rc);
    }

    return rc;
}


void test_MQC_configObject(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
//    ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_MQC);

    printf("Admin INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_MQCONNECTIVITY, NULL, (ism_config_callback_t) mqcCallback, &handle);
    printf("MQC ism_config_register(): rc: %d\n", rc);

    char *buf = NULL;
    int   buflen = 0;

    buf = "{ \"configuration\": { \"DestinationMappingRule\": { \"DMR1\": {\"Enabled\":false, \"RuleType\":2, \"Source\":\"abcd\",\"Destination\":\"abcd\",\"QueueManagerConnection\":\"testqm\"}}}}";
    buflen = strlen(buf);
    CU_ASSERT( mqc_config(buf, buflen, ISMRC_OK) == ISMRC_OK );

    return;
}

