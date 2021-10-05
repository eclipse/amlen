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
#ifndef __ISM_AUTHENTICATION_DEFINED
#define __ISM_AUTHENTICATION_DEFINED



#ifndef SA_ASSUME
#define SA_ASSUME(expression)
#endif
#include <security.h>
#include <ismutil.h>
#include <ldap.h>
#include <stdbool.h>


int ism_security_initAuthentication(ism_prop_t *props);

int ism_security_termAuthentication();

/**
 * OAuth Authentication.
 * @param sContext Security Context
 * @param token the token object
 * @return the RC which indicate whether authenticated or not.
 */
XAPI int ism_validate_oauth_token(ismSecurity_t *sContext, ismAuthToken_t * token);

XAPI int ism_security_oauth_update(ism_prop_t * props, char * oldName, ism_ConfigChangeType_t flag);
#endif
