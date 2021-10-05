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

#ifndef __ISMMON_ENGINEMONDATA_DEFINED
#define __ISMMON_ENGINEMONDATA_DEFINED


#include <engine.h>
#include <ismrc.h>
#include <ismjson.h>
#include <ismmonitoringobjs.h>
#include <monserialization.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Engine statistics action Types
 */
typedef enum ismmon_engineStatActionType {
    ISMMON_ENGINE_STATE_ACTION_NONE    	        = 0,
    ISMMON_ENGINE_STATE_ACTION_SUBSCRIPTION   	= 1,
    ISMMON_ENGINE_STATE_ACTION_TOPIC   	        = 2,
    ISMMON_ENGINE_STATE_ACTION_QUEUE        	    = 3,
    ISMMON_ENGINE_STATE_ACTION_MQTTCLIENT        = 4,
    ISMMON_ENGINE_STATE_ACTION_TRANSACTION       = 5,
    ISMMON_ENGINE_STATE_ACTION_RESOURCESET       = 6
} ismmon_engineStatsActionType_t;


/**
 * Engine monitoring data action handler
 *
 * The monitoring actions are invoked by users from WebUI or CLI.
 * The WebUI or CLI, creates an action string in JSON format and sends the
 * request to ISM server process on Admin port, using MQTT WebSockets protocol.
 *
 * Statistics of the following objects are maintained by Engine component:
 * - Subscription
 * - Topic
 * - Queue
 * - MQTTClient
 * Engine provides a set of APIs to expose these monitoring data.
 *
 * The ISM server monitoring component calls these APIs to collect
 * the required statistics.
 */

XAPI int32_t ism_monitoring_getEngineStats (
        char               * action,
        ism_json_parse_t   * inputJSONObj,
        concat_alloc_t     * outputBuffer );


/**
 * Memory monitoring data action handler
 *
 * The monitoring actions are invoked by users from WebUI or CLI.
 * The WebUI or CLI, creates an action string in JSON format and sends the
 * request to ISM server process on Admin port, using MQTT WebSockets protocol.
 *
 * Statistics of memory are maintained by Engine component.
 * Engine provides a set of APIs to expose these monitoring data.
 *
 * The ISM server monitoring component calls these APIs to collect
 * the required statistics.
 */

XAPI int32_t ism_monitoring_getMemoryStats (
        char               * action,
        ism_json_parse_t   * inputJSONObj,
        concat_alloc_t     * outputBuffer,
        int isExternalMonitoring );


/**
 * Detailed memory monitoring data action handler
 *
 * The monitoring actions are invoked by users from WebUI or CLI.
 * The WebUI or CLI, creates an action string in JSON format and sends the
 * request to ISM server process on Admin port, using MQTT WebSockets protocol.
 *
 * Statistics of memory are maintained by Engine component.
 * Engine provides a set of APIs to expose these monitoring data.
 *
 * The ISM server monitoring component calls these APIs to collect
 * the required statistics.
 */

XAPI int32_t ism_monitoring_getDetailedMemoryStats (
        char               * action,
        ism_json_parse_t   * inputJSONObj,
        concat_alloc_t     * outputBuffer );


#ifdef __cplusplus
}
#endif

#endif
