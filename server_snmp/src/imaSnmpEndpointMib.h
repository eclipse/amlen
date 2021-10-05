/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
/*
 * imaSnmpEdnpointMib.h
 *
 *  Created on: 2014-7-21
 *      Author:
 */

#ifndef IMASNMPENDPOINTMIB_H_
#define IMASNMPENDPOINTMIB_H_

#define IMA_SNMP_ENDPOINT_OID 5
#define IMA_SNMP_ENDPOINT_MIB IMA_SNMP_STAT_MIB, IMA_SNMP_ENDPOINT_OID

#define COLUMN_IBMIMAENDPOINT_TABLE_COL_INDEX      1
#define COLUMN_IBMIMAENDPOINTNAME         	       2
#define COLUMN_IBMIMAENDPOINTIPADDR         	   3
#define COLUMN_IBMIMAENDPOINTENABLED         	   4
#define COLUMN_IBMIMAENDPOINTTOTAL         	       5
#define COLUMN_IBMIMAENDPOINTACTIVE         	   6
#define COLUMN_IBMIMAENDPOINTMESSAGES         	   7
#define COLUMN_IBMIMAENDPOINTBYTES         	       8
#define COLUMN_IBMIMAENDPOINTLASTERRORCODE  	   9
#define COLUMN_IBMIMAENDPOINTCONFIGTIME           10
#define COLUMN_IBMIMAENDPOINTRESETTIME       	  11
#define COLUMN_IBMIMAENDPOINTBADCONNECTIONS 	  12


#define IBMIMASTATUSENDPOINTTABLE_MIN_COL   COLUMN_IBMIMAENDPOINT_TABLE_COL_INDEX
#define IBMIMASTATUSENDPOINTTABLE_MAX_COL   COLUMN_IBMIMAENDPOINTBADCONNECTIONS

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int ima_snmp_init_endpoint_table_mibs();

#ifdef __cplusplus
    }
#endif


#endif /* IMASNMPENDPOINTMIB_H_ */
