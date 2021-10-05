/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

/**
 * @brief Server MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of Server MIB initialization and registration for MessageSight.
 *
 */
#include <stdio.h>
#include <string.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <ismutil.h>
#include <ismrc.h>

#include "imaSnmpMib.h"
#include "imaSnmpServerMib.h"
#include "imaSnmpServerStat.h"

/* saved handlers to register and unregister mibs : Unused for now */
static netsnmp_handler_registration	*serverState_handler = NULL;
static netsnmp_handler_registration	*serverStateStr_handler = NULL;
static netsnmp_handler_registration	*serverAdminStateRC_handler = NULL;
static netsnmp_handler_registration	*serverUpTime_handler = NULL;
static netsnmp_handler_registration	*serverUpTimeStr_handler = NULL;
static netsnmp_handler_registration	*serverIsHAEnabled_handler = NULL;
static netsnmp_handler_registration	*serverNewRole_handler = NULL;
static netsnmp_handler_registration	*serverOldRole_handler = NULL;
static netsnmp_handler_registration	*serverActiveNodes_handler = NULL;
static netsnmp_handler_registration	*serverSyncNodes_handler = NULL;
static netsnmp_handler_registration	*serverPrimaryLastTime_handler = NULL;
static netsnmp_handler_registration	*serverPctSyncCompletion_handler = NULL;
static netsnmp_handler_registration	*serverReasonCode_handler = NULL;


//ServerState
int ima_snmp_handler_getServerState(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_SERVERSTATE);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get ServerState status from MessageSight. rc = %d\n", rc);//1
            rc = ima_snmp_get_server_state_from_shell();
            if (rc < 0)  // failed to get server state from script
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            sprintf(buf,"%d",rc); 
            TRACE(7, "Get server state from script, buf = %s \n",buf);
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getServerState\n",reqinfo->mode);//1
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_serverstate_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_SERVERSTATE_MIB };//1

   rc = netsnmp_register_scalar(serverState_handler =
		   netsnmp_create_handler_registration("ServerState", //1
           ima_snmp_handler_getServerState, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_serverstate_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_SERVERSTATE_MIB };//1

	   rc = netsnmp_unregister_handler(serverState_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//ServerStateStr
int ima_snmp_handler_getServerStateStr(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_SERVERSTATESTR);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get ServerStateStr status from MessageSight. rc = %d\n", rc);//1
            rc = ima_snmp_get_server_state_from_shell();
            switch (rc)  {
            case IMA_SERVER_STATE_UNKNOWN:
                strncpy(buf, IMA_SERVER_UNKNOWN_STR, strlen(IMA_SERVER_UNKNOWN_STR)); 
                break;

            case IMA_SERVER_STATE_MAINTENANCE:
                strncpy(buf, IMA_SERVER_MAINTENANCE_STR, strlen(IMA_SERVER_MAINTENANCE_STR)); 
                break;

            case IMA_SERVER_STATUS_STOPPED:
                strncpy(buf, IMA_SERVER_STOP_STR, strlen(IMA_SERVER_STOP_STR)); 
                break;

            case IMA_SERVER_STATE_STARTING_STORE:
                strncpy(buf, IMA_SERVER_START_STORE_STR, strlen(IMA_SERVER_START_STORE_STR)); 
                break;

            default:
                TRACE(2, "invalid rc value from cleanstore script : %d \n", rc);
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            }
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getServerStateStr\n",reqinfo->mode);//1
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_serverstatestr_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_SERVERSTATESTR_MIB };//1

   rc = netsnmp_register_scalar(serverStateStr_handler = netsnmp_create_handler_registration("ServerStateStr", //1
           ima_snmp_handler_getServerStateStr, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_serverstatestr_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_SERVERSTATESTR_MIB };//1

	   rc = netsnmp_unregister_handler(serverStateStr_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//AdminStateRC
int ima_snmp_handler_getAdminStateRC(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_ADMINSTATERC);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_adminstaterc_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_ADMINSTATERC_MIB };//1

   rc = netsnmp_register_scalar(serverAdminStateRC_handler = netsnmp_create_handler_registration("AdminStateRC", //1
           ima_snmp_handler_getAdminStateRC, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_adminstaterc_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_ADMINSTATERC_MIB };//1

	   rc = netsnmp_unregister_handler(serverAdminStateRC_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//ServerUPTime
int ima_snmp_handler_getServerUPTime(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_SERVERUPTIME);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_serveruptime_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_SERVERUPTIME_MIB };//1

   rc = netsnmp_register_scalar(serverUpTime_handler = netsnmp_create_handler_registration("ServerUPTime", //1
           ima_snmp_handler_getServerUPTime, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_serveruptime_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_SERVERUPTIME_MIB };//1

	   rc = netsnmp_unregister_handler(serverUpTime_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//ServerUPTimeStr
int ima_snmp_handler_getServerUPTimeStr(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[256];
        bzero(buf,256);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,256, imaSnmpServer_SERVERUPTIMESTR);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_serveruptimestr_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_SERVERUPTIMESTR_MIB };//1

   rc = netsnmp_register_scalar(serverUpTimeStr_handler = netsnmp_create_handler_registration("ServerUPTimeStr", //1
           ima_snmp_handler_getServerUPTimeStr, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_serveruptimestr_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_SERVERUPTIMESTR_MIB };//1

	   rc = netsnmp_unregister_handler(serverUpTimeStr_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//IsHAEnabled
int ima_snmp_handler_getIsHAEnabled(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_ISHAENABLED);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_ishaenabled_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_ISHAENABLED_MIB };//1

   rc = netsnmp_register_scalar(serverIsHAEnabled_handler = netsnmp_create_handler_registration("IsHAEnabled", //1
           ima_snmp_handler_getIsHAEnabled, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_ishaenabled_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_ISHAENABLED_MIB };//1

	   rc = netsnmp_unregister_handler(serverIsHAEnabled_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}
//NewRole
int ima_snmp_handler_getNewRole(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_NEWROLE);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_newrole_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_NEWROLE_MIB };//1

   rc = netsnmp_register_scalar(serverNewRole_handler = netsnmp_create_handler_registration("NewRole", //1
           ima_snmp_handler_getNewRole, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_newrole_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_NEWROLE_MIB };//1

	   rc = netsnmp_unregister_handler(serverNewRole_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//OldRole
int ima_snmp_handler_getOldRole(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_OLDROLE);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_oldrole_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_OLDROLE_MIB };//1

   rc = netsnmp_register_scalar(serverOldRole_handler = netsnmp_create_handler_registration("OldRole", //1
           ima_snmp_handler_getOldRole, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_oldrole_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_OLDROLE_MIB };//1

	   rc = netsnmp_unregister_handler(serverOldRole_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//ActiveNodes
int ima_snmp_handler_getActiveNodes(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_ACTIVENODES);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_activenodes_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_ACTIVENODES_MIB };//1

   rc = netsnmp_register_scalar(serverActiveNodes_handler = netsnmp_create_handler_registration("ActiveNodes", //1
           ima_snmp_handler_getActiveNodes, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_activenodes_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_ACTIVENODES_MIB };//1

	   rc = netsnmp_unregister_handler(serverActiveNodes_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//SyncNodes
int ima_snmp_handler_getSyncNodes(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_SYNCNODES);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_syncnodes_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_SYNCNODES_MIB };//1

   rc = netsnmp_register_scalar(serverSyncNodes_handler = netsnmp_create_handler_registration("SyncNodes", //1
           ima_snmp_handler_getSyncNodes, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_syncnodes_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_SYNCNODES_MIB };//1

	   rc = netsnmp_unregister_handler(serverSyncNodes_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//PrimaryLastTime
int ima_snmp_handler_getPrimaryLastTime(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[256];
        bzero(buf,256);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,256, imaSnmpServer_PRIMARYLASTTIME);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_primarylasttime_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_PRIMARYLASTTIME_MIB };//1

   rc = netsnmp_register_scalar(serverPrimaryLastTime_handler = netsnmp_create_handler_registration("PrimaryLastTime", //1
           ima_snmp_handler_getPrimaryLastTime, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_primarylasttime_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_PRIMARYLASTTIME_MIB };//1

	   rc = netsnmp_unregister_handler(serverPrimaryLastTime_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//PctSyncCompletion
int ima_snmp_handler_getPctSyncCompletion(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_PCTSYNCCOMPLETION);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_pctsynccompletion_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_PCTSYNCCOMPLETION_MIB };//1

   rc = netsnmp_register_scalar(serverPctSyncCompletion_handler = netsnmp_create_handler_registration("PctSyncCompletion", //1
           ima_snmp_handler_getPctSyncCompletion, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_pctsynccompletion_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_PCTSYNCCOMPLETION_MIB };//1

	   rc = netsnmp_unregister_handler(serverPctSyncCompletion_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

//ReasonCode
int ima_snmp_handler_getReasonCode(netsnmp_mib_handler *handler,//1
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[100];
        bzero(buf,100);
        int rc = 0;
        rc = ima_snmp_get_server_stat(buf,100, imaSnmpServer_REASONCODE);//2
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to ima_snmp_get_server_stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getxxx function\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_server_reasoncode_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  oid_info[] = { IMA_SNMP_SERVER_REASONCODE_MIB };//1

   rc = netsnmp_register_scalar(serverReasonCode_handler = netsnmp_create_handler_registration("ReasonCode", //1
           ima_snmp_handler_getReasonCode, oid_info, //1
        OID_LENGTH(oid_info), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

static int ima_snmp_uninit_server_reasoncode_mib()
{
	   int rc = MIB_UNREGISTERED_OK;
	   // const oid  oid_info[] = { IMA_SNMP_SERVER_REASONCODE_MIB };//1

	   rc = netsnmp_unregister_handler(serverReasonCode_handler);

	   if (rc != MIB_UNREGISTERED_OK) return rc;
	   return 0;
}

int ima_snmp_init_server_mibs()
{
    int rc = 0;

    rc = ima_snmp_init_server_serverstate_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_init_server_serverstate_mib for MessageSight SNMP service\n");
       return rc;
    }

    rc = ima_snmp_init_server_serverstatestr_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_serverstatestr_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_init_server_adminstaterc_mib();
    if (rc != 0)
    {
        TRACE(2, "failed to ima_snmp_init_server_adminstaterc_mib for MessageSight SNMP service\n");
        return rc;
    }
    rc = ima_snmp_init_server_serveruptime_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_serveruptime_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_init_server_serveruptimestr_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_serveruptimestr_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_init_server_ishaenabled_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_ishaenabled_mib for MessageSight SNMP service\n");
      return rc;
    }
    rc = ima_snmp_init_server_newrole_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_newrole_mib for MessageSight SNMP service\n");
      return rc;
    }
    rc = ima_snmp_init_server_oldrole_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_oldrole_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_init_server_activenodes_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_activenodes_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_init_server_syncnodes_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_syncnodes_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_init_server_primarylasttime_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_primarylasttime_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_init_server_pctsynccompletion_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_pctsynccompletion_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_init_server_reasoncode_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_init_server_reasoncode_mib for MessageSight SNMP service\n");
      return rc;
    }

    return rc;
}

/* Unused for now */
int ima_snmp_uninit_server_mibs()
{
    int rc = 0;

    rc = ima_snmp_uninit_server_serverstate_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_uninit_server_serverstate_mib for MessageSight SNMP service\n");
       return rc;
    }

    rc = ima_snmp_uninit_server_serverstatestr_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_serverstatestr_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_uninit_server_adminstaterc_mib();
    if (rc != 0)
    {
        TRACE(2, "failed to ima_snmp_uninit_server_adminstaterc_mib for MessageSight SNMP service\n");
        return rc;
    }
    rc = ima_snmp_uninit_server_serveruptime_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_serveruptime_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_uninit_server_serveruptimestr_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_serveruptimestr_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_uninit_server_ishaenabled_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_ishaenabled_mib for MessageSight SNMP service\n");
      return rc;
    }
    rc = ima_snmp_uninit_server_newrole_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_newrole_mib for MessageSight SNMP service\n");
      return rc;
    }
    rc = ima_snmp_uninit_server_oldrole_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_oldrole_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_uninit_server_activenodes_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_activenodes_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_uninit_server_syncnodes_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_syncnodes_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_uninit_server_primarylasttime_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_primarylasttime_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_uninit_server_pctsynccompletion_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_pctsynccompletion_mib for MessageSight SNMP service\n");
      return rc;
    }

    rc = ima_snmp_uninit_server_reasoncode_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to ima_snmp_uninit_server_reasoncode_mib for MessageSight SNMP service\n");
      return rc;
    }

    return rc;
}
