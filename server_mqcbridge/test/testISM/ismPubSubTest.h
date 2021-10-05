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

/* Common include file for all test programs at the ISM end         */

#include <MQTTClient.h>

typedef struct _imbt_config_parameter_t
{
   char* pServerAddress;
   size_t serverAddressLength;
   size_t serverAddressSize;
   char* pClientId;
   size_t clientIdLength;
   size_t clientIdSize;
} imbt_config_parameter_t;
