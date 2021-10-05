/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

#ifndef REMOTESERVERINFO_H_
#define REMOTESERVERINFO_H_


#ifdef __cplusplus
#include "MCPTypes.h"
typedef struct ismCluster_RemoteServer_t * ismCluster_RemoteServerHandle_t;
extern "C"
{
#else
#include <stdint.h>
#endif

struct ismCluster_RemoteServer_t
{
	uint16_t index;

	ismEngine_RemoteServerHandle_t engineHandle;

	ismProtocol_RemoteServerHandle_t protocolHandle;

	/* is deleted? 0 = false, >0 = true */
	uint8_t deletedFlag;
};


#ifdef __cplusplus
}
#endif

#endif /* REMOTESERVERINFO_H_ */
