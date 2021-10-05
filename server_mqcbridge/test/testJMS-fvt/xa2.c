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

/* A stand alone ismc client test program to recreate ismc issues outside of the bridge */

/*
gcc -g -o xa2 xa2.c -lismc -lssl -lismengine -lismmonitoring -I $BROOT/server_ship/include -I $SROOT/server_ismc/src -I $SROOT/server_protocol/src -L $BROOT/server_ship/lib  -Wl,-rpath $BROOT/server_ship/lib
*/
#define _GNU_SOURCE

#include <ismc.h>
#include <ismc_p.h>
#include <stdbool.h>

#define TOPIC "IMATOPIC"
#define SUBNAME "MYSUB"

int messages = 2;

int publish(ismc_connection_t * pConn)
{
    int i, rc = 0;
    ismc_session_t * pSess;
    ismc_message_t * pMessage;
    int transacted, ackmode;

    /* Create session */
    transacted = 1;
    ackmode    = SESSION_TRANSACTED;

    pSess = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSess <%p>\n", pSess);

    /* Publish messages */
    pMessage = ismc_createMessage(pSess, MTYPE_BytesMessage);
    printf("ismc_createMessage <%p>\n", pMessage);

    ismc_setDeliveryMode(pMessage, ISMC_PERSISTENT);

    for (i = 0; i < messages; i++)
    {
        rc = ismc_sendX(pSess, 2, TOPIC, pMessage);
        printf("ismc_sendX rc(%d)\n", rc);
    }

    /* Commit session */
    rc = ismc_commitSession(pSess);
    printf("ismc_commitSession rc(%d)\n", rc);

    return rc;
}

int main(int argc, char **argv)
{
    int xanum = 0, rc = 0;
    ismc_connection_t * pConn = NULL;
    ismc_session_t * pSessA, * pSessB;
    ismc_message_t * pMessage = NULL;
    ismc_consumer_t * pConsumer = NULL;
    bool connected = false;
    int transacted, ackmode, nolocal;
    char input[256];
    ism_xid_t xid;
    ism_xid_t xid2;
    time_t now;

    if (argc < 3)
    {
        printf("args: server port <messages>\n");
        exit(1);
    }

    char *server = argv[1];
    int port = atoi(argv[2]);

    if (argc > 3)
    {
        messages = atoi(argv[3]);
    }

    /* Create connection */
    pConn = ismc_createConnection();
    printf("ismc_createConnection pConn <%p>\n", pConn);
    if (pConn == NULL) goto MOD_EXIT;

    ismc_setStringProperty(pConn, "ClientID", "Test0001");
    ismc_setStringProperty(pConn, "Protocol", "tcp");
    ismc_setStringProperty(pConn, "Server", server);
    ismc_setIntProperty(pConn, "Port",port, VT_Integer);

    rc = ismc_connect(pConn);
    printf("ismc_connect rc(%d)\n", rc);
    if (rc) goto MOD_EXIT;

    connected = true;

    rc = ismc_startConnection(pConn);
    printf("ismc_startConenction rc(%d)\n", rc);
    if (rc) goto MOD_EXIT;

    /* Create session A */
    transacted = 1;
    ackmode    = SESSION_TRANSACTED_GLOBAL;

    pSessA = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSessA <%p>\n", pSessA);
    if (pSessA == NULL) goto MOD_EXIT;

    ismc_setIntProperty(pSessA, "RequestOrderID", true, VT_Byte);

    /* Create session B */
    transacted = 1;
    ackmode    = SESSION_TRANSACTED_GLOBAL;

    pSessB = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSessB <%p>\n", pSessB);
    if (pSessB == NULL) goto MOD_EXIT;

    ismc_setIntProperty(pSessB, "RequestOrderID", true, VT_Byte);

    /* Subscribe */
    nolocal = 0;

    pConsumer = ismc_subscribe(pSessA, TOPIC, SUBNAME, NULL, nolocal);
    printf("ismc_subscribe pConsumer <%p>\n", pConsumer);

    /* Clear subscription queue */
    while (1)
    {
        rc = ismc_receive(pConsumer, 1000, &pMessage);
        printf("ismc_receive rc(%d) pMessage<%p> orderID(%lu)\n", rc, pMessage, ismc_getOrderID(pMessage));
        if (pMessage == NULL) break;
    }

    /* Commit session A */
    rc = ismc_commitSession(pSessA);
    printf("ismc_commitSession(pSessA) rc(%d)\n", rc);

    /* Publish messages on separate session */
    if (messages) publish(pConn);

    /* Start global transaction on session A */
    time(&now);
    xid.formatID = 0;
    xid2.formatID = 0;
    sprintf((char *)xid.data, "ISM.%d", (int)now);
    sprintf((char *)xid2.data, "ISM.%d", (int)now);
    xid.gtrid_length = strlen((const char *)xid.data);
    xid2.gtrid_length = strlen((const char *)xid2.data);

    sprintf((char *)&xid.data[xid.gtrid_length], "%d", xanum++);
    xid.bqual_length = strlen((const char *)&xid.data[xid.gtrid_length]);

    rc = ismc_startGlobalTransaction(pSessA, &xid);
    printf("ismc_startGlobalTransaction(pSessA) rc(%d)\n", rc);

    rc = ismc_receive(pConsumer, 1000, &pMessage);
    printf("ismc_receive rc(%d) pMessage<%p> orderID(%lu)\n", rc, pMessage, ismc_getOrderID(pMessage));

    /* End global transaction on session A */
    rc = ismc_endGlobalTransaction(pSessA);
    printf("ismc_endGlobalTransaction(pSessA) rc(%d)\n", rc);

    /* Start new global transaction on session A */
    sprintf((char *)&xid2.data[xid2.gtrid_length], "%d", xanum++);
    xid2.bqual_length = strlen((const char *)&xid2.data[xid2.gtrid_length]);

    rc = ismc_startGlobalTransaction(pSessA, &xid2);
    printf("ismc_startGlobalTransaction(pSessA) rc(%d)\n", rc);

    rc = ismc_receive(pConsumer, 1000, &pMessage);
    printf("ismc_receive rc(%d) pMessage<%p> orderID(%lu)\n", rc, pMessage, ismc_getOrderID(pMessage));

    printf("Press any key to continue or ctrl-c to kill process.\n");
    fgets(input, sizeof(input), stdin);

    /* Prepare transactions */
    rc = ismc_prepareGlobalTransaction(pSessA, &xid2);
    printf("ismc_prepareGlobalTransaction(pSessA, &xid2) rc(%d)\n", rc);

    rc = ismc_prepareGlobalTransaction(pSessB, &xid);
    printf("ismc_prepareGlobalTransaction(pSessB, &xid) rc(%d)\n", rc);

    /* Commit transactions */
    rc = ismc_commitGlobalTransaction(pSessA, &xid2, 0);
    printf("ismc_commitGlobalTransaction(pSessA, &xid2) rc(%d)\n", rc);

    rc = ismc_commitGlobalTransaction(pSessB, &xid, 0);
    printf("ismc_commitGlobalTransaction(pSessB, &xid) rc(%d)\n", rc);

    rc = ismc_closeConsumer(pConsumer);
    printf("ismc_closeConsumer rc(%d)\n", rc);

    rc = ismc_unsubscribe(pSessA, SUBNAME);
    printf("ismc_unsubscribe rc(%d)\n", rc);

    rc = ismc_closeSession(pSessA);
    printf("ismc_closeSession(pSessA) rc(%d)\n", rc);

    rc = ismc_closeSession(pSessB);
    printf("ismc_closeSession(pSessB) rc(%d)\n", rc);

MOD_EXIT:

    if (connected)
    {
        rc = ismc_closeConnection(pConn);
        printf("ismc_closeConnection rc(%d)\n", rc);
    }

    return rc;
}

