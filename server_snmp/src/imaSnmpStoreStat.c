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
 * @brief Store Stat interface for MessageSight
 * @date July
 *
 * This provides the interface to access store stats of MessageSight.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <ismutil.h>
#include <ismjson.h>
#include <ismclient.h>
#include <ismrc.h>
#include <monitoring.h>

#include "imaSnmpJson.h"
#include "imaSnmpStoreStat.h"

#define _SNMP_THREADED_

extern int agentRefreshCycle;

#ifndef _SNMP_THREADED_
static int PROC_TYPE=0;  /* use ism admin port */
static char* MONITOR_PROTOCOL="monitoring.ism.ibm.com";
#endif

static char* USER="snmp";

static ima_snmp_store_t *storeStats = NULL;

static const ima_snmp_col_desc_t store_column_desc[] = {
    {NULL,imaSnmpCol_None, 0},  // imaSnmpStore_NONE
    {"DiskUsedPercent",imaSnmpCol_String,COLUMN_IMASTORE_DISKUSEDPCT_SIZE}, //imaSnmpStore_DISK_USED_PERCENT
    {"DiskFreeBytes",imaSnmpCol_String,COLUMN_IMASTORE_DISKFREEBYTES_SIZE}, //imaSnmpStore_DISK_FREE_BYTES
    {"MemoryUsedPercent", imaSnmpCol_String,COLUMN_IMASTORE_MEMUSEDPCT_SIZE}, //imaSnmpStore_MEM_USED_PERCENT
    {"MemoryTotalBytes", imaSnmpCol_String, COLUMN_IMASTORE_MEMTOTALBYTES_SIZE}, //imaSnmpStore_MEM_TOTAL_BYTES
    {"Pool1TotalBytes", imaSnmpCol_String, COLUMN_IMASTORE_POOL1TOTALBYTES_SIZE}, //imaSnmpStore_POOL1_TOTAL_BYTES
    {"Pool1UsedBytes", imaSnmpCol_String, COLUMN_IMASTORE_POOL1USEDBYTES_SIZE}, //imaSnmpStore_POOL1_USED_BYTES
    {"Pool1UsedPercent", imaSnmpCol_String, COLUMN_IMASTORE_POOL1USEDPCT_SIZE}, //imaSnmpStore_POOL1_USED_PERCENT
    {"Pool1RecordsLimitBytes", imaSnmpCol_String, COLUMN_IMASTORE_POOL1RECLIMIT_SIZE},//imaSnmpStore_POOL1_RECORDS_LIMITBYTES
    {"Pool1RecordsUsedBytes", imaSnmpCol_String, COLUMN_IMASTORE_POOL1RECUSED_SIZE}, //imaSnmpStore_POOL1_RECORDS_USEDBYTES
    {"Pool2TotalBytes", imaSnmpCol_String, COLUMN_IMASTORE_POOL2TOTALBYTES_SIZE}, //imaSnmpStore_POOL2_TOTAL_BYTES
    {"Pool2UsedBytes", imaSnmpCol_String, COLUMN_IMASTORE_POOL2USEDBYTES_SIZE}, //imaSnmpStore_POOL2_USED_BYTES
    {"Pool2UsedPercent", imaSnmpCol_String, COLUMN_IMASTORE_POOL2USEDPCT_SIZE} //imaSnmpStore_POOL2_USED_PERCENT
};

#define STORE_CMD_STRING "{\"Action\":\"Store\",\"User\":\"%s\",\"Locale\":\"en_US\",\"SubType\":\"Current\",\"StatType\":\"%s\",\"Duration\":\"1800\"}"

static int get_store_stat_cmd(char* cmd, int cmd_len, imaSnmpStoreDataType_t statType)
{
    if ((NULL == cmd) || (cmd_len < 200))
    {
        TRACE(2,"invalid cmd buffer for store stat command creation \n");
        return ISMRC_Error;
    }
    if ((statType <= imaSnmpStore_NONE) || (statType >= imaSnmpStore_Col_MAX))
    {
        TRACE(2,"invalid store statType for command creation \n");
        return ISMRC_Error;
    }
    //TRACE(2,"store cmd : %s\n",StoreDataType[statType]);
    snprintf(cmd, cmd_len, STORE_CMD_STRING, USER, store_column_desc[statType].colName);
    return ISMRC_OK;
}

/* free Store Stats entry*/
static inline void ima_snmp_store_stats_free(ima_snmp_store_t *pStoreStats)
{
    int i;
    if (NULL == pStoreStats) return;
    for (i = imaSnmpStore_Col_MIN; i < imaSnmpStore_Col_MAX; i++)
    {
        if ((store_column_desc[i].colType == imaSnmpCol_String) &&
            (pStoreStats->storeItem[i].ptr != NULL))
            ism_common_free(ism_memory_snmp_misc,pStoreStats->storeItem[i].ptr);
    }
    ism_common_free(ism_memory_snmp_misc,pStoreStats);
}

static ima_snmp_store_t *ima_snmp_store_stats_init()
{
    int i;
    ima_snmp_store_t *pStoreStats = NULL;

    pStoreStats = (ima_snmp_store_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,50),sizeof(ima_snmp_store_t));
    if (NULL == pStoreStats)
    {
        TRACE(2,"failed to allocate resources for Store Stats cache.");
        return NULL;
    }
    memset(pStoreStats,0,sizeof(ima_snmp_store_t));
    for (i = imaSnmpStore_Col_MIN; i < imaSnmpStore_Col_MAX; i++)
    {
        if (store_column_desc[i].colType == imaSnmpCol_String)
        { // allocate string buffer for the item;
            char *tempPtr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,51),store_column_desc[i].colSize * sizeof(char));
            if (tempPtr == NULL)
            {
                TRACE(2,"failed to allocated memory for store item %s \n",
                    store_column_desc[i].colName);
                break;
            }
            pStoreStats->storeItem[i].ptr = tempPtr;
        }
    }
    if (i < imaSnmpStore_Col_MAX)
    {
        TRACE(2,"free store Items due to malloc issue");
        ima_snmp_store_stats_free(pStoreStats);
        return NULL;
    }
    return pStoreStats;
}

static int ima_snmp_update_store_stats(imaSnmpStoreDataType_t statType)
{
    int  rc = ISMRC_Error;
    char storeStatCmd[256];
    int  i;
    struct timeval stat_time;

    rc = get_store_stat_cmd(storeStatCmd, 256,statType);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to create cmd for stat %d \n",statType);
        return rc;
    }

    rc = ISMRC_Error;

#ifndef _SNMP_THREADED_
    char *result = NULL;
    result = ismcli_ISMClient(USER, MONITOR_PROTOCOL, storeStatCmd, PROC_TYPE);

    /* process result */
    if ( result ) {
    //    TRACE(2, "Result Returned from WebSocket: %s \n\n",result);

        /* Check if returned result is a json string */
        int buflen = 0;
        ism_json_parse_t pobj = {0};
        ism_json_entry_t ents[MAX_JSON_ENTRIES];

        char *tmpbuf;
        buflen = strlen(result);
        //tmpbuf = (char *)alloca(buflen + 1);
        tmpbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,52),buflen + 1);
        memcpy(tmpbuf, result, buflen);
        tmpbuf[buflen] = 0;

        pobj.ent_alloc = MAX_JSON_ENTRIES;
        pobj.ent = ents;
        pobj.source = (char *) tmpbuf;
        pobj.src_len = buflen;
        ism_json_parse(&pobj);
        if (pobj.rc) {
            TRACE(2,"result is not json string: %s\n", result);
            fflush(stderr);
            ismCLI_FREE(result);
            ism_common_free(ism_memory_snmp_misc,tmpbuf);
            return pobj.rc;
        }

        /* check is there is an error string */
        char *errstr = (char *)ism_json_getString(&pobj, "ErrorString");
        char *rcstr = (char *)ism_json_getString(&pobj, "RC");
        if ( rcstr )
            rc = atoi(rcstr);
        if ( errstr ) {
            TRACE(2,"json parse return ErrorString: %s : RC=%s\n", errstr,rcstr);
            fflush(stderr);
            ismCLI_FREE(result);
            ism_common_free(ism_memory_snmp_misc,tmpbuf);
            return rc;
        }  // if ( errstr) 

        /*Free the result.*/
        ismCLI_FREE(result);

        /*Parse store stats from json object.*/
        for (i = imaSnmpStore_Col_MIN; i < imaSnmpStore_Col_MAX; i++)
        {
            char* tempStr=NULL;
            //int   tempVal = 0;
            int buflen=0;
            if (store_column_desc[i].colType == imaSnmpCol_String)
            {
                tempStr =  (char *)ism_json_getString(&pobj, store_column_desc[i].colName);
                if (NULL == tempStr)
                {
                    TRACE(2,"Failed to find %s in store stat message \n",
                            store_column_desc[i].colName);
                    break;
                }
                buflen = strlen(tempStr);
                if (buflen >= store_column_desc[i].colSize)
                    buflen = store_column_desc[i].colSize - 1;
                strncpy(storeStats->storeItem[i].ptr, tempStr, buflen);
                storeStats->storeItem[i].ptr[buflen] = 0;
             }
        }
        //update stat time in storeStats
        gettimeofday(&stat_time,NULL);
        storeStats->time_storeStats = stat_time.tv_sec;

        rc = ISMRC_OK;

        ism_common_free(ism_memory_snmp_misc,tmpbuf);
    } // if ( result) 
#else
    int storeBufLen = 1024;
    char storeBuf[storeBufLen];
    concat_alloc_t output_buffer;

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.buf = storeBuf;
    output_buffer.len = storeBufLen;

    ism_process_monitoring_action(NULL, storeStatCmd, strlen(storeStatCmd), &output_buffer, &rc);

    if (rc == ISMRC_OK)
    {
        ism_json_parse_t pobj = {0};
        ism_json_entry_t ents[MAX_JSON_ENTRIES];
        pobj.ent_alloc = MAX_JSON_ENTRIES;
        pobj.ent = ents;
        pobj.source = output_buffer.buf;
        pobj.src_len = output_buffer.len;
        ism_json_parse(&pobj);

        if (pobj.rc) {
            TRACE(2,"result is not json string: %s\n", output_buffer.buf);
            fflush(stderr);
            return pobj.rc;
        }

        /* check is there is an error string */
        char *errstr = (char *)ism_json_getString(&pobj, "ErrorString");
        char *rcstr = (char *)ism_json_getString(&pobj, "RC");
        if ( rcstr )
            rc = atoi(rcstr);
        if ( errstr ) {
            TRACE(2,"json parse return ErrorString: %s : RC=%s\n", errstr,rcstr);
            fflush(stderr);
            return rc;
        }

        /*Parse store stats from json object.*/
        for (i = imaSnmpStore_Col_MIN; i < imaSnmpStore_Col_MAX; i++)
        {
            char* tempStr=NULL;
            //int   tempVal = 0;
            int buflen=0;
            if (store_column_desc[i].colType == imaSnmpCol_String)
            {
                tempStr =  (char *)ism_json_getString(&pobj, store_column_desc[i].colName);
                if (NULL == tempStr)
                {
                    TRACE(2,"Failed to find %s in store stat message \n",
                            store_column_desc[i].colName);
                    break;
                }
                buflen = strlen(tempStr);
                if (buflen >= store_column_desc[i].colSize)
                    buflen = store_column_desc[i].colSize - 1;
                strncpy(storeStats->storeItem[i].ptr, tempStr, buflen);
                storeStats->storeItem[i].ptr[buflen] = 0;
             }
        }
    	//update stat time in memStats
    	gettimeofday(&stat_time,NULL);
    	storeStats->time_storeStats = stat_time.tv_sec;


    }

	if (output_buffer.inheap)
		ism_common_free(ism_memory_snmp_misc,output_buffer.buf);
#endif
    return rc;
}

int ima_snmp_get_store_stat(char *buf, int len, imaSnmpStoreDataType_t statType)
{
    struct timeval new_time;
    int  rc = ISMRC_Error;
    char *statString;

    if ((NULL == buf) || (len <= 0))
    {
        TRACE(2, "invalid buf parms for ima_snmp_get_store_stat \n");
        return ISMRC_NullPointer;
    } 
    if ((statType <= imaSnmpStore_NONE) || (statType >= imaSnmpStore_Col_MAX))
    {
        TRACE(2,"invalid store statType for store stats query \n");
        return ISMRC_Error;
    }
    if (storeStats == NULL)
    {
        storeStats = ima_snmp_store_stats_init();
        if (NULL == storeStats)
        {
            TRACE(2, "failed to allocate store stat cache \n");
            return rc;
        }
    }
    gettimeofday(&new_time,NULL);
    if ((new_time.tv_sec - storeStats->time_storeStats) > agentRefreshCycle)
    {
        // stat is out-of-date, get new one
        rc = ima_snmp_update_store_stats(statType);
        if (rc != ISMRC_OK)
        {
            TRACE(2, "failed to get store stats from imaserver \n");
            return rc;
        }
    }
        
    statString = storeStats->storeItem[statType].ptr;
    if ((statString) && (strlen(statString) >= 0))
    {
        snprintf(buf,len,statString);
        rc = ISMRC_OK;
    }
    return rc;
}
