/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
 */
#ifndef __ISM_CLIENT_H_DEFINED
#define __ISM_CLIENT_H_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <ismmonitoringobjs.h>
#include <stdarg.h>
#include <ismutil.h>

/**
 * Monitoring Type
 */
typedef enum {
	ismMon_NONE_TYPE                        = 0,
	ismMon_LISTENER_TYPE	                = 1,
	ismMon_CONNECTION_TYPE	                = 2,
	ismMon_LISTENER_HISTORICAL_TYPE	        = 3,
	ismMon_SUBSCRIPTION_TYPE                = 4,
	ismMon_TOPIC_TYPE                       = 5,
	ismMon_QUEUE_TYPE                       = 6,
	ismMon_MQTT_TYPE                        = 7,
	ismMon_DESTMAPRULE_TYPE                 = 8,
	ismMon_TRANSACTION_TYPE                 = 9,
	ismMon_MEMORY_TYPE                		= 10,
	ismMon_MEMORY_HISTORICAL_TYPE        	= 11,
	ismMon_STORE_TYPE			        	= 12,
	ismMon_STORE_HISTORICAL_TYPE        	= 13,
	ismMon_STORE_MEM_DETAIL_TYPE        	= 14,
} ismMonitoringType_t;

/**
 * Monitoring Output Format
 */
typedef enum {
	ismMon_CSV_FORMAT		                = 0, 
	ismMon_JSON_FORMAT		                = 1,
} ismMonitoringOutputFormat_t;

#define PROTYPE_SERVER  0
#define PROCTYPE_MQC    1
#define PROCTYPE_TRACE  2
#define PROCTYPE_SNMP   3

#define MAX_JSON_ENTRIES 2000


#define ismCLI_FREE(x)   if (x != NULL) { ism_common_free(ism_memory_snmp_misc,(void *)x); x = NULL; }

XAPI char * ismcli_ISMClient(char *user, char *protocol, char *command, int proctype);



#ifdef __cplusplus
}
#endif

#endif

