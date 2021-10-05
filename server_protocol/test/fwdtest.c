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

#include <forwarder.c>
#include <fwdsender.c>
#include <fwdreceiver.c>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <protocol_test_globals.h>
#include <fakeengine.h>

void testfwdnotify(void);
void testfwdverify(void);
void xalink_test(void);
void recover_test(void);
void dhmap_test(void);


/**
 * Test array for API tests.
 */
extern CU_TestInfo ISM_Protocol_CUnit_Fwd[];

CU_TestInfo ISM_Protocol_CUnit_Fwd[] = {
    {"FwdNotify          ",      testfwdnotify },
    {"FwdVerify          ",      testfwdverify },
    {"FwdXALink          ",      xalink_test   },
#if 0
    {"FwdRecover",     recover_test  },
#endif
    {"FwdDHMap           ",       dhmap_test    },
    CU_TEST_INFO_NULL
};

#define BASIC_TEST_MODE       0
#define FULL_TEST_MODE        1
#define BYNAME_TEST_MODE      2

extern int g_verbose;

int ism_plugin_stats(void * action, int stat_type, uint64_t heap_size, uint64_t heap_used,
        uint32_t gc_rate, uint32_t cpu_percent);

/*
 * Test the notification from cluster
 */
void testfwdnotify(void) {
    // ism_common_setTraceLevel(7);

    ismEngine_RemoteServerHandle_t engineHandle = NULL;
    ismCluster_RemoteServerHandle_t clusterHandle = NULL;
    int  rc;
    ismProtocol_RemoteServerHandle_t ch0;
    ismProtocol_RemoteServerHandle_t ch1;
    ismProtocol_RemoteServerHandle_t ch2;

    engineHandle = (ismEngine_RemoteServerHandle_t)(uintptr_t *)1;
    clusterHandle = (ismCluster_RemoteServerHandle_t)(uintptr_t *)3;
    rc = ism_fwd_cluster_notification(PROTOCOL_RS_CREATE, NULL, "myname", "my_012345", "127.0.0.1", 5432, 0,
            clusterHandle, engineHandle, NULL,  &ch0);

    CU_ASSERT(rc == 0);
    CU_ASSERT(ch0 != NULL);
    CU_ASSERT(ch0 == ism_fwd_findChannel("my_012345"));
    if (ch0) {
        CU_ASSERT(ch0->port == 5432);
        CU_ASSERT(ch0->secure == 0);
        CU_ASSERT(!strcmp(ch0->name, "myname"));
        CU_ASSERT(!strcmp(ch0->uid, "my_012345"));
        CU_ASSERT(ch0->engineHandle == engineHandle);
        CU_ASSERT(ch0->clusterHandle == clusterHandle);
    }

    /* Create another */
    engineHandle = (ismEngine_RemoteServerHandle_t)(uintptr_t *)5;
    clusterHandle = (ismCluster_RemoteServerHandle_t)(uintptr_t *)7;
    rc = ism_fwd_cluster_notification(PROTOCOL_RS_CREATE, NULL, "myname", "my_123456", "127.0.0.1", 5432, 0,
            clusterHandle, engineHandle, NULL,  &ch1);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ch1 != NULL);
    CU_ASSERT(ch1 == ism_fwd_findChannel("my_123456"));
    CU_ASSERT(ch0 == ism_fwd_findChannel("my_012345"));
    if (ch1) {
        CU_ASSERT(ch1->port == 5432);
        CU_ASSERT(ch1->secure == 0);
        CU_ASSERT(!strcmp(ch1->name, "myname"));
        CU_ASSERT(!strcmp(ch1->uid, "my_123456"));
        CU_ASSERT(ch1->engineHandle == engineHandle);
        CU_ASSERT(ch1->clusterHandle == clusterHandle);
    }

    /* Create one more */
    engineHandle = (ismEngine_RemoteServerHandle_t)(uintptr_t *)9;
    clusterHandle = (ismCluster_RemoteServerHandle_t)(uintptr_t *)11;
    rc = ism_fwd_cluster_notification(PROTOCOL_RS_CREATE, NULL, "name2", "my_234567", "127.0.0.1", 1234, 1,
            clusterHandle, engineHandle, NULL, &ch2);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ch2 != NULL);
    CU_ASSERT(ch2 == ism_fwd_findChannel("my_234567"));
    CU_ASSERT(ch1 == ism_fwd_findChannel("my_123456"));
    CU_ASSERT(ch0 == ism_fwd_findChannel("my_012345"));
    if (ch2) {
        CU_ASSERT(ch2->port == 1234);
        CU_ASSERT(ch2->secure == 1);
        CU_ASSERT(!strcmp(ch2->name, "name2"));
        CU_ASSERT(!strcmp(ch2->uid, "my_234567"));
        CU_ASSERT(ch2->engineHandle == engineHandle);
        CU_ASSERT(ch2->clusterHandle == clusterHandle);
    }

    /* Delete 1 */
    rc = ism_fwd_cluster_notification(PROTOCOL_RS_REMOVE, ch1, NULL, NULL, NULL, 0, 0,
            NULL, NULL, NULL, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ism_fwd_findChannel("my_123456") == NULL);
    CU_ASSERT(ch2 == ism_fwd_findChannel("my_234567"));
    CU_ASSERT(ch0 == ism_fwd_findChannel("my_012345"));

    /* Delete 0 */
    rc = ism_fwd_cluster_notification(PROTOCOL_RS_REMOVE, ch0, NULL, NULL, NULL, 0, 0,
            NULL, NULL, NULL, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ism_fwd_findChannel("my_123456") == NULL);
    CU_ASSERT(ch2 == ism_fwd_findChannel("my_234567"));
    CU_ASSERT(ism_fwd_findChannel("my_012345") == NULL);

    /* Delete 2 */
    rc = ism_fwd_cluster_notification(PROTOCOL_RS_REMOVE, ch2, NULL, NULL, NULL, 0, 0,
            NULL, NULL, NULL, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(ism_fwd_findChannel("my_123456") == NULL);
    CU_ASSERT(ism_fwd_findChannel("my_234567") == NULL);
    CU_ASSERT(ism_fwd_findChannel("my_012345") == NULL);

    if (g_verbose) {
        ism_plugin_stats(NULL, 1, 512*1024*1024, 255*1024*1024, 42, 12);
    }
}

/*
 * Test the verification logic
 */
void testfwdverify(void) {
    ism_fwd_act_t action = {0};
    ism_field_t hdr[15];
    ism_field_t body;
    ism_field_t props;
    ism_transport_t * transport;

    transport = ism_transport_newTransport(&outEndpoint, 0, 0);
    transport->protocol_family = "fwd";
    transport->protocol = "fwd";

    action.transport = transport;
    action.seqnum = 0;
    action.hdrs = hdr;

    /*
     * Test connect
     */
    action.action = FwdAction_Connect;
    hdr[0].type = VT_Integer;
    hdr[0].val.i = 2000000;
    hdr[1].type = VT_Long;
    hdr[1].val.l = 99;
    hdr[2].type = VT_String;
    hdr[2].val.s = "abc";
    hdr[3].type = VT_String;
    hdr[3].val.s = "abc_12345";
    hdr[4].type = VT_Byte;
    hdr[4].val.i = 0;
    transport->originated = 0;
    action.hdrcount = 5;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == 0);

    /* Test in originated client */
    transport->originated = 1;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == ISMRC_BadClientData);
    transport->originated = 0;

    /* Test with small hdr count */
    action.hdrcount = 3;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == ISMRC_BadClientData);
    action.hdrcount = 4;

    /* Test with big hdr count */
    action.hdrcount = 6;
    hdr[5].type = VT_Null;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == 0);

    /* Test with wrong hdr2 */
    hdr[2].type = VT_Byte;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == ISMRC_BadClientData);
    hdr[2].type = VT_String;

    /* Test with byte instead of int */
    hdr[0].type = VT_Byte;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == 0);
    hdr[0].type = VT_Integer;


    /* Test connect_reply */
    action.action = FwdAction_ConnectReply;
    hdr[0].type = VT_Integer;
    hdr[0].val.i = 2000000;
    hdr[1].type = VT_Long;
    hdr[1].val.l = 99;
    hdr[2].type = VT_Integer;
    hdr[2].val.s = 0;
    hdr[3].type = VT_Integer;
    transport->originated = 1;
    action.hdrcount = 4;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == 0);

    /* Test on incoming */
    transport->originated = 0;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == ISMRC_BadClientData);
    transport->originated = 1;

    /*
     * Test message
     */
    /* Test connect_reply */
    action.action = FwdAction_Message;
    hdr[0].type = VT_Long;
    hdr[0].val.l = 55;
    hdr[1].type = VT_Byte;
    hdr[1].val.i = 1;
    hdr[2].type = VT_Byte;
    hdr[2].val.i = 2;
    hdr[3].type = VT_Null;
    hdr[4].val.i = 229;
    hdr[4].type = VT_Integer;
    props.type = VT_Null;
    body.type = VT_ByteArray;
    body.val.s = "Now is the time";
    body.len = 15;
    transport->originated = 0;
    action.hdrcount = 5;
    action.pfield = props;
    action.body = body;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == 0);
    CU_ASSERT(action.seqnum == 55);

    /* Test on incoming */
    transport->originated = 1;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == ISMRC_BadClientData);
    transport->originated = 0;

    /* Test null seqnum */
    hdr[0].type = VT_Null;
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == ISMRC_BadClientData);
    hdr[0].type = VT_Long;
    hdr[0].val.l = 99;

    /* Send in a destination */
    hdr[3].type = VT_String;
    hdr[3].val.s = "topic/string";
    action.rc = 0;
    validateAction(&action);
    CU_ASSERT(action.rc == 0);
    CU_ASSERT(action.seqnum == 99);

}

void ism_common_setServerUID(const char * value);

/*
 * Unit test for xa linking
 */
void xalink_test(void) {
    ism_fwd_channel_t * channel;
    channel = ism_fwd_newChannel("thisserver", "ThisIsMyServer");
    fwd_xa_t * xa = calloc(1, sizeof(fwd_xa_t) );
    fwd_xa_t * xa1 = calloc(1, sizeof(fwd_xa_t) );
    fwd_xa_t * xa2 = calloc(1, sizeof(fwd_xa_t) );
    fwd_xa_t * xa3 = calloc(1, sizeof(fwd_xa_t) );

    ism_common_setServerUID("thisserver");

    fwd_xid_seqn = 100;
    ism_fwd_xa_init(xa1, "otherserver");
    CU_ASSERT(xa1->sequence == 101);
    CU_ASSERT(!strcmp(xa1->gtrid, "otherserver_thisserver_101"));
    ism_fwd_xa_init(xa2, "otherserver");
    ism_fwd_xa_init(xa3, "otherserver");
    CU_ASSERT(xa3->sequence == 103);
    CU_ASSERT(!strcmp(xa3->gtrid, "otherserver_thisserver_103"));

    ism_fwd_linkXA(channel, xa2, 0, 1);
    CU_ASSERT(channel->receiver_xa->sequence == 102);
    xa = ism_fwd_findXA(channel, xa2->gtrid, 0, 1);
    CU_ASSERT(xa == xa2);

    ism_fwd_linkXA(channel, xa3, 0, 1);
    CU_ASSERT(channel->receiver_xa->sequence == 102);
    xa = ism_fwd_findXA(channel, xa2->gtrid, 0, 1);
    CU_ASSERT(xa == xa2);
    xa = ism_fwd_findXA(channel, xa3->gtrid, 0, 1);
    CU_ASSERT(xa == xa3);

    pthread_mutex_lock(&channel->lock);
    ism_fwd_linkXA(channel, xa1, 0, 0);
    pthread_mutex_unlock(&channel->lock);
    CU_ASSERT(channel->receiver_xa->sequence == 101);
    xa = ism_fwd_findXA(channel, xa2->gtrid, 0, 1);
    CU_ASSERT(xa == xa2);
    xa = ism_fwd_findXA(channel, xa3->gtrid, 0, 1);
    CU_ASSERT(xa == xa3);
    xa = ism_fwd_findXA(channel, xa1->gtrid, 0, 1);
    CU_ASSERT(xa == xa1);

    ism_fwd_unlinkXA(channel, xa1, 0, 1);
    CU_ASSERT(channel->receiver_xa->sequence == 102);
    ism_fwd_unlinkXA(channel, xa3, 0, 1);
    CU_ASSERT(channel->receiver_xa->sequence == 102);
    ism_fwd_unlinkXA(channel, xa2, 0, 1);
    CU_ASSERT(channel->receiver_xa == NULL);



    /*
     * Reverse the sender and receiver and do the same tests of sender side
     */
    strcpy(xa1->gtrid, "thisserver_otherserver_101");
    strcpy(xa2->gtrid, "thisserver_otherserver_102");
    strcpy(xa3->gtrid, "thisserver_otherserver_103");
    xa1->next = NULL;
    xa2->next = NULL;
    xa3->next = NULL;

    ism_fwd_linkXA(channel, xa2, 1, 1);
    CU_ASSERT(channel->sender_xa->sequence == 102);
    xa = ism_fwd_findXA(channel, xa2->gtrid, 1, 1);
    CU_ASSERT(xa == xa2);

    ism_fwd_linkXA(channel, xa3, 1, 1);
    CU_ASSERT(channel->sender_xa->sequence == 102);
    xa = ism_fwd_findXA(channel, xa2->gtrid, 1, 1);
    CU_ASSERT(xa == xa2);
    xa = ism_fwd_findXA(channel, xa3->gtrid, 1, 1);
    CU_ASSERT(xa == xa3);

    ism_fwd_linkXA(channel, xa1, 1, 1);
    CU_ASSERT(channel->sender_xa->sequence == 101);
    xa = ism_fwd_findXA(channel, xa2->gtrid, 1, 1);
    CU_ASSERT(xa == xa2);
    xa = ism_fwd_findXA(channel, xa3->gtrid, 1, 1);
    CU_ASSERT(xa == xa3);
    xa = ism_fwd_findXA(channel, xa1->gtrid, 1, 1);
    CU_ASSERT(xa == xa1);

    ism_fwd_unlinkXA(channel, xa1, 1, 1);
    CU_ASSERT(channel->sender_xa->sequence == 102);
    ism_fwd_unlinkXA(channel, xa3, 1, 1);
    CU_ASSERT(channel->sender_xa->sequence == 102);
    ism_fwd_unlinkXA(channel, xa2, 1, 1);
    CU_ASSERT(channel->sender_xa == NULL);

}

static void makeOtherXid(ism_xid_t * xid, uint32_t formatID, const char * branch, const char * gtrid) {
    xid->formatID = formatID;
    xid->bqual_length = strlen(branch);
    xid->gtrid_length = strlen(gtrid);
    memcpy(xid->data, branch, xid->bqual_length);
    memcpy(xid->data+xid->bqual_length, gtrid, xid->gtrid_length);
    xid->data[xid->bqual_length + xid->gtrid_length] = 0;
}

/*
 * Fake engine API for CUnit test
 */
uint32_t ism_engine_XARecover( ismEngine_SessionHandle_t hSession,
             ism_xid_t * xidlist, uint32_t count, uint32_t rmid, uint32_t flags) {
    ism_fwd_makeXid(xidlist+0, 'R', "otherserver_thisserver_1233");
    makeOtherXid(xidlist+1, 1234, "branch", "some_other_xid_format");
    ism_fwd_makeXid(xidlist+2, 'R', "otherserver_thisserver_1231");
    ism_fwd_makeXid(xidlist+3, 'R', "otherserver_thisserver_1232");
    ism_fwd_makeXid(xidlist+4, 'S', "thisserver_otherserver_3456");
    ism_fwd_makeXid(xidlist+5, 'S', "thisserver_otherserver_3457");
    return 6;
}

pthread_mutex_t fwd_configLock;

#if 0
void recover_test(void) {
    pthread_mutex_init(&fwd_configLock, 0);
    int  rc;
    fwd_xa_t * xa;
    ism_fwd_channel_t * channel;


    fwd_xid_seqn = 0;

    CU_ASSERT(!strcmp("thisserver", ism_common_getServerUID()));
    rc = ism_fwd_recoverTransactions();
    CU_ASSERT(rc == 0);
    printf("\nfwd_xid_seqn=%d\n", fwd_xid_seqn);
    CU_ASSERT(fwd_xid_seqn > 1233);

    channel = ism_fwd_findChannel("otherserver");
    CU_ASSERT(channel != NULL);


    if (channel) {
        xa = ism_fwd_findXA(channel, "otherserver_thisserver_1231", 0, 1);
        CU_ASSERT(xa != NULL);
        xa = ism_fwd_findXA(channel, "otherserver_thisserver_1232", 0, 1);
        CU_ASSERT(xa != NULL);
        xa = ism_fwd_findXA(channel, "thisserver_otherserver_3456", 1, 1);
        CU_ASSERT(xa != NULL);
        xa = ism_fwd_findXA(channel, "thisserver_otherserver_3457", 1, 1);
        CU_ASSERT(xa != NULL);
    }
}
#endif

/*
 * Test dhmap functions
 */
void dhmap_test(void) {
    ism_fwd_channel_t * channel = ism_fwd_newChannel("myserver", "name");
    uint64_t i;
    int  count = 200001;   /* multiple of 10 + 1 */
    uint64_t seqn [20];
    int      err = 0;
    ismEngine_DeliveryHandle_t hand [20];

    ism_common_setTraceLevel(2);

    for (i=1; i<count; i++) {
        ismEngine_DeliveryHandle_t dhandle = i+1000000;
        ism_fwd_addDeliveryHandle(channel, (uint64_t)i, dhandle);
    }
    CU_ASSERT(channel->dhcount == count-1);


    err = 0;
    for (i=1; i<count; i++) {
        ismEngine_DeliveryHandle_t dh = ism_fwd_findDeliveryHandle(channel, (uint64_t)i, 0);
        err += (uint64_t)dh != ((uint64_t)i+1000000);
    }
    CU_ASSERT(channel->dhcount == count-1);
    CU_ASSERT(err == 0);

    err = 0;
    for (i=1; i<count; i++) {
        ismEngine_DeliveryHandle_t dh = ism_fwd_findDeliveryHandle(channel, (uint64_t)i, 1);
        err += (uint64_t)dh != ((uint64_t)i+1000000);
    }
    CU_ASSERT(channel->dhcount == 0);
    CU_ASSERT(err == 0);
    if (channel->dhcount || err)
        printf("fwdtest: line=%u dhcount=%u err=%u\n", __LINE__, channel->dhcount, err);

    err = 0;
    for (i=1; i<count; i++) {
        ismEngine_DeliveryHandle_t dh = ism_fwd_findDeliveryHandle(channel, (uint64_t)i, 1);
        err += dh != 0;
    }
    CU_ASSERT(err == 0);

    err = 0;
    channel = ism_fwd_newChannel("myserver2", "name2");
    for (i=1; i<count; i++) {
        ismEngine_DeliveryHandle_t dhandle = i+2000000;
        ism_fwd_addDeliveryHandle(channel, (uint64_t)i, dhandle);
    }
    CU_ASSERT(channel->dhcount == count-1);
    CU_ASSERT(err == 0);

    err = 0;
    for (i=1; i<count; i += 10) {
        int found;
        int j;
        for (j=0; j<10; j++) {
            seqn[j] = (uint64_t)(i+j);
        }
        found = ism_fwd_listDeliveryHandle(channel, seqn, hand, 10);
        err += found != 10;
        if (found != 10)
            printf("invalid count at %u = %u\n", (int)i, found);
        for (j=0; j<found; j++) {
            err += (uint64_t)hand[j] != (seqn[j]+2000000);
        }
    }
    CU_ASSERT(channel->dhcount == 0);
    CU_ASSERT(err == 0);
    if (channel->dhcount || err)
        printf("fwdtest: line=%u dhcount=%u err=%u\n", __LINE__, channel->dhcount, err);

    err = 0;
    for (i=1; i<count; i++) {
        ismEngine_DeliveryHandle_t dh = ism_fwd_findDeliveryHandle(channel, (uint64_t)i, 1);
        err += dh != 0;
    }
    CU_ASSERT(err == 0);
}
