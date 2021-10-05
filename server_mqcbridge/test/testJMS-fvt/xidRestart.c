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

/* A stand alone ismc client test program to verify that xid's are
 * correctly recovered on re-start */

/* gcc -g -o xidRestart xidRestart.c -lismc -lssl -I $BROOT/server_ship/include -I $SROOT/server_ismc/src -I $SROOT/server_protocol/src -L $BROOT/server_ship/lib  -Wl,-rpath $BROOT/server_ship/lib */

#include <ismc.h>
#include <ismc_p.h>
#include <stdbool.h>

#define QM_COUNT 4
#define SIZE_OF_ERRORTEXTBUFFER 1024

static const int XAFormatID1 = 0x314D5849;
static const int XAFormatID2 = 0x324D5849;

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

int printKnownXARecords(ismc_session_t *pSess, ismc_manrec_t inQMRecord)
{
    int rc = 0;
    ismc_xarec_list_t *pXidRecords = NULL;
    int xidRecordCount = 0;
    int i = 0;

    rc = ismc_getXARecords(pSess, inQMRecord, &pXidRecords, &xidRecordCount);
    printf("ismc_getXARecords rc = %d, xidRecordCount = %d\n",
           rc,
           xidRecordCount);

    for (i = 0; i < xidRecordCount; i++)
    {
        printf("xid record %d\n", i);

        printf("pXidRecords[i].data = %.*s\n", pXidRecords[i].len,
               (char *) pXidRecords[i].data);
        printf("pXidRecords[i].len = %d\n\n", pXidRecords[i].len);
    }

    ismc_freeXARecords(pXidRecords);

    return rc;
}

int main(int argc, char **argv)
{
    int rc = 0;
    ismc_connection_t * pConn = NULL;
    ismc_session_t * pSess = NULL;
    bool connected = false;
    int transacted, ackmode;

    // char errorTextBuffer[SIZE_OF_ERRORTEXTBUFFER] = "Poison Pattern";

    ismc_manrec_list_t *pManagerRecords = NULL;
    int managerRecordCount = 0;

    ismc_manrec_t inQMRecord[QM_COUNT];

    ismc_xarec_t pXidRecord;
    
    // ismc_xarec_list_t *pXidRecords = NULL;
    // int xidRecordCount = 0;

    char* QMData[QM_COUNT] = { "Data for Queue Manager One",
                               "Data for Queue Manager Two",
                               "Data for Queue Manager Three",
                               "Data for Queue Manager Four" };
    char actualXid[64];

    time_t now;

    int i = 0, j = 0;

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
    printf("\nismc_createSession pSess = %p\n", pSess);
    if (pSess == NULL)
    {
        goto MOD_EXIT;
    }

    /* Check for any existing queue manager records. */

    printf("\n--> Check for any existing queue manager records ... \n");
    rc = ismc_getManagerRecords(pSess, &pManagerRecords, &managerRecordCount);

    /* Loop through inQMRecord. For each entry, copy the handle value */
    /* from pManagerRecord if there is one. If not, then create a     */
    /* manager record and assign the resulting handle.                */

    for (i = 0; i < QM_COUNT; i++)
    {
        bool foundManagerRecord = false;

        for (j = 0; j < managerRecordCount; j++)
        {
            if (pManagerRecords[j].len == strlen(QMData[i]))
            {
                if ((memcmp(pManagerRecords[j].data, QMData[i], pManagerRecords[j].len) == 0))
                {
                    printf("\t found record for %.*s\n",
                           pManagerRecords[j].len,
                           (char *)pManagerRecords[j].data);
                    inQMRecord[i] = pManagerRecords[j].handle;
                    foundManagerRecord = true;
                    break;
                }
            }
        }

        if (!foundManagerRecord)
        {
            printf("\t creating record for %s\n",
                   QMData[i]);
            inQMRecord[i] =
                ismc_createManagerRecord(pSess,
                                         (void*) QMData[i],
                                         strlen(QMData[i]));
        }
    }

    /* ismc_freeManagerRecords(pManagerRecords); */

    /* All of the queue managers should now have a manager record. */

    /* Show what we found or created. */
    rc = printKnownManagerRecords(pSess);
    if (rc != 0)
    {
        goto MOD_EXIT;
    }

    /* Check for any existing XA records in QM 0. */
    printf("\n--> Check for any existing XA records in QM %.*s.\n",
           pManagerRecords[0].len,
           (char *)pManagerRecords[0].data);

    rc = printKnownXARecords(pSess, inQMRecord[0]);
    if (rc != 0)
    {
        goto MOD_EXIT;
    }

    /* Store two XA records in QM 0. Commit one and leave the */
    /* other unresolved. */

    memset(actualXid, 0, sizeof(actualXid));
    memcpy(actualXid, &XAFormatID1, sizeof(int));

    now = time(NULL);

    sprintf(actualXid+sizeof(int), "%s", asctime(localtime(&now)));
    actualXid[strlen(actualXid) - 1] = 0;

    printf("Creating XA record. actual XID = <%s>\n", actualXid);
    pXidRecord = ismc_createXARecord(pSess,
				     inQMRecord[0],
				     actualXid,
				     strlen(actualXid));
    if (pXidRecord != NULL)
    {
	printf("struct ismc_xarec_t contains, xa_record_id = %lu\n", pXidRecord->xa_record_id);
    }
    else
    {

        int last_rc = 0;
        char errorText[1024];
        
	printf("ismc_createXARecord failed.\n");

        last_rc = ismc_getLastError(errorText, 1024);
	printf("Error text: %s. (%d)\n", errorText, last_rc);

	goto MOD_EXIT;
    }

    /* Commit the create of the xid record. */
    rc = ismc_commitSession(pSess);
    printf("ismc_commitSession rc = %d\n", rc);

    /* Create a new session for the second Xa record. */
    pSess = ismc_createSession(pConn, transacted, ackmode);
    printf("ismc_createSession pSess = %p\n", pSess);
    if (pSess == NULL)
    {
        printf("Re-create session failed.\n");
        goto MOD_EXIT;
    }

    memset(actualXid, 0, sizeof(actualXid));
    memcpy(actualXid, &XAFormatID2, sizeof(int));

    now = time(NULL);

    sprintf(actualXid+sizeof(int), "%s", asctime(localtime(&now)));
    actualXid[strlen(actualXid) - 1] = 0;

    printf("Creating XA record. actual XID = <%s>\n", actualXid);
    pXidRecord = ismc_createXARecord(pSess,
				     inQMRecord[0],
				     actualXid,
				     strlen(actualXid));
    if (pXidRecord != NULL)
    {
	printf("struct ismc_xarec_t contains, xa_record_id = %lu\n", pXidRecord->xa_record_id);
    }
    else
    {
	printf("ismc_createXARecord failed.\n");
	goto MOD_EXIT;
    }

    /* Wait for input */
    /* At this point the user is expected to Ctrl/C */
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

