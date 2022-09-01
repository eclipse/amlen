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
#ifndef __ISM_CRLPROFILE_DEFINED
#define __ISM_CRLPROFILE_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XAPI
    #define XAPI extern
#endif

#include <ismutil.h>

XAPI int ism_config_deleteCRLProfile(const char * crlProfileName);
XAPI int ism_config_purgeSecurityCRLProfile(const char * secProfileName);
XAPI int ism_config_updateSecurityCRLProfile(const char * securityProfile, const char * crlProfile);
XAPI int ism_config_updateCRLProfileForSecurity(const char * crlProfileName, const char * crlFileName);
XAPI int ism_config_sendCRLCurlRequest(char * url, char * destFilePath, int * rc);
XAPI int ism_config_startEndpointCRLTimer(const char * epname);
XAPI int ism_config_deleteEndpointCRLTimer(char * epname);
XAPI int ism_admin_executeCRLRevalidateOptionOneEndpoint(const char * epname);
XAPI int ism_admin_executeCRLRevalidateOptionAllEndpoints(const char * crlProfileName);
XAPI int ism_admin_executeCRLRevalidateOptionForSecurityProfile(const char * secProfileName);


#ifdef __cplusplus
}
#endif

#endif
