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

/* A stand alone ismc client test program to exercise the xid store outside of the bridge */

/* gcc -g -o xidStore xidStore.c -lismc -lssl -I $BROOT/server_ship/include -I $SROOT/server_ismc/src -I $SROOT/server_protocol/src -L $BROOT/server_ship/lib  -Wl,-rpath $BROOT/server_ship/lib */

#include <ismc.h>
#include <ismc_p.h>
#include <stdbool.h>

#define QM_COUNT 4
#define SIZE_OF_ERRORTEXTBUFFER 1024

int printKnownManagerRecords(ismc_session_t *pSess)
{
    int rc = 0;
    int i = 0;
    ismc_manrec_list_t *pManagerRecords = NULL;
    int managerRecordCount = 0;

    rc = ismc_getManagerRecords(pSess, &pManagerRecords, &managerRecordCount);
    printf("ismc_getManagerRecords rc = %d\n", rc);
    printf("\tmanagerRecordCount = %d\n", managerRecordCount);

    for (i = 0; i < managerRecordCount; i++)
    {
        printf("manager record %d\n", i);
        printf("\tpManagerRecords[i].handle = %p\n",
               pManagerRecords[i].handle);
        printf("pManagerRecords[i].data = %.*s\n",
               pManagerRecords[i].len,
               (char*) pManagerRecords[i].data);
        printf("pManagerRecords[i].len = %d\n\n", pManagerRecords[i].len);
    }

    ismc_freeManagerRecords(pManagerRecords);

    return rc;

}

int main(int argc, char **argv)
{
    int rc = 0;
    ismc_connection_t * pConn = NULL;
    ismc_session_t * pSess = NULL;
    bool connected = false;
    int transacted, ackmode;

    char errorTextBuffer[SIZE_OF_ERRORTEXTBUFFER] = "Poison Pattern";

    ismc_manrec_t inQMRecord[QM_COUNT];

    ismc_xarec_t pXidRecord;
    ismc_xarec_t pXidRecord2;
    
    ismc_xarec_list_t *pXidRecords = NULL;
    int xidRecordCount = 0;

    char* QMData[QM_COUNT] = { "Data for Queue Manager One",
                               "Data for Queue Manager Two",
                               "Data for Queue Manager Three",
                               "Data for Queue Manager Four" };
    char* actualXid[] = { "This is a sample XID - really!",
			  "Second sample XID" };

    int i = 0;

    /* Create connection */
    pConn = ismc_createConnection();
    printf("ismc_createConnection pConn = %p\n", pConn);
    if (pConn == NULL)
    {
        goto MOD_EXIT;
    }

    ismc_setStringProperty(pConn, "ClientID", "xidStoreTest");
    ismc_setStringProperty(pConn, "Protocol", "tcp");
    ismc_setStringProperty(pConn, "Server", "127.0.0.1");
    ismc_setIntProperty(pConn, "Port", 16102, VT_Integer);

    rc = ismc_connect(pConn);
    printf("ismc_connect rc = %d\n", rc);
    if (rc != 0)
    {
        goto MOD_EXIT;
    }

    connected = true;

    rc = ismc_startConnection(pConn);
    printf("ismc_startConnection rc = %d\n", rc);
    if (rc != 0)
    {
        goto MOD_EXIT;
    }

    /* Create session */
    transacted = 1;
    ackmode = SESSION_CLIENT_ACKNOWLEDGE;

    pSess = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSess = %p\n", pSess);
    if (pSess == NULL)
    {
        goto MOD_EXIT;
    }

    /* Create some queue manager records. */
    for (i = 0; i < QM_COUNT - 1; i++)
    {
        printf("%d: Call ismc_createManagerRecord.\n", i);
        inQMRecord[i] =
            ismc_createManagerRecord(pSess,
                                     (void*) QMData[i],
                                     strlen(QMData[i]));
    }

    printf("Call ismc_deleteManagerRecord[1].\n");
    rc = ismc_deleteManagerRecord(pSess, inQMRecord[1]);
    printf("ismc_deleteManagerRecord rc = %d\n", rc);
    if (rc != 0)
    {
        goto MOD_EXIT;
    }

    printf("\n *** Expect two manager record blocks ***\n");
    rc = printKnownManagerRecords(pSess);
    if (rc != 0)
    {
        goto MOD_EXIT;
    }

    /* The functions that manipulate queue manager records should not
     * be aware of transactions */

    rc = ismc_rollbackSession(pSess);
    printf("ismc_rollbackSession rc = %d\n", rc);

    if (rc != 0)
    {
        printf("\t%s\n", errorTextBuffer);
        ismc_getLastError(errorTextBuffer, SIZE_OF_ERRORTEXTBUFFER);
        printf("\t%s\n", errorTextBuffer);
    }

    printf("\n *** Expect two manager record blocks ***\n");
    rc = printKnownManagerRecords(pSess);
    if (rc != 0)
    {
        goto MOD_EXIT;
    }

    printf("%d: Call ismc_createManagerRecord.\n", QM_COUNT - 1);
    inQMRecord[(QM_COUNT - 1)] =
        ismc_createManagerRecord(pSess,
                                 (void*) QMData[(QM_COUNT - 1)],
                                 strlen(QMData[QM_COUNT - 1]));

    printf("\n *** Expect three manager record blocks ***\n");
    rc = printKnownManagerRecords(pSess);
    if (rc != 0)
    {
        goto MOD_EXIT;
    }

    /* Now use the xid functions. We will need a live session for
     * these to operate within a transaction. */


    pSess = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSess = %p\n", pSess);
    if (pSess == NULL)
    {
        printf("Re-create session failed.\n");
        goto MOD_EXIT;
    }

    printf("Check for any existing XA records.\n");

    rc = ismc_getXARecords(pSess, inQMRecord[0], &pXidRecords, &xidRecordCount);
    printf("ismc_getXARecords rc = %d, xidRecordCount = %d\n", rc,
            xidRecordCount);

    for (i = 0; i < xidRecordCount; i++)
    {
        printf("xid record %d\n", i);

        printf("pXidRecords[i].data = %.*s\n", pXidRecords[i].len,
                (char *) pXidRecords[i].data);
        printf("pXidRecords[i].len = %d\n\n", pXidRecords[i].len);
    }

    pXidRecord = ismc_createXARecord(pSess,
				     inQMRecord[0],
				     (const void *) actualXid[0],
				     strlen(actualXid[0]));
    if (pXidRecord != NULL)
    {
	printf("struct ismc_xarec_t contains, xa_record_id = %lu\n", pXidRecord->xa_record_id);
    }
    else
    {
	printf("ismc_createXARecord failed.\n");
	goto MOD_EXIT;
    }

    /* Commit the create of the xid record. */

    rc = ismc_commitSession(pSess);
    printf("ismc_commitSession rc = %d\n", rc);

    pSess = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSess = %p\n", pSess);

    rc = ismc_getXARecords(pSess,
                           inQMRecord[0],
                           &pXidRecords,
                           &xidRecordCount);
    printf("ismc_getXARecords rc = %d, xidRecordCount = %d\n", rc, xidRecordCount);
    
    for (i = 0; i < xidRecordCount; i++)
    {
        printf("xid record %d\n", i);

	printf("pXidRecords[i].data = %.*s\n",
	       pXidRecords[i].len,
	       (char *) pXidRecords[i].data);
        printf("pXidRecords[i].len = %d\n\n", pXidRecords[i].len);
    }

    /* Write a new XA record and delete the previous one as part of */
    /* a single transaction (session). */

    printf("\n--> Delete an existing xid record.\n");

    rc = ismc_deleteXARecord(pSess, pXidRecord);
    if (rc != 0)
    {
	printf("ismc_deleteXARecord failed. rc = %d\n", rc);
        goto MOD_EXIT;
    }

    printf("\n--> Create a new xid record.\n");

    pXidRecord2 = ismc_createXARecord(pSess,
				     inQMRecord[0],
				     (const void *) actualXid[1],
				     strlen(actualXid[1]));
    if (pXidRecord2 != NULL)
    {
	printf("struct ismc_xarec_t contains, xa_record_id = %lu\n",
	       pXidRecord2->xa_record_id);
    }
    else
    {
	printf("ismc_createXARecord failed.\n");
	goto MOD_EXIT;
    }

    /* Commit the create and delete of the two xid records. */

    rc = ismc_commitSession(pSess);
    printf("ismc_commitSession rc = %d\n", rc);

    pSess = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSess = %p\n", pSess);

    rc = ismc_getXARecords(pSess,
                           inQMRecord[0],
                           &pXidRecords,
                           &xidRecordCount);
    printf("ismc_getXARecords rc = %d, xidRecordCount = %d\n", rc, xidRecordCount);
    
    for (i = 0; i < xidRecordCount; i++)
    {
        printf("xid record %d\n", i);

	printf("pXidRecords[i].data = %.*s\n",
	       pXidRecords[i].len,
	       (char *) pXidRecords[i].data);
        printf("pXidRecords[i].len = %d\n\n", pXidRecords[i].len);
    }

    /* Wait for input */
    while (1)
    {
        switch (getchar())
        {
        case 'q':
        case 'Q':
            if (pSess != NULL)
            {
                rc = ismc_closeSession(pSess);
            }
            goto MOD_EXIT;
            break;
        default:
            break;
        }
    }

MOD_EXIT:

    if (pConn)
    {
        if (pSess)
        {
            rc = ismc_free(pSess);
            printf("ismc_free(pSess) rc(%d)\n", rc);
        }

        if (connected)
        {
            rc = ismc_closeConnection(pConn);
            printf("ismc_closeConnection rc(%d)\n", rc);
        }

        rc = ismc_free(pConn);
        printf("ismc_free(pConn) rc(%d)\n", rc);
    }

    return rc;
}

