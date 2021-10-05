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
 * @brief Topic MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of Topic MIB interface for MessageSight.
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
#include "imaSnmpTopicMib.h"
#include "imaSnmpTopicStat.h"
#include "imaSnmpUtils.h"

netsnmp_variable_list *ima_snmp_topic_getNextRow(void **topic_loop_context,
                void **topic_data_context, netsnmp_variable_list *put_index_data,
                netsnmp_iterator_info *topic_data)
{
    ima_snmp_topic_t *topicEntry = (ima_snmp_topic_t *)*topic_loop_context;
    netsnmp_variable_list *idx = put_index_data;
    int len;

    if (NULL == topicEntry)
    	return NULL;

    if (NULL == topicEntry->topicItem[imaSnmpTopic_COLINDEX].ptr)
    	return NULL;

    len = strlen(topicEntry->topicItem[imaSnmpTopic_COLINDEX].ptr);

    ima_snmp_set_var_typed_value(idx, ASN_INTEGER,
        topicEntry->topicItem[imaSnmpTopic_COLINDEX].ptr,
        len);
    idx = idx->next_variable;

    *topic_data_context = (void *) topicEntry;
    *topic_loop_context = (void *) topicEntry->next;

    return put_index_data;
}

netsnmp_variable_list *ima_snmp_topic_getFirstRow(void **topic_loop_context,
                void **topic_data_context, netsnmp_variable_list *put_index_data,
                netsnmp_iterator_info *topic_data)
{
    // build a new topic table
    int rc = ima_snmp_get_topic_stat();
    if (rc != ISMRC_OK)
    {
        TRACE(8,"Either failed to get topic stats or cahced values are still valid. Reuse old values. RC=%d \n",rc);
    }
    *topic_loop_context = (void *)ima_snmp_topic_get_table_head();
    return ima_snmp_topic_getNextRow(topic_loop_context, topic_data_context,
        put_index_data, topic_data);
}

/** handles requests for the imaSnmpTopicTable */
int imaSnmpTopicTable_handler(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    ima_snmp_topic_t *topicEntry;
    int len;

    switch (reqinfo->mode) {
    /*
     * Read-support (also covers GetNext requests)
     */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            topicEntry = (ima_snmp_topic_t *)netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);
            if (NULL == topicEntry || NULL == table_info) {
                netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                continue;
            }

            if (table_info->colnum >= imaSnmpTopic_Col_MAX  || table_info->colnum < imaSnmpTopic_COLINDEX ) {
            	netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
            	TRACE(3, "imaSnmpTopicTable_handler: table_info colnum is out of range: %d\n", table_info->colnum);
            	continue;
            }

            if (topicEntry->topicItem[table_info->colnum].ptr) {
                len = strlen(topicEntry->topicItem[table_info->colnum].ptr);
            } else {
                len = 0;
            }

            TRACE(9, "imaSnmpTopicTable_handler: colnum: %d, ptr: %s, len: %d\n",
				table_info->colnum,
				topicEntry->topicItem[table_info->colnum].ptr?topicEntry->topicItem[table_info->colnum].ptr:"",
				len);

            switch (table_info->colnum) {
                /* (INDEX) /INTEGER32/ASN_INTEGER/long(u_long)//l/A/w/E/r/d/h */
                case imaSnmpTopic_COLINDEX:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_INTEGER,
                        topicEntry->topicItem[imaSnmpTopic_COLINDEX].ptr,
                        len);
                    break;

                /* (1)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpTopic_TOPICSTRING:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        topicEntry->topicItem[imaSnmpTopic_TOPICSTRING].ptr,
                        len);
                    break;

                /*  (2)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpTopic_SUBSCRIPTIONS:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        topicEntry->topicItem[imaSnmpTopic_SUBSCRIPTIONS].ptr,
                        len);
                    break;

                /* (3)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpTopic_RESETTIME:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        topicEntry->topicItem[imaSnmpTopic_RESETTIME].ptr,
                        len);
                    break;

                /* (4)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpTopic_PUBLISHEDMSGS:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        topicEntry->topicItem[imaSnmpTopic_PUBLISHEDMSGS].ptr,
                        len);
                    break;

                /* (5)/DisplayString/char(char)//L/A/w/e/R/d/H */
                case imaSnmpTopic_REJECTEDMSGS:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        topicEntry->topicItem[imaSnmpTopic_REJECTEDMSGS].ptr,
                        len);
                    break;


                /* (6)/DisplayString/char(char)//L/A/w/e/R/d/H */
                case imaSnmpTopic_FAILEDPUBLISHES:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        topicEntry->topicItem[imaSnmpTopic_FAILEDPUBLISHES].ptr,
                        len);
                    break;

                default:
                    TRACE(2,"unknown column %d in imaSnmpTopicTable_handler\n",
                          table_info->colnum);
                    break;
            }
        }
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(2, "unknown mode (%d) in imaSnmpTopicTable_handler\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    } //switch ...

    return SNMP_ERR_NOERROR;
}

int ima_snmp_init_topic_table_mibs(void)
{
    int rc = 0;
    netsnmp_handler_registration *reginfo;
//    netsnmp_mib_handler *handler;
    const oid  topic_table_oid[]= { IMA_SNMP_TOPIC_MIB };
    netsnmp_iterator_info *iinfo;
    netsnmp_table_registration_info *table_info;

    TRACE(9,"imaSnmpTopicTable init: \n");

    reginfo = netsnmp_create_handler_registration("imaSnmpTopicTable",
                                            imaSnmpTopicTable_handler,
                                            topic_table_oid,
                                            OID_LENGTH(topic_table_oid),
                                            HANDLER_CAN_RONLY);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER,  0);

    table_info->min_column = IBMIMASTATUSTOPICTABLE_MIN_COL;
    table_info->max_column = IBMIMASTATUSTOPICTABLE_MAX_COL;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    iinfo->get_first_data_point = ima_snmp_topic_getFirstRow;
    iinfo->get_next_data_point = ima_snmp_topic_getNextRow;
    iinfo->table_reginfo = table_info;

    netsnmp_register_table_iterator(reginfo, iinfo);

    return rc;
}
