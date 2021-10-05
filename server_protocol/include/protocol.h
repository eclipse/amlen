/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

#ifndef __PROTOCOL_DEFINED
#define __PROTOCOL_DEFINED

#include <ismutil.h>
struct ism_transport_t;

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file protocol.h
 *
 * There can be multiple protocols
 */

/*
 * Header for JMS action or message.
 * All multibyte header fields are passed as big endian.
 */
typedef struct actionhdr {
    uint8_t   action;        /* Action to do                                 */
    uint8_t   flags;         /* See ACTFLAG_*                                */
    uint8_t   hdrcount;      /* Count of extra header fields (max 16)        */
    uint8_t   bodytype;      /* Type of body/message                         */
    uint8_t   priority;      /* Priority  0-9                                */
    uint8_t   dup;           /* Duplicate count  (255 means >= 255)          */
    uint8_t   resv;          /* Reserved for future use                      */
    uint8_t   itemtype;      /* The type of the item data                    */
    uint64_t  msgid;         /* The id number                                */
    uint32_t  item;          /* The item                                     */
} __attribute__((packed)) actionhdr;


#define ACTFLAG_Persistent    0x80
#define ACTFLAG_Retain        0x40
#define ACTFLAG_Expires       0x20
#define ACTFLAG_Suspended     0x10
#define ACTFLAG_RetainPublish 0x08
#define ACTFLAG_QoS           0x07


/**
 * Defines action item types. Should be synchronized with equivalent
 * enum in com.ibm.ima.jms.impl.IsmAction.java.
 */
enum itemtypes_e {
    ITEMT_None     = 0,	  /* Asynchronous ACKs                                                       */
    ITEMT_Thread   = 1,   /* Connection-related actions, like create connection, create session, etc */
    ITEMT_Session  = 2,   /* Session-related actions, like create consumer, create transaction, etc  */
    ITEMT_Consumer = 3,   /* Consumer-related actions, like close consumer, set msg listener, etc    */
    ITEMT_Producer = 4    /* Producer-related actions, like close producer, send message, etc        */
};

/*
 * JMS based messaging actions
 */
enum action_e {
    Action_message                = 1,
    Action_messageWait            = 2,
    Action_messageNoProd          = 3,
    Action_msgNoProdWait          = 4,
    Action_receiveMsg             = 5,
    Action_receiveMsgNoWait       = 6,
    Action_reply                  = 9,
    Action_createConnection       = 10,
    Action_startConnection        = 11,
    Action_stopConnection         = 12,
    Action_closeConnection        = 13,
    Action_createSession          = 14,
    Action_closeSession           = 15,
    Action_createConsumer         = 16,
    Action_createBrowser          = 17,
    Action_createDurable          = 18,
    Action_unsubscribeDurable     = 19,
    Action_closeConsumer          = 20,
    Action_createProducer         = 21,
    Action_closeProducer          = 22,
    Action_setMsgListener         = 23,
    Action_removeMsgListener      = 24,
    Action_rollbackSession        = 25,
    Action_commitSession          = 26,
    Action_ack                    = 27,
    Action_ackSync                = 28,
    Action_recover                = 29,
    Action_getTime                = 30,
    Action_createTransaction      = 31,
    Action_sendWill               = 32,
    Action_checkBrowser           = 33,
    Action_resumeSession          = 34,
    Action_stopSession            = 35,
    Action_raiseException         = 36,
    Action_startConsumer          = 37,
    Action_suspendConsumer        = 38,
    Action_createDestination      = 39,
    Action_initConnection         = 40,
    Action_termConnection         = 43,
    Action_updateSubscription     = 44,
    Action_createQMRecord         = 45,
    Action_destroyQMRecord        = 46,
    Action_getQMRecords           = 47,
    Action_createQMXidRecord      = 48,
    Action_destroyQMXidRecord     = 49,
    Action_getQMXidRecords        = 50,
    Action_listSubscriptions      = 51,
    Action_startGlobalTransaction = 52,
    Action_endGlobalTransaction   = 53,
    Action_prepareGlobalTransaction  = 54,
    Action_recoverGlobalTransactions  = 55,
    Action_commitGlobalTransaction  = 56,
    Action_rollbackGlobalTransaction  = 57,
    Action_forgetGlobalTransaction  = 58
};

#define MAX_ACTION_VALUE        58

#ifdef ACTION_NAMES
static xUNUSED const char * ism_action_names[MAX_ACTION_VALUE+1] = {
    "0",
    "message",                  /*  1 */
    "messageWait",              /*  2 */
    "messageNoProd",            /*  3 */
    "messageNoProdWait",        /*  4 */
    "receiveMsg",               /*  5 */
    "receiveMsgNoWait",         /*  6 */
    "7",
    "8",
    "reply",                    /*  9 */
    "createConnection",         /* 10 */
    "startConnection",          /* 11 */
    "stopConnection",           /* 12 */
    "closeConnection",          /* 13 */
    "createSession",            /* 14 */
    "closeSession",             /* 15 */
    "createConsumer",           /* 16 */
    "createBrowser",            /* 17 */
    "createDurable",            /* 18 */
    "unsubscribeDurable",       /* 19 */
    "closeConsumer",            /* 20 */
    "createProducer",           /* 21 */
    "closeProducer",            /* 22 */
    "setMsgListener",           /* 23 */
    "removeMsgListener",        /* 24 */
    "rollbackSession",          /* 25 */
    "commitSession",            /* 26 */
    "ack",                      /* 27 */
    "ackSync",                  /* 28 */
    "recover",                  /* 29 */
    "getTime",                  /* 30 */
    "createTransaction",        /* 31 */
    "sendWill",                 /* 32 */
    "checkBrowser",             /* 33 */
    "resumeSession",            /* 34 */
    "stopSession",              /* 35 */
    "raiseException",           /* 36 */
    "startConsumer",            /* 37 */
    "suspendConsumer",          /* 38 */
    "createDestination",        /* 39 */
    "initConnection",           /* 40 */
    "41",
    "42",
    "termConnection",           /* 43 */
    "updateSubscription",       /* 44 */
    "createManRecord",          /* 45 */
    "destroyManRecord",         /* 46 */
    "getManRecords",            /* 47 */
    "createXARecord",           /* 48 */
    "destroyXARecord",          /* 49 */
    "getXARecords",             /* 50 */
    "listDurableSubscriptions", /* 51 */
    "startGlobalTransaction",   /* 52 */
    "endGlobalTransaction",     /* 53 */
    "prepareGlobalTransaction", /* 54 */
    "recoverGlobalTransactions", /* 55 */
    "commitGlobalTransaction", /* 56 */
    "rollbackGlobalTransaction", /* 57 */
    "forgetGlobalTransaction" /* 58 */
};
#endif

/*
 * Return a handle to the shared client.
 * @param durable True if the durable shared client handle is to be returned
 */
void * ism_protocol_getSharedClient(int durable);

/*
 * Set the max TCP buffer size
 * This must be done after the connection policy is filled in during authentication
 * @param transport   The transport object
 */
XAPI void ism_protocol_setSocketBuffer(struct ism_transport_t * transport);

/*
 * Initialize the protocol component
 * @return A return code, 0=good
 */
XAPI int ism_protocol_init(void);


/*
 * Start the protocol
 * @return A return code, 0=good
 */
XAPI int ism_protocol_start(void);


/*
 * Terminate the protocol
 * @return A return code, 0=good
 */
XAPI int ism_protocol_term(void);

/*
 * Terminate the protocol plugin
 * @param isRestart 1 for restart. Otherwise, it is terminated.
 * @return A return code, 0=good
 */
XAPI void ism_protocol_termPlugin(void);

/**
 * 	Start or Restart the plugin.
 * 	@param isRestart Terminate and start plugin again if 1.
 */
XAPI int ism_protocol_restartPlugin(void);

XAPI int ism_protocol_isPluginServerRunning(void);
#define SUSPENDED_BY_TRANSPORT  1
#define SUSPENDED_BY_PROTOCOL   2

#ifdef __cplusplus
}
#endif
#endif
