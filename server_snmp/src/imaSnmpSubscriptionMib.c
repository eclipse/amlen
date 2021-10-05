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
 * @brief Subscription MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of Subscription MIB interface for MessageSight.
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
#include "imaSnmpSubscriptionMib.h"
#include "imaSnmpSubscriptionStat.h"
#include "imaSnmpUtils.h"

const int SIZEOFINTEGER=sizeof(long);

netsnmp_variable_list *ima_snmp_subscription_getNextRow(void **subscription_loop_context,
                void **subscription_data_context, netsnmp_variable_list *put_index_data,
                netsnmp_iterator_info *subscription_data)
{
    ima_snmp_subscription_t *subscriptionEntry = (ima_snmp_subscription_t *)*subscription_loop_context;
    netsnmp_variable_list *idx = put_index_data;
    int len;

    if (NULL == subscriptionEntry)
    	return NULL;
    if (NULL == subscriptionEntry->subscriptionItem[imaSnmpSubscription_COLINDEX].ptr)
        return NULL;

    len = strlen(subscriptionEntry->subscriptionItem[imaSnmpSubscription_COLINDEX].ptr);

    ima_snmp_set_var_typed_value(idx, ASN_INTEGER,
        subscriptionEntry->subscriptionItem[imaSnmpSubscription_COLINDEX].ptr,
        len);
    idx = idx->next_variable;

    *subscription_data_context = (void *) subscriptionEntry;
    *subscription_loop_context = (void *) subscriptionEntry->next;

    return put_index_data;
}

netsnmp_variable_list *ima_snmp_subscription_getFirstRow(void **subscription_loop_context,
                void **subscription_data_context, netsnmp_variable_list *put_index_data,
                netsnmp_iterator_info *subscription_data)
{
    // build a new subscription table
    int rc = ima_snmp_get_subscription_stat();
    if (rc != ISMRC_OK)
    {
        TRACE(8,"Either failed to get subscription stats or cached values are still valid. Reuse old values. RC=%d \n",rc);
    }
    *subscription_loop_context = (void *)ima_snmp_subscription_get_table_head();
    return ima_snmp_subscription_getNextRow(subscription_loop_context, subscription_data_context,
        put_index_data, subscription_data);
}

/** handles requests for the imaSnmpSubscriptionTable */
int imaSnmpSubscriptionTable_handler(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    ima_snmp_subscription_t *subscriptionEntry;
    int len = 0;

    switch (reqinfo->mode) {
    /*
     * Read-support (also covers GetNext requests)
     */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            subscriptionEntry = (ima_snmp_subscription_t *)netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);
            if (NULL == subscriptionEntry || NULL == table_info) {
                netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                TRACE(8, "imaSnmpSubscriptionTable_handler: subscription entry or table_info is NULL\n");
                continue;
            }

            if (table_info->colnum >= imaSnmpSubscription_Col_MAX  || table_info->colnum < imaSnmpSubscription_COLINDEX ) {
            	netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
            	TRACE(3, "imaSnmpSubscriptionTable_handler: table_info colnum is out of range: %d\n", table_info->colnum);
            	continue;
            }

            if (subscriptionEntry->subscriptionItem[table_info->colnum].ptr) {
				len = strlen(subscriptionEntry->subscriptionItem[table_info->colnum].ptr);
			} else {
				len = 0;
			}

            TRACE(9, "imaSnmpSubscriptionTable_handler: colnum: %d, ptr: %s, len: %d\n",
				table_info->colnum,
				subscriptionEntry->subscriptionItem[table_info->colnum].ptr?subscriptionEntry->subscriptionItem[table_info->colnum].ptr:"",
				len);

            switch (table_info->colnum) {
                /* (INDEX) /INTEGER32/ASN_INTEGER/long(u_long)//l/A/w/E/r/d/h */
                case imaSnmpSubscription_COLINDEX:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_INTEGER,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_COLINDEX].ptr,
                        len);
                    break;

                /* ibmImaSubscriptionSubName(1)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpSubscription_SUBNAME:
                	//subname can be null
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_SUBNAME].ptr,
                        len);
                    break;

                /*  (2)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpSubscription_TOPICSTRING:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_TOPICSTRING].ptr,
                        len);
                    break;

                /* (3)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpSubscription_CLIENTID:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_CLIENTID].ptr,
                        len);
                    break;

                /* (4)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpSubscription_ISDURABLE:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_ISDURABLE].ptr,
                        len);
                    break;

                /* (5)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */             // int
                case imaSnmpSubscription_BUFFEREDMSGS:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_BUFFEREDMSGS].ptr,
                        len);
                    break;


                /* (6)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */           //int
                case imaSnmpSubscription_BUFFEREDMSGSHWM:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_BUFFEREDMSGSHWM].ptr,
                        len);
                    break;

                /* (7)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */         //float/ number
                case imaSnmpSubscription_BUFFEREDPERCENT:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_UNSIGNED,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_BUFFEREDPERCENT].ptr,
                        len);
                    break;

                /* (8)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */          //int
                case imaSnmpSubscription_MAXMESSAGES:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_MAXMESSAGES].ptr,
                        len);
                    break;

                /* (9)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */        //int
                case imaSnmpSubscription_PUBLISHEDMSGS:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_PUBLISHEDMSGS].ptr,
                        len);
                    break;

                /* (10)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */       //int
                case imaSnmpSubscription_REJECTEDMSGS:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_REJECTEDMSGS].ptr,
                        len);
                    break;

                /* (11)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */    //float/ number
                case imaSnmpSubscription_BUFFEREDHWMPERCENT:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_UNSIGNED,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_BUFFEREDHWMPERCENT].ptr,
                        len);
                    break;

	            /* (12)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
				case imaSnmpSubscription_ISSHARED:
					ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
					    subscriptionEntry->subscriptionItem[imaSnmpSubscription_ISSHARED].ptr,
					    len);
					break;

	            /* (13)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */       //int
				case imaSnmpSubscription_CONSUMERS:
					ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_CONSUMERS].ptr,
                        len);
					break;
	            /* (14)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */       //int
				case imaSnmpSubscription_DISCARDEDMSGS:
					ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_DISCARDEDMSGS].ptr,
                        len);
					break;
	            /* (15)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */       //int
				case imaSnmpSubscription_EXPIREDMSGS:
					ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        subscriptionEntry->subscriptionItem[imaSnmpSubscription_EXPIREDMSGS].ptr,
                        len);
					break;
                /* (16)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
				case imaSnmpSubscription_MESSAGINGPOLICY:
					ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
					    subscriptionEntry->subscriptionItem[imaSnmpSubscription_MESSAGINGPOLICY].ptr,
					    len);
					break;

                default:
                    TRACE(2,"unknown column %d in imaSnmpSubscriptionTable_handler\n",
                          table_info->colnum);
                    break;
            }
        }
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(2, "unknown mode (%d) in imaSnmpSubscriptionTable_handler\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    } //switch ...

    return SNMP_ERR_NOERROR;
}

int ima_snmp_init_subscription_table_mibs()
{
    int rc = 0;
    netsnmp_handler_registration *reginfo;
//    netsnmp_mib_handler *handler;
    const oid  subscription_table_oid[]= { IMA_SNMP_SUBSCRIPTION_MIB };
    netsnmp_iterator_info *iinfo;
    netsnmp_table_registration_info *table_info;

    TRACE(9,"imaSnmpSubscriptionTable init: \n");

    reginfo = netsnmp_create_handler_registration("imaSnmpSubscriptionTable",
                                            imaSnmpSubscriptionTable_handler,
                                            subscription_table_oid,
                                            OID_LENGTH(subscription_table_oid),
                                            HANDLER_CAN_RONLY);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER,  0);

    table_info->min_column = IBMIMASTATUSSUBSCRIPTIONTABLE_MIN_COL;
    table_info->max_column = IBMIMASTATUSSUBSCRIPTIONTABLE_MAX_COL;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    iinfo->get_first_data_point = ima_snmp_subscription_getFirstRow;
    iinfo->get_next_data_point = ima_snmp_subscription_getNextRow;
    iinfo->table_reginfo = table_info;

    netsnmp_register_table_iterator(reginfo, iinfo);

    return rc;
}
