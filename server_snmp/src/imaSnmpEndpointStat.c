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
 * @brief Endpoint Stat interface for MessageSight
 *
 * This provides the interface to access Endpoint stats of MessageSight.
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
#include "imaSnmpEndpointStat.h"
#include "imaSnmpJson.h"

#define _SNMP_THREADED_

extern  int agentRefreshCycle;

#ifndef _SNMP_THREADED_
static int PROC_TYPE=0;  /* use ism admin port */
static char* MONITOR_PROTOCOL="monitoring.ism.ibm.com";
#endif

static char* USER="snmp";
static int TableIndex=1;

static const ima_snmp_col_desc_t endpoint_column_desc[] = {
   {NULL,imaSnmpCol_None, 0},  // imaSnmpEndpoint_NONE
   {"ColIndex",imaSnmpCol_String,COLUMN_IBMIMAENDPOINT_TABLE_COL_INDEX_SIZE},
   {"Name",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTNAME_SIZE},  // imaSnmpEndpoint_NAME
   {"IPAddr",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTIPADDR_SIZE},//imaSnmpEndpoint_IPADDR
   {"Enabled",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTENABLED_SIZE},//imaSnmpEndpoint_ENABLED
   {"Total",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTTOTAL_SIZE},  // imaSnmpEndpoint_TOTAL
   {"Active",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTACTIVE_SIZE},  // imaSnmpEndpoint_ACTIVE
   {"Messages",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTMESSAGES_SIZE},  // imaSnmpEndpoint_MESSAGES
   {"Bytes",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTBYTES_SIZE},  // imaSnmpEndpoint_BYTES
   {"LastErrorCode",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTLASTERRORCODE_SIZE},  // imaSnmpEndpoint_LAST_ERROR_CODE
   {"ConfigTime",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTCONFIGTIME_SIZE},  // imaSnmpEndpoint_CONFIG_TIME
   {"ResetTime",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTRESETTIME_SIZE},  // imaSnmpEndpoint_RESET_TIME
   {"BadConnections",imaSnmpCol_String, COLUMN_IBMIMAENDPOINTBADCONNECTIONS_SIZE},  // imaSnmpEndpoint_BAD_CONNECTIONS

};

static ima_snmp_endpoint_t *endpoint_table_head=NULL;
static ima_snmp_endpoint_t *endpoint_table_tail=NULL;

static time_t  time_endpointStats = 0;

#define ENDPOINT_CMD_STRING "{ \"Action\":\"Endpoint\",\"User\":\"%s\",\"Locale\":\"en_US\",\"SubType\":\"Current\",\"StatType\":\"ActiveConnections\",\"Duration\":\"1800\" }"


/*
 * Brief: construct the cmd of getting endpoint statistics
 * Para:
 * cmd: a buffer used to cache the constructed cmd.
 * len: buffersize
 * */
static int get_endpoint_stat_cmd(char* cmd, int cmd_len)
{
    if ((NULL == cmd) || (cmd_len < 200))
    {
        TRACE(2,"invalid cmd buffer for endpoint stat command creation \n");
        return ISMRC_Error;
    }

    snprintf(cmd, cmd_len, ENDPOINT_CMD_STRING,USER);
    return ISMRC_OK;
}

/*Sample:
[
{ "Name":"DemoEndpoint","IPAddr":"ALL","Enabled":"1","Total":"0","Active":"0","Messages":"0","Bytes":"0","LastErrorCode":"0","ConfigTime":"1405015188035380000","ResetTime":"0","BadConnections":"0" },
{ "Name":"DemoMqttEndpoint","IPAddr":"ALL","Enabled":"1","Total":"0","Active":"0","Messages":"0","Bytes":"0","LastErrorCode":"0","ConfigTime":"1405015188035428000","ResetTime":"0","BadConnections":"0" }
]
*/
/* free an endpoint table entry*/
static inline void ima_snmp_endpoint_free_entry(ima_snmp_endpoint_t *endpointEntry)
{
    int i;
    if (NULL == endpointEntry) return;
    for (i = imaSnmpEndpoint_Col_MIN; i < imaSnmpEndpoint_Col_MAX; i++)
    {
        if ((endpoint_column_desc[i].colType == imaSnmpCol_String) && 
            (endpointEntry->endpointItem[i].ptr != NULL)) 
            ism_common_free(ism_memory_snmp_misc,endpointEntry->endpointItem[i].ptr);
    }
    ism_common_free(ism_memory_snmp_misc,endpointEntry);
}

/*
 * Brief :  free the resource of global endpoint table 
 * */
int ima_snmp_free_endpoint_table()
{
    int rc=ISMRC_OK;
    if(endpoint_table_head)
    {
        ima_snmp_endpoint_t * nextNode=endpoint_table_head;
        ima_snmp_endpoint_t * preNode=NULL;

        while(nextNode != NULL)
        {
            preNode = nextNode;
            nextNode=preNode->next;
            ima_snmp_endpoint_free_entry(preNode);
        }
        endpoint_table_head = endpoint_table_tail = NULL;
    }
    return rc;
}

/* Create a new entry for endpoint table */
static inline ima_snmp_endpoint_t *ima_snmp_endpoint_create_entry()
{
    int i;
    ima_snmp_endpoint_t *endpointEntry = NULL;
    
    endpointEntry = (ima_snmp_endpoint_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,82),sizeof(ima_snmp_endpoint_t));
    if (NULL == endpointEntry)
    {
        TRACE(2,"failed to allocate resources for new endpoint table entry.");
        return NULL;
    }
    memset(endpointEntry,0,sizeof(ima_snmp_endpoint_t));
    for (i = imaSnmpEndpoint_Col_MIN; i < imaSnmpEndpoint_Col_MAX; i++)
    {
        if (endpoint_column_desc[i].colType == imaSnmpCol_String)
        { // allocate string buffer for the item;
            char *tempPtr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,83),endpoint_column_desc[i].colSize * sizeof(char));
            if (tempPtr == NULL)
            {
                TRACE(2,"failed to allocated memory for endpoint item %s \n",
                    endpoint_column_desc[i].colName);
                break;
            }
            endpointEntry->endpointItem[i].ptr = tempPtr;
        }
    }

    if (i < imaSnmpEndpoint_Col_MAX)
    {
        TRACE(2,"free endpoint entry due to malloc issue");
        ima_snmp_endpoint_free_entry(endpointEntry);
        return NULL;
    }
    return endpointEntry;
}

static  ima_snmp_endpoint_t *ima_snmp_endpoint_create_default_entry()
{
    int i;
    ima_snmp_endpoint_t *pEntry = NULL;

    pEntry = (ima_snmp_endpoint_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,84),sizeof(ima_snmp_endpoint_t));
    if (NULL == pEntry)
    {
        TRACE(2,"failed to allocate resources for default endpoint table entry.");
        return NULL;
    }
    memset(pEntry,0,sizeof(ima_snmp_endpoint_t));
    for (i = imaSnmpEndpoint_Col_MIN; i < imaSnmpEndpoint_Col_MAX; i++)
    {
        if (endpoint_column_desc[i].colType == imaSnmpCol_Integer)
        { // set default value for the item;
            pEntry->endpointItem[i].val = IMA_SNMP_ENDPOINT_INT_DEFAULT;
        }
    }
    return pEntry;
}

static inline void ima_snmp_endpoint_insert_entry(ima_snmp_endpoint_t *endpointEntry)
{
    if (NULL == endpointEntry) return;
    
    // insert new Entry to global endpoint table.
    if (NULL == endpoint_table_tail)
        endpoint_table_tail = endpoint_table_head = endpointEntry;
    else {
        endpoint_table_tail->next = endpointEntry;
        endpoint_table_tail = endpointEntry;
    }
}

int ima_snmp_endpoint_add_entry(ism_json_parse_t pobj, int entnum)
{
    ima_snmp_endpoint_t *endpointEntry = NULL;
    int i;

    endpointEntry = ima_snmp_endpoint_create_entry();
    if (NULL == endpointEntry)
    {
        TRACE(2,"failed to create an entry for endpoint row ");
        return -1;
    }
    //TRACE(2,"endpoint_add_entry %d \n",entnum);
    for (i = imaSnmpEndpoint_Col_MIN; i < imaSnmpEndpoint_Col_MAX; i++)
    {
        char* tempStr=NULL;
        //int   tempVal = 0;
        int buflen=0;

        if(!strcmp(endpoint_column_desc[i].colName, "ColIndex"))
		{
			memset(endpointEntry->endpointItem[i].ptr,0,endpoint_column_desc[i].colSize * sizeof(char));
			sprintf(endpointEntry->endpointItem[i].ptr,"%d",TableIndex);
			TableIndex++;
			continue;
		}

        //TRACE(2,"endpoint_add_entry loop i = %d, type = %d  \n",i, endpoint_column_desc[i].colType);
        if (endpoint_column_desc[i].colType == imaSnmpCol_String)
        {
            tempStr=(char*)ima_snmp_jsonArray_getString(&pobj,
                           endpoint_column_desc[i].colName, &entnum);
            if (NULL == tempStr)
            {
                TRACE(2,"cannot find %s in endpoint message \n",
                        endpoint_column_desc[i].colName);
                break;
            }
            TRACE(9,"%s loop i = %d, entnum = %d, val = %p \n", __FUNCTION__, i, entnum, tempStr);

            buflen = strlen(tempStr);
            if (buflen > endpoint_column_desc[i].colSize - 1 ) {
                buflen = endpoint_column_desc[i].colSize - 1;
            }
            memcpy(endpointEntry->endpointItem[i].ptr, tempStr, buflen);
            endpointEntry->endpointItem[i].ptr[buflen] = '\0';
        }
    
    }

    if (entnum > 0) 
        ima_snmp_endpoint_insert_entry(endpointEntry);
    else  ism_common_free(ism_memory_snmp_misc,endpointEntry);

    return entnum;
}

/*
 * Brief: get the endpoint statistics info from messagesight.
 * */
int ima_snmp_get_endpoint_stat()
{
    int  rc = ISMRC_Error;
    char endpointStatCmd[256];
    struct timeval new_time;

    gettimeofday(&new_time,NULL);
    if ((new_time.tv_sec - time_endpointStats) <=  agentRefreshCycle)
    {
        TRACE(7, "Info: get endpoint request is still within refresh cycle, use existed value. \n");
        return ISMRC_OK;
    }

    //if endpoint table is not NULL, free it firstly
    if (endpoint_table_head) ima_snmp_free_endpoint_table();

    rc = get_endpoint_stat_cmd(endpointStatCmd, 256);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to create cmd for endpoint stat\n");
        return rc;
    }

    rc = ISMRC_Error;

#ifndef _SNMP_THREADED_
    char *result = NULL;
    result = ismcli_ISMClient(USER, MONITOR_PROTOCOL, endpointStatCmd, PROC_TYPE);

    /* process result */
    if ( result )
    {
        /* Check if returned result is a json string */
            int buflen = 0;
            ism_json_parse_t pobj = {0};
            ism_json_entry_t ents[MAX_JSON_ENTRIES];

            char *tmpbuf;
            buflen = strlen(result);
            tmpbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,86),buflen + 1);
            memcpy(tmpbuf, result, buflen);
            tmpbuf[buflen] = 0;
            pobj.ent_alloc = MAX_JSON_ENTRIES;
            pobj.ent = ents;
            pobj.source = (char *) tmpbuf;
            pobj.src_len = buflen;
            ism_json_parse(&pobj);
            if (pobj.rc) {
                TRACE(2,"result is not json string: %s\n", result);
                ismCLI_FREE(result);
                ism_common_free(ism_memory_snmp_misc,tmpbuf);
                return pobj.rc;
            }

            /* check is there is an error string */
            char *errstr = (char *)ism_json_getString(&pobj, "ErrorString");
            char *rcstr = (char *)ism_json_getString(&pobj, "RC");
            if ( rcstr )
                rc = atoi(rcstr);
            if (rc == ISMRC_NotFound) // No Items found, return with default values.
            {
                TRACE(5,"Info: ima_snmp_get_endpoint_stat return  RC=%s\n", rcstr);
                ima_snmp_endpoint_insert_entry( ima_snmp_endpoint_create_default_entry());
                ismCLI_FREE(result);
                ism_common_free(ism_memory_snmp_misc,tmpbuf);
                time_endpointStats = new_time.tv_sec;
                return ISMRC_OK;
            }
            if ( errstr ) {
                TRACE(2,"ima_snmp_get_endpoint_stat return ErrorString: %s : RC=%s\n",
                      errstr,rcstr);
                ismCLI_FREE(result);
                ism_common_free(ism_memory_snmp_misc,tmpbuf);
                return rc;
            }  // if ( errstr)

            /*Free the result.*/
            ismCLI_FREE(result);

            //testJsonStr(&pobj);

            // Parse json array to endpoint_table
            TableIndex=1; //index for table colume
            int entnum=0;
            while(entnum<pobj.ent_count-1)
            {
                entnum = ima_snmp_endpoint_add_entry(pobj,entnum);
                //TRACE(2,"entNum %d , %d\n",entnum, pobj.ent_count);
                if (entnum < 0) break;
            } //end of while
            rc = ISMRC_OK;
            time_endpointStats = new_time.tv_sec;

            ism_common_free(ism_memory_snmp_misc,tmpbuf);
    }
#else
    int endBufLen = 1024;
    char endBuf[endBufLen];
    concat_alloc_t output_buffer;

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.buf = endBuf;
    output_buffer.len = endBufLen;

    ism_process_monitoring_action(NULL, endpointStatCmd, strlen(endpointStatCmd), &output_buffer, &rc);

    if (rc == ISMRC_OK)
    {
        ism_json_parse_t pobj = {0};
        ism_json_entry_t ents[MAX_JSON_ENTRIES];
        pobj.ent_alloc = MAX_JSON_ENTRIES;
        pobj.ent = ents;
        pobj.source = output_buffer.buf;
        pobj.src_len = output_buffer.len;
        ism_json_parse(&pobj);

        if (pobj.rc)
        {
        	TRACE(2,"result is not json string: %s\n", output_buffer.buf);
            return pobj.rc;
        }
        /* check if there is an error string */
        char *errstr = (char *)ism_json_getString(&pobj, "ErrorString");
        char *rcstr = (char *)ism_json_getString(&pobj, "RC");
        if ( rcstr )
        	rc = atoi(rcstr);
        if (rc == ISMRC_NotFound) // No Items found, return with default values.
        {
        	TRACE(5,"Info: ima_snmp_get_endpoint_stat return  RC=%s\n", rcstr);
            ima_snmp_endpoint_insert_entry( ima_snmp_endpoint_create_default_entry());
            time_endpointStats = new_time.tv_sec;
            return ISMRC_OK;
        }
        if ( errstr )
        {
        	TRACE(2,"ima_snmp_get_endpoint_stat return ErrorString: %s : RC=%s\n",
                  errstr,rcstr);
             return rc;
        }

        // Parse json array to endpoint_table
        TableIndex=1;//index for table column
        int entnum=0;
        while(entnum<pobj.ent_count-1)
        {
            entnum = ima_snmp_endpoint_add_entry(pobj,entnum);
            //TRACE(2,"------------->entnum=%d;  pobj.ent_count-1=%d \n",entnum,pobj.ent_count-1);
            if (entnum < 0) break;
        } //end of while
        rc = ISMRC_OK;

        time_endpointStats = new_time.tv_sec;

    }

	if (output_buffer.inheap)
		ism_common_free(ism_memory_snmp_misc,output_buffer.buf);
#endif
    return rc;
}

ima_snmp_endpoint_t *ima_snmp_endpoint_get_table_head()
{
    return endpoint_table_head;
}


