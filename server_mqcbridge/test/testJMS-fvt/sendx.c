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
gcc -g -o sendx sendx.c -lismc -lssl -lismengine -lismmonitoring -I $BROOT/server_ship/include -I $SROOT/server_ismc/src -I $SROOT/server_protocol/src -L $BROOT/server_ship/lib  -Wl,-rpath $BROOT/server_ship/lib
*/

#include <ismc.h>
#include <ismc_p.h>
#include <stdbool.h>

#define TOPIC "1/2/3/4/5/6/7/8/9/0/1/2/3/4/5/6/7/8/9/0/1/2/3/4/5/6/7/8/9/0/1/2/3"

void errorListener(int rc, const char * error,
                   ismc_connection_t * connect, ismc_session_t * session, void * userdata)
{
    printf("errorListener rc(%d) error '%s'\n", rc, error);
    return;
}

int main(int argc, char **argv)
{
    int rc = 0;
    ismc_connection_t * pConn;
    ismc_session_t * pSess;
    ismc_message_t * pMessage;
    int transacted, ackmode;
    bool connected = false;

    /* Create connection */
    pConn = ismc_createConnection();
    printf("ismc_createConnection pConn <%p>\n", pConn);
    if (pConn == NULL) goto MOD_EXIT;

    ismc_setStringProperty(pConn, "ClientID", "Test0001");
    ismc_setStringProperty(pConn, "Protocol", "tcp");
    ismc_setStringProperty(pConn, "Server", "127.0.0.1");
    ismc_setIntProperty(pConn, "Port", 16102, VT_Integer);

    rc = ismc_connect(pConn);
    printf("ismc_connect rc(%d)\n", rc);
    if (rc) goto MOD_EXIT;

    connected = true;

    ismc_setErrorListener(pConn, errorListener, NULL);

    rc = ismc_startConnection(pConn);
    printf("ismc_startConenction rc(%d)\n", rc);
    if (rc) goto MOD_EXIT;

    /* Create session */
    transacted = 1;
    ackmode    = SESSION_TRANSACTED;

    pSess = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSess <%p>\n", pSess);

    /* Publish message */
    pMessage = ismc_createMessage(pSess, MTYPE_BytesMessage);
    printf("ismc_createMessage <%p>\n", pMessage);

    ismc_setDeliveryMode(pMessage, ISMC_PERSISTENT);

    rc = ismc_sendX(pSess, 2, TOPIC, pMessage);
    printf("ismc_sendX rc(%d)\n", rc);

    /* Commit session */
    rc = ismc_commitSession(pSess);
    printf("ismc_commitSession rc(%d)\n", rc);

    rc = ismc_closeSession(pSess);
    printf("ismc_closeSession rc(%d)\n", rc);

MOD_EXIT:

    if (connected)
    {
        rc = ismc_closeConnection(pConn);
        printf("ismc_closeConnection rc(%d)\n", rc);
    }

    return rc;
}

