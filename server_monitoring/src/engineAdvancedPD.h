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

#ifndef __ISMMON_ENGINEADVPD_DEFINED
#define __ISMMON_ENGINEADVPD_DEFINED


#include <engine.h>
#include <ismrc.h>
#include <ismjson.h>
#include <ismmonitoringobjs.h>
#include <monserialization.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Engine advanced PD action Types
 */
typedef enum ismmon_engineAdvPDActionType {
    ISMMON_ENGINE_ADVPD_ACTION_NONE    	      = 0,
    ISMMON_ENGINE_ADVPD_ACTION_DUMPTOPIC   	  = 1,
    ISMMON_ENGINE_ADVPD_ACTION_DUMPTOPICTREE  = 2,
    ISMMON_ENGINE_ADVPD_ACTION_DUMPQUEUE      = 3,
    ISMMON_ENGINE_ADVPD_ACTION_DUMPCLIENT     = 4,
    ISMMON_ENGINE_ADVPD_ACTION_DUMPLOCKS      = 5,
} ismmon_engineAdvPDActionType_t;


/**
 * Engine Advanced PD action handler
 */

XAPI int32_t ism_monitoring_getAdvancedEnginePDData (
        char               * action,
        ism_json_parse_t   * inputJSONObj,
        concat_alloc_t     * outputBuffer );


#ifdef __cplusplus
}
#endif

#endif
