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
 * @brief ConnectionVolume Stat interface for MessageSight
 *
 * This provides the interface to access CV stats of MessageSight.
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
#include "imaSnmpJson.h"
#include "imaSnmpCVStat.h"

#define _SNMP_THREADED_

extern int agentRefreshCycle;

#ifndef _SNMP_THREADED_
static int PROC_TYPE=0;  /* use ism admin port */
static char* MONITOR_PROTOCOL="monitoring.ism.ibm.com";
#endif

static char* USER="snmp";

static ima_snmp_cv_t *CVStats = NULL;

static const ima_snmp_col_desc_t cv_column_desc[] = {
    {NULL,imaSnmpCol_None, 0},  // imaSnmpCV_NONE
    {"ActiveConnections", imaSnmpCol_String, COLUMN_IMACV_CONNACTIVE_SIZE }, //imaSnmpCV_CONNECTION_ACTIVE
    {"TotalConnections", imaSnmpCol_String, COLUMN_IMACV_CONNTOTAL_SIZE }, //imaSnmpCV_CONNECTION_TOTAL
    {"MsgRead", imaSnmpCol_String, COLUMN_IMACV_MSGREAD_SIZE }, //imaSnmpCV_MSG_READ
    {"MsgWrite", imaSnmpCol_String, COLUMN_IMACV_MSGWRITE_SIZE }, //imaSnmpCV_MSG_WRITE
    {"BytesRead", imaSnmpCol_String, COLUMN_IMACV_BYTESREAD_SIZE }, //imaSnmpCV_BYTES_READ
    {"BytesWrite", imaSnmpCol_String, COLUMN_IMACV_BYTESWRITE_SIZE }, //imaSnmpCV_BYTES_WRITE
    {"BadConnCount", imaSnmpCol_String, COLUMN_IMACV_CONNBAD_SIZE }, //imaSnmpCV_CONNECTION_BAD
    {"TotalEndpoints", imaSnmpCol_String, COLUMN_IMACV_ENDPOINTSTOTAL_SIZE }, //imaSnmpCV_ENDPOINTS_TOTAL
    {"BufferedMessages", imaSnmpCol_String, COLUMN_IMACV_MSGBUFFERED_SIZE }, //imaSnmpCV_MSG_BUFFERED
    {"RetainedMessages", imaSnmpCol_String, COLUMN_IMACV_MSGRETAINED_SIZE }, //imaSnmpCV_MSG_RETAINED
    {"ExpiredMessages", imaSnmpCol_String, COLUMN_IMACV_MSGEXPIRED_SIZE } //imaSnmpCV_MSG_EXPIRED
};

#define SERVER_CMD_STRING "{\"Action\":\"Connection\",\"User\":\"%s\",\"Option\":\"Volume\"}"

/*
 * Brief: construct the cmd of getting CV statistics
 * Para:
 * cmd: a buffer used to cache the constructed cmd.
 * len: buffersize
 * statType:
 * */
static int get_CV_stat_cmd(char* cmd, int cmd_len, imaSnmpCVDataType_t statType)
{
    if ((NULL == cmd) || (cmd_len < 200))
    {
        TRACE(2,"invalid cmd buffer for CV stat command creation \n");
        return ISMRC_Error;
    }
    if ((statType <= imaSnmpCV_NONE) || (statType >= imaSnmpCV_Col_MAX))
    {
        TRACE(2,"invalid CV statType\n");
        return ISMRC_Error;
    }
    snprintf(cmd, cmd_len, SERVER_CMD_STRING,USER);
    return ISMRC_OK;
}

/* free Store Stats entry*/
static inline void ima_snmp_cv_stats_free(ima_snmp_cv_t *pCVStats)
{
    int i;
    if (NULL == pCVStats) return;
    for (i = imaSnmpCV_Col_MIN; i < imaSnmpCV_Col_MAX; i++)
    {
        if ((cv_column_desc[i].colType == imaSnmpCol_String) &&
            (pCVStats->cvItem[i].ptr != NULL))
            ism_common_free(ism_memory_snmp_misc,pCVStats->cvItem[i].ptr);
    }
    ism_common_free(ism_memory_snmp_misc,pCVStats);
}

static ima_snmp_cv_t *ima_snmp_cv_stats_init()
{
    int i;
    ima_snmp_cv_t *pCVStats = NULL;

    pCVStats = (ima_snmp_cv_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,94),sizeof(ima_snmp_cv_t));
    if (NULL == pCVStats)
    {
        TRACE(2,"failed to allocate resources for CV Stats cache.");
        return NULL;
    }
    memset(pCVStats,0,sizeof(ima_snmp_cv_t));
    for (i = imaSnmpCV_Col_MIN; i < imaSnmpCV_Col_MAX; i++)
    {
        if (cv_column_desc[i].colType == imaSnmpCol_String)
        { // allocate string buffer for the item;
            char *tempPtr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,95),cv_column_desc[i].colSize * sizeof(char));
            if (tempPtr == NULL)
            {
                TRACE(2,"failed to allocated memory for cv item %s \n",
                    cv_column_desc[i].colName);
                break;
            }
            pCVStats->cvItem[i].ptr = tempPtr;
        }
    }
    if (i < imaSnmpCV_Col_MAX)
    {
        TRACE(2,"free cv Items due to malloc issue");
        ima_snmp_cv_stats_free(pCVStats);
        return NULL;
    }
    return pCVStats;
}

static int ima_snmp_update_cv_stats(imaSnmpCVDataType_t statType)
{
    int  rc = ISMRC_Error;
    char CVStatCmd[256];
    struct timeval stat_time;

    int  i;
    int ent_num;

    rc = get_CV_stat_cmd(CVStatCmd, 256,statType);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to create cmd for stat %d \n",statType);
        return rc;
    }

    rc = ISMRC_Error;

#ifndef _SNMP_THREADED_
    char *result = NULL;
    result = ismcli_ISMClient(USER, MONITOR_PROTOCOL, CVStatCmd, PROC_TYPE);

    /* process result */
    if ( result ) {
    	//TRACE(2, "Result Returned from WebSocket: %s \n\n",result);

        /* Check if returned result is a json string */
        int buflen = 0;
        ism_json_parse_t pobj = {0};
        ism_json_entry_t ents[MAX_JSON_ENTRIES];

        char *tmpbuf;
        buflen = strlen(result);
        //tmpbuf = (char *)alloca(buflen + 1);
        tmpbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,96),buflen + 1);
        memcpy(tmpbuf, result, buflen);
        tmpbuf[buflen] = 0;
        //TRACE(2, "ima_snmp_get_CV_stat: %s\n", tmpbuf);
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
        /*Parse cv stats from json object.*/
        for (i = imaSnmpCV_Col_MIN; i < imaSnmpCV_Col_MAX; i++)
        {
            char* tempStr=NULL;
            //int   tempVal = 0;
            int buflen=0;
            ent_num = 0;
            if (cv_column_desc[i].colType == imaSnmpCol_String)
            {
                tempStr =  (char *)ima_snmp_jsonArray_getString(&pobj, cv_column_desc[i].colName,&ent_num);
                if (NULL == tempStr)
                {
                    TRACE(2,"Failed to find %s in cv stat message , ent %d\n",
                            cv_column_desc[i].colName, ent_num);
                    continue;
                }
                buflen = strlen(tempStr);
                if (buflen >= cv_column_desc[i].colSize)
                    buflen = cv_column_desc[i].colSize - 1;
                strncpy(cvStats->cvItem[i].ptr, tempStr, buflen);
                CVStats->cvItem[i].ptr[buflen] = 0;
             }
        }
        //update stat time in cvStats
        gettimeofday(&stat_time,NULL);
        CVStats->time_cvStats = stat_time.tv_sec;

        rc = ISMRC_OK;
                    
        ism_common_free(ism_memory_snmp_misc,tmpbuf);
    } // if ( result)
#else
    int CVBufLen = 1024;
    char CVBuf[CVBufLen];
    concat_alloc_t output_buffer;

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.buf = CVBuf;
    output_buffer.len = CVBufLen;

    ism_process_monitoring_action(NULL, CVStatCmd, strlen(CVStatCmd), &output_buffer, &rc);

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
    	/*Parse CV stats from json object.*/
        for (i = imaSnmpCV_Col_MIN; i < imaSnmpCV_Col_MAX; i++)
        {
            char* tempStr=NULL;
            int tmpLen=0;
            ent_num = 0;
            if (cv_column_desc[i].colType == imaSnmpCol_String)
            {
                tempStr =  (char *)ima_snmp_jsonArray_getString(&pobj, cv_column_desc[i].colName,&ent_num);
                if (NULL == tempStr)
                {
                    TRACE(2,"Failed to find %s in cv stat message , ent %d\n",
                            cv_column_desc[i].colName, ent_num);
                    continue;
                }
                tmpLen = strlen(tempStr);
                if (tmpLen >= cv_column_desc[i].colSize)
                    tmpLen = cv_column_desc[i].colSize - 1;
                strncpy(CVStats->cvItem[i].ptr, tempStr, tmpLen);
                CVStats->cvItem[i].ptr[tmpLen] = 0;
             }
        }
    	//update stat time in memStats
    	gettimeofday(&stat_time,NULL);
    	CVStats->time_cvStats = stat_time.tv_sec;
    }

	if (output_buffer.inheap)
		ism_common_free(ism_memory_snmp_misc,output_buffer.buf);
#endif
    return rc;
}

/*
 * Brief: get the CV statistics info from messagesight.
 *
 * Para:
 * buf: a buffer used to cache returned CV statistics .
 * len: buffersize
 * statType: the specific field in the CV statistics.
 * */
int ima_snmp_get_cv_stat(char *buf, int len, imaSnmpCVDataType_t statType)
{
    struct timeval new_time;
    int  rc = ISMRC_Error;
    char *statString;

    if ((NULL == buf) || (len <= 0))
    {
        TRACE(2, "invalid buf parms for ima_snmp_get_cv_stat \n");
        return ISMRC_NullPointer;
    }
    if ((statType <= imaSnmpCV_NONE) || (statType >= imaSnmpCV_Col_MAX))
    {
        TRACE(2,"invalid CV statType\n");
        return ISMRC_Error;
    }
    if (CVStats == NULL)
    {
        CVStats = ima_snmp_cv_stats_init();
        if (NULL == CVStats)
        {
            TRACE(2, "failed to allocate cv stat cache \n");
            return rc;
        }
    }
    gettimeofday(&new_time,NULL);
    if ((new_time.tv_sec - CVStats->time_cvStats) > agentRefreshCycle)
    {
        // stat is out-of-date, get new one
        rc = ima_snmp_update_cv_stats(statType);
        if (rc != ISMRC_OK)
        {
            TRACE(2, "failed to get cv stats from imaserver \n");
            return rc;
        }
    }

    statString = CVStats->cvItem[statType].ptr;
    if ((statString) && (strlen(statString) >= 0))
    {
        snprintf(buf,len,statString);
        rc = ISMRC_OK;
    }
    return rc;        
}












