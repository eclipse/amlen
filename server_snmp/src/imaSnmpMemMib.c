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
 * @brief Memory MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of memory MIB initialization and registration for MessageSight.
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
#include "imaSnmpMemMib.h"
#include "imaSnmpMemStat.h"

int ima_snmp_handler_getMemTotalBytes(netsnmp_mib_handler *handler,
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
        //snprintf(buf,100,"%s","1024M");
        rc = ima_snmp_get_mem_stat(buf,100, imaSnmpMem_TOTAL_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get totalMemBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemTotalBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_mem_totalbytes_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  mem_totalbytes_oid[] = { IMA_SNMP_MEM_TOTALBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("MemoryTotalBytes",
        ima_snmp_handler_getMemTotalBytes, mem_totalbytes_oid, 
        OID_LENGTH(mem_totalbytes_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_getMemFreeBytes(netsnmp_mib_handler *handler,
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
        //snprintf(buf,100,"%s","1024M");
        rc = ima_snmp_get_mem_stat(buf,100, imaSnmpMem_FREE_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get freeMemBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemFreeBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_mem_freebytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  mem_freebytes_oid[] = { IMA_SNMP_MEM_FREEBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("MemoryFreeBytes",
           ima_snmp_handler_getMemFreeBytes, mem_freebytes_oid,
        OID_LENGTH(mem_freebytes_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_getMemFreePercent(netsnmp_mib_handler *handler,
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
        //snprintf(buf,100,"%s","1024M");
        rc = ima_snmp_get_mem_stat(buf,100, imaSnmpMem_FREE_PERCENT);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get freeMemPercent stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemFreePercent\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
static int ima_snmp_init_mem_freepercent_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  mem_freepercent_oid[] = { IMA_SNMP_MEM_FREEPERCENT_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("MemoryFreePercent",
           ima_snmp_handler_getMemFreePercent, mem_freepercent_oid,
        OID_LENGTH(mem_freepercent_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}
//ServerVirtualMemoryBytes
int ima_snmp_handler_getMemServerVirtualBytes(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_mem_stat(buf,100, imaSnmpMem_SVR_VRTL_MEM_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get serverVirtualMemoryBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemServerVirtualBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
static int ima_snmp_init_mem_servervirtualbytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  mem_servervirtualbytes_oid[] = { IMA_SNMP_SVR_VRTL_MEM_BYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("ServerVirtualMemoryBytes",
           ima_snmp_handler_getMemServerVirtualBytes, mem_servervirtualbytes_oid,
        OID_LENGTH(mem_servervirtualbytes_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//ServerResidentSetBytes
int ima_snmp_handler_getMemServerResidentSetBytes(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_mem_stat(buf,100, imaSnmpMem_SVR_RSDT_SET_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get serverResidentSetBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemServerResidentSetBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
static int ima_snmp_init_mem_serverresidentsetbytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  mem_serverresidentsetbytes_oid[] = { IMA_SNMP_SVR_RSDT_SET_BYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("ServerResidentSetBytes",
           ima_snmp_handler_getMemServerResidentSetBytes, mem_serverresidentsetbytes_oid,
        OID_LENGTH(mem_serverresidentsetbytes_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//MessagePayloads
int ima_snmp_handler_getMemMessagePayloads(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_mem_stat(buf,256, imaSnmpMem_GRP_MSG_PAYLOADS);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get MessagePayloads stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemMessagePayloads\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
static int ima_snmp_init_mem_messagepayloads_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  mem_messagepayloads_oid[] = { IMA_SNMP_GRP_MSG_PAYLOADS_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("MessagePayloads",
           ima_snmp_handler_getMemMessagePayloads, mem_messagepayloads_oid,
        OID_LENGTH(mem_messagepayloads_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//PublishSubscribe
int ima_snmp_handler_getMemPublishSubscribe(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_mem_stat(buf,256, imaSnmpMem_GRP_PUB_SUB);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get PublishSubscribe stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemPublishSubscribe\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
static int ima_snmp_init_mem_publishsubscribe_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  mem_publishsubscribe_oid[] = { IMA_SNMP_GRP_PUB_SUB_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("PublishSubscribe",
           ima_snmp_handler_getMemPublishSubscribe, mem_publishsubscribe_oid,
        OID_LENGTH(mem_publishsubscribe_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


//Destinations
int ima_snmp_handler_getMemDestinations(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_mem_stat(buf,256, imaSnmpMem_GRP_DESTS);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get Destinations stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemDestinations\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
static int ima_snmp_init_mem_destinations_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  mem_destinations_oid[] = { IMA_SNMP_GRP_DESTS_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("Destinations",
           ima_snmp_handler_getMemDestinations, mem_destinations_oid,
        OID_LENGTH(mem_destinations_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//CurrentActivity
int ima_snmp_handler_getMemCurrentActivity(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_mem_stat(buf,100, imaSnmpMem_GRP_CUR_ACTIVITIES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get CurrentActivity stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemCurrentActivity\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
static int ima_snmp_init_mem_currentactivity_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  mem_currentactivity_oid[] = { IMA_SNMP_GRP_CUR_ACTIVITIES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("CurrentActivity",
           ima_snmp_handler_getMemCurrentActivity, mem_currentactivity_oid,
        OID_LENGTH(mem_currentactivity_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//ClientStates
int ima_snmp_handler_getMemClientStates(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_mem_stat(buf,100, imaSnmpMem_GRP_CLIENT_STATES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get ClientStates stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMemClientStates\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
static int ima_snmp_init_mem_clientstates_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  mem_clientstates_oid[] = { IMA_SNMP_GRP_CLIENT_STATES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("ClientStates",
           ima_snmp_handler_getMemClientStates, mem_clientstates_oid,
        OID_LENGTH(mem_clientstates_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_init_mem_mibs()
{
    int rc = 0;

    rc = ima_snmp_init_mem_totalbytes_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init memoryTotalBytes MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_freebytes_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init memoryFreeBytes MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_freepercent_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init memoryFreePercent MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_servervirtualbytes_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init serverVirtualMemoryBytes MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_serverresidentsetbytes_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init serverResidentSetBytes MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_messagepayloads_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init messagepayloads MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_publishsubscribe_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init publishsubscribe MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_destinations_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init destinations MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_currentactivity_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init currentactivity MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_clientstates_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init clientstates MIB for MessageSight SNMP service\n");
       return rc;
    }

    return rc;
}

