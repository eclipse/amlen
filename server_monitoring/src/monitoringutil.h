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
#ifndef __ISM_MONUTIL_DEFINED
#define __ISM_MONUTIL_DEFINED

#include <monitoring.h>
#include <monserialization.h>
#include <endpointMonData.h>
#include <validateConfigData.h>
#include <admin.h>
#include <config.h>

#include <sys/time.h>

#define MONIT_EVENT_SIZE      ( sizeof (struct inotify_event) )
#define MONIT_EVENT_BUF_LEN   ( 1024 * ( MONIT_EVENT_SIZE + 16 ) )

/**
 * Get current time in sec
 */
ism_snaptime_t ism_monitoring_currentTimeSec(void);

/**
 * Get message ID
 */
XAPI void ism_monitoring_getMsgId(ism_rc_t code, char * buffer);

/*************************Sort Comparators Declaration *********************************/
/**
 * Comparator for the connection best time
 */
int ism_monitoring_sortComparatorConnTimeBest(const void *data1, const void *data2);
/**
 * Comparator for the connection worst time
 */
int ism_monitoring_sortComparatorConnTimeWorst(const void *data1, const void *data2);
/**
 * Comparator for the connection thruput bytes worst
 */
int ism_monitoring_sortComparatorTPutBytesWorst(const void *data1, const void *data2);
/**
 * Comparator for the connection thruput bytes best
 */
int ism_monitoring_sortComparatorTPutBytesBest(const void *data1, const void *data2);
/**
 * Comparator for the connection thruput messages best
 */
int ism_monitoring_sortComparatorTPutMsgBest(const void *data1, const void *data2);
/**
 * Comparator for the connection thruput messages worst
 */
int ism_monitoring_sortComparatorTPutMsgWorst(const void *data1, const void *data2);
/************************************End Sort ******************************************/
/**
 * This function constructs the Prefix for the external monitoring or alert message.
 * The prefix will contains the following:
 * -Node name
 * -Timestamp
 * -ObjectType
 * -ObjectName (if not NULL)
 *
 */
void ism_monitoring_getMsgExternalMonPrefix(ismMonitoringObjectType_t objType,
												 uint64_t currentTime,
												 const char * objectName,
												 concat_alloc_t * outbuf);
												 
/**
 * This function constructs the Topic String for External monitoring or Alert
 */
void ism_monitoring_getExtMonTopic(ismMonitoringObjectType_t objType, char *outputBuffer);

/**
 * Get the Monitoring Object Type String
 */
char * ism_monitoring_getMonObjectType(ismMonitoringObjectType_t objType);

/**
 * Create the JSON string using the error code and the error message
 */
int ism_monitoring_setReturnCodeAndStringJSON(concat_alloc_t *output_buffer, int rc, const char * returnString);

/* Handle RESTAPI GET mointoring data actions.
 * The return result or error will be in http->outbuf
 *
 * @param  procType     Process type - ISM_PROTYPE_SERVER or ISM_PROTYPE_MQC
 * @param  query_buf    The return buf to hold constructed JSON query string
 * @param  item         The monitoring object, such as Subscription, Queue
 * @param  locale       The query locale
 * @param  list         The query required item list generated from schema and also the enties default value
 * @param  props        The pass in query filter -- from http query parameters.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
//{ "Action":"Subscription","User":"admin","Locale":"en_US","ResultCount":"50","StatType":"PublishedMsgsHighest","SubName":"*","TopicString":"*","ClientID":"*","MessagingPolicy":"*","SubType":"All" }
int ism_monitoring_restapi_createQueryString(int procType, concat_alloc_t *query_buf, char *item, char *locale, ism_config_itemValidator_t * list, ism_prop_t *props);

int ism_monitoring_getStatType(char *actionString);

/*
 *
 * Handle RESTAPI get monitoring data actions.
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_monitoring_restapi_stateQuery(ism_http_t *http, ism_rest_api_cb callback);
#endif

