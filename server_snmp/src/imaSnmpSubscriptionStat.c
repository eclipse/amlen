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
 * @brief Subscription Stat interface for MessageSight
 *
 * This provides the interface to access Subscription stats of MessageSight.
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

#include "imaSnmpSubscriptionStat.h"
#include "imaSnmpJson.h"

#define _SNMP_THREADED_

extern  int agentRefreshCycle;

#ifndef _SNMP_THREADED_
static int PROC_TYPE=0;  /* use ism admin port */
static char* MONITOR_PROTOCOL="monitoring.ism.ibm.com";
#endif

static char* USER="snmp";

static int TableIndex=1;

static time_t  time_subscriptionStats = 0;

static const ima_snmp_col_desc_t subscription_column_desc[] = {
   {NULL,imaSnmpCol_None, 0},
   {"ColIndex",imaSnmpCol_String,COLUMN_IBMIMASUBSCRIPTION_TABLE_COL_INDEX_SIZE},
   {"SubName",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_SUBNAME_SIZE},
   {"TopicString",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_TOPICSTRING_SIZE},
   {"ClientID",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_CLIENTID_SIZE},
   {"IsDurable",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_ISDURABLE_SIZE},
   {"BufferedMsgs",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_BUFFEREDMSGS_SIZE},
   {"BufferedMsgsHWM",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_BUFFEREDMSGSHWM_SIZE},
   {"BufferedPercent",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_BUFFEREDPERCENT_SIZE},//?? float
   {"MaxMessages",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_MAXMESSAGES_SIZE},
   {"PublishedMsgs",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_PUBLISHEDMSGS_SIZE},
   {"RejectedMsgs",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_REJECTEDMSGS_SIZE},
   {"BufferedHWMPercent",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_BUFFEREDHWMPERCENT_SIZE},//float
   {"IsShared",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_ISSHARED_SIZE},
   {"Consumers",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_CONSUMERS_SIZE},
   {"DiscardedMsgs",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_DISCARDEDMSGS_SIZE},
   {"ExpiredMsgs",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_EXPIREDMSGS_SIZE},
   {"MessagingPolicy",imaSnmpCol_String, COLUMN_IBMIMASUBSCRIPTION_MESSAGINGPOLICY_SIZE}

};

static ima_snmp_subscription_t *subscription_table_head=NULL;
static ima_snmp_subscription_t *subscription_table_tail=NULL;

#define SUBSCRIPTION_CMD_STRING "{ \"Action\":\"Subscription\",\"User\":\"%s\",\"Locale\":\"en_US\",\"SubName\":\"*\",\"TopicString\":\"*\",\"ClientID\":\"*\",\"SubType\":\"All\",\"StatType\":\"PublishedMsgsHighest\",\"ResultCount\":\"25\" }"


/*
 * Brief: construct the cmd of getting subscription statistics
 * Para:
 * cmd: a buffer used to cache the constructed cmd.
 * len: buffersize
 * */
static int get_subscription_stat_cmd(char* cmd, int cmd_len)
{
    if ((NULL == cmd) || (cmd_len < 200))
    {
        TRACE(2,"invalid cmd buffer for subscription stat command creation \n");
        return ISMRC_Error;
    }

    snprintf(cmd, cmd_len, SUBSCRIPTION_CMD_STRING,USER);
    return ISMRC_OK;
}


/* free an subscription table entry*/
static inline void ima_snmp_subscription_free_entry(ima_snmp_subscription_t *pEntry)
{
    int i;
    if (NULL == pEntry) return;
    for (i = imaSnmpSubscription_Col_MIN; i < imaSnmpSubscription_Col_MAX; i++)
    {
        if ((subscription_column_desc[i].colType == imaSnmpCol_String) &&
            (pEntry->subscriptionItem[i].ptr != NULL))
            ism_common_free(ism_memory_snmp_misc,pEntry->subscriptionItem[i].ptr);
    }
    ism_common_free(ism_memory_snmp_misc,pEntry);
}

/*
 * Brief :  free the resource of global subscription table
 * */
int ima_snmp_free_subscription_table()
{
    int rc=ISMRC_OK;
    if(subscription_table_head)
    {
        ima_snmp_subscription_t * nextNode=subscription_table_head;
        ima_snmp_subscription_t * preNode=NULL;

        while(nextNode != NULL)
        {
            preNode = nextNode;
            nextNode=preNode->next;
            //ism_common_free(ism_memory_snmp_misc,preNode);
            ima_snmp_subscription_free_entry(preNode);
        }
        subscription_table_head = subscription_table_tail = NULL;
    }
    return rc;
}

/* Create a new entry for subscription table */
static inline ima_snmp_subscription_t *ima_snmp_subscription_create_entry()
{
    int i;
    ima_snmp_subscription_t *pEntry = NULL;
    
    pEntry = (ima_snmp_subscription_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,37),sizeof(ima_snmp_subscription_t));
    if (NULL == pEntry)
    {
        TRACE(2,"failed to allocate resources for new subscription table entry.");
        return NULL;
    }
    memset(pEntry,0,sizeof(ima_snmp_subscription_t));
    for (i = imaSnmpSubscription_Col_MIN; i < imaSnmpSubscription_Col_MAX; i++)
    {
        if (subscription_column_desc[i].colType == imaSnmpCol_String)
        { // allocate string buffer for the  item;
            char *tempPtr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,38),subscription_column_desc[i].colSize * sizeof(char));
            if (tempPtr == NULL)
            {
                TRACE(2,"failed to allocated memory for subscription item %s \n",
                        subscription_column_desc[i].colName);
                break;
            }
            pEntry->subscriptionItem[i].ptr = tempPtr;
        }
    }

    if (i < imaSnmpSubscription_Col_MAX)
    {
        TRACE(2,"free entry due to malloc issue");
        ima_snmp_subscription_free_entry(pEntry);
        return NULL;
    }
    return pEntry;
}

static  ima_snmp_subscription_t *ima_snmp_subscription_create_default_entry()
{
    int i;
    ima_snmp_subscription_t *pEntry = NULL;

    pEntry = (ima_snmp_subscription_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,39),sizeof(ima_snmp_subscription_t));
    if (NULL == pEntry)
    {
        TRACE(2,"failed to allocate resources for default subscription table entry.");
        return NULL;
    }
    memset(pEntry,0,sizeof(ima_snmp_subscription_t));
    for (i = imaSnmpSubscription_Col_MIN; i < imaSnmpSubscription_Col_MAX; i++)
    {
        if (subscription_column_desc[i].colType == imaSnmpCol_Integer)
        { // set default value for the item;
            pEntry->subscriptionItem[i].val = IMA_SNMP_SUBSCRIPTION_INT_DEFAULT;
        }
    }
    return pEntry;
}

static inline void ima_snmp_subscription_insert_entry(ima_snmp_subscription_t *pEntry)
{
    if (NULL == pEntry) return;
    
    // insert new Entry to global subscription table.
    if (NULL == subscription_table_tail)
        subscription_table_tail = subscription_table_head = pEntry;
    else {
        subscription_table_tail->next = pEntry;
        subscription_table_tail = pEntry;
    }
}

int ima_snmp_subscription_add_entry(ism_json_parse_t pobj, int entnum)
{
    ima_snmp_subscription_t *subscriptionEntry = NULL;
    int i;

    subscriptionEntry = ima_snmp_subscription_create_entry();
    if (NULL == subscriptionEntry)
    {
        TRACE(2,"failed to create an entry for subscription row ");
        return -1;
    }
    //TRACE(2,"subscription_add_entry %d \n",entnum);
    for (i = imaSnmpSubscription_Col_MIN; i < imaSnmpSubscription_Col_MAX; i++)
    {
        char* tempStr=NULL;
        int   tempVal = 0;
        int buflen=0;
        if(!strcmp(subscription_column_desc[i].colName, "ColIndex"))
        {
            memset(subscriptionEntry->subscriptionItem[i].ptr,0,subscription_column_desc[i].colSize * sizeof(char));
            sprintf(subscriptionEntry->subscriptionItem[i].ptr,"%d",TableIndex);
            TableIndex++;
            continue;
        }

        //TRACE(2,"subscription_add_entry loop i = %d, type = %d  \n",i, subscription_column_desc[i].colType);
        if (subscription_column_desc[i].colType == imaSnmpCol_String)
        {
            tempStr=(char*)ima_snmp_jsonArray_getString(&pobj,
                           subscription_column_desc[i].colName, &entnum);
            if (NULL == tempStr)
            {
                if(entnum<0)
                {
                    TRACE(2,"cannot find %s in subscription message. Break from the Loop.\n",
                            subscription_column_desc[i].colName);
                    break;
                }
                else
                {// There exists the case that the tempStr is "".
                    tempStr=" ";
                }
            }
            TRACE(9,"%s loop i = %d, entnum = %d, val = %s \n", __FUNCTION__, i, entnum, tempStr);
            buflen = strlen(tempStr);

            if (buflen > subscription_column_desc[i].colSize - 1 ) {
            	//Handle TopicString, SubName and ClientID to their real size limit
            	if (!strcmp(subscription_column_desc[i].colName, "SubName") ||
            		!strcmp(subscription_column_desc[i].colName, "TopicString") ||
            		!strcmp(subscription_column_desc[i].colName, "ClientID"))  {
            		//free the pre-allocated memory and malloc a big one
            		char *ptr = subscriptionEntry->subscriptionItem[i].ptr;
            		subscriptionEntry->subscriptionItem[i].ptr = NULL;
            		subscriptionEntry->subscriptionItem[i].ptr = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,40),buflen + 1);
            		if (ptr)
            			ism_common_free(ism_memory_snmp_misc,ptr);
            	} else {
            		//just a safety check now. No entry should be here
            	    buflen = subscription_column_desc[i].colSize - 1;
            	    TRACE(9,"%s: val %s is longer than defined length: %d\n", __FUNCTION__, tempStr, subscription_column_desc[i].colSize);
            	}
            }
            memcpy(subscriptionEntry->subscriptionItem[i].ptr, tempStr, buflen);
            subscriptionEntry->subscriptionItem[i].ptr[buflen] = '\0';
        }
        if (subscription_column_desc[i].colType == imaSnmpCol_Integer)
        {// We will deal with all the Json item with String. So, this block code will not run.
          tempVal=(int)ima_snmp_jsonArray_getInt(&pobj,
                          subscription_column_desc[i].colName,65535,&entnum);

          //TRACE(2,"****************%s value=%d  entnum=%d \n", subscription_column_desc[i].colName,tempVal,entnum);
           if (65535 == tempVal)
           {
               TRACE(2,"cannot find %s in subscription message \n.  entnum=%d",
                       subscription_column_desc[i].colName,entnum);
               break;
           }
           subscriptionEntry->subscriptionItem[i].val=tempVal;
        }
    
    }

    if (entnum > 0) 
        ima_snmp_subscription_insert_entry(subscriptionEntry);
    else  ima_snmp_subscription_free_entry(subscriptionEntry);

    return entnum;
}

/*
 * Brief: get the subscription statistics info from messagesight.
 * */
int ima_snmp_get_subscription_stat()
{
    int  rc = ISMRC_Error;
    char subStatCmd[256];
    struct timeval new_time;

    gettimeofday(&new_time,NULL);
    if ((new_time.tv_sec - time_subscriptionStats) <=  agentRefreshCycle)
    {
        TRACE(7, "Info: get subscription request is still within refresh cycle, use existed value. \n");
        return ISMRC_OK;
    }

    //if subscription table is not NULL, free it firstly
    if (subscription_table_head) ima_snmp_free_subscription_table();

    rc = get_subscription_stat_cmd(subStatCmd, 256);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to create cmd for subscription stat\n");
        return rc;
    }

    rc = ISMRC_Error;

#ifndef _SNMP_THREADED_
    char *result = NULL;
    result = ismcli_ISMClient(USER, MONITOR_PROTOCOL, subStatCmd, PROC_TYPE);

    /* process result */
    if ( result )
    {
        /* Check if returned result is a json string */
            int buflen = 0;
            ism_json_parse_t pobj = {0};
            ism_json_entry_t ents[MAX_JSON_ENTRIES];

            char *tmpbuf;
            buflen = strlen(result);
            tmpbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,42),buflen + 1);
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
                TRACE(5,"Info: ima_snmp_get_subscription_stat return  RC=%s\n", rcstr);
                ima_snmp_subscription_insert_entry( ima_snmp_subscription_create_default_entry());
                ismCLI_FREE(result);
                ism_common_free(ism_memory_snmp_misc,tmpbuf);
                time_subscriptionStats = new_time.tv_sec;
                return ISMRC_OK;
            }

            if ( errstr ) {
                TRACE(2,"ima_snmp_get_subscription_stat return ErrorString: %s : RC=%s\n",
                      errstr,rcstr);
                ismCLI_FREE(result);
                ism_common_free(ism_memory_snmp_misc,tmpbuf);
                return rc;
            }  // if ( errstr)

            /*Free the result.*/
            ismCLI_FREE(result);

            //testJsonStr(&pobj);

            // Parse json array to subscription_table
            TableIndex=1;//index for table colume
            int entnum=0;
            while(entnum<pobj.ent_count-1)
            {
                entnum = ima_snmp_subscription_add_entry(pobj,entnum);
                //TRACE(2,"------------->entnum=%d;  pobj.ent_count-1=%d \n",entnum,pobj.ent_count-1);
                if (entnum < 0) break;
            } //end of while
            rc = ISMRC_OK;

            time_subscriptionStats = new_time.tv_sec;
            ism_common_free(ism_memory_snmp_misc,tmpbuf);
    }
#else
    int subBufLen = 1024;
    char subBuf[subBufLen];
    concat_alloc_t output_buffer;

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.buf = subBuf;
    output_buffer.len = subBufLen;

    ism_process_monitoring_action(NULL, subStatCmd, strlen(subStatCmd), &output_buffer, &rc);

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
        	TRACE(5,"Info: ima_snmp_get_subscription_stat return  RC=%s\n", rcstr);
            ima_snmp_subscription_insert_entry( ima_snmp_subscription_create_default_entry());
            time_subscriptionStats = new_time.tv_sec;
            return ISMRC_OK;
        }
        if ( errstr )
        {
        	TRACE(2,"ima_snmp_get_subscription_stat return ErrorString: %s : RC=%s\n",
                  errstr,rcstr);
             return rc;
        }

        // Parse json array to subscription_table
        TableIndex=1;//index for table column
        int entnum=0;
        while(entnum<pobj.ent_count-1)
        {
            entnum = ima_snmp_subscription_add_entry(pobj,entnum);
            //TRACE(2,"------------->entnum=%d;  pobj.ent_count-1=%d \n",entnum,pobj.ent_count-1);
            if (entnum < 0) break;
        } //end of while
        rc = ISMRC_OK;

        time_subscriptionStats = new_time.tv_sec;

    }

	if (output_buffer.inheap)
		ism_common_free(ism_memory_snmp_misc,output_buffer.buf);
#endif
    return rc;
}

ima_snmp_subscription_t *ima_snmp_subscription_get_table_head()
{
    return subscription_table_head;
}

