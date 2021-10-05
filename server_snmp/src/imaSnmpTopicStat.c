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
 * @brief Topic Stat interface for MessageSight
 *
 * This provides the interface to access Topic stats of MessageSight.
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

#include "imaSnmpTopicStat.h"
#include "imaSnmpJson.h"

#define _SNMP_THREADED_

extern  int agentRefreshCycle;

#ifndef _SNMP_THREADED_
static int PROC_TYPE=0;  /* use ism admin port */
static char* MONITOR_PROTOCOL="monitoring.ism.ibm.com";
#endif

static char* USER="snmp";

static int TableIndex=1;

static time_t  time_topicStats = 0;

/* sample message
[{"TopicString":"/Test/guotao/#","Subscriptions":0,"ResetTime":1408555705193394000,
"PublishedMsgs":0,"RejectedMsgs":0,"FailedPublishes":0}
*/

static const ima_snmp_col_desc_t topic_column_desc[] = {
   {NULL,imaSnmpCol_None, 0},
   {"ColIndex",imaSnmpCol_String,COLUMN_IBMIMATOPIC_TABLE_COL_INDEX_SIZE},
   {"TopicString",imaSnmpCol_String, COLUMN_IBMIMATOPIC_TOPICSTRING_SIZE},
   {"Subscriptions",imaSnmpCol_String, COLUMN_IBMIMATOPIC_SUBSCRIPTIONS_SIZE},
   {"ResetTime",imaSnmpCol_String, COLUMN_IBMIMATOPIC_RESETTIME_SIZE},
   {"PublishedMsgs",imaSnmpCol_String, COLUMN_IBMIMATOPIC_PUBLISHEDMSGS_SIZE},
   {"RejectedMsgs",imaSnmpCol_String, COLUMN_IBMIMATOPIC_REJECTEDMSGS_SIZE},
   {"FailedPublishes",imaSnmpCol_String, COLUMN_IBMIMATOPIC_FAILEDPUBLISHES_SIZE}
};

static ima_snmp_topic_t *topic_table_head=NULL;
static ima_snmp_topic_t *topic_table_tail=NULL;

#define TOPIC_CMD_STRING "{ \"Action\":\"Topic\",\"User\":\"%s\",\"Locale\":\"en_US\",\"TopicString\":\"*\",\"StatType\":\"PublishedMsgsHighest\",\"ResultCount\":\"25\" }"
/*
 * Brief: construct the cmd of getting topic statistics
 * Para:
 * cmd: a buffer used to cache the constructed cmd.
 * len: buffersize
 * */
static int get_topic_stat_cmd(char* cmd, int cmd_len)
{
    if ((NULL == cmd) || (cmd_len < 200))
    {
        TRACE(2,"invalid cmd buffer for topic stat command creation \n");
        return ISMRC_Error;
    }

    snprintf(cmd, cmd_len, TOPIC_CMD_STRING,USER);
    return ISMRC_OK;
}


/* free an topic table entry*/
static inline void ima_snmp_topic_free_entry(ima_snmp_topic_t *pEntry)
{
    int i;
    if (NULL == pEntry) return;
    for (i = imaSnmpTopic_Col_MIN; i < imaSnmpTopic_Col_MAX; i++)
    {
        if ((topic_column_desc[i].colType == imaSnmpCol_String) &&
            (pEntry->topicItem[i].ptr != NULL))
            ism_common_free(ism_memory_snmp_misc,pEntry->topicItem[i].ptr);
    }
    ism_common_free(ism_memory_snmp_misc,pEntry);
}

/*
 * Brief :  free the resource of global topic table
 * */
int ima_snmp_free_topic_table()
{
    int rc=ISMRC_OK;
    if(topic_table_head)
    {
    	ima_snmp_topic_t * nextNode=topic_table_head;
    	ima_snmp_topic_t * preNode=NULL;

        while(nextNode != NULL)
        {
            preNode = nextNode;
            nextNode=preNode->next;
            //ism_common_free(ism_memory_snmp_misc,preNode);
            ima_snmp_topic_free_entry(preNode);
        }
        topic_table_head = topic_table_tail = NULL;
    }
    return rc;
}

/* Create a new entry for topic table */
static inline ima_snmp_topic_t *ima_snmp_topic_create_entry()
{
    int i;
    ima_snmp_topic_t *pEntry = NULL;
    
    pEntry = (ima_snmp_topic_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,23),sizeof(ima_snmp_topic_t));
    if (NULL == pEntry)
    {
        TRACE(2,"failed to allocate resources for new topic table entry.");
        return NULL;
    }
    memset(pEntry,0,sizeof(ima_snmp_topic_t));
    for (i = imaSnmpTopic_Col_MIN; i < imaSnmpTopic_Col_MAX; i++)
    {
        if (topic_column_desc[i].colType == imaSnmpCol_String)
        { // allocate string buffer for the  item;
            char *tempPtr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,24),topic_column_desc[i].colSize * sizeof(char));
            if (tempPtr == NULL)
            {
                TRACE(2,"failed to allocated memory for topic item %s \n",
                		topic_column_desc[i].colName);
                break;
            }
            pEntry->topicItem[i].ptr = tempPtr;
        }
    }

    if (i < imaSnmpTopic_Col_MAX)
    {
        TRACE(2,"free entry due to malloc issue");
        ima_snmp_topic_free_entry(pEntry);
        return NULL;
    }
    return pEntry;
}

static  ima_snmp_topic_t *ima_snmp_topic_create_default_entry()
{
    int i;
    ima_snmp_topic_t *pEntry = NULL;

    pEntry = (ima_snmp_topic_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,25),sizeof(ima_snmp_topic_t));
    if (NULL == pEntry)
    {
        TRACE(2,"failed to allocate resources for default topic table entry.");
        return NULL;
    }
    memset(pEntry,0,sizeof(ima_snmp_topic_t));
    for (i = imaSnmpTopic_Col_MIN; i < imaSnmpTopic_Col_MAX; i++)
    {
        if (topic_column_desc[i].colType == imaSnmpCol_Integer)
        { // set default value for the item;
            pEntry->topicItem[i].val = IMA_SNMP_TOPIC_INT_DEFAULT;
        }
    }
    return pEntry;
}

static inline void ima_snmp_topic_insert_entry(ima_snmp_topic_t *pEntry)
{
    if (NULL == pEntry) return;
    
    // insert new Entry to global topic table.
    if (NULL == topic_table_tail)
    	topic_table_tail = topic_table_head = pEntry;
    else {
    	topic_table_tail->next = pEntry;
    	topic_table_tail = pEntry;
    }
}

int ima_snmp_topic_add_entry(ism_json_parse_t pobj, int entnum)
{
    ima_snmp_topic_t *topicEntry = NULL;
    int i;

    topicEntry = ima_snmp_topic_create_entry();
    if (NULL == topicEntry)
    {
        TRACE(2,"failed to create an entry for topic row ");
        return -1;
    }
    //TRACE(2,"topic_add_entry %d \n",entnum);
    for (i = imaSnmpTopic_Col_MIN; i < imaSnmpTopic_Col_MAX; i++)
    {
        char* tempStr=NULL;
        int   tempVal = 0;
        int buflen=0;
	if(!strcmp(topic_column_desc[i].colName, "ColIndex"))
	{
		memset(topicEntry->topicItem[i].ptr,0,topic_column_desc[i].colSize * sizeof(char));
		sprintf(topicEntry->topicItem[i].ptr,"%d",TableIndex);
		TableIndex++;
		continue;
	}

        //TRACE(2,"topic_add_entry loop i = %d, type = %d  \n",i, topic_column_desc[i].colType);
        if (topic_column_desc[i].colType == imaSnmpCol_String)
        {
            tempStr=(char*)ima_snmp_jsonArray_getString(&pobj,
                           topic_column_desc[i].colName, &entnum);
            if (NULL == tempStr)
            {
            	if(entnum<0)
            	{
					TRACE(2,"cannot find %s in topic message. Break from the Loop.\n",
							topic_column_desc[i].colName);
					break;
            	}
            	else
            	{// There exists the case that the tempStr is "".
            		tempStr=" ";
            	}
            }
            TRACE(9,"%s loop i = %d, entnum = %d, val = %p \n", __FUNCTION__, i, entnum, tempStr);
            buflen = strlen(tempStr);
            if (buflen > topic_column_desc[i].colSize - 1 ) {
            	//Handle TopicString to its real size limit 65535
            	if (!strcmp(topic_column_desc[i].colName, "TopicString") )  {
            		//free the pre-allocated memory and malloc a big one
            		char *ptr = topicEntry->topicItem[i].ptr;
            		topicEntry->topicItem[i].ptr = NULL;
            		topicEntry->topicItem[i].ptr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,26),buflen + 1);
            		if (ptr)
            			ism_common_free(ism_memory_snmp_misc,ptr);
            	} else {
            		//just a safety check now. No entry should be here
            		buflen = topic_column_desc[i].colSize - 1;
            	    TRACE(9,"%s: val %s is longer than defined length: %d\n", __FUNCTION__, tempStr, topic_column_desc[i].colSize);
            	}
            }
            memcpy(topicEntry->topicItem[i].ptr, tempStr, buflen);
            topicEntry->topicItem[i].ptr[buflen] = '\0';
        }
        if (topic_column_desc[i].colType == imaSnmpCol_Integer)
	    {// We will deal with all the Json item with String. So, this block code will not run.
		  tempVal=(int)ima_snmp_jsonArray_getInt(&pobj,
						  topic_column_desc[i].colName,65535,&entnum);

		  //TRACE(2,"****************%s value=%d  entnum=%d \n",topic_column_desc[i].colName,tempVal,entnum);
		   if (65535 == tempVal)
		   {
			   TRACE(2,"cannot find %s in topic message \n.  entnum=%d",
					   topic_column_desc[i].colName,entnum);
			   break;
		   }
		   topicEntry->topicItem[i].val=tempVal;
	    }
    
    }

    if (entnum > 0) 
        ima_snmp_topic_insert_entry(topicEntry);
    else  ima_snmp_topic_free_entry(topicEntry);

    return entnum;
}

/*
 * Brief: get the topic statistics info from messagesight.
 * */
int ima_snmp_get_topic_stat()
{
    int  rc = ISMRC_Error;
    char topicStatCmd[256];
    struct timeval new_time;

    gettimeofday(&new_time,NULL);
    if ((new_time.tv_sec - time_topicStats) <=  agentRefreshCycle)
    {
        TRACE(7, "Info: get topic request is still within refresh cycle, use existed value. \n");
        return ISMRC_OK;
    }

    //if topic table is not NULL, free it firstly
    if (topic_table_head) ima_snmp_free_topic_table();

    rc = get_topic_stat_cmd(topicStatCmd, 256);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to create cmd for topic stat\n");
        return rc;
    }

    rc = ISMRC_Error;

#ifndef _SNMP_THREADED_
    char *result = NULL;
    result = ismcli_ISMClient(USER, MONITOR_PROTOCOL, topicStatCmd, PROC_TYPE);

    /* process result */
    if ( result )
    {
        /* Check if returned result is a json string */
            int buflen = 0;
            ism_json_parse_t pobj = {0};
            ism_json_entry_t ents[MAX_JSON_ENTRIES];

            char *tmpbuf;
            buflen = strlen(result);
            tmpbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,28),buflen + 1);
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
                TRACE(5,"Info: ima_snmp_get_topic_stat return  RC=%s\n", rcstr);
                ima_snmp_topic_insert_entry( ima_snmp_topic_create_default_entry());
                ismCLI_FREE(result);
                ism_common_free(ism_memory_snmp_misc,tmpbuf);
                time_topicStats = new_time.tv_sec;
                return ISMRC_OK;
            }
            if ( errstr ) {
                TRACE(2,"ima_snmp_get_topic_stat return ErrorString: %s : RC=%s\n",
                      errstr,rcstr);
                ismCLI_FREE(result);
                ism_common_free(ism_memory_snmp_misc,tmpbuf);
                return rc;
            }  // if ( errstr)

            /*Free the result.*/
            ismCLI_FREE(result);

            // Parse json array to topic_table
            TableIndex=1;//index for table colume
            int entnum=0;
            while(entnum<pobj.ent_count-1)
            {
                entnum = ima_snmp_topic_add_entry(pobj,entnum);
                //TRACE(2,"------------->entnum=%d;  pobj.ent_count-1=%d \n",entnum,pobj.ent_count-1);
                if (entnum < 0) break;
            } //end of while
            rc = ISMRC_OK;
            time_topicStats = new_time.tv_sec;

            ism_common_free(ism_memory_snmp_misc,tmpbuf);
    }
#else
    int topicBufLen = 1024;
    char topicBuf[topicBufLen];
    concat_alloc_t output_buffer;

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.buf = topicBuf;
    output_buffer.len = topicBufLen;

    ism_process_monitoring_action(NULL, topicStatCmd, strlen(topicStatCmd), &output_buffer, &rc);

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
        	TRACE(5,"Info: ima_snmp_get_topic_stat return  RC=%s\n", rcstr);
            ima_snmp_topic_insert_entry( ima_snmp_topic_create_default_entry());
            time_topicStats = new_time.tv_sec;
            return ISMRC_OK;
        }
        if ( errstr )
        {
        	TRACE(2,"ima_snmp_get_topic_stat return ErrorString: %s : RC=%s\n",
                  errstr,rcstr);
             return rc;
        }

        // Parse json array to endpoint_table
        TableIndex=1;//index for table column
        int entnum=0;
        while(entnum<pobj.ent_count-1)
        {
            entnum = ima_snmp_topic_add_entry(pobj,entnum);
            //TRACE(2,"------------->entnum=%d;  pobj.ent_count-1=%d \n",entnum,pobj.ent_count-1);
            if (entnum < 0) break;
        } //end of while
        rc = ISMRC_OK;

        time_topicStats = new_time.tv_sec;

    }

	if (output_buffer.inheap)
		ism_common_free(ism_memory_snmp_misc,output_buffer.buf);
#endif
    return rc;
}

ima_snmp_topic_t *ima_snmp_topic_get_table_head()
{
    return topic_table_head;
}

