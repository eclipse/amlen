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
 * File: test_jms.c
 * Component: server
 * SubComponent: server_protocol
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */
static int g_cmd_int_result2;
static int g_cmd_long_result2;
static int g_entered_jmsresume;

#include <jms.c>
#include <test_jms.h>
#include <fakeengine.h>

uint8_t g_action;

/**
 * Test functions
 */
void testGetJmsActionName(void);
void testGetProperty(void);
void testJmsReceive(void);

/**
 * Individual tests
 */
int jmsReceive_test(ism_transport_t * transport, char * cmd_buf, int buflen, int check_response);
void jmsReceive_fail_shortaction(void);
void jmsReceive_fail_pobjclosed(void);
void jmsReceive_fail_closeinprogress(void);
void jmsReceive_fail_invalidproperties(void);
void jmsReceive_fail_noclientid(void);
void jmsReceive_fail_authentication(void);
void jmsReceive_fail_authorization(void);
void jmsReceive_pass_createconnection(void);
void jmsReceive_pass_createsession(void);
void jmsReceive_fail_createsession(void);
void jmsReceive_pass_closeconnection(void);
void jmsReceive_fail_closeconnection(void);
void jmsReceive_pass_closesession(void);
void jmsReceive_fail_closesession(void);
void jmsReceive_pass_startconnection(void);
void jmsReceive_pass_stopconnection(void);
void jmsReceive_pass_createtransaction(void);
void jmsReceive_fail_createtransaction(void);
void jmsReceive_pass_createconsumer(void);
void jmsReceive_fail_createconsumer(void);

/*
 * TODO
 *
 * // General actions
createConsumer
closeConsumer
createDurable
unsubscribeDurable
createProducer
closeProducer
commitSession
rollbackSession
recover
ackSync
createQMRecord
destroyQMRecord
getQMRecords
createQMXidRecord
destroyQMXidRecord
getQMXidRecords

default - force disconnect

// Message-related
message
messageWait
messageNoProd
msgNoProdWait
ack

// Empty
setMsgListener
removeMsgListener
initConnection
getTime
 */


/* Helper functions */
static void jmsSetupTransport(ism_transport_t *transport, const char * protocol,struct ism_protobj_t * pobj,int line);
static void jmsCreateConnection(ism_transport_t * transport);
static int jmsCreateSession(ism_transport_t * transport, int transacted, int ackmode);
//static int jmsCreateConsumer(ism_transport_t * transport, int sessionid, int domain, char * destname,
//		const char * selector, int ack, int nolocal, int orderid);
static void freeTransport(ism_transport_t * transport);
static int simpleJmsResume(ism_transport_t * transport, void * userdata);

/**
 * Transport functions
 */
int jmsclose(ism_transport_t * trans, int rc, int clean, const char * reason);
int jmsclosed(ism_transport_t * trans);
int jmssend(ism_transport_t * trans, char * buf, int len, int kind, int flags);

CU_TestInfo ISM_Protocol_CUnit_JmsBasic[] = {
    {"JmsGetJmsActionName", testGetJmsActionName },
    {"JmsGetProperty     ", testGetProperty },
    {"JmsReceive         ", testJmsReceive },
    CU_TEST_INFO_NULL
};

void jmsSetupTransport(ism_transport_t *transport, const char * protocol,struct ism_protobj_t * pobj,int line) {
    transport->protocol = protocol;
    transport->close = jmsclose;
    transport->closed = jmsclosed;
    transport->send = jmssend;
    transport->pobj = pobj;
    transport->closing = jmsClosing;
    transport->resume = simpleJmsResume;
    pthread_spin_init(&pobj->lock, 0);
    pthread_spin_init(&pobj->sessionlock, 0);
//    if(pobj){
//    	fprintf(stderr,"setupTransport called from line %d, trans = %p\n",line,transport);
//    }
    ism_listener_t * listener = calloc(1, sizeof(ism_listener_t) + sizeof(ism_endstat_t));
    listener->stats = (ism_endstat_t *)(listener+1);
    transport->listener = listener;
    transport->listener->name = "jms_test";
    if(transport->security_context==NULL){
    	ism_security_create_context(ismSEC_POLICY_CONNECTION,
    									transport,
    									&transport->security_context);
    }
}

void testGetJmsActionName(void) {
    CU_ASSERT(strcmp(getActionName(Action_message),"message") == 0);
    CU_ASSERT(strcmp(getActionName(Action_messageWait),"messageWait") == 0);
    CU_ASSERT(strcmp(getActionName(Action_ack),"ack") == 0);
    CU_ASSERT(strcmp(getActionName(Action_reply),"reply") == 0);
    CU_ASSERT(strcmp(getActionName(Action_receiveMsg),"receiveMsg") == 0);
    CU_ASSERT(strcmp(getActionName(Action_receiveMsgNoWait),"receiveMsgNoWait") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createConnection),"createConnection") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createSession),"createSession") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createConsumer),"createConsumer") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createBrowser),"createBrowser") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createDurable),"createDurable") == 0);
    CU_ASSERT(strcmp(getActionName(Action_closeConsumer),"closeConsumer") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createProducer),"createProducer") == 0);
    CU_ASSERT(strcmp(getActionName(Action_closeProducer),"closeProducer") == 0);
    CU_ASSERT(strcmp(getActionName(Action_setMsgListener),"setMsgListener") == 0);
    CU_ASSERT(strcmp(getActionName(Action_removeMsgListener),"removeMsgListener") == 0);
    CU_ASSERT(strcmp(getActionName(Action_rollbackSession),"rollbackSession") == 0);
    CU_ASSERT(strcmp(getActionName(Action_commitSession),"commitSession") == 0);
    CU_ASSERT(strcmp(getActionName(Action_ackSync),"ackSync") == 0);
    CU_ASSERT(strcmp(getActionName(Action_recover),"recover") == 0);
    CU_ASSERT(strcmp(getActionName(Action_getTime),"getTime") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createTransaction),"createTransaction") == 0);
    CU_ASSERT(strcmp(getActionName(Action_sendWill),"sendWill") == 0);
    CU_ASSERT(strcmp(getActionName(Action_initConnection),"initConnection") == 0);
    CU_ASSERT(strcmp(getActionName(Action_termConnection),"termConnection") == 0);

    CU_ASSERT(strcmp(getActionName(-1),"Unknown") == 0);
    CU_ASSERT(strcmp(getActionName(MAX_ACTION_VALUE+1),"Unknown") == 0);
}

/**
 * Test functions
 * getintproperty (and str2l)
 * getbooleanproperty
 * getproperty
 */
void testGetProperty(void) {
	ism_protocol_action_t action;
	ism_field_t field;
	ism_prop_t * props = ism_common_newProperties(20);
	int i;
	const char * name;
	ism_propent_t * ent;

    action.propcount = 0;

    field.type = VT_Boolean;
    field.val.i = 1;
    ism_common_setProperty(props, "prop1", &field);

   	field.val.i = 0;
   	ism_common_setProperty(props, "prop2", &field);

    field.type = VT_Integer;
    field.val.i = 150;
    ism_common_setProperty(props, "prop3", &field);

    field.type = VT_String;
    field.val.s = "enabled";
   	ism_common_setProperty(props, "prop4", &field);

    field.type = VT_Float;
    field.val.f = 15.0;
    ism_common_setProperty(props, "prop5", &field);

    field.type = VT_String;
    field.val.s = "256";
   	ism_common_setProperty(props, "prop6", &field);

    field.type = VT_String;
    field.val.s = "0256";
   	ism_common_setProperty(props, "prop7", &field);

    field.type = VT_String;
    field.val.s = "0x100";
   	ism_common_setProperty(props, "prop8", &field);

    action.propcount = 8;
    action.props = malloc(action.propcount * sizeof(ism_propent_t));
    for (i = 0; ism_common_getPropertyIndex(props, i, &name) == 0; i++) {
        ism_common_getProperty(props, name, &field);

        ent = &action.props[i];
        memcpy(&ent->f, &field, sizeof(field));
        ent->name = name;
    }

    CU_ASSERT(getintproperty(&action, "prop1", 100) == 100);
    CU_ASSERT(getintproperty(&action, "prop2", 100) == 100);
    CU_ASSERT(getintproperty(&action, "prop3", 100) == 150);
    CU_ASSERT(getintproperty(&action, "prop4", 100) == 100);
    CU_ASSERT(getintproperty(&action, "prop5", 100) == 100);
    CU_ASSERT(getintproperty(&action, "prop6", 100) == 256);	// String to decimal conversion
    CU_ASSERT(getintproperty(&action, "prop7", 100) == 256);	// No octal conversion, still decimal
    CU_ASSERT(getintproperty(&action, "prop8", 100) == 0x100);	// String to hex
    CU_ASSERT(getintproperty(&action, "none",  100) == 100);

    CU_ASSERT(getbooleanproperty(&action, "prop1") == 1);
    CU_ASSERT(getbooleanproperty(&action, "prop2") == 0);
    CU_ASSERT(getbooleanproperty(&action, "prop3") == 1);
    CU_ASSERT(getbooleanproperty(&action, "prop4") == 1);
    CU_ASSERT(getbooleanproperty(&action, "prop5") == 0);
    CU_ASSERT(getbooleanproperty(&action, "prop6") == 0);
    CU_ASSERT(getbooleanproperty(&action, "prop7") == 0);
    CU_ASSERT(getbooleanproperty(&action, "prop8") == 0);
    CU_ASSERT(getbooleanproperty(&action, "none")  == 0);

    CU_ASSERT(getproperty(&action, "prop1") == NULL);
    CU_ASSERT(getproperty(&action, "prop2") == NULL);
    CU_ASSERT(getproperty(&action, "prop3") == NULL);
    CU_ASSERT(getproperty(&action, "prop4") != NULL &&
    		!strcmp(getproperty(&action, "prop4"), "enabled"));
    CU_ASSERT(getproperty(&action, "prop5") == NULL);
    CU_ASSERT(getproperty(&action, "prop6") != NULL &&
    		!strcmp(getproperty(&action, "prop6"), "256"));
    CU_ASSERT(getproperty(&action, "prop7") != NULL &&
    		!strcmp(getproperty(&action, "prop7"), "0256"));
    CU_ASSERT(getproperty(&action, "prop8") != NULL &&
    		!strcmp(getproperty(&action, "prop8"), "0x100"));
    CU_ASSERT(getproperty(&action, "none") == NULL);

    ism_common_freeProperties(props);
    free(action.props);
}

/**
 * The close method for the transport object used for tests in
 * this source file.
 */
int jmsclose(ism_transport_t * trans, int rc, int clean, const char * reason) {
    g_entered_close_cb += 1;
    trans->closing(trans, rc, clean, reason);
    return 0;
}

/**
 * The closed method for the transport object used for tests in * this source file.
 */
int jmsclosed(ism_transport_t * trans) {
    return 0;
}

/**
 * The send method for the transport object used for tests in
 * this source file.
 */
int jmssend(ism_transport_t * trans, char * buf, int len, int kind, int flags) {
	concat_alloc_t cbuf;
	ism_field_t rcfield;
	actionhdr * hdr = (actionhdr *)buf;

    CU_ASSERT(kind == 0);
    CU_ASSERT(len >= sizeof(actionhdr));

    cbuf.buf = buf;
    cbuf.len = len;
    cbuf.used = len;
    cbuf.pos = sizeof(actionhdr);

    g_cmd_result = -99;
    if (hdr->hdrcount > 0) {
    	ism_protocol_getObjectValue(&cbuf, &rcfield);
    	if (rcfield.type == VT_Integer) {
    		g_cmd_result = rcfield.val.i;
    	}
    }

    if (hdr->hdrcount > 1) {
    	ism_protocol_getObjectValue(&cbuf, &rcfield);
    	if (rcfield.type == VT_Integer) {
    		g_cmd_int_result2 = rcfield.val.i;
    	}
    	if (rcfield.type == VT_Long) {
    		g_cmd_long_result2 = rcfield.val.l;
    	}
    }

    return 0;
}

int jmsReceive_test(ism_transport_t * transport, char * cmd_buf, int buflen, int check_response) {
    g_cmd = NULL;
    g_cmd_result = -1;
    char l_conn_buf[70000];
    char l_cmd_buf[70000];
    actionhdr action;
    memset(l_conn_buf, 0, sizeof(l_conn_buf));
    memset(l_cmd_buf, 0, sizeof(l_cmd_buf));

    if (cmd_buf)
        memcpy(l_cmd_buf,  cmd_buf, buflen);

    if (cmd_buf && check_response) {
        memcpy(&action, cmd_buf, sizeof(actionhdr));
        g_action = action.action;
    }

    return jmsReceive(transport, l_cmd_buf, buflen, 0);
}

/* Test pobj closed causes failure return 1 */
void jmsReceive_fail_shortaction(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    struct ism_protobj_t pobj = {0};
    actionhdr action = { 0 };
    char * buf = (char*)&action;

    jmsSetupTransport(transport, "jms", &pobj, __LINE__);

    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr) - 1, 0) == ISMRC_MessageNotValid);

    ism_transport_freeTransport(transport);
}

void jmsReceive_fail_pobjclosed(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    struct ism_protobj_t pobj = {0};
    actionhdr action = { 0 };
    char * buf = (char*)&action;

    action.itemtype = ITEMT_Thread;		/* To pass itemtype/item value check */
    pobj.closed = 1;

    jmsSetupTransport(transport, "jms", &pobj, __LINE__);

    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == ISMRC_Closed);

    freeTransport(transport);
}

/*
 * Simulate close in progress
 */
void jmsReceive_fail_closeinprogress(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    struct ism_protobj_t pobj = {0};
    actionhdr action = { 0 };
    char * buf = (char*)&action;

    action.itemtype = ITEMT_Thread;		/* To pass itemtype/item value check */
    pobj.inprogress = -1;

    jmsSetupTransport(transport, "jms", &pobj, __LINE__);

    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == ISMRC_Closed);

    freeTransport(transport);
}

/*
 * Simulate bad properties
 */
void jmsReceive_fail_invalidproperties(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    struct ism_protobj_t pobj = {0};
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    ism_actionbuf_t map;
    int len;
    int savepos;
    ism_field_t field;

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConnection;
    action->itemtype = ITEMT_Thread;

    jmsSetupTransport(transport, "jms", &pobj, __LINE__);

    /* Empty buffer instead of properties */
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(buf), 0) == ISMRC_MessageNotValid);

    /* Bad properties */
    memset(&pobj, 0x00, sizeof(pobj));
    memset(&buf, 0x00, sizeof(buf));

    action->action = Action_createConnection;
    action->itemtype = ITEMT_Thread;

    map.buf = buf;
    map.used = sizeof(actionhdr);
    map.pos = sizeof(actionhdr);
    map.len = sizeof(buf);

    savepos = map.pos;

    map.buf[map.used] = (char)(S_Map+3);
    map.used += 4;

    ism_protocol_putNameValue(&map, "ClientID");
    field.type = VT_String;
    field.val.s = "test12345";
    ism_protocol_putObjectValue(&map, &field);
    ism_protocol_putNameValue(&map, "userid");
    field.type = VT_String;
    field.val.s = "someuser123";
    ism_protocol_putObjectValue(&map, &field);

    /* Specify that message is larger than it is */
    len = map.used - savepos - 4 + 1;
    map.buf[savepos+1] = (char)(len >> 16);
    map.buf[savepos+2] = (char)((len >> 8) & 0xff);
    map.buf[savepos+3] = (char)(len & 0xff);

    jmsSetupTransport(transport, "jms", &pobj, __LINE__);

    /* The buffer has incorrect length specified -> invalid properties */
    CU_ASSERT(jmsReceive_test(transport, buf, map.used, 0) == ISMRC_PropertiesNotValid);

    freeTransport(transport);
}

/*
 * Connect without client id
 */
void jmsReceive_fail_noclientid(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    struct ism_protobj_t pobj = {0};
    ism_actionbuf_t map;
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    int len;
    int savepos;
    ism_field_t field;

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConnection;
    action->itemtype = ITEMT_Thread;

    map.buf = buf;
    map.used = sizeof(actionhdr);
    map.pos = sizeof(actionhdr);
    map.len = sizeof(buf);
    savepos = map.used;

    map.buf[map.used] = (char)(S_Map+3);
    map.used += 4;

    ism_protocol_putNameValue(&map, "userid");
    field.type = VT_String;
    field.val.s = "someuser123";
    ism_protocol_putObjectValue(&map, &field);

    len = map.used - savepos - 4;
    map.buf[savepos+1] = (char)(len >> 16);
    map.buf[savepos+2] = (char)((len >> 8) & 0xff);
    map.buf[savepos+3] = (char)(len & 0xff);

    jmsSetupTransport(transport, "jms", &pobj, __LINE__);

    /* Empty buffer instead of properties */
    CU_ASSERT(jmsReceive_test(transport, buf, map.used, 0) == 0);
    CU_ASSERT(g_cmd_result == ISMRC_ClientIDRequired);

    freeTransport(transport);
}

/*
 * Simulate authentication failure.
 */
void jmsReceive_fail_authentication(void) {
    ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    struct ism_protobj_t pobj = {0};
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    ism_actionbuf_t map;
    int len;
    int savepos;
    ism_field_t field;

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConnection;
    action->itemtype = ITEMT_Thread;

    map.buf = buf;
    map.used = sizeof(actionhdr);
    map.pos = sizeof(actionhdr);
    map.len = sizeof(buf);
    savepos = map.pos;

    map.buf[map.used] = (char)(S_Map+3);
    map.used += 4;

    ism_protocol_putNameValue(&map, "ClientID");
    field.type = VT_String;
    field.val.s = "test12345";
    ism_protocol_putObjectValue(&map, &field);

    ism_protocol_putNameValue(&map, "userid");
    field.type = VT_String;
    field.val.s = "someuser123";
    ism_protocol_putObjectValue(&map, &field);

    len = map.used - savepos - 4;
    map.buf[savepos+1] = (char)(len >> 16);
    map.buf[savepos+2] = (char)((len >> 8) & 0xff);
    map.buf[savepos+3] = (char)(len & 0xff);

    jmsSetupTransport(transport, "jms", &pobj, __LINE__);

    /* Force authentication failure */
    g_authentication_fails = 1;
    g_entered_authenticate_user = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, map.used, 0) == 0);
    CU_ASSERT(g_cmd_result == ISMRC_NotAuthenticated);
    CU_ASSERT(g_entered_authenticate_user == 1);
    g_authentication_fails = 0;

    freeTransport(transport);
}

/*
 * Simulate authorization failure.
 */
void jmsReceive_fail_authorization(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    struct ism_protobj_t pobj = {0};
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    ism_actionbuf_t map;
    int len;
    int savepos;
    ism_field_t field;

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConnection;
    action->itemtype = ITEMT_Thread;

    map.buf = buf;
    map.used = sizeof(actionhdr);
    map.pos = sizeof(actionhdr);
    map.len = sizeof(buf);
    savepos = map.pos;

    map.buf[map.used] = (char)(S_Map+3);
    map.used += 4;

    ism_protocol_putNameValue(&map, "ClientID");
    field.type = VT_String;
    field.val.s = "test12345";
    ism_protocol_putObjectValue(&map, &field);

    ism_protocol_putNameValue(&map, "userid");
    field.type = VT_String;
    field.val.s = "someuser123";
    ism_protocol_putObjectValue(&map, &field);

    len = map.used - savepos - 4;
    map.buf[savepos+1] = (char)(len >> 16);
    map.buf[savepos+2] = (char)((len >> 8) & 0xff);
    map.buf[savepos+3] = (char)(len & 0xff);

    jmsSetupTransport(transport, "jms", &pobj, __LINE__);

    /* Force authorization failure */
    g_authentication_fails = 0;
    g_authorization_fails = 1;
    g_entered_authorize_user = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, map.used, 0) == 0);
    // TODO: Reimplement this to match current security code
    //CU_ASSERT(g_cmd_result == ISMRC_NotAuthorized);
    //CU_ASSERT(g_entered_authorize_user == 1);
    g_authorization_fails = 0;

    freeTransport(transport);
}

/*
 * Establish connection (client id specified);
 */
void jmsReceive_pass_createconnection(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    struct ism_protobj_t pobj = {0};
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    ism_actionbuf_t map;
    int len;
    int savepos;
    ism_field_t field;

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConnection;
    action->itemtype = ITEMT_Thread;

    map.buf = buf;
    map.used = sizeof(actionhdr);
    map.pos = sizeof(actionhdr);
    map.len = sizeof(buf);
    savepos = map.pos;

    map.buf[map.used] = (char)(S_Map+3);
    map.used += 4;

    ism_protocol_putNameValue(&map, "ClientID");
    field.type = VT_String;
    field.val.s = "test12345";
    ism_protocol_putObjectValue(&map, &field);

    ism_protocol_putNameValue(&map, "userid");
    field.type = VT_String;
    field.val.s = "someuser123";
    ism_protocol_putObjectValue(&map, &field);

    len = map.used - savepos - 4;
    map.buf[savepos+1] = (char)(len >> 16);
    map.buf[savepos+2] = (char)((len >> 8) & 0xff);
    map.buf[savepos+3] = (char)(len & 0xff);

    jmsSetupTransport(transport, "jms", &pobj, __LINE__);

    /* Ensure nothing fails and the connection is not closed */
    g_entered_close_cb = 0;
    g_entered_createClient = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, map.used, 0) == 0);
    CU_ASSERT(g_entered_createClient == 1);
    CU_ASSERT(g_cmd_result == 0);
    CU_ASSERT(g_entered_close_cb == 0);

    freeTransport(transport);
}

void testJmsReceive(void) {
	/* Test short message failure */
	jmsReceive_fail_shortaction();

    /* Test pobj closed causes failure */
    jmsReceive_fail_pobjclosed();

    /* Test "close in progress" failure */
    jmsReceive_fail_closeinprogress();

    /* Test bad properties failure */
    jmsReceive_fail_invalidproperties();

    /* Test "no client id" failure during createConnection */
    jmsReceive_fail_noclientid();

    /* Test authentication failure */
    jmsReceive_fail_authentication();

    /* Test authorization failure */
    jmsReceive_fail_authorization();

    /* Test establishing connection */
    jmsReceive_pass_createconnection();

    /* Test create session */
    jmsReceive_pass_createsession();

    /* Test failure to create session */
    jmsReceive_fail_createsession();

    /* Test close session */
    jmsReceive_pass_closesession();

    /* Test failure to close session */
    jmsReceive_fail_closesession();

    /* Test starting connection */
    jmsReceive_pass_startconnection();

    /* Test stopping connection */
    jmsReceive_pass_stopconnection();

    /* Test create transaction */
    jmsReceive_pass_createtransaction();
    jmsReceive_fail_createtransaction();

    /* Test create consumer */
    jmsReceive_pass_createconsumer();
    jmsReceive_fail_createconsumer();
}

void jmsReceive_pass_createsession(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
	jmsProtoObj_t * pobj;
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;

    jmsCreateConnection(transport);

    /* New request should create session */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createSession;
    action->itemtype = ITEMT_Thread;

    g_entered_close_cb = 0;
    g_cmd_int_result2 = -1;
    g_entered_createSession = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    CU_ASSERT(g_cmd_result == 0);
    CU_ASSERT(g_cmd_int_result2 > 0);
    CU_ASSERT(g_entered_close_cb == 0);
    CU_ASSERT(g_entered_createSession == 1);
    jmsCreateSession(transport, 0, 1);
    pobj = transport->pobj;
    CU_ASSERT(pobj->sessions_used == 2);
    jmsCreateSession(transport, 0, 1);
    CU_ASSERT(pobj->sessions_used == 3);

    freeTransport(transport);
}

void jmsReceive_fail_createsession(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
	jmsProtoObj_t * pobj;

    jmsCreateConnection(transport);

    /* New request should create session */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createSession;
    action->itemtype = ITEMT_Thread;
	pobj = transport->pobj;

    g_entered_close_cb = 0;
    g_cmd_result = 0;
    pobj->handle = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    CU_ASSERT(g_entered_close_cb == 1);
    CU_ASSERT(g_cmd_result == ISMRC_BadClientData);

    if (pobj) {
    	int i;
    	ism_common_free_anyType(pobj->sessions);
    	for (i = 0; i < pobj->prodcons_alloc; i++) {
    		ism_common_free_anyType(pobj->prodcons[i]);
    	}
    	ism_common_free_anyType(pobj->prodcons);
    }

    ism_transport_freeTransport(transport);
}

void jmsCreateConnection(ism_transport_t * transport) {
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    ism_actionbuf_t map;
    int len;
    int savepos;
    ism_field_t field;

    transport->protocol = "jms";
    jmsConnection(transport);

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConnection;
    action->itemtype = ITEMT_Thread;

    map.buf = buf;
    map.used = sizeof(actionhdr);
    map.pos = sizeof(actionhdr);
    map.len = sizeof(buf);
    savepos = map.pos;

    map.buf[map.used] = (char)(S_Map+3);
    map.used += 4;

    ism_protocol_putNameValue(&map, "ClientID");
    field.type = VT_String;
    field.val.s = "test12345";
    ism_protocol_putObjectValue(&map, &field);

    ism_protocol_putNameValue(&map, "userid");
    field.type = VT_String;
    field.val.s = "someuser123";
    ism_protocol_putObjectValue(&map, &field);

    len = map.used - savepos - 4;
    map.buf[savepos+1] = (char)(len >> 16);
    map.buf[savepos+2] = (char)((len >> 8) & 0xff);
    map.buf[savepos+3] = (char)(len & 0xff);

    jmsSetupTransport(transport, "jms", transport->pobj, __LINE__);

    /* Ensure nothing fails and the connection is not closed */
    g_entered_close_cb = 0;
    g_entered_createClient = 0;
    CU_ASSERT_FATAL(jmsReceive_test(transport, buf, map.used, 0) == 0);
    CU_ASSERT_FATAL(g_entered_createClient == 1);
    CU_ASSERT_FATAL(g_cmd_result == 0);
    CU_ASSERT_FATAL(g_entered_close_cb == 0);

    ((jmsProtoObj_t*)transport->pobj)->handle = (void *)(uint64_t)1000;
}

int jmsCreateSession(ism_transport_t * transport, int transacted, int ackmode) {
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    ism_actionbuf_t map;
	jmsProtoObj_t * pobj;

    /* New request should create session */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createSession;
    action->itemtype = ITEMT_Thread;

    map.buf = buf;
    map.used = sizeof(actionhdr);
    map.pos = sizeof(actionhdr);
    map.len = sizeof(buf);

    ism_protocol_putIntValue(&map, 0);                 /* val0 = domain     */
    ism_protocol_putBooleanValue(&map, transacted);    /* val1 = transacted */
    ism_protocol_putIntValue(&map, ackmode);           /* val2 = ackmode    */
    action->hdrcount = 3;

    g_entered_close_cb = 0;
    g_cmd_int_result2 = -1;
    g_entered_createSession = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, map.used, 0) == 0);
    CU_ASSERT_FATAL(g_cmd_result == 0);
    CU_ASSERT_FATAL(g_cmd_int_result2 > 0);
    CU_ASSERT(g_entered_close_cb == 0);
    CU_ASSERT_FATAL(g_entered_createSession == 1);
    pobj = transport->pobj;

    pobj->sessions[g_cmd_int_result2]->handle = (void *)(uint64_t)g_cmd_int_result2;
    CU_ASSERT(pobj->sessions[g_cmd_int_result2]->transacted == transacted);
    CU_ASSERT(pobj->sessions[g_cmd_int_result2]->ackmode == ackmode);

    return g_cmd_int_result2;
}

//xUNUSED int jmsCreateConsumer(ism_transport_t * transport, int sessionid, int domain, char * destname,
//		const char * selector, int ack, int nolocal, int orderid) {
//    char buf[200];
//    actionhdr * action = (actionhdr *)&buf;
//    ism_field_t field;
//    concat_alloc_t xbuf = { buf, sizeof(buf) };
//    ism_prop_t * props = ism_common_newProperties(20);
//
//    /* New request should create consumer */
//    memset(buf, 0x00, sizeof(buf));
//    action->action = Action_createConsumer;
//    action->itemtype = ITEMT_Session;
//    action->item = endian_int32(sessionid);
//
//    field.type = VT_String;
//    field.val.s = destname;
//    ism_common_setProperty(props, "Name", &field);
//
//    field.type = VT_Boolean;
//    field.val.i = nolocal?1:0;
//    ism_common_setProperty(props, "noLocal", &field);
//
//    if (!ack) {
//        field.type = VT_Boolean;
//        field.val.i = 1;
//        ism_common_setProperty(props, "DisableACK", &field);
//    }
//
//    if (orderid) {
//        field.type = VT_Boolean;
//        field.val.i = 1;
//        ism_common_setProperty(props, "RequestOrderID", &field);
//    }
//
//    xbuf.used = sizeof(actionhdr);
//
//    ism_protocol_putByteValue(&xbuf, domain);                /* val0 = domain   */
//    ism_protocol_putBooleanValue(&xbuf, nolocal);            /* val1 = nolocal  */
//    if (selector) {
//    	ism_protocol_putStringValue(&xbuf, selector);        /* val2 = selector */
//    	action->hdrcount = 3;
//    } else {
//    	action->hdrcount = 2;
//    }
//
//    ism_protocol_putMapProperties(&xbuf, props);
//
//    ism_common_freeProperties(props);
//
//    g_entered_close_cb = 0;
//    g_entered_createConsumer = 0;
//    CU_ASSERT(jmsReceive_test(transport, buf, xbuf.used, 0) == 0);
//    CU_ASSERT_FATAL(g_cmd_result == 0);
//    CU_ASSERT_FATAL(g_entered_close_cb == 0);
//    CU_ASSERT_FATAL(g_entered_createConsumer == 1);
//    CU_ASSERT_FATAL(g_cmd_int_result2 > 0);
//
//    ((jmsProtoObj_t*)transport->pobj)->prodcons[g_cmd_int_result2]->handle = (void *)(uint64_t)g_cmd_int_result2;
//
//    return g_cmd_int_result2;
//}

void jmsReceive_pass_closeconnection(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
	jmsProtoObj_t * pobj;

    jmsCreateConnection(transport);

    /* New request should create session */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_closeConnection;
    action->itemtype = ITEMT_Thread;
	pobj = transport->pobj;

    pobj->started = 1;
    pobj->prodcons_used = 3;
    pobj->sessions_used = 2;

    g_cmd_result = 0;
    g_entered_destroyClient = 0;
    g_entered_cancel_timer = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    CU_ASSERT(g_entered_destroyClient == 1);
    CU_ASSERT(g_cmd_result == 0);
    CU_ASSERT(pobj->started == 0);
    CU_ASSERT(pobj->prodcons_used == 0);
    CU_ASSERT(pobj->sessions_used == 0);
    CU_ASSERT(g_entered_cancel_timer == 1);

    freeTransport(transport);
}

void jmsReceive_fail_closeconnection(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
	jmsProtoObj_t * pobj;

    jmsCreateConnection(transport);

    /* New request should create session */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_closeConnection;
    action->itemtype = ITEMT_Thread;

	pobj = transport->pobj;
    pobj->handle = 0;

    g_cmd_result = 0;
    g_entered_destroyClient = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    CU_ASSERT(g_entered_destroyClient == 0);
    CU_ASSERT(g_cmd_result == ISMRC_BadClientData);

    freeTransport(transport);
}

void jmsReceive_pass_closesession(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    int sessionId1, sessionId2;
	jmsProtoObj_t * pobj = transport->pobj;

    jmsCreateConnection(transport);
    sessionId1 = jmsCreateSession(transport, 0, 1);
    sessionId2 = jmsCreateSession(transport, 0, 1);
	pobj = transport->pobj;

    CU_ASSERT_FATAL(pobj->sessions_used == 2);

    /* The request should close session */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_closeSession;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId1);

    g_cmd_result = 0;
    g_entered_stopMessageDelivery = 0;
    g_entered_destroySession = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    CU_ASSERT(g_entered_stopMessageDelivery == 0);		// Session was not started
    CU_ASSERT(g_cmd_result == 0);
    CU_ASSERT(g_entered_destroySession == 1);
    CU_ASSERT(pobj->sessions_used == 1);

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_closeSession;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId2);

    g_cmd_result = 0;
    g_entered_stopMessageDelivery = 0;
    g_entered_destroySession = 0;
    pobj->sessions[sessionId2]->suspended = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    CU_ASSERT(g_entered_stopMessageDelivery == 1);
    CU_ASSERT(g_cmd_result == 0);
    CU_ASSERT(pobj->sessions_used == 0);
    CU_ASSERT(g_entered_destroySession == 1);

    freeTransport(transport);
}

void jmsReceive_fail_closesession(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    int sessionId;
	jmsProtoObj_t * pobj;

    jmsCreateConnection(transport);
    sessionId = jmsCreateSession(transport, 0, 1);
	pobj = transport->pobj;

    CU_ASSERT_FATAL(pobj->sessions_used == 1);

    /* The request should close session */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_closeSession;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId);

    g_cmd_result = 0;
    g_entered_stopMessageDelivery = 0;
    g_entered_destroySession = 0;
    pobj->sessions[sessionId]->suspended = 0;	// "Start" the session
    pobj->sessions[sessionId]->handle = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == ISMRC_MessageNotValid);
    CU_ASSERT(g_entered_stopMessageDelivery == 0);
    CU_ASSERT(g_cmd_result == -1);
    CU_ASSERT(g_entered_destroySession == 0);
    CU_ASSERT(pobj->sessions_used == 0);

    freeTransport(transport);
}

void freeTransport(ism_transport_t * transport) {
    if (transport->pobj) {
    	int i;
    	jmsProtoObj_t * pobj = transport->pobj;
    	ism_common_free_anyType(pobj->sessions);
    	for (i = 0; i < pobj->prodcons_alloc; i++) {
    		ism_common_free_anyType(pobj->prodcons[i]);
    	}
    	ism_common_free_anyType(pobj->prodcons);
    }

    ism_transport_freeTransport(transport);
}

void jmsReceive_pass_startconnection(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    int sessionId1, sessionId2, sessionId3;
	jmsProtoObj_t * pobj;

    jmsCreateConnection(transport);
    sessionId1 = jmsCreateSession(transport, 0, 1);
    sessionId2 = jmsCreateSession(transport, 0, 1);
    sessionId3 = jmsCreateSession(transport, 0, 1);

    /* The request should start the connection */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_startConnection;
    action->itemtype = ITEMT_Thread;

	pobj = transport->pobj;
    g_cmd_result = 0;
    g_entered_jmsresume = 0;
    pobj->started = 0;
    CU_ASSERT(pobj->sessions[sessionId1]->suspended == SUSPENDED_BY_PROTOCOL);
    CU_ASSERT(pobj->sessions[sessionId2]->suspended == SUSPENDED_BY_PROTOCOL);
    CU_ASSERT(pobj->sessions[sessionId3]->suspended == SUSPENDED_BY_PROTOCOL);
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
//    CU_ASSERT(g_entered_jmsresume == 1);
    CU_ASSERT(g_cmd_result == 0);
    CU_ASSERT(pobj->started == 1);
    CU_ASSERT(pobj->sessions[sessionId1]->suspended == 0);
    CU_ASSERT(pobj->sessions[sessionId2]->suspended == 0);
    CU_ASSERT(pobj->sessions[sessionId3]->suspended == 0);

    freeTransport(transport);
}

void jmsReceive_pass_stopconnection(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    int sessionId[3];
    jmsProtoObj_t * pobj;

    jmsCreateConnection(transport);
    for(int i = 0; i < 3; i++) {
        sessionId[i] = jmsCreateSession(transport, 0, 1);
    }

    for(int i = 0; i < 3; i++) {
        /* The request should start the connection */
        memset(buf, 0x00, sizeof(buf));
        action->action = Action_stopSession;
        action->itemtype = ITEMT_Session;
        action->item = endian_int32(sessionId[i]);
        pobj = transport->pobj;

        g_cmd_result = 0;
        pobj->started = 1;
        g_entered_stopMessageDelivery = 0;
        pobj->sessions[sessionId[i]]->suspended = 0;
        CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
        CU_ASSERT(g_entered_stopMessageDelivery == 1);
        CU_ASSERT(g_cmd_result == 0);
        /* We have only stopped message delivery for the session.  The 
         * connection is not yet stopped.
         */
        CU_ASSERT(pobj->started == 1);
        CU_ASSERT(pobj->sessions[sessionId[i]]->suspended == SUSPENDED_BY_PROTOCOL);
    }

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_stopConnection;
    action->itemtype = ITEMT_Thread;

    g_cmd_result = 0;
    pobj->started = 1;
    g_entered_stopMessageDelivery = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    /*
     * We only call stopMessageDelivery while stopping the individual sessions
     * with the stopSession action.
     */
    CU_ASSERT(g_entered_stopMessageDelivery == 0);
    CU_ASSERT(g_cmd_result == 0);
    /*
     * The connection stop has completed and the state is now relected as not started.
     */
    CU_ASSERT(pobj->started == 0);

    freeTransport(transport);
}

int simpleJmsResume(ism_transport_t * transport, void * userdata) {
	int i;
	jmsProtoObj_t * pobj = transport->pobj;

	g_entered_jmsresume = 1;

	if (!pobj->started) {
		return 0;
	}

	for (i = 0; i < pobj->sessions_alloc; i++) {
		if (pobj->sessions[i]) {
			pobj->sessions[i]->suspended = 0;
		}
	}

	return 0;
}

void jmsReceive_pass_createtransaction(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    int sessionId1, sessionId2, sessionId3;
	jmsProtoObj_t * pobj;

    jmsCreateConnection(transport);
    sessionId1 = jmsCreateSession(transport, 0, 1);
    sessionId2 = jmsCreateSession(transport, 1, 0);
    sessionId3 = jmsCreateSession(transport, 0, 1);

    /* The request should start the connection */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createTransaction;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId2);
	pobj = transport->pobj;

    g_cmd_result = 0;
    pobj->started = 1;
    g_entered_createLocalTransaction = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    CU_ASSERT(g_entered_createLocalTransaction == 1);
    CU_ASSERT(g_cmd_result == 0);
    CU_ASSERT(pobj->sessions[sessionId1]->transaction == NULL);
    CU_ASSERT(pobj->sessions[sessionId2]->transaction != NULL);
    CU_ASSERT(pobj->sessions[sessionId3]->transaction == NULL);

    freeTransport(transport);
}

void jmsReceive_fail_createtransaction(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    xUNUSED int sessionId1, sessionId2, sessionId3;
	jmsProtoObj_t * pobj = transport->pobj;

    jmsCreateConnection(transport);
    sessionId1 = jmsCreateSession(transport, 1, 0);
    sessionId2 = jmsCreateSession(transport, 0, 1);
    sessionId3 = jmsCreateSession(transport, 0, 1);

    /* The request should create transaction for transacted session */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createTransaction;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId1);

	pobj = transport->pobj;
    g_cmd_result = 0;
    g_entered_createLocalTransaction = 0;
    /* Handle 0, fail */
    pobj->sessions[sessionId1]->handle = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == ISMRC_MessageNotValid);
    CU_ASSERT(g_entered_createLocalTransaction == 0);
    CU_ASSERT(g_cmd_result == -1);

    freeTransport(transport);

    transport = ism_transport_newTransport(NULL, 64, 0);

    jmsCreateConnection(transport);
    sessionId1 = jmsCreateSession(transport, 1, 0);
    sessionId2 = jmsCreateSession(transport, 0, 1);
    sessionId3 = jmsCreateSession(transport, 1, 0);

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createTransaction;
    action->itemtype = ITEMT_Session;
    /* Non-transacted session, fail */
    action->item = endian_int32(sessionId2);

    g_cmd_result = 0;
    g_entered_createLocalTransaction = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    CU_ASSERT(g_entered_createLocalTransaction == 0);
    CU_ASSERT(g_cmd_result == ISMRC_BadClientData);

    freeTransport(transport);

    transport = ism_transport_newTransport(NULL, 64, 0);

    jmsCreateConnection(transport);
    sessionId1 = jmsCreateSession(transport, 1, 0);
    sessionId2 = jmsCreateSession(transport, 1, 0);
    sessionId3 = jmsCreateSession(transport, 1, 0);

    /* The request should start the connection */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createTransaction;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId1);

	pobj = transport->pobj;
    g_cmd_result = 0;
    g_entered_createLocalTransaction = 0;
    /* Bad connection, fail */
    pobj->handle = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, sizeof(actionhdr), 0) == 0);
    CU_ASSERT(g_entered_createLocalTransaction == 0);
    CU_ASSERT(g_cmd_result == ISMRC_BadClientData);

    freeTransport(transport);
}

void jmsReceive_pass_createconsumer(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    xUNUSED int sessionId1, sessionId2, sessionId3;
    int consId1, consId2, consId3;
    ism_field_t field;
    concat_alloc_t xbuf = { buf, sizeof(buf) };
    ism_prop_t * props = ism_common_newProperties(20);
	jmsProtoObj_t * pobj;

    jmsCreateConnection(transport);
    sessionId1 = jmsCreateSession(transport, 0, 1);
    sessionId2 = jmsCreateSession(transport, 0, 1);
    sessionId3 = jmsCreateSession(transport, 0, 1);

    /* New request should create consumer */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConsumer;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId1);

    /* val0=domain, val1=nolocal, val2=selector, props=dest */
    field.type = VT_String;
    field.val.s = "TestName";
    ism_common_setProperty(props, "Name", &field);

    field.type = VT_Boolean;
    field.val.i = 0;
    ism_common_setProperty(props, "noLocal", &field);

    xbuf.used = sizeof(actionhdr);

    ism_protocol_putByteValue(&xbuf, 1);                  /* val0 = domain   */
//    ism_protocol_putBooleanValue(&xbuf, 0);             /* val1 = nolocal  */
//    ism_protocol_putStringValue(&xbuf, "");             /* val2 = selector */
    action->hdrcount = 1;

    ism_protocol_putMapProperties(&xbuf, props);

    g_entered_close_cb = 0;
    g_cmd_int_result2 = -1;
    g_entered_createConsumer = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, xbuf.used, 0) == 0);
    CU_ASSERT_FATAL(g_cmd_result == 0);
    CU_ASSERT_FATAL(g_cmd_int_result2 > 0);
    CU_ASSERT(g_entered_close_cb == 0);
    CU_ASSERT_FATAL(g_entered_createConsumer == 1);

    consId1 = g_cmd_int_result2;

	pobj = transport->pobj;
    CU_ASSERT(pobj->prodcons_used == 1);
    CU_ASSERT_FATAL(pobj->prodcons[consId1] != NULL);
    CU_ASSERT(pobj->prodcons[consId1]->closing == 0);

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConsumer;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId2);

    xbuf.used = sizeof(actionhdr);

    ism_protocol_putByteValue(&xbuf, 1);                  /* val0 = domain   */
    action->hdrcount = 1;

    ism_protocol_putMapProperties(&xbuf, props);

    g_entered_close_cb = 0;
    g_cmd_int_result2 = -1;
    g_entered_createConsumer = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, xbuf.used, 0) == 0);
    CU_ASSERT_FATAL(g_cmd_result == 0);
    CU_ASSERT_FATAL(g_cmd_int_result2 > 0);
    CU_ASSERT(g_entered_close_cb == 0);
    CU_ASSERT_FATAL(g_entered_createConsumer == 1);

    consId2 = g_cmd_int_result2;
	pobj = transport->pobj;

    CU_ASSERT(pobj->prodcons_used == 2);
    CU_ASSERT_FATAL(pobj->prodcons[consId2] != NULL);
    CU_ASSERT(pobj->prodcons[consId2]->closing == 0);

    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConsumer;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId2);

    xbuf.used = sizeof(actionhdr);

    ism_protocol_putByteValue(&xbuf, 1);                  /* val0 = domain   */
    action->hdrcount = 1;

    ism_protocol_putMapProperties(&xbuf, props);

    g_entered_close_cb = 0;
    g_cmd_int_result2 = -1;
    g_entered_createConsumer = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, xbuf.used, 0) == 0);
    CU_ASSERT_FATAL(g_cmd_result == 0);
    CU_ASSERT_FATAL(g_cmd_int_result2 > 0);
    CU_ASSERT(g_entered_close_cb == 0);
    CU_ASSERT_FATAL(g_entered_createConsumer == 1);

    consId3 = g_cmd_int_result2;
	pobj = transport->pobj;

    CU_ASSERT(pobj->prodcons_used == 3);
    CU_ASSERT_FATAL(pobj->prodcons[consId3] != NULL);
    CU_ASSERT(pobj->prodcons[consId3]->closing == 0);

    ism_common_freeProperties(props);
    freeTransport(transport);
}

/*
 * Make sure consumer creation fails if destination name is not specified
 * or if session does not exist
 */
void jmsReceive_fail_createconsumer(void) {
	ism_transport_t * transport = ism_transport_newTransport(NULL, 64, 0);
    char buf[200];
    actionhdr * action = (actionhdr *)&buf;
    int sessionId;
    ism_field_t field;
    concat_alloc_t xbuf = { buf, sizeof(buf) };
    ism_prop_t * props = ism_common_newProperties(20);
	jmsProtoObj_t * pobj;

    jmsCreateConnection(transport);
    sessionId = jmsCreateSession(transport, 0, 1);

    /* New request should create consumer */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConsumer;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId);

    /* Skip "Name" property - should fail */

    field.type = VT_Boolean;
    field.val.i = 0;
    ism_common_setProperty(props, "noLocal", &field);

    xbuf.used = sizeof(actionhdr);

    ism_protocol_putByteValue(&xbuf, 1);                /* val0 = domain   */
    ism_protocol_putBooleanValue(&xbuf, 0);             /* val1 = nolocal  */
    ism_protocol_putStringValue(&xbuf, "");             /* val2 = selector */
    action->hdrcount = 3;

    ism_protocol_putMapProperties(&xbuf, props);

    g_entered_close_cb = 0;
    g_entered_createConsumer = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, xbuf.used, 0) == 0);
    CU_ASSERT(g_cmd_result == ISMRC_NoDestination);
    CU_ASSERT(g_entered_close_cb == 0);
    CU_ASSERT(g_entered_createConsumer == 0);
	pobj = transport->pobj;

    CU_ASSERT(pobj->prodcons_used == 0);

    ism_common_freeProperties(props);
    freeTransport(transport);

    props = ism_common_newProperties(20);
    transport = ism_transport_newTransport(NULL, 64, 0);
    jmsCreateConnection(transport);
    sessionId = jmsCreateSession(transport, 0, 1);

    /* New request should create consumer */
    memset(buf, 0x00, sizeof(buf));
    action->action = Action_createConsumer;
    action->itemtype = ITEMT_Session;
    action->item = endian_int32(sessionId);

    field.type = VT_String;
    field.val.s = "TestName";
    ism_common_setProperty(props, "Name", &field);

    xbuf.used = sizeof(actionhdr);

    ism_protocol_putByteValue(&xbuf, 1);                /* val0 = domain   */
    ism_protocol_putBooleanValue(&xbuf, 0);             /* val1 = nolocal  */
    ism_protocol_putStringValue(&xbuf, "");             /* val2 = selector */
    action->hdrcount = 3;

    ism_protocol_putMapProperties(&xbuf, props);

    g_entered_close_cb = 0;
    g_entered_createConsumer = 0;
    /* Set session handle to NULL, should fail */
	pobj = transport->pobj;
    pobj->sessions[sessionId]->handle = 0;
    CU_ASSERT(jmsReceive_test(transport, buf, xbuf.used, 0) == ISMRC_MessageNotValid);
    CU_ASSERT(g_cmd_result == -1);
    CU_ASSERT(g_entered_close_cb == 1);
    CU_ASSERT(g_entered_createConsumer == 0);

    CU_ASSERT(pobj->prodcons_used == 0);

    ism_common_freeProperties(props);
    freeTransport(transport);

}
