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

#ifndef _CPHCONFIG

#include <stdio.h>
#include "cphLog.h"
#include "cphTrace.h"
#include "cphStringBuffer.h"
#include "cphNameVal.h"

#define CPHCONFIG_UNSPECIFIED "UNSPECIFIED"

/* This is the name of the environment variable used to denote where cph has been installed */
#define CPH_INSTALLDIR "CPH_INSTALLDIR"

/* The length of memory we use for the install directory */
#define CPH_INSTALL_DIRLEN 256

#define CPH_DEFAULTOPTIONS_FILE "defaultOptions.txt"

typedef struct _cphconfig {
	char clazzName[80];
    char cphInstallDir[CPH_INSTALL_DIRLEN];
    int moduleType;
	CPH_LOG *pLog;
    CPH_ARRAYLIST *bundles;
	CPH_NAMEVAL *defaultOptions;
	CPH_NAMEVAL *options;
	int inValid;
    CPH_TRACE *pTrc;  // This is initialised by the cph main program
} CPH_CONFIG;

void cphConfigIni(CPH_CONFIG **ppConfig, CPH_TRACE *pTrc);
void cphConfigFree(CPH_CONFIG **ppConfig);
int cphConfigFreeBundles(CPH_CONFIG *pConfig);
void cphConfigReadCommandLine(CPH_CONFIG *pConfig, int argc, char *argv[]);
void cphConfigInvalidParameter(CPH_CONFIG *pConfig, char* error);
int  cphConfigGetInt(CPH_CONFIG *pConfig, int *res, char *in);
int  cphConfigGetString(CPH_CONFIG *pConfig, char *res, char *in);
int  cphConfigGetBoolean(CPH_CONFIG *pConfig, int *res, char *in);
int cphConfigStringToBoolean(int *res, char *aString);

int cphConfigContainsKey(CPH_CONFIG *pConfig, char *name);

int cphConfigIsInvalid(CPH_CONFIG *pConfig);
void cphConfigMarkLoaded(CPH_CONFIG *pConfig);

int cphConfigIsEmpty(CPH_CONFIG *pConfig);

int cphConfigStartTrace(CPH_CONFIG *pConfig);
int cphDisplayHelpIfNeeded(CPH_CONFIG *pConfig);
int	cphConfigValidateHelpOptions(CPH_CONFIG *pConfig);

char * cphConfigGetVersion(CPH_CONFIG *pConfigl, char *versionString);

char *cphConfigDescribe(CPH_CONFIG *pConfig, int inFull);
char *cphConfigDescribeModules(CPH_CONFIG *pConfig, char *module, int inFull);
int cphConfigDescribeModuleAsText(CPH_CONFIG *pConfig, CPH_STRINGBUFFER *pSb, void *pBundle, int inFull);

int cphConfigRegisterBaseModules(CPH_CONFIG *pConfig);
int cphConfigRegisterTCModule(CPH_CONFIG *pConfig);
int cphConfigRegisterModule(CPH_CONFIG *pConfig, char *moduleName);
void *cphConfigGetBundle(CPH_CONFIG *pConfig, char *moduleName);
int cphConfigImportResources(CPH_CONFIG *pConfig, void *pBundle);

#define _CPHCONFIG
#endif
