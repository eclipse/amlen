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

//****************************************************************************
/// @file  engineTimers.h
/// @brief Module for Engine timer tasks header file
//****************************************************************************
#ifndef __ISM_ENGINETIMERS_DEFINED
#define __ISM_ENGINETIMERS_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/**********************************************************************/
#include <engine.h>           /* Engine external header file         */
#include <engineCommon.h>     /* Engine common internal header file  */
#include <stdint.h>           /* Standard integer defns header file  */

///> Amount of time we're willing to wait for timers at shutdown.
#define ietmMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS 60

/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/

/*
 * Set up the Engine's periodic timers
 */
int32_t ietm_setUpTimers(void);

int ietm_updateServerTimestamp(ism_timer_t key, ism_time_t timestamp, void * userdata);
int ietm_updateRetMinActiveOrderId(ism_timer_t key, ism_time_t timestamp, void * userdata);
int ietm_syncClusterRetained(ism_timer_t key, ism_time_t timestamp, void * userdata);

/*
 * Clean up the Engine's periodic timers
 */
void ietm_cleanUpTimers(void);

#endif /* __ISM_ENGINETIMERS_DEFINED */

/*********************************************************************/
/* End of engineTimers.h                                             */
/*********************************************************************/
