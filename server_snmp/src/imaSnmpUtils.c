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
 * @brief General functions for SNMP
 * @date  Oct. 01
 *
 *
 */


#include "imaSnmpUtils.h"


int ima_snmp_set_var_typed_value(netsnmp_variable_list * requestvb,
		                     u_char type,
		                     char * strval,
		                     size_t len)
{
	int rc = ISMRC_OK;
	long number = 0;
	unsigned long long int ulval = 0;
	// char *p;

	TRACE(9, "%s: strval:%s, len:%d\n", __FUNCTION__, strval, (int)len);
    switch(type) {
    case ASN_OCTET_STR:
        rc = snmp_set_var_typed_value(requestvb, ASN_OCTET_STR, (const void *)strval, len);
        break;

    case ASN_INTEGER:
    	if (len == 0) {
    		number = 0;
    	}else {
            number = atol(strval);
    	}
        rc = snmp_set_var_typed_integer(requestvb, ASN_INTEGER, number);
        break;

    case ASN_COUNTER64:
    {
        struct counter64 count;

        if (len == 0) {
        	ulval = 0;
        } else {
    	    ulval = strtoull(strval, (char **)NULL, 10);
        }

    	count.high = (unsigned long) (ulval >> 32);
    	count.low  = (unsigned long) ulval & 0xffffffff;

    	rc = snmp_set_var_typed_value(requestvb, ASN_COUNTER64, &count, sizeof(struct counter64));

        break;
    }
    case ASN_UNSIGNED:
    	number = atol(strval);
    	rc = snmp_set_var_typed_integer(requestvb, ASN_UNSIGNED, number);
    	break;
    default:
    	TRACE(9,"%s: The type %d is not supported. strval: %s\n", __FUNCTION__, type, strval?strval:"");
    	rc = ISMRC_Error;
        break;
    }

    return rc;
}

