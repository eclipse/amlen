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
gcc -g -o qmxid qmxid.c -lismc -lssl -lismengine -lismmonitoring -I $BROOT/server_ship/include -I $SROOT/server_ismc/src -I $SROOT/server_protocol/src -L $BROOT/server_ship/lib -Wl,-rpath $BROOT/server_ship/lib
*/

#include <ismc.h>
#include <ismc_p.h>
#include <stdbool.h>

#define OBJECTIDENTIFIER "00164136CBBB"
#define QMNAME "XP1"

int main(int argc, char **argv)
{
    int index = 0, rc = 0;
    ismc_connection_t * pConn = NULL;
    ismc_session_t * pSess = NULL;
    ismc_message_t * pMessage = NULL;
    bool connected = false;
    int transacted, ackmode;
    ism_xid_t xid;

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

    rc = ismc_startConnection(pConn);
    printf("ismc_startConenction rc(%d)\n", rc);
    if (rc) goto MOD_EXIT;

    /* Create session */
    transacted = 1;
    ackmode    = SESSION_TRANSACTED_GLOBAL;

    pSess = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSess <%p>\n", pSess);
    if (pSess == NULL) goto MOD_EXIT;

    /* Create message */
    pMessage = ismc_createMessage(pSess, MTYPE_BytesMessage);
    printf("ismc_createMessage pMessage <%p>\n", pMessage);
    if (pMessage == NULL) goto MOD_EXIT;

    /* XAStart */
    xid.formatID = 0x494D514D;
    xid.gtrid_length = 36;
    xid.bqual_length = strlen(QMNAME);

    memcpy(xid.data, OBJECTIDENTIFIER, 12);
    memcpy(&xid.data[12], &index, 4);
    memcpy(&xid.data[16], &index, 4);
    memset(&xid.data[17], 0, 16);
    memcpy(&xid.data[36], QMNAME, strlen(QMNAME));

    rc = ismc_startGlobalTransaction(pSess, &xid);
    printf("ismc_startGlobalTransaction rc(%d)\n", rc);

    /* ismc_sendX */
    rc = ismc_sendX(pSess, 2, "IMATOPIC/Mark", pMessage);
    printf("ismc_sendX 'IMATOPIC/Mark' rc(%d)\n", rc);

    /* XAPrepare */
    rc = ismc_prepareGlobalTransaction(pSess, &xid);
    printf("ismc_prepareGlobalTransaction rc(%d)\n", rc);

    printf("Press ctrl-c to exit or any other key to continue\n");
    getchar();

    /* Close session */
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

