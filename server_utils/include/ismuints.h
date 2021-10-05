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

/** @file ismunits.h
 * Define standard integer types.
 * This file is normally included from ismutil.h
 */

#ifndef ISMUNITS_H_
#define ISMUNITS_H_

#ifdef _WIN32
#include <stddef.h>

#ifndef __INT8_T_DEFINED
#define __INT8_T_DEFINED
typedef signed char int8_t;
#endif

#ifndef __INT16_T_DEFINED
#define __INT16_T_DEFINED
typedef short int int16_t;
#endif

#ifndef __INT32_T_DEFINED
#define __INT32_T_DEFINED
typedef int int32_t;
#endif

#ifndef __INT64_T_DEFINED
#define __INT64_T_DEFINED
typedef __int64 int64_t;
#endif

#ifndef __UINT8_T_DEFINED
#define __UINT8_T_DEFINED
typedef unsigned char uint8_t;
#endif

#ifndef __UINT16_T_DEFINED
#define __UINT16_T_DEFINED
typedef unsigned short int uint16_t;
#endif

#ifndef __UINT32_T_DEFINED
#define __UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif

#ifndef __UINT64_T_DEFINED
#define __UINT64_T_DEFINED
typedef unsigned __int64 uint64_t;
#endif

#else

#include <inttypes.h>

#endif

#ifndef __ULL_T_DEFINED
#define __ULL_T_DEFINED
typedef unsigned long long ULL;
#endif

#endif /* ISMUNITS_H_ */
