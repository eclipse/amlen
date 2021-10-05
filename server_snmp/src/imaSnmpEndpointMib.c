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
 * @brief Endpoint MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of Endpoint MIB interface for MessageSight.
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
#include "imaSnmpEndpointMib.h"
#include "imaSnmpEndpointStat.h"

netsnmp_variable_list *ima_snmp_endpoint_getNextRow(void **endpoint_loop_context,
                void **endpoint_data_context, netsnmp_variable_list *put_index_data,
                netsnmp_iterator_info *endpoint_data)
{
    ima_snmp_endpoint_t *endpointEntry = (ima_snmp_endpoint_t *)*endpoint_loop_context;
    netsnmp_variable_list *idx = put_index_data;
    int len;

    if (NULL == endpointEntry)
    	return NULL;

    if (NULL == endpointEntry->endpointItem[imaSnmpEndpoint_COLINDEX].ptr)
    	return NULL;

    len = strlen(endpointEntry->endpointItem[imaSnmpEndpoint_COLINDEX].ptr);

    ima_snmp_set_var_typed_value(idx, ASN_INTEGER,
        endpointEntry->endpointItem[imaSnmpEndpoint_COLINDEX].ptr,
        len);
    idx = idx->next_variable;

    *endpoint_data_context = (void *) endpointEntry;
    *endpoint_loop_context = (void *) endpointEntry->next;

    return put_index_data;
}

netsnmp_variable_list *ima_snmp_endpoint_getFirstRow(void **endpoint_loop_context,
                void **endpoint_data_context, netsnmp_variable_list *put_index_data,
                netsnmp_iterator_info *endpoint_data)
{
    // build a new endpoint table
    int rc = ima_snmp_get_endpoint_stat();
    if (rc != ISMRC_OK)
    {
        TRACE(8,"Either failed to get endpoint stats or cached values are sill valid. Reuse cached values. RC= %d \n",rc);
    }
    *endpoint_loop_context = (void *)ima_snmp_endpoint_get_table_head();
    return ima_snmp_endpoint_getNextRow(endpoint_loop_context, endpoint_data_context,
        put_index_data, endpoint_data);
}

/** handles requests for the imaSnmpEndpointTable */
int imaSnmpEndpointTable_handler(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    ima_snmp_endpoint_t *endpointEntry;
    int len;

    switch (reqinfo->mode) {
    /*
     * Read-support (also covers GetNext requests)
     */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            endpointEntry = (ima_snmp_endpoint_t *)netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);
            if (NULL == endpointEntry || NULL == table_info) {
                netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                continue;
            }

            if (table_info->colnum >= imaSnmpEndpoint_Col_MAX  || table_info->colnum < imaSnmpEndpoint_COLINDEX ) {
             	netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
             	TRACE(3, "imaSnmpEndpointTable_handler: table_info colnum is out of range: %d\n", table_info->colnum);
             	continue;
            }

            if (endpointEntry->endpointItem[table_info->colnum].ptr) {
                len = strlen(endpointEntry->endpointItem[table_info->colnum].ptr);
            } else {
                len = 0;
            }

            TRACE(9, "imaSnmpEndpointTable_handler: colnum: %d, ptr: %s, len: %d\n",
				table_info->colnum,
				endpointEntry->endpointItem[table_info->colnum].ptr?endpointEntry->endpointItem[table_info->colnum].ptr:"",
				len);

            switch (table_info->colnum) {
            /* (INDEX) /INTEGER32/ASN_INTEGER/long(u_long)//l/A/w/E/r/d/h */
                 case imaSnmpEndpoint_COLINDEX:
                     ima_snmp_set_var_typed_value(request->requestvb, ASN_INTEGER,
                         endpointEntry->endpointItem[imaSnmpEndpoint_COLINDEX].ptr,
                         len);
                 break;

    /* (INDEX) ibmImaEndpointName(1)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_NAME:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        endpointEntry->endpointItem[imaSnmpEndpoint_NAME].ptr,
                        len);
                    break;

    /* ibmImaEndpointIpAddr(2)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_IPADDR:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        endpointEntry->endpointItem[imaSnmpEndpoint_IPADDR].ptr,
                        len);
                    break;

    /* ibmImaEndpointEnabled(3)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_ENABLED:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_INTEGER,
                        endpointEntry->endpointItem[imaSnmpEndpoint_ENABLED].ptr,
                        len);
                    break;

    /* ibmImaEndpointTotal(4)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_TOTAL:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        endpointEntry->endpointItem[imaSnmpEndpoint_TOTAL].ptr,
                        len);
                    break;

    /* ibmImaEndpointActive(5)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_ACTIVE:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        endpointEntry->endpointItem[imaSnmpEndpoint_ACTIVE].ptr,
                        len);
                    break;

    /* ibmImaEndpointMessages(6)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_MESSAGES:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        endpointEntry->endpointItem[imaSnmpEndpoint_MESSAGES].ptr,
                        len);
                    break;

    /* ibmImaEndpointBytes(7)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_BYTES:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        endpointEntry->endpointItem[imaSnmpEndpoint_BYTES].ptr,
                        len);
                    break;

   /* ibmImaEndpointLastErrorCode(8)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_LAST_ERROR_CODE:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_INTEGER,
                        endpointEntry->endpointItem[imaSnmpEndpoint_LAST_ERROR_CODE].ptr,
                        len);
                    break;

    /* ibmImaEndpointConfigTime(9)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_CONFIG_TIME:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        endpointEntry->endpointItem[imaSnmpEndpoint_CONFIG_TIME].ptr,
                        len);
                    break;

    /* ibmImaEndpointResetTime(10)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_RESET_TIME:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        endpointEntry->endpointItem[imaSnmpEndpoint_RESET_TIME].ptr,
                        len);
                    break;

    /* ibmImaEndpointBadConnections(11)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpEndpoint_BAD_CONNECTIONS:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        endpointEntry->endpointItem[imaSnmpEndpoint_BAD_CONNECTIONS].ptr,
                        len);
                    break;

                default:
                    TRACE(2,"unknown column %d in imaSnmpEndpointTable_handler\n",
                          table_info->colnum);
                    break;
            }
        }
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(2, "unknown mode (%d) in imaSnmpEndpointTable_handler\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    } //switch ...

    return SNMP_ERR_NOERROR;
}

int ima_snmp_init_endpoint_table_mibs()
{
    int rc = 0;
    netsnmp_handler_registration *reginfo;
//    netsnmp_mib_handler *handler;
    const oid  endpoint_table_oid[]= { IMA_SNMP_ENDPOINT_MIB };
    netsnmp_iterator_info *iinfo;
    netsnmp_table_registration_info *table_info;

    TRACE(9,"imaSnmpEndpointTable init: \n");

    reginfo = netsnmp_create_handler_registration("imaSnmpEndpointTable",
                                            imaSnmpEndpointTable_handler,
                                            endpoint_table_oid,
                                            OID_LENGTH(endpoint_table_oid),
                                            HANDLER_CAN_RONLY);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER,  0);

    table_info->min_column = IBMIMASTATUSENDPOINTTABLE_MIN_COL;
    table_info->max_column = IBMIMASTATUSENDPOINTTABLE_MAX_COL;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    iinfo->get_first_data_point = ima_snmp_endpoint_getFirstRow;
    iinfo->get_next_data_point = ima_snmp_endpoint_getNextRow;
    iinfo->table_reginfo = table_info;

    netsnmp_register_table_iterator(reginfo, iinfo);

    return rc;
}
