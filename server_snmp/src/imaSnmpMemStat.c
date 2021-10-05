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
 * @brief Memory Stat interface for MessageSight
 * @date July
 *
 * This provides the interface to access memory stats of MessageSight.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <ismutil.h>
#include <ismjson.h>
#include <ismrc.h>
#include <ismclient.h>
#include <monitoring.h>

#include "imaSnmpMemStat.h"

#define _SNMP_THREADED_
extern int agentRefreshCycle;

#ifndef _SNMP_THREADED_
static int PROC_TYPE=0;  /* use ism admin port */
static char* MONITOR_PROTOCOL="monitoring.ism.ibm.com";
#endif

static char* USER="snmp";

static ima_snmp_mem_t *memStats = NULL;

static const ima_snmp_col_desc_t mem_column_desc[] = {
   {NULL,imaSnmpCol_None, 0},  // imaSnmpMem_NONE
   {"MemoryTotalBytes", imaSnmpCol_String, COLUMN_IMAMEMITEM_DEFAULT_SIZE}, // imaSnmpMem_TOTAL_BYTES
   {"MemoryFreeBytes", imaSnmpCol_String, COLUMN_IMAMEMITEM_DEFAULT_SIZE}, //imaSnmpMem_FREE_BYTES
   {"MemoryFreePercent", imaSnmpCol_String, COLUMN_IMAMEMITEM_DEFAULT_SIZE}, //imaSnmpMem_FREE_PERCENT
   {"ServerVirtualMemoryBytes", imaSnmpCol_String, COLUMN_IMAMEMITEM_DEFAULT_SIZE}, //imaSnmpMem_SVR_VRTL_MEM_BYTES
   {"ServerResidentSetBytes", imaSnmpCol_String, COLUMN_IMAMEMITEM_DEFAULT_SIZE}, //imaSnmpMem_SVR_RSDT_SET_BYTES
   {"MessagePayloads", imaSnmpCol_String, MAX_STR_SIZE}, //imaSnmpMem_GRP_MSG_PAYLOADS
   {"PublishSubscribe", imaSnmpCol_String, MAX_STR_SIZE}, //imaSnmpMem_GRP_PUB_SUB
   {"Destinations", imaSnmpCol_String, MAX_STR_SIZE}, //imaSnmpMem_GRP_DESTS
   {"CurrentActivity", imaSnmpCol_String, COLUMN_IMAMEMITEM_DEFAULT_SIZE}, //imaSnmpMem_GRP_CUR_ACTIVITIES
   {"ClientStates", imaSnmpCol_String, COLUMN_IMAMEMITEM_DEFAULT_SIZE} //imaSnmpMem_GRP_CLIENT_STATES
};

#define MEM_CMD_STRING "{\"Action\":\"Memory\",\"User\":\"%s\",\"Locale\":\"en_US\",\"SubType\":\"Current\",\"StatType\":\"%s\",\"Duration\":\"1800\"}"

static int get_mem_stat_cmd(char* cmd, int cmd_len, imaSnmpMemDataType_t statType)
{
    if ((NULL == cmd) || (cmd_len < 200))
    {
        TRACE(2,"invalid cmd buffer for memory stat command creation \n");
        return ISMRC_Error;
    }
    if ((statType <= imaSnmpMem_NONE) || (statType >= imaSnmpMem_Col_MAX))
    {
        TRACE(2,"invalid memory statType for command creation \n");
        return ISMRC_Error;
    }
    //TRACE(2,"mem cmd : %s\n",MemoryDataType[statType]);
    snprintf(cmd, cmd_len, MEM_CMD_STRING, USER, mem_column_desc[statType].colName);
    return ISMRC_OK;
}

/* free Memory Stats entry*/
static inline void ima_snmp_mem_stats_free(ima_snmp_mem_t *pMemStats)
{
    int i;
    if (NULL == pMemStats) return;
    for (i = imaSnmpMem_Col_MIN; i < imaSnmpMem_Col_MAX; i++)
    {
        if ((mem_column_desc[i].colType == imaSnmpCol_String) &&
            (pMemStats->memItem[i].ptr != NULL))
            ism_common_free(ism_memory_snmp_misc,pMemStats->memItem[i].ptr);
    }
    ism_common_free(ism_memory_snmp_misc,pMemStats);
}

static ima_snmp_mem_t *ima_snmp_mem_stats_init()
{
    int i;
    ima_snmp_mem_t *pMemStats = NULL;

    pMemStats = (ima_snmp_mem_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,73),sizeof(ima_snmp_mem_t));
    if (NULL == pMemStats)
    {
        TRACE(2,"failed to allocate resources for memory Stats cache.");
        return NULL;
    }
    memset(pMemStats,0,sizeof(ima_snmp_mem_t));
    for (i = imaSnmpMem_Col_MIN; i < imaSnmpMem_Col_MAX; i++)
    {
        if (mem_column_desc[i].colType == imaSnmpCol_String)
        { // allocate string buffer for the item;
            char *tempPtr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,74),mem_column_desc[i].colSize * sizeof(char));
            if (tempPtr == NULL)
            {
                TRACE(2,"failed to allocated memory for memory item %s \n",
                    mem_column_desc[i].colName);
                break;
            }
            pMemStats->memItem[i].ptr = tempPtr;
        }
    }
    if (i < imaSnmpMem_Col_MAX)
    {
        TRACE(2,"free mem Items due to malloc issue");
        ima_snmp_mem_stats_free(pMemStats);
        return NULL;
    }
    return pMemStats;
}

static int ima_snmp_update_mem_stats(imaSnmpMemDataType_t statType)
{
    int  rc = ISMRC_Error;
    char memStatCmd[256];
    int  i;
    struct timeval stat_time;


    rc = get_mem_stat_cmd(memStatCmd, 256,statType);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to create cmd for stat %d \n",statType);
        return rc;
    }

    rc = ISMRC_Error;

#ifndef _SNMP_THREADED_
    char *result = NULL;
    result = ismcli_ISMClient(USER, MONITOR_PROTOCOL, memStatCmd, PROC_TYPE);
    /* process result */
        if ( result ) {
        //	TRACE(2, "Result Returned from WebSocket: %s \n\n",result);

            /* Check if returned result is a json string */
            int buflen = 0;
            ism_json_parse_t pobj = {0};
            ism_json_entry_t ents[MAX_JSON_ENTRIES];

            char *tmpbuf;
            buflen = strlen(result);
            //tmpbuf = (char *)alloca(buflen + 1);
            tmpbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,75),buflen + 1);
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

            /*Parse mem stats from json object.*/
            for (i = imaSnmpMem_Col_MIN; i < imaSnmpMem_Col_MAX; i++)
            {
                char* tempStr=NULL;
                //int   tempVal = 0;
                int buflen=0;
                if (mem_column_desc[i].colType == imaSnmpCol_String)
                {
                    tempStr =  (char *)ism_json_getString(&pobj, mem_column_desc[i].colName);
                    if (NULL == tempStr)
                    {
                        TRACE(2,"Failed to find %s in memory stat message \n",
                                mem_column_desc[i].colName);
                        break;
                    }
                    buflen = strlen(tempStr);
                    if (buflen >= mem_column_desc[i].colSize)
                        buflen = mem_column_desc[i].colSize - 1;
                    strncpy(memStats->memItem[i].ptr, tempStr, buflen);
                    memStats->memItem[i].ptr[buflen] = 0;
                 }
            }
            //update stat time in memStats
            gettimeofday(&stat_time,NULL);
            memStats->time_memStats = stat_time.tv_sec;

            rc = ISMRC_OK;
            ism_common_free(ism_memory_snmp_misc,tmpbuf);
        } // if ( result)
#else
    int memBufLen = 1024;
    char memBuf[memBufLen];
    concat_alloc_t output_buffer;

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.buf = memBuf;
    output_buffer.len = memBufLen;

    ism_process_monitoring_action(NULL, memStatCmd, strlen(memStatCmd), &output_buffer, &rc);

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

    	/*Parse mem stats from json object.*/
    	for (i = imaSnmpMem_Col_MIN; i < imaSnmpMem_Col_MAX; i++)
    	{
    		char* tempStr=NULL;
    	    int tmpLen=0;
    	    if (mem_column_desc[i].colType == imaSnmpCol_String)
    	    {
    	    	tempStr =  (char *)ism_json_getString(&pobj, mem_column_desc[i].colName);
    	        if (NULL == tempStr)
    	        {
    	        	TRACE(2,"Failed to find %s in memory stat message \n",
    	            mem_column_desc[i].colName);
    	            break;
    	        }
    	        tmpLen = strlen(tempStr);
    	        if (tmpLen >= mem_column_desc[i].colSize)
    	        	tmpLen = mem_column_desc[i].colSize - 1;
    	        strncpy(memStats->memItem[i].ptr, tempStr, tmpLen);
    	        memStats->memItem[i].ptr[tmpLen] = 0;
    	    }
    	}
    	//update stat time in memStats
    	gettimeofday(&stat_time,NULL);
    	memStats->time_memStats = stat_time.tv_sec;


    }

	if (output_buffer.inheap)
		ism_common_free(ism_memory_snmp_misc,output_buffer.buf);
#endif
    return rc;
}

int ima_snmp_get_mem_stat(char *buf, int len, imaSnmpMemDataType_t statType)
{
    struct timeval new_time;
    int  rc = ISMRC_Error;
    char *statString;

    if ((NULL == buf) || (len <= 0))
    {
        TRACE(2, "invalid buf parms for ima_snmp_get_stat \n");
        return ISMRC_NullPointer;
    } 
    if ((statType <= imaSnmpMem_NONE) || (statType >= imaSnmpMem_Col_MAX))
    {
        TRACE(2,"invalid memory statType for memory stat query \n");
        return ISMRC_Error;
    }
    if (memStats == NULL)
    {
        memStats = ima_snmp_mem_stats_init();
        if (NULL == memStats)
        {
            TRACE(2, "failed to allocate mem stat cache \n");
            return rc;
        }
    }
    gettimeofday(&new_time,NULL);
    if ((new_time.tv_sec - memStats->time_memStats) > agentRefreshCycle)
    {
        // stat is out-of-date, get new one
        rc = ima_snmp_update_mem_stats(statType);
        if (rc != ISMRC_OK)
        {
            TRACE(2, "failed to get memory stats from imaserver \n");
            return rc;
        }
    }
    statString = memStats->memItem[statType].ptr;
    if ((statString) && (strlen(statString) >= 0))
    {
        snprintf(buf,len,statString);
        rc = ISMRC_OK;
    }
    return rc;
}

