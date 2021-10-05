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
 * @brief SNMP MIB interface for MessageSight memory event control
 * @date July
 *
 * This provides initialization and registration interface of memory event control MIBs
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
#include "imaSnmpMemEventMib.h"
#include "imaSnmpMemEventCtrlMib.h"
#include "imaSnmpMqttClient.h"

static int bMemUsageEvents;
static int th_memUsageWarn;
static int th_memUsageAlert;
static int durn_memTraps;
static time_t time_memUsageWarn;
static time_t time_memUsageAlert;

int ima_snmp_handler_memoryUsageWarningEnable(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = (bMemUsageEvents&MEMORY_USAGE_WARNING_EVENT)? 
               MEM_USAGE_WARNING_ENABLE : MEM_USAGE_WARNING_DISABLE ;
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
            TRACE(2,"invalid parm type for memUsageWarnEnable set \n");
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
        case MEM_USAGE_WARNING_ENABLE:
            data = MEMORY_USAGE_WARNING_EVENT;
            if (!bMemUsageEvents) 
            {
#ifndef	_SNMP_THREADED_
            	// Subscribe to $sys/memory topic
                ret = imaSnmpMqtt_subscribe(imaSnmpTopic_MEMORY);
#else
                ret = imaSnmp_subscribe(imaSnmpTopic_MEMORY);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in subscribe to memory topic \n");
                    
                    break;
                }
            } 
            bMemUsageEvents |= data;
            time_memUsageWarn = 0;
            break;

        case MEM_USAGE_WARNING_DISABLE:
            data = ~MEMORY_USAGE_WARNING_EVENT;
            if (bMemUsageEvents == MEMORY_USAGE_WARNING_EVENT)
            {
#ifndef _SNMP_THREADED_
            	// unsubscribe to $sys/memory topic
                ret = imaSnmpMqtt_unsubscribe(imaSnmpTopic_MEMORY);
#else
                ret = imaSnmp_unsubscribe(imaSnmpTopic_MEMORY);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in unsubscribe to memory topic \n");

                    break;
                }
            }
            bMemUsageEvents &= data;
            break;

        default:
            TRACE(2,"invalid parms to set memUsageWarnEnable: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_memoryUsageWarningEnable\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_mem_usageWarnEn_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaMemoryUsageWarningEnable_oid[] = {IMA_SNMP_MEM_USAGE_WARN_EN_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("MemoryUsageWarningEnable",
           ima_snmp_handler_memoryUsageWarningEnable, ibmImaMemoryUsageWarningEnable_oid, 
           OID_LENGTH(ibmImaMemoryUsageWarningEnable_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}



int ima_snmp_handler_memoryUsageWarningThreshold(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = th_memUsageWarn;
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
            TRACE(2,"invalid parm type for memUsageWarnThreshold set \n");
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
            TRACE(2,"invalid parms to set memUsageWarnThreshold: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        th_memUsageWarn = data;
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_memoryUsageWarningThreshold\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_mem_usageWarnTh_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaMemoryUsageWarningTh_oid[] = {IMA_SNMP_MEM_USAGE_WARN_TH_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("MemoryUsageWarningThreshold",
           ima_snmp_handler_memoryUsageWarningThreshold, ibmImaMemoryUsageWarningTh_oid, 
           OID_LENGTH(ibmImaMemoryUsageWarningTh_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_memoryUsageWarningDuration(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = durn_memTraps;
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
            TRACE(2,"invalid parm type for memUsageWarnThreshold set \n");
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
            TRACE(2,"invalid parms to set memUsageWarnDuration: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        durn_memTraps = data;
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_memoryUsageWarningDuration\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_mem_usageWarnDurn_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaMemoryUsageWarningDurn_oid[] = {IMA_SNMP_MEM_USAGE_WARN_DURN_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("MemoryUsageWarningDuration",
           ima_snmp_handler_memoryUsageWarningDuration, ibmImaMemoryUsageWarningDurn_oid, 
           OID_LENGTH(ibmImaMemoryUsageWarningDurn_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_memoryUsageAlertEnable(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data = (bMemUsageEvents&MEMORY_USAGE_ALERT_EVENT)? 
               MEM_USAGE_ALERT_ENABLE : MEM_USAGE_ALERT_DISABLE ;
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
            TRACE(2,"invalid parm type for memUsageAlertEnable set \n");
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
        case MEM_USAGE_ALERT_ENABLE:
            data = MEMORY_USAGE_ALERT_EVENT;
            if (!bMemUsageEvents) 
            {
#ifndef _SNMP_THREADED_
            	// Subscribe to $sys/memory topic
                ret = imaSnmpMqtt_subscribe(imaSnmpTopic_MEMORY);
#else
                ret = imaSnmp_subscribe(imaSnmpTopic_MEMORY);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in subscribe to memory topic \n");
                    
                    break;
                }
            } 
            time_memUsageAlert = 0;
            bMemUsageEvents |= data;
            break;

        case MEM_USAGE_ALERT_DISABLE:
            data = ~MEMORY_USAGE_ALERT_EVENT;
            if (bMemUsageEvents == MEMORY_USAGE_ALERT_EVENT)
            {
#ifndef _SNMP_THREADED_
            	// unsubscribe to $sys/memory topic
                ret = imaSnmpMqtt_unsubscribe(imaSnmpTopic_MEMORY);
#else
                ret = imaSnmp_unsubscribe(imaSnmpTopic_MEMORY);
#endif
                if (ret !=  ISMRC_OK)
                {
                    TRACE(2,"error in unsubscribe to memory topic \n");
                    
                    break;
                }
            } 
            bMemUsageEvents &= data;
            break;

        default:
            TRACE(2,"invalid parms to set memUsageAlertEnable: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        break;
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_memoryUsageAlertEnable\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_mem_usageAlertEn_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaMemoryUsageAlertEnable_oid[] = {IMA_SNMP_MEM_USAGE_ALERT_EN_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("MemoryUsageAlertEnable",
           ima_snmp_handler_memoryUsageAlertEnable, ibmImaMemoryUsageAlertEnable_oid, 
           OID_LENGTH(ibmImaMemoryUsageAlertEnable_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_memoryUsageAlertTh(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    int ret = SNMP_ERR_NOERROR;
    int data;
    switch (reqinfo->mode) {
    case MODE_GET:
    { 
        data =  th_memUsageAlert;
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
            TRACE(2,"invalid parm type for memUsageAlertTh set \n");
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
            TRACE(2,"invalid parms to set memUsageAlertTh: %d \n",data);
            netsnmp_set_request_error(reqinfo, requests,0);
            return SNMP_ERR_GENERR;
        }
        th_memUsageAlert = data;
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_memoryUsageAlertThreshold\n",
               reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return ret;
}

int ima_snmp_init_mem_usageAlertTh_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  ibmImaMemoryUsageAlertThreshold_oid[] = {IMA_SNMP_MEM_USAGE_ALERT_TH_MIB};

   rc = netsnmp_register_scalar(
        netsnmp_create_handler_registration("MemoryUsageAlertThreshold",
           ima_snmp_handler_memoryUsageAlertTh, ibmImaMemoryUsageAlertThreshold_oid, 
           OID_LENGTH(ibmImaMemoryUsageAlertThreshold_oid), HANDLER_CAN_RWRITE));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_init_mem_ctrl_mibs()
{
    int rc = 0;

#ifdef  MEMORY_TRAP_INIT_DISABLE
    bMemUsageEvents = MEMORY_USAGE_EVENT_NONE;
#else
    bMemUsageEvents = MEMORY_USAGE_WARNING_EVENT| MEMORY_USAGE_ALERT_EVENT;
/*    rc = imaSnmpMqtt_subscribe(imaSnmpTopic_MEMORY);
    if (rc !=  ISMRC_OK)
    {
        TRACE(2,"error in subscribe to memory topic at mem ctrl init\n");
        return rc;
    }
*/
#endif
    th_memUsageWarn = MEM_USAGE_WARNING_TH;
    th_memUsageAlert = MEM_USAGE_ALERT_TH;
    durn_memTraps = MEM_USAGE_WARNING_DURN ;

    rc = ima_snmp_init_mem_usageWarnEn_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to init memoryUsageWarningEnable MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_usageWarnTh_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init memoryUsageWarningThreshold MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_usageAlertEn_mib();
    if (rc != 0)
    {
      TRACE(2, "failed to init memoryUsageAlertEnable MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_usageAlertTh_mib();
    if (rc != 0)
    {
    TRACE(2, "failed to init memoryUsageAlertThreshold MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_mem_usageWarnDurn_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init memoryUsageWarningDuration MIB for MessageSight SNMP service\n");
       return rc;
    }

    return rc;
}

int ima_snmp_mem_events_enabled()
{
    return ((bMemUsageEvents)?1:0);
}

int ima_snmp_mem_events_check(int memUsagePercent)
{
    struct timeval event_time;
    if ((bMemUsageEvents & MEMORY_USAGE_ALERT_EVENT) && (memUsagePercent >= th_memUsageAlert))
    {
        if (durn_memTraps)
        {
            gettimeofday(&event_time,NULL);
            if ((event_time.tv_sec - time_memUsageAlert) < durn_memTraps)
            {
                TRACE(9,"memUsageAlert trap happens on %ld, was suppressed due to under duration period of previous one \n",
                      event_time.tv_sec);
                return MEMORY_USAGE_EVENT_NONE;
            }
            time_memUsageAlert = event_time.tv_sec;
        }
        return MEMORY_USAGE_ALERT_EVENT;
    }
    if ((bMemUsageEvents & MEMORY_USAGE_WARNING_EVENT) &&
        (memUsagePercent >= th_memUsageWarn))
    {
        if (durn_memTraps)
        {
            gettimeofday(&event_time,NULL);
            if ((event_time.tv_sec - time_memUsageWarn) < durn_memTraps)
            {
                TRACE(9,"memUsageWarn trap happens on %ld, was suppressed due to under duration period of previous one \n",
                      event_time.tv_sec);
                return MEMORY_USAGE_EVENT_NONE;
            }
            time_memUsageWarn = event_time.tv_sec;
        }
        return MEMORY_USAGE_WARNING_EVENT;
    }
    return MEMORY_USAGE_EVENT_NONE;
}

void ima_snmp_reinit_mem_ctrl_mibs()
{
    th_memUsageWarn = MEM_USAGE_WARNING_TH;
    th_memUsageAlert = MEM_USAGE_ALERT_TH;
    durn_memTraps = MEM_USAGE_WARNING_DURN ;
}
