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
 * monitoringSnmpTrap.h
 *
 *  Created on: Oct 16, 2015
 */

#ifndef SRC_MONITORINGSNMPTRAP_H_
#define SRC_MONITORINGSNMPTRAP_H_

typedef enum {
    imaSnmpTopic_NONE          	    	= 0,
    imaSnmpTopic_SERVER    		= 1,   	//IMA $SYS/ResourceStatistics/Server topic
    imaSnmpTopic_MEMORY      		= 2,	//$SYS/ResourceStatistics/Memory topic
    imaSnmpTopic_STORE    		= 3,	//$SYS/ResourceStatistics/Store  topic
    imaSnmpTopic_TOPIC			= 4,	//$SYS/ResourceStatistics/Topic  topic
    imaSnmpTopic_ENDPOINT		= 5,	//$SYS/ResourceStatistics/Endpoint topic
    imaSnmpTopic_ALL			= 10,	//$SYS/ResourceStatistics/

} imaSnmpTopicType_t;


#define IMA_SYS_ROOT  "$SYS/ResourceStatistics/"

#define IMA_SYS_SERVER "Server"
#define IMA_SYS_MEMORY "Memory"
#define IMA_SYS_STORE  "Store"
#define IMA_SYS_TOPIC  "Topic"
#define IMA_SYS_ENDPOINT  "Endpoint"

#define QOS_0	0
#define QOS_1	1
#define QOS_2	2

#define CONN_CYCLE  10000  //10ms
#define MAX_CONN_CYCLE 100000000  //100s
#define CONN_SLEEP_CYCLE 60 //1min

#define _SNMP_THREADED_
/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

typedef int (*imaSnmpEvent_msgHandler_t)(ism_json_parse_t *pDataObj);

#ifdef _SNMP_THREADED_
int imaSnmp_topic_handler_register(int topicType, imaSnmpEvent_msgHandler_t msgHandler);
int imaSnmp_subscribe(int topicType);
int imaSnmp_unsubscribe(int topicType);
XAPI int imaSnmp_messageArrived(char* topicName, const char *msg, int msgLen);
#endif

#ifdef __cplusplus
    }
#endif

#endif /* SRC_MONITORINGSNMPTRAP_H_ */
