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
/// @file  test_utils_debug.h
/// @brief Utility functions to be optionally included in product builds
//****************************************************************************
#ifndef __ISM_TEST_UTILS_DEBUG_H_DEFINED
#define __ISM_TEST_UTILS_DEBUG_H_DEFINED

#include <assert.h>

#define TEST_KILL_ON_MSG_CONTENT(msg, content)                          \
assert(((msg)->AreaCount < 2) ||                                        \
       ((msg)->AreaLengths[1] != strlen((content))) ||                  \
       (memcmp((msg)->pAreaData[1], (content), (msg)->AreaLengths[1])))

#endif /* __ISM_TEST_UTILS_DEBUG_H_DEFINED */
