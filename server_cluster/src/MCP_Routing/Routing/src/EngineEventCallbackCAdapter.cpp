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

#include <MCPReturnCode.h>
#include <EngineEventCallbackCAdapter.h>

namespace mcp
{

EngineEventCallbackCAdapter::EngineEventCallbackCAdapter(
		ismEngine_RemoteServerCallback_t callback, void* pContext) :
		remoteServerEventCallback(callback), pCtx(pContext), mutex(), closed(false)
{
}

EngineEventCallbackCAdapter::~EngineEventCallbackCAdapter()
{
}

/*
 * @see ServerRegistration
 */
int EngineEventCallbackCAdapter::createLocal(
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			ismEngine_RemoteServerHandle_t    *phEngineHandle)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;


	if (remoteServerEventCallback != NULL)
	{
		return remoteServerEventCallback(
				ENGINE_RS_CREATE_LOCAL,
				NULL,
				hClusterHandle,
				pServerName,
				pServerUID,
				NULL, 0,
				NULL, 0,
				0,
				0, NULL,
				NULL,
				pCtx,
				phEngineHandle);
	}
	else
	{
		return ISMRC_NullPointer;
	}
}

/*
 * @see ServerRegistration
 */
int EngineEventCallbackCAdapter::add(
		ismCluster_RemoteServerHandle_t hClusterHandle,
		const char *pServerName,
		const char                        *pServerUID,
		ismEngine_RemoteServerHandle_t *phEngineHandle)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (remoteServerEventCallback != NULL)
    {
        return remoteServerEventCallback(
                ENGINE_RS_CREATE,
                NULL,
                hClusterHandle,
                pServerName,
                pServerUID,
                NULL, 0,
                NULL, 0,
                0,
                0, NULL,
                NULL,
                pCtx,
                phEngineHandle);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

/*
 * @see ServerRegistration
 */
int EngineEventCallbackCAdapter::connected(
		ismEngine_RemoteServerHandle_t hRemoteServer,
		ismCluster_RemoteServerHandle_t hClusterHandle,
		const char *pServerName,
		const char *pServerUID)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (remoteServerEventCallback != NULL)
    {
        return remoteServerEventCallback(
                ENGINE_RS_CONNECT,
                hRemoteServer,
                hClusterHandle,
                pServerName,
                pServerUID,
                NULL, 0,
                NULL, 0,
                0,
                0, NULL,
                NULL,
                pCtx,
                NULL);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

/*
 * @see ServerRegistration
 */
int EngineEventCallbackCAdapter::disconnected(
		ismEngine_RemoteServerHandle_t hRemoteServer,
		ismCluster_RemoteServerHandle_t hClusterHandle, const char *pServerName,
		const char *pServerUID)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (remoteServerEventCallback != NULL)
    {
        return remoteServerEventCallback(
                ENGINE_RS_DISCONNECT,
                hRemoteServer,
                hClusterHandle,
                pServerName,
                pServerUID,
                NULL, 0,
                NULL, 0,
                0,
                0, NULL,
                NULL,
                pCtx,
                NULL);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

/*
 * @see ServerRegistration
 */
int EngineEventCallbackCAdapter::remove(
		ismEngine_RemoteServerHandle_t hRemoteServer,
		ismCluster_RemoteServerHandle_t hClusterHandle,
		const char *pServerName,
		const char *pServerUID)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (remoteServerEventCallback != NULL)
    {
        return remoteServerEventCallback(
                ENGINE_RS_REMOVE,
                hRemoteServer,
                hClusterHandle,
                pServerName,
                pServerUID,
                NULL, 0,
                NULL, 0,
                0,
                0, NULL,
                NULL,
                pCtx,
                NULL);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

/*
 * @see ServerRegistration
 */
int EngineEventCallbackCAdapter::route(
		ismEngine_RemoteServerHandle_t     hRemoteServer,
		ismCluster_RemoteServerHandle_t    hClusterHandle,
		const char                        *pServerName,
		const char                        *pServerUID,
		uint8_t                            fIsRoutAll)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (remoteServerEventCallback != NULL)
    {
        return remoteServerEventCallback(
                ENGINE_RS_ROUTE,
                hRemoteServer,
                hClusterHandle,
                pServerName,
                pServerUID,
                NULL, 0,
                NULL, 0,
                fIsRoutAll,
                0, NULL,
                NULL,
                pCtx,
                NULL);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}


/*
 * @see StoreNodeData
 */
int EngineEventCallbackCAdapter::update(
		ismEngine_RemoteServerHandle_t hRemoteServer,
		ismCluster_RemoteServerHandle_t hClusterHandle,
		const char *pServerName,
		const char *pServerUID,
		void *pRemoteServerData, size_t remoteServerDataLength,
		uint8_t fCommitUpdate,
		ismEngine_RemoteServer_PendingUpdateHandle_t  *phPendingUpdateHandle)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (remoteServerEventCallback != NULL)
    {
        return remoteServerEventCallback(
                ENGINE_RS_UPDATE,
                hRemoteServer,
                hClusterHandle,
                pServerName,
                pServerUID,
                pRemoteServerData, remoteServerDataLength,
                NULL, 0,
                0,
                fCommitUpdate, phPendingUpdateHandle,
                NULL,
                pCtx,
                NULL);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

/*
 * @see RemoteTopicTreeSubscriptionEvents
 */
int EngineEventCallbackCAdapter::addSubscriptions(
		ismEngine_RemoteServerHandle_t hRemoteServer,
		ismCluster_RemoteServerHandle_t hClusterHandle,
		const char *pServerName,
		const char *pServerUID,
		char **pSubscriptions,
		size_t subscriptionsLength)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (remoteServerEventCallback != NULL)
    {
        return remoteServerEventCallback(
                ENGINE_RS_ADD_SUB,
                hRemoteServer,
                hClusterHandle,
                pServerName,
                pServerUID,
                NULL, 0,
                pSubscriptions, subscriptionsLength,
                0,
                0, NULL,
                NULL,
                pCtx,
                NULL);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

/*
 * @see RemoteTopicTreeSubscriptionEvents
 */
int EngineEventCallbackCAdapter::removeSubscriptions(
		ismEngine_RemoteServerHandle_t hRemoteServer,
		ismCluster_RemoteServerHandle_t hClusterHandle,
		const char *pServerName,
		const char *pServerUID,
		char **pSubscriptions,
		size_t subscriptionsLength)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (remoteServerEventCallback != NULL)
    {
        return remoteServerEventCallback(
                ENGINE_RS_DEL_SUB,
                hRemoteServer,
                hClusterHandle,
                pServerName,
                pServerUID,
                NULL, 0,
                pSubscriptions, subscriptionsLength,
                0,
                0, NULL,
                NULL,
                pCtx,
                NULL);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

int EngineEventCallbackCAdapter::reportEngineStatistics(ismCluster_EngineStatistics_t * pEngineStatistics)
{
    boost::recursive_mutex::scoped_lock lock(mutex);
        if (closed)
            return ISMRC_OK;

        if (remoteServerEventCallback != NULL)
        {
            return remoteServerEventCallback(
                    ENGINE_RS_REPORT_STATS,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL, 0,
                    NULL, 0,
                    0,
                    0, NULL,
                    pEngineStatistics,
                    pCtx,
                    NULL);
        }
        else
        {
            return ISMRC_NullPointer;
        }
}

int EngineEventCallbackCAdapter::term()
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    if (closed)
        return ISMRC_OK;

    if (remoteServerEventCallback != NULL)
    {
        return remoteServerEventCallback(
                ENGINE_RS_TERM,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL, 0,
                NULL, 0,
                0,
                0, NULL,
                NULL,
                pCtx,
                NULL);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

void EngineEventCallbackCAdapter::close()
{
    boost::recursive_mutex::scoped_lock lock(mutex);
    closed = true;
}


} /* namespace mcp */
