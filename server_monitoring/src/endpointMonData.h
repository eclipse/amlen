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
#ifndef __ISM_ENDPOINTMONDATA_DEFINED
#define __ISM_ENDPOINTMONDATA_DEFINED

//typedef uint64_t ism_snaptime_t;         /**< Time in second of snap shot */

/*
 * Holding monitoring data for each snap shot.
 */
typedef struct ism_mondata_t ism_mondata_t;

/*
 * The snap shot range object is used to monitoring an endpoint object
 * Throughput will be calculated using bytes_count/interval
 */
typedef struct ism_srange_t ism_srange_t;


/*
 * monitoring object.
 * It will contains all aggregate data for an endpoint. It is up to the GUI/CLI to define
 * the intervals for each snapshot.
 * TODO: History data?
 */
typedef struct ism_monitoring_t  ism_monitoring_t;


/*
 * The main monitoring List that hold all the monitoring aggregation data
 */
typedef struct ism_monitoringList_t ism_monitoringList_t;

typedef enum {
    ismMon_NONE                = 0,
    ismMon_ENABLED             = 1,
    ismMon_ERR_CODE            = 2,
    ismMon_CONNECT_ACTIVE      = 3,
    ismMon_CONNECT_COUNT       = 4,
    ismMon_BAD_CONNECT_COUNT   = 5,
    ismMon_LOST_MSG_COUNT      = 6,
    ismMon_READ_MSG_COUNT      = 7,
    ismMon_WRITE_MSG_COUNT     = 8,
    ismMon_READ_BYTES_COUNT    = 9,
    ismMon_WRITE_BYTES_COUNT   = 10,
    ismMon_WARN_MSG_COUNT      = 11,
    ismMon_RECV_MSG_RATE       = 12,
    ismMon_SEND_MSG_RATE0      = 13,
    ismMon_SEND_MSG_RATE1      = 14,
    ismMon_CHANNEL_COUNT       = 15,
    ismMon_SEND_MSG_RATE       = 16,
} ismMonitoringDataType_t;

/*
 * Holding monitoring data for each snap shot.
 */
typedef struct ism_fwdmondata_t ism_fwdmondata_t;

/*
 * The snap shot range object is used to monitoring forwarder object
 * Throughput will be calculated using bytes_count/interval
 */
typedef struct ism_fwdrange_t ism_fwdrange_t;


/*
 * monitoring object.
 * It will contains all aggregate data for forwsrder. It is up to the GUI/CLI to define
 * the intervals for each snapshot.
 */
typedef struct ism_fwdmonitoring_t  ism_fwdmonitoring_t;

/*
 * The main monitoring List that hold all the monitoring aggregation data
 */
typedef struct ism_fwdmonitoringList_t ism_fwdmonitoringList_t;


XAPI int ism_monitoring_getEndpointMonData(char * action, ism_json_parse_t * parseobj, concat_alloc_t * output_buffer);
XAPI int ism_monitoring_initMonitoringList(void);
XAPI int ism_monitoring_checkAction(ism_snaptime_t curtime);
XAPI int ism_monitoring_initFwdMonitoringList(void);
XAPI int ism_monitoring_getForwarderMonData(char * action, ism_json_parse_t * parseobj, concat_alloc_t * output_buffer);
XAPI int ism_fwdmonitoring_checkAction(ism_snaptime_t curtime);


#endif
