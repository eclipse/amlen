/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
* Change Log:
* -------- ----------- ------------- ------------------------------------
* Reason:  Date:       Originator:   Comments:
* -------- ----------- ------------- ------------------------------------
*          10-04-07    Jerry Stevens File Creation
*/

#ifndef _CPHBUNDLE

#include "cphConfig.h"

/* Directory separators for the different platforms */
#if defined(WIN32)
#define CPH_DIRSEP '\\'
#elif defined(AMQ_UNIX)
#define CPH_DIRSEP '/'
#endif

/* Definition of the CPH_BUNDLE structure */
typedef struct _cphbundle {
    CPH_CONFIG *pConfig;        /* Pointer to the configuration control block */
    char clazzName[80];         /* The name of the module to which this CPH_BUNDLE applies */
    CPH_NAMEVAL *pNameValList;  /* A CPH_NAMEVAL linked list representing the entries in the property file */

} CPH_BUNDLE;

/* Function prototypes */
void *cphBundleLoad(CPH_CONFIG *pConfig, char *moduleName);
void cphBundleIni(CPH_BUNDLE **ppBundle, CPH_CONFIG *pConfig, char *moduleName);
int  cphBundleFree(CPH_BUNDLE **ppBundle);

#define _CPHBUNDLE
#endif
