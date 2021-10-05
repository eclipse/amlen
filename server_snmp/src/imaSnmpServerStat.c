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
 * @brief Server Stat interface for MessageSight
 *
 * This provides the interface to access server stats of MessageSight.
 *
 */

#define _SNMP_THREADED_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <admin.h>
#include <jansson.h>
#include <ismutil.h>
#include <ismrc.h>
#include "imaSnmpServerStat.h"

#ifndef _SNMP_THREADED_
#include <ismjson.h>
#include <ismclient.h>
#include "imaSnmpJson.h"
#endif
extern int agentRefreshCycle;

#ifndef _SNMP_THREADED_
static int PROC_TYPE=0;  /* use ism admin port */
static char* ADMIN_PROTOCOL="admin.ism.ibm.com";
#endif

static char* USER="snmp";
static char*  IMA_STATUS_CHECK= IMA_SVR_INSTALL_PATH "/bin/cleanstorelockfilecheck.sh";
/*
 Response: "{ \"Version\":\"v1\",\n  \"Server\":{ \"Name\":\"\",\"UID\":\"V7KIZr01\",\"State\":1,
 	 	 	  \"StateDescription\":\"Status = Running (production)\",\"ErrorCode \":0,
 	 	 	  \"UpTimeSeconds\":59485,\"UpTimeDescription\":\"0 days 16 hours 31 minutes 25 seconds\",
 	 	 	  \"Version\":\" \",\"ErrorMessage\":\"\"},  \"Container\": {},
 	 	 	  \"HighAvailability\": {\"Role\":\"HADISABLED\",\"IsHAEnabled\":0,\"NewRole\":\"HADISABLED\",
 	 	 	  \"OldRole\":\"HADISABLED\",\"ActiveNodes\":0,\"SyncNodes\":0,
 	 	 	  \"PrimaryLastTime\":\"2015-11-19 16:12:25Z\",\"PctSyncCompletion\":0,\"ReasonCode\":0,
 	 	 	  \"ReasonString\":\"\"},  \"Cluster\": {\"Status\": \"Inactive\"},
 	 	 	  \"Plugin\": {\"Status\": \"Stopped\"},  \"MQConnectivity\": {\"Status\": \"Stopped\" }}"
*/

static ima_snmp_server_t *serverStats = NULL;

static const ima_snmp_col_desc_t server_column_desc[] = {
    {NULL,imaSnmpCol_None, 0},  // imaSnmpServer_NONE
    {"State", imaSnmpCol_Integer, COLUMN_IMASERVER_SERVERSTATE_SIZE},//imaSnmpServer_SERVERSTATE
    {"StateDescription", imaSnmpCol_String, COLUMN_IMASERVER_SERVERSTATESTR_SIZE},//imaSnmpServer_SERVERSTATESTR
	{"ErrorCode ", imaSnmpCol_Integer, COLUMN_IMASERVER_ADMINSTATERC_SIZE},//imaSnmpServer_ADMINSTATERC
    {"UpTimeSeconds", imaSnmpCol_Integer, COLUMN_IMASERVER_SERVERUPTIME_SIZE},//imaSnmpServer_SERVERUPTIME
    {"UpTimeDescription", imaSnmpCol_String, COLUMN_IMASERVER_SERVERUPTIMESTR_SIZE},//imaSnmpServer_SERVERUPTIMESTR
    {"EnableHA", imaSnmpCol_Integer, COLUMN_IMASERVER_ISHAENABLED_SIZE},//imaSnmpServer_ISHAENABLED
    {"NewRole", imaSnmpCol_String, COLUMN_IMASERVER_NEWROLE_SIZE},//imaSnmpServer_NEWROLE
    {"OldRole", imaSnmpCol_String, COLUMN_IMASERVER_OLDROLE_SIZE},//imaSnmpServer_OLDROLE
    {"ActiveNodes", imaSnmpCol_Integer, COLUMN_IMASERVER_ACTIVENODES_SIZE},//imaSnmpServer_ACTIVENODES
    {"SyncNodes", imaSnmpCol_Integer, COLUMN_IMASERVER_SYNCNODES_SIZE},//imaSnmpServer_SYNCNODES
    {"PrimaryLastTime", imaSnmpCol_String, COLUMN_IMASERVER_PRIMARYLASTTIME_SIZE},//imaSnmpServer_PRIMARYLASTTIME
    {"PctSyncCompletion", imaSnmpCol_Integer, COLUMN_IMASERVER_PCTSYNCCOMPLETION_SIZE},//imaSnmpServer_PCTSYNCCOMPLETION
    {"ReasonCode", imaSnmpCol_Integer, COLUMN_IMASERVER_REASONCODE_SIZE} //imaSnmpServer_REASONCODE
};

static const char *server_comp_obj_keys[] = {
		"Server",
		"Container",
		"HighAvailability",
		"Cluster",
		NULL
};

#define SERVER_CMD_STRING "{\"Action\":\"status\",\"User\":\"%s\"}"


/*
 * Brief: construct the cmd of getting server statistics
 * Para:
 * cmd: a buffer used to cache the constructed cmd.
 * len: buffersize
 * statType:
 * */
static int get_server_stat_cmd(char* cmd, int cmd_len, imaSnmpServerDataType_t statType)
{
    if ((NULL == cmd) || (cmd_len < 200))
    {
        TRACE(2,"invalid cmd buffer for server stat command creation \n");
        return ISMRC_Error;
    }
    if ((statType <= imaSnmpServer_NONE) || (statType >= imaSnmpServer_Col_MAX))
    {
        TRACE(2,"invalid server statType\n");
        return ISMRC_Error;
    }
    snprintf(cmd, cmd_len, SERVER_CMD_STRING,USER);
    return ISMRC_OK;
}

/* free Server Stats entry*/
static inline void ima_snmp_server_stats_free(ima_snmp_server_t *pServerStats)
{
    int i;
    if (NULL == pServerStats) return;
    for (i = imaSnmpServer_Col_MIN; i < imaSnmpServer_Col_MAX; i++)
    {
        if ((server_column_desc[i].colType == imaSnmpCol_String) &&
            (pServerStats->serverItem[i].ptr != NULL))
            ism_common_free(ism_memory_snmp_misc,pServerStats->serverItem[i].ptr);
    }
    ism_common_free(ism_memory_snmp_misc,pServerStats);
}

static ima_snmp_server_t *ima_snmp_server_stats_init()
{
    int i;
    ima_snmp_server_t *pServerStats = NULL;

    pServerStats = (ima_snmp_server_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,59),sizeof(ima_snmp_server_t));
    if (NULL == pServerStats)
    {
        TRACE(2,"failed to allocate resources for Server Stats cache.");
        return NULL;
    }
    memset(pServerStats,0,sizeof(ima_snmp_server_t));
    for (i = imaSnmpServer_Col_MIN; i < imaSnmpServer_Col_MAX; i++)
    {
        if (server_column_desc[i].colType == imaSnmpCol_String)
        { // allocate string buffer for the item;
            char *tempPtr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,60),server_column_desc[i].colSize * sizeof(char));
            if (tempPtr == NULL)
            {
                TRACE(2,"failed to allocated memory for server item %s \n",
                    server_column_desc[i].colName);
                break;
            }
            pServerStats->serverItem[i].ptr = tempPtr;
        }
    }
    if (i < imaSnmpServer_Col_MAX)
    {
        TRACE(2,"free server items due to malloc issue");
        ima_snmp_server_stats_free(pServerStats);
        return NULL;
    }
    return pServerStats;
}

static int ima_snmp_update_server_stats(imaSnmpServerDataType_t statType)
{
    int  rc = ISMRC_Error;
    char serverStatCmd[256];
    struct timeval stat_time;

    json_t			*pRootObj = NULL;
    json_error_t	jsonError;

    rc = get_server_stat_cmd(serverStatCmd, 256,statType);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to create cmd for stat %d \n",statType);
        return rc;
    }

    rc = ISMRC_Error;

#ifndef _SNMP_THREADED_
    char *result = NULL;
    result = ismcli_ISMClient(USER, ADMIN_PROTOCOL, serverStatCmd, PROC_TYPE);


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
        tmpbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,61),buflen + 1);
        memcpy(tmpbuf, result, buflen);
        tmpbuf[buflen] = 0;
        //TRACE(2, "ima_snmp_get_server_stat: %s\n", tmpbuf);
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

        /*Parse server stats from json object.*/
        for (i = imaSnmpServer_Col_MIN; i < imaSnmpServer_Col_MAX; i++)
        {
            char* tempStr=NULL;
            //int   tempVal = 0;
            int buflen=0;
            if (server_column_desc[i].colType == imaSnmpCol_String)
            {
                tempStr =  (char *)ism_json_getString(&pobj, server_column_desc[i].colName);
                if (NULL == tempStr)
                {
                    TRACE(2,"Failed to find %s in server stat message \n",
                            server_column_desc[i].colName);
                    break;
                }
                buflen = strlen(tempStr);
                if (buflen >= server_column_desc[i].colSize)
                    buflen = server_column_desc[i].colSize - 1;
                strncpy(serverStats->serverItem[i].ptr, tempStr, buflen);
                serverStats->serverItem[i].ptr[buflen] = 0;
             }
        }
        //update stat time in serverStats
        gettimeofday(&stat_time,NULL);
        serverStats->time_serverStats = stat_time.tv_sec;

        rc = ISMRC_OK;
                    
        ism_common_free(ism_memory_snmp_misc,tmpbuf);
    } // if ( result)
#else
    int 			serverBufLen = 1024;
    ism_http_t		*http = NULL;
    concat_alloc_t 	*output_buffer = NULL;

    http = ism_http_newHttp(HTTP_OP_GET, NULL, NULL, NULL, NULL, 0, NULL, NULL, 0, serverBufLen);
    if (!http)
    {
    	rc = ISMRC_AllocateError;
    	return rc;
    }
    http->outbuf.inheap = 0;
    http->outbuf.used = 0;
    http->outbuf.pos = 0;

    rc = ism_admin_getServerStatus(http, NULL, 1);
    output_buffer = &http->outbuf;

    if (rc == ISMRC_OK)
    {
    	int compIdx = 0;
    	int i = 0;
    	int keysFound[imaSnmpServer_Col_MAX] = { 0 };

    	void *iter;
		const char jsonInput[output_buffer->used+1];
    	const char *key;
    	json_t *compObj;
    	json_t *value;

	memset((void *)jsonInput, 0, output_buffer->used+1);
	strncpy((char *)jsonInput, output_buffer->buf, output_buffer->used);
    	pRootObj = json_loads(jsonInput, 0, &jsonError);

    	if (!pRootObj)
    	{
    		TRACE(2, "Error: Not a valid json string on line: %d %s\n", jsonError.line, jsonError.text);
    		rc = ISMRC_NotFound;
    		goto outHttp;
    	}

    	if (!json_is_object(pRootObj))
    	{
    		TRACE(2, "Error: Expected a json object from valid json string");
    		rc = ISMRC_NotFound;
    		goto outHttp;
    	}

    	while (server_comp_obj_keys[compIdx])
    	{
    		compObj = json_object_get(pRootObj, server_comp_obj_keys[compIdx]);
    		if (compObj)
    		{
    			iter = json_object_iter(compObj);
    			while(iter)
    			{
    				key = json_object_iter_key(iter);
    				for (i = imaSnmpServer_Col_MIN; i < imaSnmpServer_Col_MAX; i++)
    				{
    					if (keysFound[i])
    						continue;
    					/* Did we find a matching key ? */
    					if (!strcmp(key, server_column_desc[i].colName))
    					{
    						const char *tempStr = NULL;
    						int tempStrLen = 0;

    						keysFound[i] = 1;
    						value = json_object_iter_value(iter);
    						switch (json_typeof(value))
    						{
    						case JSON_INTEGER:
    							serverStats->serverItem[i].val = json_integer_value(value);
    							break;
    						case JSON_STRING:
    							tempStr = json_string_value(value);
    							if (tempStr)
    							{
    								tempStrLen = strlen(tempStr);
    								if (tempStrLen >= server_column_desc[i].colSize)
    									tempStrLen = server_column_desc[i].colSize - 1;
    								strncpy(serverStats->serverItem[i].ptr, tempStr, tempStrLen);
    								serverStats->serverItem[i].ptr[tempStrLen] = 0;
    							}
    							break;
    						case JSON_TRUE:
    							serverStats->serverItem[i].val = 1;
    							break;
    						case JSON_FALSE:
    							serverStats->serverItem[i].val = 0;
    							break;
    						default:
    							TRACE(2, "Unexpected JSON object type encountered\n");
    							rc = ISMRC_NotFound;
    							goto outHttp;
    						}
    						break;
    					}
    				} /* end for */
    				i = 0;
    				iter = json_object_iter_next(compObj, iter);
    			} /* end while iter exists */
    		} /* end if composite object found */
    		compIdx++;
    	} /* end while composite keys exist*/
		compIdx = 0;

    	if (!keysFound[statType])
    		TRACE(2, "Failed to find %s in server stat message \n", server_column_desc[statType].colName);
        //update stat time in serverStats
        gettimeofday(&stat_time,NULL);
        serverStats->time_serverStats = stat_time.tv_sec;
    }
outHttp:
	if (pRootObj)
		json_decref(pRootObj);
    if (output_buffer->inheap)
    	ism_common_free(ism_memory_snmp_misc,output_buffer->buf);
    ism_common_free(ism_memory_snmp_misc,http);
#endif
    return rc;
}

/*
 * Brief: get the server statistics info from messagesight.
 *
 * Para:
 * buf: a buffer used to cache returned server statistics .
 * len: buffersize
 * statType: the specific field in the server statistics.
 * */
int ima_snmp_get_server_stat(char *buf, int len, imaSnmpServerDataType_t statType)
{
    struct timeval new_time;
    int  rc = ISMRC_Error;
    char *statString;

    if ((NULL == buf) || (len <= 0))
    {
        TRACE(2, "invalid buf parms for ima_snmp_get_server_stat \n");
        return ISMRC_NullPointer;
    }
    if ((statType <= imaSnmpServer_NONE) || (statType >= imaSnmpServer_Col_MAX))
    {
        TRACE(2,"invalid server statType\n");
        return ISMRC_Error;
    }
    if (serverStats == NULL)
    {
        serverStats = ima_snmp_server_stats_init();
        if (NULL == serverStats)
        {
            TRACE(2, "failed to allocate server stat cache \n");
            return rc;
        }
    }
    gettimeofday(&new_time,NULL);
    if ((new_time.tv_sec - serverStats->time_serverStats) > agentRefreshCycle)
    {
        // stat is out-of-date, get new one
        rc = ima_snmp_update_server_stats(statType);
        if (rc != ISMRC_OK)
        {
            TRACE(2, "failed to get server stats from imaserver \n");
            return rc;
        }
    }

    switch(server_column_desc[statType].colType)
    {
    case imaSnmpCol_Integer:
        sprintf(buf,"%ld", serverStats->serverItem[statType].val);
    	    rc = ISMRC_OK;
    	    break;
    case imaSnmpCol_String:
    	    statString = serverStats->serverItem[statType].ptr;
    	    if ((statString) && (strlen(statString) >= 0))
    	    {
    		    snprintf(buf,len,statString);
    	        rc = ISMRC_OK;
    	    }
    	    break;
    	case imaSnmpCol_None:
    	default:
    	    break;
    }
    return rc;
}

int ima_snmp_get_server_state_from_shell()
{
    int ret;
    
    ret = WEXITSTATUS(system(IMA_STATUS_CHECK));
    if (( ret < 0 ) || (ret > 3) )  // unexpected returen value from script.
       ret = -1;
    return ret;

}

