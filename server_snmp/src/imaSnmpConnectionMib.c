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
 * @brief Connection MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of Connection MIB interface for MessageSight.
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
#include "imaSnmpConnectionStat.h"
#include "imaSnmpConnectionMib.h"
#include "imaSnmpUtils.h"


netsnmp_variable_list *ima_snmp_connection_getNextRow(void **connection_loop_context,
                void **connection_data_context, netsnmp_variable_list *put_index_data,
                netsnmp_iterator_info *connection_data)
{
    ima_snmp_connection_t *connectionEntry = (ima_snmp_connection_t *)*connection_loop_context;
    netsnmp_variable_list *idx = put_index_data;
    int len = 0;

    if (NULL == connectionEntry)
    	return NULL;

    if (NULL == connectionEntry->connectionItem[imaSnmpConnection_COLINDEX].ptr)
    	return NULL;

    len = strlen(connectionEntry->connectionItem[imaSnmpConnection_COLINDEX].ptr);

    ima_snmp_set_var_typed_value(idx, ASN_INTEGER,
        connectionEntry->connectionItem[imaSnmpConnection_COLINDEX].ptr,
        len);
    idx = idx->next_variable;

    *connection_data_context = (void *) connectionEntry;
    *connection_loop_context = (void *) connectionEntry->next;

    return put_index_data;
}

netsnmp_variable_list *ima_snmp_connection_getFirstRow(void **connection_loop_context,
                void **connection_data_context, netsnmp_variable_list *put_index_data,
                netsnmp_iterator_info *connection_data)
{
    // build a new connection table
    int rc = ima_snmp_get_connection_stat();
    if (rc != ISMRC_OK)
    {
        TRACE(7,"No new connection was found. Using cached value. Return code:  %d \n",rc);
    }
    *connection_loop_context = (void *)ima_snmp_connection_get_table_head();
    return ima_snmp_connection_getNextRow(connection_loop_context, connection_data_context,
        put_index_data, connection_data);
}

/** handles requests for the imaSnmpConnectionTable */
int imaSnmpConnectionTable_handler(netsnmp_mib_handler *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info *reqinfo,
               netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    ima_snmp_connection_t *connectionEntry;
    int len;

    switch (reqinfo->mode) {
    /*
     * Read-support (also covers GetNext requests)
     */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            connectionEntry = (ima_snmp_connection_t *)netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);
            if (NULL == connectionEntry || NULL == table_info) {
                netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                continue;
            }

            if (table_info->colnum >= imaSnmpConnection_Col_MAX  || table_info->colnum < imaSnmpConnection_COLINDEX ) {
				netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
				TRACE(3, "imaSnmpConnectionTable_handler: table_info colnum is out of range: %d\n", table_info->colnum);
				continue;
		    }

            if (connectionEntry->connectionItem[table_info->colnum].ptr) {
                len = strlen(connectionEntry->connectionItem[table_info->colnum].ptr);
            } else {
                len = 0;
            }

            TRACE(9, "imaSnmpConnectionTable_handler: colnum: %d, ptr: %s, len: %d\n",
				table_info->colnum,
				connectionEntry->connectionItem[table_info->colnum].ptr?connectionEntry->connectionItem[table_info->colnum].ptr:"",
				len);

            switch (table_info->colnum) {
                /* (INDEX) /INTEGER32/ASN_INTEGER/long(u_long)//l/A/w/E/r/d/h */
                case imaSnmpConnection_COLINDEX:
                    if (NULL == connectionEntry->connectionItem[imaSnmpConnection_COLINDEX].ptr)
                    { // set a default value to the column index.
                        ima_snmp_set_var_typed_value(request->requestvb,ASN_INTEGER,
                            IMA_SNMP_CONN_INDEX_DEFAULT, strlen(IMA_SNMP_CONN_INDEX_DEFAULT));
                    }
                    else 
                    {
                        ima_snmp_set_var_typed_value(request->requestvb, ASN_INTEGER,
                            connectionEntry->connectionItem[imaSnmpConnection_COLINDEX].ptr,
                            strlen(connectionEntry->connectionItem[imaSnmpConnection_COLINDEX].ptr));
                    }
                    break;

                /* (1)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpConnection_NAME:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        connectionEntry->connectionItem[imaSnmpConnection_NAME].ptr,
                        len);
                    break;

                /*  (2)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpConnection_PROTOCOL:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        connectionEntry->connectionItem[imaSnmpConnection_PROTOCOL].ptr,
                        len);
                    break;

                /* (3)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpConnection_CLIENTADDR:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        connectionEntry->connectionItem[imaSnmpConnection_CLIENTADDR].ptr,
                        len);
                    break;

                /* (4)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */
                case imaSnmpConnection_USERID:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        connectionEntry->connectionItem[imaSnmpConnection_USERID].ptr,
                        len);
                    break;

                /* (5)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */             // int
                case imaSnmpConnection_ENDPOINT:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        connectionEntry->connectionItem[imaSnmpConnection_ENDPOINT].ptr,
                        len);
                    break;


                /* (6)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */           //int
                case imaSnmpConnection_PORT:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_INTEGER,
                        connectionEntry->connectionItem[imaSnmpConnection_PORT].ptr,
                        len);
                    break;

                /* (7)/DisplayString/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */         //float/ number
                case imaSnmpConnection_CONNECTTIME:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                        connectionEntry->connectionItem[imaSnmpConnection_CONNECTTIME].ptr,
                        len);
                    break;

                /* (8)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */          //int
                case imaSnmpConnection_DURATION:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        connectionEntry->connectionItem[imaSnmpConnection_DURATION].ptr,
                        len);
                    break;

                /* (9)/ASN_INTEGER/char(char)//L/A/w/e/R/d/H */        //int
                case imaSnmpConnection_READBYTES:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        connectionEntry->connectionItem[imaSnmpConnection_READBYTES].ptr,
                        len);
                    break;

                /* (10)/ASN_COUNTER64/char(char)//L/A/w/e/R/d/H */       //int
                case imaSnmpConnection_READMSG:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        connectionEntry->connectionItem[imaSnmpConnection_READMSG].ptr,
                        len);
                    break;

                /* (11)/ASN_COUNTER64/ASN_OCTET_STR/char(char)//L/A/w/e/R/d/H */    //float/ number
                case imaSnmpConnection_WRITEBYTES:
                    ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
                        connectionEntry->connectionItem[imaSnmpConnection_WRITEBYTES].ptr,
                        len);
                    break;

	            /* (12)/CounterBasedGauge64/ASN_COUNTER64/char(char)//L/A/w/e/R/d/H */
				case imaSnmpConnection_WRITEMSG:
					ima_snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64,
					    connectionEntry->connectionItem[imaSnmpConnection_WRITEMSG].ptr,
					    len);
					break;


                default:
                    TRACE(2,"unknown column %d in imaSnmpConnectionTable_handler\n",
                          table_info->colnum);
                    break;
            }
        }
        break;

    default:
    /*
     * we should never get here, so this is a really bad error
     */
       TRACE(2, "unknown mode (%d) in imaSnmpConnectionTable_handler\n",reqinfo->mode);
       return SNMP_ERR_GENERR;
    } //switch ...

    return SNMP_ERR_NOERROR;
}

int ima_snmp_init_connection_table_mibs()
{
    int rc = 0;
    netsnmp_handler_registration *reginfo;
//    netsnmp_mib_handler *handler;
    const oid  connection_table_oid[]= { IMA_SNMP_CONNECTION_MIB };
    netsnmp_iterator_info *iinfo;
    netsnmp_table_registration_info *table_info;

    TRACE(9,"imaSnmpConnectionTable init: \n");

    reginfo = netsnmp_create_handler_registration("imaSnmpConnectionTable",
                                            imaSnmpConnectionTable_handler,
                                            connection_table_oid,
                                            OID_LENGTH(connection_table_oid),
                                            HANDLER_CAN_RONLY);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER,  0);

    table_info->min_column = IBMIMASTATUSCONNECTIONTABLE_MIN_COL;
    table_info->max_column = IBMIMASTATUSCONNECTIONTABLE_MAX_COL;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    iinfo->get_first_data_point = ima_snmp_connection_getFirstRow;
    iinfo->get_next_data_point = ima_snmp_connection_getNextRow;
    iinfo->table_reginfo = table_info;

    netsnmp_register_table_iterator(reginfo, iinfo);

    return rc;
}
