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
 * @brief ConnectionVolume ("stat server") MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of CV MIB initialization and registration for MessageSight.
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
#include "imaSnmpCVMib.h"
#include "imaSnmpCVStat.h"

//ActiveConnections
int ima_snmp_handler_getActiveConnections(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_CONNECTION_ACTIVE);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get ActiveConnections stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getActiveConnections\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_activeconnections_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_activeconnections_oid[] = { IMA_SNMP_CV_ACTIVECONNECTIONS_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("ActiveConnections",
        ima_snmp_handler_getActiveConnections, cv_activeconnections_oid,
        OID_LENGTH(cv_activeconnections_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//TotalConnections
int ima_snmp_handler_getTotalConnections(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_CONNECTION_TOTAL);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get TotalConnections stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getTotalConnections\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_totalconnections_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_totalconnections_oid[] = { IMA_SNMP_CV_TOTALCONNECTIONS_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("TotalConnections",
		   ima_snmp_handler_getTotalConnections, cv_totalconnections_oid,
        OID_LENGTH(cv_totalconnections_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


//MsgRead
int ima_snmp_handler_getMsgRead(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_MSG_READ);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get MsgRead stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMsgRead\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_msgread_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_msgread_oid[] = { IMA_SNMP_CV_MSGREAD_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("MsgRead",
		   ima_snmp_handler_getMsgRead, cv_msgread_oid,
        OID_LENGTH(cv_msgread_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


//MsgWrite
int ima_snmp_handler_getMsgWrite(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_MSG_WRITE);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get MsgWrite stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getMsgWrite\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_msgwrite_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_msgwrite_oid[] = { IMA_SNMP_CV_MSGWRITE_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("MsgWrite",
		   ima_snmp_handler_getMsgWrite, cv_msgwrite_oid,
        OID_LENGTH(cv_msgwrite_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


//BytesRead
int ima_snmp_handler_getBytesRead(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_BYTES_READ);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get BytesRead stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getBytesRead\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_bytesread_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_bytesread_oid[] = { IMA_SNMP_CV_BYTESREAD_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("BytesRead",
		   ima_snmp_handler_getBytesRead, cv_bytesread_oid,
        OID_LENGTH(cv_bytesread_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


//BytesWrite
int ima_snmp_handler_getBytesWrite(netsnmp_mib_handler *handler,//1
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_BYTES_WRITE);//1
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get BytesWrite stat from MessageSight. rc = %d\n", rc);//1
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getBytesWrite\n",reqinfo->mode);//1
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_byteswrite_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_byteswrite_oid[] = { IMA_SNMP_CV_BYTESWRITE_MIB };//2

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("BytesWrite", //1
		   ima_snmp_handler_getBytesWrite, cv_byteswrite_oid, //2
        OID_LENGTH(cv_byteswrite_oid), HANDLER_CAN_RONLY)); //1
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//BadConnCount
int ima_snmp_handler_getBadConnCount(netsnmp_mib_handler *handler,//1
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_CONNECTION_BAD);//1
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get BadConnCount stat from MessageSight. rc = %d\n", rc);//1
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getBadConnCount\n",reqinfo->mode);//1
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_badconncount_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_badconncount_oid[] = { IMA_SNMP_CV_BADCONNCOUNT_MIB };//2

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("BadConnCount", //1
		   ima_snmp_handler_getBadConnCount, cv_badconncount_oid, //2
        OID_LENGTH(cv_badconncount_oid), HANDLER_CAN_RONLY)); //1
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


//TotalEndpoints
int ima_snmp_handler_getTotalEndpoints(netsnmp_mib_handler *handler,//1
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_ENDPOINTS_TOTAL);//1
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get TotalEndpoints stat from MessageSight. rc = %d\n", rc);//1
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getTotalEndpoints\n",reqinfo->mode);//1
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_totalendpoints_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_totalendpoints_oid[] = { IMA_SNMP_CV_TOTALENDPOINTS_MIB };//2

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("TotalEndpoints", //1
		   ima_snmp_handler_getTotalEndpoints, cv_totalendpoints_oid, //2
        OID_LENGTH(cv_totalendpoints_oid), HANDLER_CAN_RONLY)); //1
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


//BufferedMessages
int ima_snmp_handler_getBufferedMessages(netsnmp_mib_handler *handler,//1
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_MSG_BUFFERED);//1
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get BufferedMessages stat from MessageSight. rc = %d\n", rc);//1
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getBufferedMessages\n",reqinfo->mode);//1
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_bufferedmessages_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_bufferedmessages_oid[] = { IMA_SNMP_CV_BUFFEREDMESSAGES_MIB };//2

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("BufferedMessages", //1
		   ima_snmp_handler_getBufferedMessages, cv_bufferedmessages_oid, //2
        OID_LENGTH(cv_bufferedmessages_oid), HANDLER_CAN_RONLY)); //1
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//RetainedMessages
int ima_snmp_handler_getRetainedMessages(netsnmp_mib_handler *handler,//1
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_MSG_RETAINED);//1
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get RetainedMessages stat from MessageSight. rc = %d\n", rc);//1
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getRetainedMessages\n",reqinfo->mode);//1
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_retainedmessages_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_retainedmessages_oid[] = { IMA_SNMP_CV_RETAINEDMESSAGES_MIB };//2

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("RetainedMessages", //1
		   ima_snmp_handler_getRetainedMessages, cv_retainedmessages_oid, //2
        OID_LENGTH(cv_retainedmessages_oid), HANDLER_CAN_RONLY)); //1
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//ExpiredMessages
int ima_snmp_handler_getExpiredMessages(netsnmp_mib_handler *handler,//1
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
        rc = ima_snmp_get_cv_stat(buf,100, imaSnmpCV_MSG_EXPIRED);//1
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get ExpiredMessages stat from MessageSight. rc = %d\n", rc);//1
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getExpiredMessages\n",reqinfo->mode);//1
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

static int ima_snmp_init_cv_expiredmessages_mib() //1
{
   int rc = MIB_REGISTERED_OK;
   const oid  cv_expiredmessages_oid[] = { IMA_SNMP_CV_EXPIREDMESSAGES_MIB };//2

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("ExpiredMessages", //1
		   ima_snmp_handler_getExpiredMessages, cv_expiredmessages_oid, //2
        OID_LENGTH(cv_expiredmessages_oid), HANDLER_CAN_RONLY)); //1
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


int ima_snmp_init_cv_mibs()
{
    int rc = 0;

    rc = ima_snmp_init_cv_activeconnections_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init cv_activeconnections MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_totalconnections_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init cv_totalconnections MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_msgread_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_init_cv_msgread_mib for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_msgwrite_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_init_cv_msgwrite_mib for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_bytesread_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_init_cv_bytesread_mib for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_byteswrite_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_init_cv_byteswrite_mib for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_badconncount_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_init_cv_badconncount_mib for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_totalendpoints_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_init_cv_totalendpoints_mib for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_bufferedmessages_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_init_cv_bufferedmessages_mib for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_retainedmessages_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to ima_snmp_init_cv_retainedmessages_mib for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_cv_expiredmessages_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to ima_snmp_init_cv_expiredmessages_mib for MessageSight SNMP service\n");
	   return rc;
	}

    return rc;
}
