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
 * @brief Connection Stat interface for MessageSight
 *
 * This provides the interface to access Connection stats of MessageSight.
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
#include "imaSnmpConnectionStat.h"
#include "imaSnmpJson.h"

#define _SNMP_THREADED_

extern  int agentRefreshCycle;

#ifndef _SNMP_THREADED_
static int PROC_TYPE=0;  /* use ism admin port */
static char* MONITOR_PROTOCOL="monitoring.ism.ibm.com";
#endif

static char* USER="snmp";


static int TableIndex=1;

static time_t  time_connStats = 0;

/* sample message
[{"Name":"SNMPAgent","Protocol":"mqtt","ClientAddr":"127.0.0.1","UserId":"",
    "Endpoint":"DemoMqttEndpoint","Port":1883,"ConnectTime":1406878643454542000,
    "Duration":296915899383000,"ReadBytes":24802,"ReadMsg":0,"WriteBytes":64141327,
    "WriteMsg":148418},]
*/

static const ima_snmp_col_desc_t connection_column_desc[] = {
   {NULL,imaSnmpCol_None, 0},
   {"ColIndex",imaSnmpCol_String,COLUMN_IBMIMACONNECTION_TABLE_COL_INDEX_SIZE},
   {"Name",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_NAME_SIZE},
   {"Protocol",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_PROTOCOL_SIZE},
   {"ClientAddr",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_CLIENTADDR_SIZE},
   {"UserId",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_USERID_SIZE},
   {"Endpoint",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_ENDPOINT_SIZE},
   {"Port",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_PORT_SIZE},
   {"ConnectTime",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_CONNECTTIME_SIZE},
   {"Duration",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_DURATION_SIZE},
   {"ReadBytes",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_READBYTES_SIZE},
   {"ReadMsg",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_READMSG_SIZE},
   {"WriteBytes",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_WRITEBYTES_SIZE},
   {"WriteMsg",imaSnmpCol_String, COLUMN_IBMIMACONNECTION_WRITEMSG_SIZE}
};

static ima_snmp_connection_t *connection_table_head=NULL;
static ima_snmp_connection_t *connection_table_tail=NULL;

#define CONNECTION_CMD_STRING "{ \"Action\":\"Connection\",\"User\":\"%s\",\"Locale\":\"en_US\",\"StatType\":\"NewestConnection\" }"
/*
 * Brief: construct the cmd of getting connection statistics
 * Para:
 * cmd: a buffer used to cache the constructed cmd.
 * len: buffersize
 * */
static int get_connection_stat_cmd(char* cmd, int cmd_len)
{
    if ((NULL == cmd) || (cmd_len < 200))
    {
        TRACE(2,"invalid cmd buffer for connection stat command creation \n");
        return ISMRC_Error;
    }

    snprintf(cmd, cmd_len, CONNECTION_CMD_STRING,USER);
    return ISMRC_OK;
}


/* free an connection table entry*/
static inline void ima_snmp_connection_free_entry(ima_snmp_connection_t *pEntry)
{
    int i;
    if (NULL == pEntry) return;
    for (i = imaSnmpConnection_Col_MIN; i < imaSnmpConnection_Col_MAX; i++)
    {
        if ((connection_column_desc[i].colType == imaSnmpCol_String) &&
            (pEntry->connectionItem[i].ptr != NULL))
            ism_common_free(ism_memory_snmp_misc,pEntry->connectionItem[i].ptr);
    }
    ism_common_free(ism_memory_snmp_misc,pEntry);
}

/*
 * Brief :  free the resource of global connection table
 * */
int ima_snmp_free_connection_table()
{
    int rc=ISMRC_OK;
    if(connection_table_head)
    {
        ima_snmp_connection_t * nextNode=connection_table_head;
        ima_snmp_connection_t * preNode=NULL;

        while(nextNode != NULL)
        {
            preNode = nextNode;
            nextNode=preNode->next;
            //ism_common_free(ism_memory_snmp_misc,preNode);
            ima_snmp_connection_free_entry(preNode);
        }
        connection_table_head = connection_table_tail = NULL;
    }
    return rc;
}

/* Create a new entry for connection table */
static inline ima_snmp_connection_t *ima_snmp_connection_create_entry()
{
    int i;
    ima_snmp_connection_t *pEntry = NULL;
    
    pEntry = (ima_snmp_connection_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,104),sizeof(ima_snmp_connection_t));
    if (NULL == pEntry)
    {
        TRACE(2,"failed to allocate resources for new connection table entry.");
        return NULL;
    }
    memset(pEntry,0,sizeof(ima_snmp_connection_t));
    for (i = imaSnmpConnection_Col_MIN; i < imaSnmpConnection_Col_MAX; i++)
    {
        if (connection_column_desc[i].colType == imaSnmpCol_String)
        { // allocate string buffer for the  item;
            char *tempPtr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,105),connection_column_desc[i].colSize * sizeof(char));
            if (tempPtr == NULL)
            {
                TRACE(2,"failed to allocated memory for connection item %s \n",
                        connection_column_desc[i].colName);
                break;
            }
            pEntry->connectionItem[i].ptr = tempPtr;
        }
    }

    if (i < imaSnmpConnection_Col_MAX)
    {
        TRACE(2,"free entry due to malloc issue");
        ima_snmp_connection_free_entry(pEntry);
        return NULL;
    }
    return pEntry;
}

static  ima_snmp_connection_t *ima_snmp_connection_create_default_entry()
{
    int i;
    ima_snmp_connection_t *pEntry = NULL;
    
    pEntry = (ima_snmp_connection_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,106),sizeof(ima_snmp_connection_t));
    if (NULL == pEntry)
    {
        TRACE(2,"failed to allocate resources for default connection table entry.");
        return NULL;
    }
    memset(pEntry,0,sizeof(ima_snmp_connection_t));
    for (i = imaSnmpConnection_Col_MIN; i < imaSnmpConnection_Col_MAX; i++)
    {
        if (connection_column_desc[i].colType == imaSnmpCol_Integer)
        { // set default value for the item;
            pEntry->connectionItem[i].val = IMA_SNMP_CONN_INT_DEFAULT;
        }
    }
    return pEntry;
}
 
static inline void ima_snmp_connection_insert_entry(ima_snmp_connection_t *pEntry)
{
    if (NULL == pEntry) return;
    
    // insert new Entry to global connection table.
    if (NULL == connection_table_tail)
        connection_table_tail = connection_table_head = pEntry;
    else {
        connection_table_tail->next = pEntry;
        connection_table_tail = pEntry;
    }
}

int ima_snmp_connection_add_entry(ism_json_parse_t pobj, int entnum)
{
    ima_snmp_connection_t *connectionEntry = NULL;
    int i;

    connectionEntry = ima_snmp_connection_create_entry();
    if (NULL == connectionEntry)
    {
        TRACE(2,"failed to create an entry for connection row ");
        return -1;
    }
    //TRACE(2,"connection_add_entry %d \n",entnum);
    for (i = imaSnmpConnection_Col_MIN; i < imaSnmpConnection_Col_MAX; i++)
    {
        char* tempStr=NULL;
        int   tempVal = 0;
        int buflen=0;
    if(!strcmp(connection_column_desc[i].colName, "ColIndex"))
    {
        memset(connectionEntry->connectionItem[i].ptr,0,connection_column_desc[i].colSize * sizeof(char));
        sprintf(connectionEntry->connectionItem[i].ptr,"%d",TableIndex);
        TableIndex++;
        continue;
    }

        //TRACE(2,"connection_add_entry loop i = %d, type = %d  \n",i, connection_column_desc[i].colType);
        if (connection_column_desc[i].colType == imaSnmpCol_String)
        {
            tempStr=(char*)ima_snmp_jsonArray_getString(&pobj,
                           connection_column_desc[i].colName, &entnum);
            if (NULL == tempStr)
            {
                if(entnum<0)
                {
                    TRACE(2,"cannot find %s in connection message. Break from the Loop.\n",
                            connection_column_desc[i].colName);
                    break;
                }
                else
                {// There exists the case that the tempStr is "".
                    tempStr=" ";
                }
            }
            TRACE(9,"%s loop i = %d, entnum = %d, val = %s \n", __FUNCTION__, i, entnum, tempStr);

            buflen = strlen(tempStr);
            if (buflen > connection_column_desc[i].colSize - 1 ) {
                buflen = connection_column_desc[i].colSize - 1;
            }
            memcpy(connectionEntry->connectionItem[i].ptr, tempStr, buflen );
            connectionEntry->connectionItem[i].ptr[buflen] = '\0';
        }
        if (connection_column_desc[i].colType == imaSnmpCol_Integer)
        {// We will deal with all the Json item with String. So, this block code will not run.
          tempVal=(int)ima_snmp_jsonArray_getInt(&pobj,
                          connection_column_desc[i].colName,65535,&entnum);

          //TRACE(2,"****************%s value=%d  entnum=%d \n",connection_column_desc[i].colName,tempVal,entnum);
           if (65535 == tempVal)
           {
               TRACE(2,"cannot find %s in connection message \n.  entnum=%d",
                       connection_column_desc[i].colName,entnum);
               break;
           }
           connectionEntry->connectionItem[i].val=tempVal;
        }
    
    }

    if (entnum > 0) 
        ima_snmp_connection_insert_entry(connectionEntry);
    else  ima_snmp_connection_free_entry(connectionEntry);

    return entnum;
}

/*
 * Brief: get the connection statistics info from messagesight.
 * */
int ima_snmp_get_connection_stat()
{
    int  rc = ISMRC_Error;
    char connStatCmd[256];
    struct timeval new_time;

    gettimeofday(&new_time,NULL);
    if ((new_time.tv_sec - time_connStats) <=  agentRefreshCycle)
    {
        TRACE(7, "Info: get connection request is still within refresh cycle, use existed value. \n");
        return ISMRC_OK;
    }
    //if connection table is not NULL, free it firstly
    if (connection_table_head) ima_snmp_free_connection_table();

    rc = get_connection_stat_cmd(connStatCmd, 256);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to create cmd for connection stat\n");
        return rc;
    }

    rc = ISMRC_Error;

#ifndef _SNMP_THREADED_
    char *result = NULL;
    result = ismcli_ISMClient(USER, MONITOR_PROTOCOL, connStatCmd, PROC_TYPE);

    /* process result */
    if ( result )
    {
        /* Check if returned result is a json string */
            int buflen = 0;
            ism_json_parse_t pobj = {0};
            ism_json_entry_t ents[MAX_JSON_ENTRIES];

            char *tmpbuf;
            buflen = strlen(result);
            tmpbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,107),buflen + 1);
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
                TRACE(5,"Info: ima_snmp_get_connection_stat return  RC=%s\n", rcstr);
                ima_snmp_connection_insert_entry( ima_snmp_connection_create_default_entry());
                ismCLI_FREE(result);
                ism_common_free(ism_memory_snmp_misc,tmpbuf);
                time_connStats = new_time.tv_sec;
                return ISMRC_OK;
            }  
            if ( errstr ) {
                TRACE(2,"ima_snmp_get_connection_stat return ErrorString: %s : RC=%s\n",
                      errstr,rcstr);
                ismCLI_FREE(result);
                ism_common_free(ism_memory_snmp_misc,tmpbuf);
                return rc;
            }  // if ( errstr)

            /*Free the result.*/
            ismCLI_FREE(result);

            //testJsonStr(&pobj);

            // Parse json array to connection_table
            TableIndex=1;//index for table column
            int entnum=0;
            while(entnum<pobj.ent_count-1)
            {
                entnum = ima_snmp_connection_add_entry(pobj,entnum);
                //TRACE(2,"------------->entnum=%d;  pobj.ent_count-1=%d \n",entnum,pobj.ent_count-1);
                if (entnum < 0) break;
            } //end of while
            rc = ISMRC_OK;

            time_connStats = new_time.tv_sec;
            ism_common_free(ism_memory_snmp_misc,tmpbuf);
    }
#else
    int connBufLen = 1024;
    char connBuf[connBufLen];
    concat_alloc_t output_buffer;

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.buf = connBuf;
    output_buffer.len = connBufLen;

    ism_process_monitoring_action(NULL, connStatCmd, strlen(connStatCmd), &output_buffer, &rc);

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
        /* check is there is an error string */
        char *errstr = (char *)ism_json_getString(&pobj, "ErrorString");
        char *rcstr = (char *)ism_json_getString(&pobj, "RC");
        if ( rcstr )
        	rc = atoi(rcstr);
        if (rc == ISMRC_NotFound) // No Items found, return with default values.
        {
        	TRACE(5,"Info: ima_snmp_get_connection_stat return  RC=%s\n", rcstr);
            ima_snmp_connection_insert_entry( ima_snmp_connection_create_default_entry());
            time_connStats = new_time.tv_sec;
            return ISMRC_OK;
        }
        if ( errstr )
        {
        	TRACE(2,"ima_snmp_get_connection_stat return ErrorString: %s : RC=%s\n",
                  errstr,rcstr);
             return rc;
        }

        // Parse json array to connection_table
        TableIndex=1;//index for table column
        int entnum=0;
        while(entnum<pobj.ent_count-1)
        {
            entnum = ima_snmp_connection_add_entry(pobj,entnum);
            //TRACE(2,"------------->entnum=%d;  pobj.ent_count-1=%d \n",entnum,pobj.ent_count-1);
            if (entnum < 0) break;
        } //end of while
        rc = ISMRC_OK;

        time_connStats = new_time.tv_sec;

    }

	if (output_buffer.inheap)
		ism_common_free(ism_memory_snmp_misc,output_buffer.buf);
#endif
    return rc;
}

ima_snmp_connection_t *ima_snmp_connection_get_table_head()
{
    return connection_table_head;
}

