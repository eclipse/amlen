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

/*
 * Note: this header file provides definitions for MessageSight server stats,
 *       as well as the interface to access server stats.
 */

#ifndef _IMASNMPCOLUMN_H_
#define _IMASNMPCOLUMN_H_

#define STR_SIZE_DEFAULT 255
#define STR_SIZE_PERCENT 10

typedef enum {
    imaSnmpCol_None    = 0,
    imaSnmpCol_Integer    = 1,
    imaSnmpCol_String    = 2,
}  imaSnmpColumn_Type;

typedef struct ima_snmp_col_desc_tag {
    char *colName;
    imaSnmpColumn_Type colType;
    int  colSize;
} ima_snmp_col_desc_t;


typedef union ima_snmp_col_val_tag {
    long val;
    char *ptr;
}   ima_snmp_col_val_t;


/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPCOLUMN_H_ */
