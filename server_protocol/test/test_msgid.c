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

//#include <mqtt.h>
#include <msgid.c>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void msgid_test(void) {
    int  i;
    int  rc;
    int msgid [65536];
    ism_transport_t trans = {0};
    mqttProtoObj_t pobjx = {0};
    ism_transport_t * transport = &trans;
    ism_msgid_init(15, 1024*1024);
    transport->pobj = &pobjx;
    pobjx.msgids = ism_create_msgid_list(transport,0,0xFFFF);
    pobjx.incompmsgids = ism_create_msgid_list(transport,1,0xFFFF);
    pthread_spin_init(&pobjx.lock, 0);
    int msgid_count = 0;
    int found_msgid_count = 0;
    int freed_msgid_count = 0;

    /* Assign 100 message IDs */
    for (i=0; i<100; i++) {
        msgid[i] = i+1;
        rc = ism_msgid_addMsgIdInfo(pobjx.msgids,msgid[i],i+1, ISM_MQTT_PUBLISH);
        CU_ASSERT(rc == 0);
        msgid_count++;
    }
    CU_ASSERT(msgid_count == 100);
    CU_ASSERT(pobjx.msgids->inUseCount == 100);

    /* Make sure we can find them all */
    for (i=0; i<100; i++) {
    	void * msg = ism_msgid_getMsgIdInfo(pobjx.msgids,msgid[i]);
    	if(msg)
    		found_msgid_count++;
    	CU_ASSERT(msg != NULL);
    }
    CU_ASSERT(found_msgid_count == 100);


    /* Free the message IDs */
    for (i=0; i<99; i++) {
        ism_msgid_delMsgIdInfo(pobjx.msgids, msgid[i], NULL);
    }
    CU_ASSERT(pobjx.msgids->inUseCount = 1);

    /* Make sure we cannot find them */
    for (i=0; i<99; i++) {
    	void * msg = ism_msgid_getMsgIdInfo(pobjx.msgids,msgid[i]);
        if (msg == NULL)
            freed_msgid_count += 1;
        CU_ASSERT(msg == NULL);
    }
    CU_ASSERT(freed_msgid_count == 99);

    /* Except the last one */
    {
        void * msg = ism_msgid_getMsgIdInfo(pobjx.msgids,msgid[99]);
        CU_ASSERT(msg != NULL);
    }
    /* Assign some more now that we have free entries */
    for (i=0; i<25; i++) {
        msgid[i] = i+1;
        rc = ism_msgid_addMsgIdInfo(pobjx.msgids,msgid[i],i+1, ISM_MQTT_PUBLISH);
        CU_ASSERT(rc == 0);
    }

    /* Assign some more */
    for (i=25; i<50; i++) {
        msgid[i] = i+1;
        rc = ism_msgid_addMsgIdInfo(pobjx.msgids,msgid[i],i+1, ISM_MQTT_PUBLISH);
        CU_ASSERT(rc == 0);
    }
    CU_ASSERT(pobjx.msgids->inUseCount == 51);

    /* Free all message ids */
    ism_msgid_freelist(pobjx.msgids);
    pobjx.msgids = ism_create_msgid_list(transport,0,0xFFFF);

    /* Assign 65535 IDs */
    msgid_count = 0;
    for (i=0; i<65535; i++) {
        msgid[i] = i+1;
        rc = ism_msgid_addMsgIdInfo(pobjx.msgids,msgid[i],i+1, ISM_MQTT_PUBLISH);
        CU_ASSERT(rc == 0);
        msgid_count++;
    }
    CU_ASSERT(msgid_count == 65535);
    CU_ASSERT(pobjx.msgids->inUseCount == 65535);

    ism_msgid_delMsgIdInfo(pobjx.msgids, msgid[15], NULL);
    msgid[15] = 0;
    ism_msgid_delMsgIdInfo(pobjx.msgids, msgid[35], NULL);
    msgid[35] = 0;
    ism_msgid_delMsgIdInfo(pobjx.msgids, msgid[25], NULL);
    msgid[25] = 0;
    CU_ASSERT(pobjx.msgids->inUseCount == 65532);

    /* Free 65535 IDs */
    for (i=0; i<1000; i++) {
        if(msgid[i])
            ism_msgid_delMsgIdInfo(pobjx.msgids, msgid[i], NULL);
    }
    ism_msgid_freelist(pobjx.msgids);
}

CU_TestInfo ISM_Protocol_CUnit_Msgid[] = {
    {"MsgidTest          ", msgid_test },
    CU_TEST_INFO_NULL
};
