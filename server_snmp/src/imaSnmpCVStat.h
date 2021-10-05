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
 * Note: this header file provides definitions for MessageSight ConnectionVolume stats,
 *       as well as the interface to access ConnectionVolume stats.
 */

#ifndef IMASNMPCVSTAT_H_
#define IMASNMPCVSTAT_H_

#include "imaSnmpColumn.h"

#define COLUMN_IMACV_DEFAULT_SIZE  100

#define COLUMN_IMACV_CONNACTIVE_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_CONNTOTAL_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_MSGREAD_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_MSGWRITE_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_BYTESREAD_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_BYTESWRITE_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_CONNBAD_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_ENDPOINTSTOTAL_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_MSGBUFFERED_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_MSGRETAINED_SIZE    COLUMN_IMACV_DEFAULT_SIZE
#define COLUMN_IMACV_MSGEXPIRED_SIZE    COLUMN_IMACV_DEFAULT_SIZE

typedef enum {
    imaSnmpCV_NONE          	    	= 0,
    imaSnmpCV_CONNECTION_ACTIVE    		= 1,   	//ActiveConnections
    imaSnmpCV_CONNECTION_TOTAL     		= 2,	//TotalConnections
    imaSnmpCV_MSG_READ    		= 3,	//MsgRead
    imaSnmpCV_MSG_WRITE	= 4,	//MsgWrite
    imaSnmpCV_BYTES_READ	= 5,	//BytesRead
    imaSnmpCV_BYTES_WRITE		= 6,	//BytesWrite
    imaSnmpCV_CONNECTION_BAD		= 7,	//BadConnCount
    imaSnmpCV_ENDPOINTS_TOTAL		= 8,	//TotalEndpoints
    imaSnmpCV_MSG_BUFFERED	= 9,	//BufferedMessages
    imaSnmpCV_MSG_RETAINED	= 10,	//RetainedMessages
    imaSnmpCV_MSG_EXPIRED	= 11,	//ExpiredMessages
    imaSnmpCV_Col_MAX
} imaSnmpCVDataType_t;

#define imaSnmpCV_Col_MIN imaSnmpCV_CONNECTION_ACTIVE

typedef struct ima_snmp_cv_tag {
    ima_snmp_col_val_t  cvItem[imaSnmpCV_Col_MAX];
    time_t  time_cvStats;
} ima_snmp_cv_t;

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int ima_snmp_get_cv_stat(char *buf, int len, imaSnmpCVDataType_t statType);

#ifdef __cplusplus
    }
#endif

#endif /* IMASNMPCVSTAT_H_ */
