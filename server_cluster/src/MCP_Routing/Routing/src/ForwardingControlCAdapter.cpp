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

#include <stdint.h>

#include "MCPReturnCode.h"
#include <ForwardingControlCAdapter.h>

namespace mcp
{

ForwardingControlCAdapter::ForwardingControlCAdapter(ismProtocol_RemoteServerCallback_t callback, void *pContext) :
		protocolCallback(callback), pCtx(pContext), mutex(), closed(false)
{
}

ForwardingControlCAdapter::~ForwardingControlCAdapter()
{
}


int ForwardingControlCAdapter::add(
		const char                        *pServerName,
		const char                        *pServerUID,
		const char                        *pRemoteServerAddress,
		int                                remoteServerPort,
		uint8_t                            fUseTLS,
		ismCluster_RemoteServerHandle_t    hClusterHandle,
		ismEngine_RemoteServerHandle_t     hEngineHandle,
		ismProtocol_RemoteServerHandle_t  *phProtocolHandle)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

	if (protocolCallback != NULL)
	{
		return protocolCallback(
				PROTOCOL_RS_CREATE,
				NULL,
				pServerName,
				pServerUID,
				pRemoteServerAddress,
				remoteServerPort,
				fUseTLS,
				hClusterHandle,
				hEngineHandle,
				pCtx,
				phProtocolHandle);
	}
	else
	{
		return ISMRC_NullPointer;
	}

}

int ForwardingControlCAdapter::connect(
		ismProtocol_RemoteServerHandle_t   hRemoteServer,
		const char                        *pServerName,
		const char                        *pServerUID,
		const char                        *pRemoteServerAddress,
		int                                remoteServerPort,
		uint8_t                            fUseTLS,
		ismCluster_RemoteServerHandle_t    hClusterHandle,
		ismEngine_RemoteServerHandle_t     hEngineHandle)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

	if (protocolCallback != NULL)
	{
		return protocolCallback(
				PROTOCOL_RS_CONNECT,
				hRemoteServer,
				pServerName,
				pServerUID,
				pRemoteServerAddress,
				remoteServerPort,
				fUseTLS,
				hClusterHandle,
				hEngineHandle,
				pCtx,
				NULL);
	}
	else
	{
		return ISMRC_NullPointer;
	}
}


int ForwardingControlCAdapter::disconnect(
		ismProtocol_RemoteServerHandle_t   hRemoteServer,
		const char                        *pServerName,
		const char                        *pServerUID,
		const char                        *pRemoteServerAddress,
		int                                remoteServerPort,
		uint8_t                            fUseTLS,
		ismCluster_RemoteServerHandle_t    hClusterHandle,
		ismEngine_RemoteServerHandle_t     hEngineHandle)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

	if (protocolCallback != NULL)
	{
		return protocolCallback(
				PROTOCOL_RS_DISCONNECT,
				hRemoteServer,
				pServerName,
				pServerUID,
				pRemoteServerAddress,
				remoteServerPort,
				fUseTLS,
				hClusterHandle,
				hEngineHandle,
				pCtx,
				NULL);
	}
	else
	{
		return ISMRC_NullPointer;
	}
}



int ForwardingControlCAdapter::remove(
		ismProtocol_RemoteServerHandle_t   hRemoteServer,
		const char                        *pServerName,
		const char                        *pServerUID,
		const char                        *pRemoteServerAddress,
		int                                remoteServerPort,
		uint8_t                            fUseTLS,
		ismCluster_RemoteServerHandle_t    hClusterHandle,
		ismEngine_RemoteServerHandle_t     hEngineHandle)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

	if (protocolCallback != NULL)
	{
		return protocolCallback(
				PROTOCOL_RS_REMOVE,
				hRemoteServer,
				pServerName,
				pServerUID,
				pRemoteServerAddress,
				remoteServerPort,
				fUseTLS,
				hClusterHandle,
				hEngineHandle,
				pCtx,
				NULL);
	}
	else
	{
		return ISMRC_NullPointer;
	}
}

int ForwardingControlCAdapter::term()
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (protocolCallback != NULL)
    {
        return protocolCallback(
                PROTOCOL_RS_TERM,
                NULL,
                NULL,
                NULL,
                NULL,
                0,
                0,
                NULL,
                NULL,
                pCtx,
                NULL);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}


void ForwardingControlCAdapter::close()
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    closed = true;
}
} /* namespace mcp */
