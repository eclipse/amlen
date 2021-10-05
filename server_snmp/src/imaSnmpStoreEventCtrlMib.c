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
 * @brief SNMP MIB interface for MessageSight store event control
 * @date August
 *
 * This provides initialization and registration interface of store event control MIBs
 * for MessageSight.
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <ismrc.h>
#include <ismutil.h>
#include <ismjson.h>

#include "imaSnmpMib.h"
#include "imaSnmpStoreEventMib.h"
#include "imaSnmpStoreEventCtrlMib.h"
#include "imaSnmpMqttClient.h"

static int bStoreDiskUsageEvents;
static int th_storeDiskUsageWarn;
static int th_storeDiskUsageAlert;
static int durn_storeTraps;
static time_t time_storeDiskUsageWarn;
static time_t time_storeDiskUsageAlert;
static time_t time_storePool1MemLowAlert;
static int bStorePool1Events;
static unsigned int th_storePool1MemLowAlert;

int ima_snmp_handler_storeDiskUsageWarningEnable(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = (bStoreDiskUsageEvents&STORE_USAGE_WARNING_EVENT)? 
               STORE_USAGE_WARNING_ENABLE : STORE_USAGE_WARNING_DISABLE ;
        snmp_set_var_typed_integer(requests->requestvb, ASN_INTEGER, data);

        break;
    }
        /*
         * SET REQUEST
         *
         * handle multiple states in the transaction.
         */
    case MODE_SET_RESERVE1:
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
        if (ret != SNMP_ERR_NOERROR) {
            TRACE(2,"invalid parm type for storeDiskUsageWarnEnable set \n");
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

    case MODE_SET_RESERVE2:
    case MODE_SET_FREE:
    case MODE_SET_ACTION:
    case MODE_SET_UNDO:

        break;

    case MODE_SET_COMMIT:
        data = *(requests->requestvb->val.integer);
        switch (data)
        {
        case STORE_USAGE_WARNING_ENABLE:
            data = STORE_USAGE_WARNING_EVENT;
            if (!ima_snmp_store_events_enabled()) 
            {
#ifndef _SNMP_THREADED_
            	// Subscribe to $sys/store topic
                ret = imaSnmpMqtt_subscribe(imaSnmpTopic_STORE);
#else
                ret = imaSnmp_subscribe(imaSnmpTopic_STORE);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in subscribe to store topic \n");
                    
                    break;
                }
            } 
            time_storeDiskUsageWarn = 0;
            bStoreDiskUsageEvents |= data;
            break;

        case STORE_USAGE_WARNING_DISABLE:
            data = ~STORE_USAGE_WARNING_EVENT;
            if ((bStoreDiskUsageEvents == STORE_USAGE_WARNING_EVENT) && (!bStorePool1Events))
            {
#ifndef _SNMP_THREADED_
            	// unsubscribe to $sys/store topic
                ret = imaSnmpMqtt_unsubscribe(imaSnmpTopic_STORE);
#else
                ret = imaSnmp_unsubscribe(imaSnmpTopic_STORE);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in unsubscribe to store topic \n");

                    break;
                }
            }
            bStoreDiskUsageEvents &= data;
            break;

        default:
            TRACE(2,"invalid parms to set storeDiskUsageWarnEnable: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_storeDiskUsageWarningEnable\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_store_diskUsageWarnEn_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaStoreDiskUsageWarningEnable_oid[] = {IMA_SNMP_STORE_DISK_USAGE_WARN_EN_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("StoreDiskUsageWarningEnable",
           ima_snmp_handler_storeDiskUsageWarningEnable, ibmImaStoreDiskUsageWarningEnable_oid, 
           OID_LENGTH(ibmImaStoreDiskUsageWarningEnable_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


int ima_snmp_handler_storeDiskUsageWarningThreshold(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = th_storeDiskUsageWarn;
        snmp_set_var_typed_integer(requests->requestvb, ASN_INTEGER, data);

        break;
    }
        /*
         * SET REQUEST
         *
         * handle multiple states in the transaction.
         */
    case MODE_SET_RESERVE1:
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
        if (ret != SNMP_ERR_NOERROR) {
            TRACE(2,"invalid parm type for storeDiskUsageWarnThreshold set \n");
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

    case MODE_SET_RESERVE2:
    case MODE_SET_FREE:
    case MODE_SET_ACTION:
    case MODE_SET_UNDO:

        break;

    case MODE_SET_COMMIT:
        data = *(requests->requestvb->val.integer);
        if ((data < 0) || (data > 100))
        {
            TRACE(2,"invalid parms to set storeDiskUsageWarnThreshold: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        th_storeDiskUsageWarn = data;
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_storeDiskUsageWarningThreshold\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_store_diskUsageWarnTh_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaStoreDiskUsageWarningTh_oid[] = {IMA_SNMP_STORE_DISK_USAGE_WARN_TH_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("StoreDiskUsageWarningThreshold",
           ima_snmp_handler_storeDiskUsageWarningThreshold, ibmImaStoreDiskUsageWarningTh_oid, 
           OID_LENGTH(ibmImaStoreDiskUsageWarningTh_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_storeDiskUsageWarningDuration(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = durn_storeTraps;
        snmp_set_var_typed_integer(requests->requestvb, ASN_INTEGER, data);

        break;
    }
        /*
         * SET REQUEST
         *
         * handle multiple states in the transaction.
         */
    case MODE_SET_RESERVE1:
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
        if (ret != SNMP_ERR_NOERROR) {
            TRACE(2,"invalid parm type for storeDiskUsageWarnThreshold set \n");
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

    case MODE_SET_RESERVE2:
    case MODE_SET_FREE:
    case MODE_SET_ACTION:
    case MODE_SET_UNDO:

        break;

    case MODE_SET_COMMIT:
        data = *(requests->requestvb->val.integer);
        if (data < 0)
        {
            TRACE(2,"invalid parms to set storeDiskUsageWarnDuration: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        durn_storeTraps = data;
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_storeDiskUsageWarningDuration\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_store_diskUsageWarnDurn_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaStoreDiskUsageWarningDurn_oid[] = {IMA_SNMP_STORE_DISK_USAGE_WARN_DURN_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("StoreDiskUsageWarningDuration",
           ima_snmp_handler_storeDiskUsageWarningDuration, ibmImaStoreDiskUsageWarningDurn_oid, 
           OID_LENGTH(ibmImaStoreDiskUsageWarningDurn_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_storeDiskUsageAlertEnable(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = (bStoreDiskUsageEvents&STORE_USAGE_ALERT_EVENT)? 
               STORE_USAGE_ALERT_ENABLE : STORE_USAGE_ALERT_DISABLE ;
        snmp_set_var_typed_integer(requests->requestvb, ASN_INTEGER, data);

        break;
    }
        /*
         * SET REQUEST
         *
         * handle multiple states in the transaction.
         */
    case MODE_SET_RESERVE1:
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
        if (ret != SNMP_ERR_NOERROR) {
            TRACE(2,"invalid parm type for storeDiskUsageAlertEnable set \n");
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

    case MODE_SET_RESERVE2:
    case MODE_SET_FREE:
    case MODE_SET_ACTION:
    case MODE_SET_UNDO:

        break;

    case MODE_SET_COMMIT:
        data = *(requests->requestvb->val.integer);
        switch (data)
        {
        case STORE_USAGE_ALERT_ENABLE:
            data = STORE_USAGE_ALERT_EVENT;
            if (!ima_snmp_store_events_enabled()) 
            {
#ifndef _SNMP_THREADED_
            	// Subscribe to $sys/store topic
                ret = imaSnmpMqtt_subscribe(imaSnmpTopic_STORE);
#else
                ret = imaSnmp_subscribe(imaSnmpTopic_STORE);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in subscribe to store topic \n");
                    
                    break;
                }
            } 
            time_storeDiskUsageAlert = 0;
            bStoreDiskUsageEvents |= data;
            break;

        case STORE_USAGE_ALERT_DISABLE:
            data = ~STORE_USAGE_ALERT_EVENT;
            if ((bStoreDiskUsageEvents == STORE_USAGE_ALERT_EVENT) && (!bStorePool1Events))
            {
#ifndef _SNMP_THREADED_
            	// unsubscribe to $sys/store topic
                ret = imaSnmpMqtt_unsubscribe(imaSnmpTopic_STORE);
#else
                ret = imaSnmp_unsubscribe(imaSnmpTopic_STORE);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in unsubscribe to store topic \n");
                    
                    break;
                }
            } 
            bStoreDiskUsageEvents &= data;
            break;

        default:
            TRACE(2,"invalid parms to set storeDiskUsageAlertEnable: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        break;
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_storeDiskUsageAlertEnable\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_store_diskUsageAlertEn_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaStoreDiskUsageAlertEnable_oid[] = {IMA_SNMP_STORE_DISK_USAGE_ALERT_EN_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("StoreDiskUsageAlertEnable",
           ima_snmp_handler_storeDiskUsageAlertEnable, ibmImaStoreDiskUsageAlertEnable_oid, 
           OID_LENGTH(ibmImaStoreDiskUsageAlertEnable_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_storeDiskUsageAlertTh(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data =  th_storeDiskUsageAlert;
        snmp_set_var_typed_integer(requests->requestvb, ASN_INTEGER, data);

        break;
    }
        /*
         * SET REQUEST
         *
         * handle multiple states in the transaction.
         */
    case MODE_SET_RESERVE1:
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
        if (ret != SNMP_ERR_NOERROR) {
            TRACE(2,"invalid parm type for storeDiskUsageAlertTh set \n");
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

    case MODE_SET_RESERVE2:
    case MODE_SET_FREE:
    case MODE_SET_ACTION:
    case MODE_SET_UNDO:

        break;

    case MODE_SET_COMMIT:
        data = *(requests->requestvb->val.integer);
        if ((data < 0) || (data > 100))
        {
            TRACE(2,"invalid parms to set storeDiskUsageAlertTh: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        th_storeDiskUsageAlert = data;
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_storeDiskUsageAlertThreshold\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_store_diskUsageAlertTh_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaStoreDiskUsageAlertThreshold_oid[] = {IMA_SNMP_STORE_DISK_USAGE_ALERT_TH_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("StoreDiskUsageAlertThreshold",
           ima_snmp_handler_storeDiskUsageAlertTh, ibmImaStoreDiskUsageAlertThreshold_oid, 
           OID_LENGTH(ibmImaStoreDiskUsageAlertThreshold_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_storePool1MemLowAlertEnable(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = (bStorePool1Events& STORE_POOL1MEMLOW_ALERT_EVENT )? 
                STORE_POOL1MEMLOW_ALERT_ENABLE : STORE_POOL1MEMLOW_ALERT_DISABLE ;
        snmp_set_var_typed_integer(requests->requestvb, ASN_INTEGER, data);

        break;
    }
        /*
         * SET REQUEST
         *
         * handle multiple states in the transaction.
         */
    case MODE_SET_RESERVE1:
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
        if (ret != SNMP_ERR_NOERROR) {
            TRACE(2,"invalid parm type for storePool1MemoryLowAlertEnable set \n");
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

    case MODE_SET_RESERVE2:
    case MODE_SET_FREE:
    case MODE_SET_ACTION:
    case MODE_SET_UNDO:

        break;

    case MODE_SET_COMMIT:
        data = *(requests->requestvb->val.integer);
        switch (data)
        {
        case STORE_POOL1MEMLOW_ALERT_ENABLE:
            data = STORE_POOL1MEMLOW_ALERT_EVENT ;
            if (!ima_snmp_store_events_enabled()) 
            {
#ifndef _SNMP_THREADED_
            	// Subscribe to $sys/store topic
                ret = imaSnmpMqtt_subscribe(imaSnmpTopic_STORE);
#else
                ret = imaSnmp_subscribe(imaSnmpTopic_STORE);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in subscribe to store topic \n");
                    
                    break;
                }
            } 
            time_storePool1MemLowAlert = 0;
            bStorePool1Events |= data;
            break;

        case STORE_USAGE_WARNING_DISABLE:
            data = ~STORE_POOL1MEMLOW_ALERT_EVENT;
            if ((bStorePool1Events == STORE_POOL1MEMLOW_ALERT_EVENT) && (!bStoreDiskUsageEvents)) 
            {
#ifndef _SNMP_THREADED_
            	// unsubscribe to $sys/store topic
                ret = imaSnmpMqtt_unsubscribe(imaSnmpTopic_STORE);
#else
                ret = imaSnmp_subscribe(imaSnmpTopic_STORE);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in unsubscribe to store topic \n");

                    break;
                }
            }
            bStorePool1Events &= data;
            break;

        default:
            TRACE(2,"invalid parms to set storePool1MemoryLowAlertEnable: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_storePool1MemoryLowAlertEnable\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_store_pool1MemLowAlertEn_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaStorePool1MemoryLowAlertEnable_oid[] = {IMA_SNMP_STORE_POOL1_MEM_LOW_ALERT_EN_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("StorePool1MemoryLowAlertEnable",
           ima_snmp_handler_storePool1MemLowAlertEnable, ibmImaStorePool1MemoryLowAlertEnable_oid, 
           OID_LENGTH(ibmImaStorePool1MemoryLowAlertEnable_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


int ima_snmp_handler_storePool1MemLowAlertThreshold(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = th_storePool1MemLowAlert;
        snmp_set_var_typed_integer(requests->requestvb, ASN_INTEGER, data);

        break;
    }
        /*
         * SET REQUEST
         *
         * handle multiple states in the transaction.
         */
    case MODE_SET_RESERVE1:
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
        if (ret != SNMP_ERR_NOERROR) {
            TRACE(2,"invalid parm type for storePool1MemeoryLowAlertThreshold set \n");
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

    case MODE_SET_RESERVE2:
    case MODE_SET_FREE:
    case MODE_SET_ACTION:
    case MODE_SET_UNDO:

        break;

    case MODE_SET_COMMIT:
        data = *(requests->requestvb->val.integer);
        if (data < 0) 
        {
            TRACE(2,"invalid parms to set storePool1MemoryLowAlertThreshold: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        th_storePool1MemLowAlert = data;
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_storePool1MemoryLowAlertThreshold\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_store_pool1MemLowAlertTh_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaStorePool1MemLowAlertTh_oid[] = {IMA_SNMP_STORE_POOL1_MEM_LOW_ALERT_TH_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("StorePool1MemoryLowAlertThreshold",
           ima_snmp_handler_storePool1MemLowAlertThreshold, ibmImaStorePool1MemLowAlertTh_oid, 
           OID_LENGTH(ibmImaStorePool1MemLowAlertTh_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_init_store_ctrl_mibs()
{
    int rc = 0;

#ifdef  STORE_TRAP_INIT_DISABLE
    bStoreDiskUsageEvents = STORE_USAGE_EVENT_NONE;
    bStorePool1Events = STORE_USAGE_EVENT_NONE;
#else
    bStoreDiskUsageEvents = STORE_USAGE_WARNING_EVENT| STORE_USAGE_ALERT_EVENT;
    bStorePool1Events = STORE_POOL1MEMLOW_ALERT_EVENT ;
/*    rc = imaSnmpMqtt_subscribe(imaSnmpTopic_STORE);
    if (rc !=  ISMRC_OK)
    {
        TRACE(2,"error in subscribe to store topic at store ctrl init\n");
        return rc;
    }
*/
#endif
    th_storeDiskUsageWarn = STORE_USAGE_WARNING_TH;
    th_storeDiskUsageAlert = STORE_USAGE_ALERT_TH;
    durn_storeTraps = STORE_USAGE_WARNING_DURN;
    th_storePool1MemLowAlert = STORE_POOL1MEMLOW_ALERT_TH;

    rc = ima_snmp_init_store_diskUsageWarnEn_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storeDiskUsageWarningEnable MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_store_diskUsageWarnTh_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storeDiskUsageWarningThreshold MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_store_diskUsageWarnDurn_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storeDiskUsageWarningDuration MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_store_diskUsageAlertEn_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storeDiskUsageAlertEnable MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_store_diskUsageAlertTh_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storeDiskUsageAlertThreshold MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_store_pool1MemLowAlertEn_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storePool1MemoryLowAlertEnable MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_store_pool1MemLowAlertTh_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storePool1MemoryLowAlertThreshold MIB for MessageSight SNMP service\n");
       return rc;
    }

    return rc;
}

int ima_snmp_store_events_enabled()
{
    return ((bStoreDiskUsageEvents | bStorePool1Events)?1:0);
}

int ima_snmp_store_disk_events_check(int storeDiskUsagePercent)
{
    struct timeval event_time;

    if ((bStoreDiskUsageEvents & STORE_USAGE_ALERT_EVENT) && (storeDiskUsagePercent >= th_storeDiskUsageAlert))
    {
        if (durn_storeTraps > 0)
        {
            gettimeofday(&event_time,NULL);
            if ((event_time.tv_sec - time_storeDiskUsageAlert) < durn_storeTraps)
            {
                TRACE(9,"storeDiskUsageAlert trap happens on %ld, was suppressed due to under duration period of previous one \n",
                      event_time.tv_sec);
                return STORE_USAGE_EVENT_NONE;
            }
            time_storeDiskUsageAlert = event_time.tv_sec;
        }
        return STORE_USAGE_ALERT_EVENT;
    }
    if ((bStoreDiskUsageEvents & STORE_USAGE_WARNING_EVENT) &&
        (storeDiskUsagePercent >= th_storeDiskUsageWarn))
    {
        if (durn_storeTraps > 0)
        {
            gettimeofday(&event_time,NULL);
            if ((event_time.tv_sec - time_storeDiskUsageWarn) < durn_storeTraps)
            {
                TRACE(9,"storeDiskUsageWarn trap happens on %ld, was suppressed due to under duration period of previous one \n",
                      event_time.tv_sec);
                return STORE_USAGE_EVENT_NONE;
            }
            time_storeDiskUsageWarn = event_time.tv_sec;
        }
        return STORE_USAGE_WARNING_EVENT;
    }
    return STORE_USAGE_EVENT_NONE;
}

int ima_snmp_store_pool1_events_check(long storePool1UsedBytes, long storePool1LimitBytes)
{
    long pool1FreeBytes;
    struct timeval event_time;

    pool1FreeBytes = storePool1LimitBytes - storePool1UsedBytes;
    if (pool1FreeBytes <= (long)th_storePool1MemLowAlert) 
    {
        if (durn_storeTraps > 0)
        {
            gettimeofday(&event_time,NULL);
            if ((event_time.tv_sec - time_storePool1MemLowAlert) < durn_storeTraps)
            {
                TRACE(9,"storePool1MemLowAlert trap happens on %ld, was suppressed due to under duration period of previous one \n",
                      event_time.tv_sec);
                return STORE_USAGE_EVENT_NONE;
            }
            time_storePool1MemLowAlert = event_time.tv_sec;
        }
        TRACE(9,"storePool1MemoryLowAlert trap happens  \n");
        return STORE_POOL1MEMLOW_ALERT_EVENT;
    }
    return STORE_USAGE_EVENT_NONE;
}

void ima_snmp_reinit_store_ctrl_mibs()
{
    th_storeDiskUsageWarn = STORE_USAGE_WARNING_TH;
    th_storeDiskUsageAlert = STORE_USAGE_ALERT_TH;
    durn_storeTraps = STORE_USAGE_WARNING_DURN;
    th_storePool1MemLowAlert = STORE_POOL1MEMLOW_ALERT_TH;
}
