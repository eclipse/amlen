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
#ifndef __ISM_STOREMONDATA_DEFINED
#define __ISM_STOREMONDATA_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Basic store monitoring data
 */
XAPI int ism_monitoring_getStoreStats(char * action, ism_json_parse_t * inputJSONObj, concat_alloc_t * outputBuffer, int isExternalMonitoring);


/**
 * Returns HA monitoring data
 */
XAPI int ism_monitoring_getHAStats(char * action, ism_json_parse_t * inputJSONObj, concat_alloc_t * outputBuffer);



#ifdef __cplusplus
}
#endif

#endif

