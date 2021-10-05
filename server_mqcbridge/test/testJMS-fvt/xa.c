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
gcc -g -o xa xa.c -lismc -lssl -lismengine -lismmonitoring -I $BROOT/server_ship/include -I $SROOT/server_ismc/src -I $SROOT/server_protocol/src -L $BROOT/server_ship/lib  -Wl,-rpath $BROOT/server_ship/lib
*/

#include <ismc.h>
#include <ismc_p.h>
#include <stdbool.h>

#define XID_COUNT 2

int main(int argc, char **argv)
{
    int i, xidcount, flags, xanum = 0, rc = 0;
    ismc_connection_t * pConn = NULL;
    ismc_session_t * pSess = NULL;
    ismc_message_t * pMessage = NULL;
    bool connected = false;
    int transacted, ackmode;
    ism_xid_t xid;
    ism_xid_t xids[XID_COUNT];
    time_t now;
    char input[256];

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

    /* XARecover */
    flags = TMSTARTRSCAN;

    while (1)
    {
        xidcount = ismc_recoverGlobalTransactions(pSess, xids, XID_COUNT, flags);
        printf("ismc_recoverGlobalTransactions xidcount(%d)\n", xidcount);

        if (xidcount > 0)
        {
            if (xidcount > XID_COUNT)
            {
                printf("This shouldn't happen!!!\n");
                break;
            }

            for (i = 0; i < xidcount; i++)
            {
                printf("xid '%.*s'\n", xids[i].gtrid_length + xids[i].bqual_length, xids[i].data);

                printf("Press 'c' to commit, 'r' to rollback, or any other key to continue.\n");
                fgets(input, sizeof(input), stdin);
                switch (input[0])
                {
                case 'c':
                    /* XACommit */
                    rc = ismc_commitGlobalTransaction(pSess, &xids[i], 0);
                    printf("ismc_commitGlobalTransaction rc(%d)\n", rc);
                    break;

                case 'r':
                    /* XARollback */
                    rc = ismc_rollbackGlobalTransaction(pSess, &xids[i]);
                    printf("ismc_rollbackGlobalTransaction rc(%d)\n", rc);
                    break;

                default:
                    break;
                }
            }

            if (xidcount == XID_COUNT)
            {
                flags = 0;
                continue;
            }
        }

        break;
    }

    /* Create message */
    pMessage = ismc_createMessage(pSess, MTYPE_BytesMessage);
    printf("ismc_createMessage pMessage <%p>\n", pMessage);
    if (pMessage == NULL) goto MOD_EXIT;

    /* XAStart */
    time(&now);
    xid.formatID = 0;
    sprintf((char *)xid.data, "ISM.%d", (int)now);
    xid.gtrid_length = strlen((const char *)xid.data);

    sprintf((char *)(&xid.data[xid.gtrid_length]), "%d", xanum);
    xid.bqual_length = strlen((const char *)(&xid.data[xid.gtrid_length]));

    rc = ismc_startGlobalTransaction(pSess, &xid);
    printf("ismc_startGlobalTransaction rc(%d)\n", rc);

    /* ismc_sendX */
    rc = ismc_sendX(pSess, 2, "ISMTOPIC/Mark", pMessage);
    printf("ismc_sendX 'ISMTOPIC/Mark' rc(%d)\n", rc);

    rc = ismc_endGlobalTransaction(pSess);
    printf("ismc_endGlobalTransaction rc(%d)\n", rc);

    /* XAPrepare */
    rc = ismc_prepareGlobalTransaction(pSess, &xid);
    printf("ismc_prepareGlobalTransaction rc(%d)\n", rc);

    printf("Press 'c' to commit, 'r' to rollback, or any other key to continue.\n");
    fgets(input, sizeof(input), stdin);
    switch (input[0])
    {
    case 'c':
        /* XACommit */
        rc = ismc_commitGlobalTransaction(pSess, &xid, 0);
        printf("ismc_commitGlobalTransaction rc(%d)\n", rc);
        break;

    case 'r':
        /* XARollback */
        rc = ismc_rollbackGlobalTransaction(pSess, &xid);
        printf("ismc_rollbackGlobalTransaction rc(%d)\n", rc);
        break;

    default:
        break;
    }

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

