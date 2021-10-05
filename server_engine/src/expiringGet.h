/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
/// @file  expiringGet.h
/// @brief Get a single message (without an external consumer) with a TimeOut
//****************************************************************************
#ifndef __ISM_EXPIRINGGET_DEFINED
#define __ISM_EXPIRINGGET_DEFINED

#include "engineCommon.h"
#include "engineInternal.h"

void iegiConsumerRecursivelyDestroyed(ieutThreadData_t *pThreadData, ismEngine_Consumer_t *pConsumer);

#endif
