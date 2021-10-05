const static char sccsid[] = "%Z% %W% %I% %E% %U%";
/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
* Change Log:
* -------- ----------- ------------- ------------------------------------
* Reason:  Date:       Originator:   Comments:
* -------- ----------- ------------- ------------------------------------
*          10-04-07    Jerry Stevens File Creation
*          12-04-10    Rachel Norris Added support for sharecnv so that obeys server setting
*/

/* The functions in this module are used to parse and store connection parameters common to the publisher and
and subscriber modules
*/

#include <string.h>
#include "cphConnection.h"

extern int cphMQSplitterCheckMQLoaded(CPH_LOG *pLog, int isClient);

/*
** Method: cphConnectionIni
**
** This method creates and initialises a new connection control block
**
** Input Parameters: pConfig - pointer to the configuration control block
**
** Output parameters: ppConection - a double pointer to the newly created connection structure
**
**
*/
void cphConnectionIni(CPH_CONNECTION **ppConnection, CPH_CONFIG *pConfig) {

    CPH_CONNECTION *pConnection = NULL;


    CPH_TRACE *pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphConnectionIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    pConnection = malloc(sizeof(CPH_CONNECTION));
    if (NULL != pConnection) {
        pConnection->pConfig = pConfig;
    }

    *ppConnection = pConnection;

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

}

/*
** Method: cphConnectionFree
**
** Disposes of a connection control block
**
** Input Parameters: ppConnection - double pointer to the connection control block to be freed
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConnectionFree(CPH_CONNECTION **ppConnection) {
    CPH_TRACE *pTrc;
    int res = CPHFALSE;

    pTrc = (*ppConnection)->pConfig->pTrc;
#define CPH_ROUTINE "cphConnectionFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (NULL != *ppConnection) {
        free(*ppConnection);
        *ppConnection = NULL;
        res = CPHTRUE;
    }


    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
    return(res);
}

/*
** Method: cphConnectionParseArgs
**
** This method parses the following command line parameters and validates them. cphConnectionParseArgs is
** called by the initialising publisher or subscriber routine.
**
** jb - queue manager name
** jt - connection type (client or server)
** jc - channel name
** jh - host name
** jp - port number 
** tx - transactionality
** cc - commit count
**
** The destination options are also parsed via a call to cphDestinationFactoryGenerateDestination
**
** Input Parameters: pConnection - a pointer to the connection control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConnectionParseArgs(CPH_CONNECTION *pConnection, CPH_DESTINATIONFACTORY *pDestinationFactory) {
    CPH_CONFIG *pConfig;
    CPH_TRACE *pTrc;

    char connectType[80];
    int isFastPath;
    int status = CPHTRUE;

    pConfig = pConnection->pConfig;
    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphConnectionParseArgs"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (CPHTRUE != cphConfigGetString(pConfig, pConnection->QMName, "jb")) {
        cphConfigInvalidParameter(pConfig, "(jb) Cannot retrieve Queue Manager name");
        status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "QMName: %s.", pConnection->QMName);
    }

    if ( (CPHTRUE == status) && 
        (CPHFALSE == cphDestinationFactoryGenerateDestination(pDestinationFactory, pConnection->destination.topic))) {
            cphLogPrintLn(pConfig->pLog, LOGERROR, "Could not generate destination name" );
            status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Topic: %s.", pConnection->destination.topic);
    }


    if (CPHTRUE != cphConfigGetString(pConfig, connectType, "jt")) {
        cphConfigInvalidParameter(pConfig, "(jt) Connection type (client/bindings) cannot be retrieved");
        status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Connection type: %s.", connectType);
        pConnection->isClientConnection = CPHTRUE;
        if (0 == strcmp(connectType, "mqb")) pConnection->isClientConnection = CPHFALSE;
    }

    if (CPHTRUE != cphConfigGetBoolean(pConfig, &isFastPath, "jf")) {
        cphConfigInvalidParameter(pConfig, "(jf) Cannot retrive fastpath option");
        status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "isFastPath: %d.", isFastPath);
        pConnection->isFastPath = isFastPath;
    }

    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &(pConnection->isTransacted), "tx"))) {
        cphConfigInvalidParameter(pConfig, "(tx) Cannot retrieve Transactionality option");
        status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "isTransacted: %d.", pConnection->isTransacted);
    }

    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetInt(pConfig, &(pConnection->commitCount), "cc"))) {
        cphConfigInvalidParameter(pConfig, "(cc) Cannot retrieve commit count value");
        status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "commit count: %d.", pConnection->commitCount);
    }

    /* Load the server or client MQ library as appropriate */
    if (0 != cphMQSplitterCheckMQLoaded(pConfig->pLog, pConnection->isClientConnection)) {
        cphLogPrintLn(pConfig->pLog, LOGERROR,  "ERROR - loading MQ dll.");
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "ERROR - loading MQ dll.");
    }

    if (CPHTRUE != cphConfigGetString(pConfig, pConnection->channelName, "jc")) {
        cphConfigInvalidParameter(pConfig, "(jc) Default channel name cannot be retrieved");
        status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Default channel name: %s.", pConnection->channelName);
    }


    if (CPHTRUE != cphConfigGetString(pConfig, pConnection->hostName, "jh")) {
        cphConfigInvalidParameter(pConfig,  "(jh) Default host name cannot be retrieved");
        status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Default host name: %s.", pConnection->hostName);
    }


    if (CPHTRUE != cphConfigGetInt(pConfig, &(pConnection->portNum), "jp")) {
        cphConfigInvalidParameter(pConfig, "(jp) Default port number cannot be retrieved");
        status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Default port number: %d.", pConnection->portNum);
    }

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(status);
}

/*
** Method: cphConnectionSetConnectOpts
**
** For a client connection, this function prepares an MQ connect options structure for an MQCONNX
** call. This is with reference to an MQ channel definition structure, a pointer to which 
** is passed in as a parameter.
** 
** Input Parameters: pConnection  - pointer to the connection control block
**                   pClientConn  - pointer to MQ channel definition structure
**                   pConnectOpts - pointer to MQ connect options structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConnectionSetConnectOpts(CPH_CONNECTION *pConnection, MQCD  *pClientConn, MQCNO *pConnectOpts) {
    int rc=CPHTRUE;
    CPH_CONFIG *pConfig;
    CPH_TRACE *pTrc;
 
	MQLONG sharedConv = 99999999;

    char tempName[MQ_CONN_NAME_LENGTH];

    pConfig = pConnection->pConfig;
    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphConnectionSetConnectOpts"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (CPHTRUE == pConnection->isClientConnection) {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Setting up client connection.");
        sprintf(tempName, "%s(%d)", pConnection->hostName, pConnection->portNum);
        strncpy(pClientConn->ConnectionName, tempName, MQ_CONN_NAME_LENGTH);
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Connection name: %s.", pClientConn->ConnectionName);

        strncpy(pClientConn->ChannelName, pConnection->channelName, MQ_CHANNEL_NAME_LENGTH);
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Channel name: %s.", pClientConn->ChannelName);

        /* Setting a very large value means that the client will obey the setting on the server */
        pClientConn->SharingConversations = sharedConv;
		CPHTRACEMSG(pTrc, CPH_ROUTINE, "Setting SharingConversations to: %s.", pClientConn->SharingConversations);
        
        /* This needs to be set to at least version 9 for shared conv to work */ 
        pClientConn->Version = 9;
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Setting MQCD Version to: %s.", pClientConn->Version);

        /* Point the MQCNO to the client connection definition */
        pConnectOpts->ClientConnPtr = pClientConn;

        /* Client connection fields are in the version 2 part of the
        MQCNO so we must set the version number to 2 or they will
        be ignored */
        pConnectOpts->Version = MQCNO_VERSION_2;
    } else {
        /* Server bindings - set the fastpath option if this was requested */
        if (CPHTRUE == pConnection->isFastPath) {
#if 0
            int rc=0;
            struct passwd *mqmuser;
            mqmuser=getpwnam("mqm");
            rc=setuid(mqmuser->pw_uid);
            if (rc!=0){
              fprintf(stderr,"Call to setuid() for 'mqm' failed, make sure you have setuid permissions.\n");
              rc=CPHFALSE;
            } else {
#endif
              CPHTRACEMSG(pTrc, CPH_ROUTINE, "Setting fastpath option");
              pConnectOpts->Options |= MQCNO_FASTPATH_BINDING;
#if 0
            }
#endif
        }
    }

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(rc);
}
