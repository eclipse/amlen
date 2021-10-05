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
 * @brief Store MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of store MIB initialization and registration for MessageSight.
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
#include "imaSnmpStoreMib.h"
#include "imaSnmpStoreStat.h"

int ima_snmp_handler_getStoreMemUsedPercent(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[10];
        bzero(buf,10);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,10, imaSnmpStore_MEM_USED_PERCENT);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get storeMemUsedPercent stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getStoreMemUsedPercent\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int ima_snmp_init_store_memusedpct_mib()
{
   int rc = MIB_REGISTERED_OK; 
   const oid  store_memusagepercent_oid[] = { IMA_SNMP_STORE_MEMUSEDPCT_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("StoreMemoryUsedPercent",
		   ima_snmp_handler_getStoreMemUsedPercent, store_memusagepercent_oid,
        OID_LENGTH(store_memusagepercent_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_getStoreDiskUsedPercent(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
        char buf[10];
        bzero(buf,10);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,10, imaSnmpStore_DISK_USED_PERCENT);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get storeDiskUsedPercent stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getStoreDiskUsedPercent\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int ima_snmp_init_store_diskusedpct_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_diskusagepct_oid[] = { IMA_SNMP_STORE_DISKUSEDPCT_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("StoreDiskUsedPercent",
		   ima_snmp_handler_getStoreDiskUsedPercent, store_diskusagepct_oid,
        OID_LENGTH(store_diskusagepct_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_handler_getStoreDiskFreeBytes(netsnmp_mib_handler *handler,
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
        rc = ima_snmp_get_store_stat(buf,100, imaSnmpStore_DISK_FREE_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get storeDiskFreeBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getStoreDiskFreeBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_diskfreebytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_diskfreebytes_oid[] = { IMA_SNMP_STORE_DISKFREEBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("StoreDiskFreeBytes",
           ima_snmp_handler_getStoreDiskFreeBytes, store_diskfreebytes_oid,
        OID_LENGTH(store_diskfreebytes_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//MemoryTotalBytes
int ima_snmp_handler_getStoreMemoryTotalBytes(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
    	const int LENGTH=50;
        char buf[LENGTH];
        bzero(buf,LENGTH);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,LENGTH, imaSnmpStore_MEM_TOTAL_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get MemoryTotalBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getStoreMemoryTotalBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_memorytotalbytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_temp_oid[] = { IMA_SNMP_STORE_MEMTOTALBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("MemoryTotalBytes",
		   ima_snmp_handler_getStoreMemoryTotalBytes, store_temp_oid,
        OID_LENGTH(store_temp_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


//Pool1TotalBytes
int ima_snmp_handler_getPool1TotalBytes(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
    	const int LENGTH=50;
        char buf[LENGTH];
        bzero(buf,LENGTH);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,LENGTH, imaSnmpStore_POOL1_TOTAL_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get Pool1TotalBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getPool1TotalBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_pool1totalbytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_temp_oid[] = { IMA_SNMP_STORE_POOL1TOTALBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("Pool1TotalBytes",
		   ima_snmp_handler_getPool1TotalBytes, store_temp_oid,
        OID_LENGTH(store_temp_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//Pool1UsedBytes
int ima_snmp_handler_getPool1UsedBytes(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
    	const int LENGTH=50;
        char buf[LENGTH];
        bzero(buf,LENGTH);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,LENGTH, imaSnmpStore_POOL1_USED_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get Pool1UsedBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getPool1UsedBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_pool1usedbytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_temp_oid[] = { IMA_SNMP_STORE_POOL1USEDBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("Pool1UsedBytes",
		   ima_snmp_handler_getPool1UsedBytes, store_temp_oid,
        OID_LENGTH(store_temp_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}


//Pool1UsedPercent
int ima_snmp_handler_getPool1UsedPercent(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
    	const int LENGTH=10;
        char buf[LENGTH];
        bzero(buf,LENGTH);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,LENGTH, imaSnmpStore_POOL1_USED_PERCENT);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get Pool1UsedPercent stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getPool1UsedPercent\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_pool1usedpercent_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_temp_oid[] = { IMA_SNMP_STORE_POOL1USEDPERCENT_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("Pool1UsedPercent",
		   ima_snmp_handler_getPool1UsedPercent, store_temp_oid,
        OID_LENGTH(store_temp_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//Pool1RecordsLimitBytes
int ima_snmp_handler_getPool1RecordsLimitBytes(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
    	const int LENGTH=50;
        char buf[LENGTH];
        bzero(buf,LENGTH);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,LENGTH, imaSnmpStore_POOL1_RECORDS_LIMITBYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get Pool1RecordsLimitBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getPool1RecordsLimitBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_pool1recordslimitbytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_temp_oid[] = { IMA_SNMP_STORE_POOL1RECORDSLIMITBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("Pool1RecordsLimitBytes",
		   ima_snmp_handler_getPool1RecordsLimitBytes, store_temp_oid,
        OID_LENGTH(store_temp_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//Pool1RecordsUsedBytes
int ima_snmp_handler_getPool1RecordsUsedBytes(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
    	const int LENGTH=50;
        char buf[LENGTH];
        bzero(buf,LENGTH);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,LENGTH, imaSnmpStore_POOL1_RECORDS_USEDBYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get Pool1RecordsUsedBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getPool1RecordsUsedBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_pool1recordsusedbytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_temp_oid[] = { IMA_SNMP_STORE_POOL1RECORDSUSEDBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("Pool1RecordsUsedBytes",
		   ima_snmp_handler_getPool1RecordsUsedBytes, store_temp_oid,
        OID_LENGTH(store_temp_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//Pool2TotalBytes
int ima_snmp_handler_getPool2TotalBytes(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
    	const int LENGTH=50;
        char buf[LENGTH];
        bzero(buf,LENGTH);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,LENGTH, imaSnmpStore_POOL2_TOTAL_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get Pool2TotalBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getPool2TotalBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_pool2totalbytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_temp_oid[] = { IMA_SNMP_STORE_POOL2TOTALBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("Pool2TotalBytes",
		   ima_snmp_handler_getPool2TotalBytes, store_temp_oid,
        OID_LENGTH(store_temp_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//Pool2UsedBytes
int ima_snmp_handler_getPool2UsedBytes(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
    	const int LENGTH=50;
        char buf[LENGTH];
        bzero(buf,LENGTH);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,LENGTH, imaSnmpStore_POOL2_USED_BYTES);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get Pool2UsedBytes stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER64, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getPool2UsedBytes\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_pool2usedbytes_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_temp_oid[] = { IMA_SNMP_STORE_POOL2USEDBYTES_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("Pool2UsedBytes",
		   ima_snmp_handler_getPool2UsedBytes, store_temp_oid,
        OID_LENGTH(store_temp_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

//Pool2UsedPercent
int ima_snmp_handler_getPool2UsedPercent(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {
    case MODE_GET:
    {
    	const int LENGTH=10;
        char buf[LENGTH];
        bzero(buf,LENGTH);
        int rc = 0;
        rc = ima_snmp_get_store_stat(buf,LENGTH, imaSnmpStore_POOL2_USED_PERCENT);
        if (rc != ISMRC_OK)
        {
            TRACE(3,"failed to get Pool2UsedPercent stat from MessageSight. rc = %d\n", rc);
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        ima_snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED, buf, strlen(buf));
        break;
    }
    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(3, "unknown mode (%d) in ima_snmp_handler_getPool2UsedPercent\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int ima_snmp_init_store_pool2usedpercent_mib()
{
   int rc = MIB_REGISTERED_OK;
   const oid  store_temp_oid[] = { IMA_SNMP_STORE_POOL2USEDPERCENT_MIB };

   rc = netsnmp_register_scalar(netsnmp_create_handler_registration("Pool2UsedPercent",
		   ima_snmp_handler_getPool2UsedPercent, store_temp_oid,
        OID_LENGTH(store_temp_oid), HANDLER_CAN_RONLY));
   if (rc != MIB_REGISTERED_OK) return rc;
   return 0;
}

int ima_snmp_init_store_mibs()
{
    int rc = 0;

    rc = ima_snmp_init_store_memusedpct_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storeMemUsagePercent MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_store_diskusedpct_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storeDiskUsagePercen MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_store_diskfreebytes_mib();
    if (rc != 0)
    {
       TRACE(2, "failed to init storeDiskFreeBytes MIB for MessageSight SNMP service\n");
       return rc;
    }
    rc = ima_snmp_init_store_memorytotalbytes_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to init memorytotalbytes MIB for MessageSight SNMP service\n");
	   return rc;
	}
    rc = ima_snmp_init_store_pool1totalbytes_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to init pool1totalbytes MIB for MessageSight SNMP service\n");
	   return rc;
	}
    rc = ima_snmp_init_store_pool1usedbytes_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to init Pool1UsedBytes MIB for MessageSight SNMP service\n");
	   return rc;
	}
    rc = ima_snmp_init_store_pool1usedpercent_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to init Pool1UsedPercent MIB for MessageSight SNMP service\n");
	   return rc;
	}
    rc = ima_snmp_init_store_pool1recordslimitbytes_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to init Pool1RecordsLimitBytes MIB for MessageSight SNMP service\n");
	   return rc;
	}
    rc = ima_snmp_init_store_pool1recordsusedbytes_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to init Pool1RecordsUsedBytes MIB for MessageSight SNMP service\n");
	   return rc;
	}
    rc = ima_snmp_init_store_pool2totalbytes_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to init Pool2TotalBytes MIB for MessageSight SNMP service\n");
	   return rc;
	}
    rc = ima_snmp_init_store_pool2usedbytes_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to init Pool2UsedBytes MIB for MessageSight SNMP service\n");
	   return rc;
	}
    rc = ima_snmp_init_store_pool2usedpercent_mib();
	if (rc != 0)
	{
	   TRACE(2, "failed to init Pool2UsedPercent MIB for MessageSight SNMP service\n");
	   return rc;
	}

    return rc;
}

