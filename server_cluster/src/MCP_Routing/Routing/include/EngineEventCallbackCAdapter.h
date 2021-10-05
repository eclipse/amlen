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

#ifndef SERVERREGISTRATIONCADAPTER_H_
#define SERVERREGISTRATIONCADAPTER_H_

#include <boost/thread/recursive_mutex.hpp>

#include "cluster.h"
#include <EngineEventCallback.h>


namespace mcp
{

class EngineEventCallbackCAdapter: public EngineEventCallback
{
public:
	EngineEventCallbackCAdapter(ismEngine_RemoteServerCallback_t callback, void* pContext);
	virtual ~EngineEventCallbackCAdapter();

	/*
	 * @see ServerRegistration
	 */
	virtual int createLocal(
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			ismEngine_RemoteServerHandle_t    *phEngineHandle);

	/*
	 * @see ServerRegistration
	 */
	virtual int add(
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			ismEngine_RemoteServerHandle_t    *phEngineHandle);
	/*
	 * @see ServerRegistration
	 */
	virtual int connected(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
		    ismCluster_RemoteServerHandle_t    hClusterHandle,
		    const char                        *pServerName,
		    const char                        *pServerUID);
	/*
	 * @see ServerRegistration
	 */
	virtual int disconnected(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
		    ismCluster_RemoteServerHandle_t    hClusterHandle,
		    const char                        *pServerName,
		    const char                        *pServerUID);
	/*
	 * @see ServerRegistration
	 */
	virtual int remove(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
		    ismCluster_RemoteServerHandle_t    hClusterHandle,
		    const char                        *pServerName,
		    const char                        *pServerUID);

	/*
	 * @see ServerRegistration
	 */
	virtual int route(
				ismEngine_RemoteServerHandle_t     hRemoteServer,
			    ismCluster_RemoteServerHandle_t    hClusterHandle,
			    const char                        *pServerName,
			    const char                        *pServerUID,
			    uint8_t                            fIsRoutAll);

	virtual int term();

	/*
	 * @see StoreNodeData
	 */
	virtual int update(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			void                              *pRemoteServerData,
			size_t                             remoteServerDataLength,
			uint8_t fCommitUpdate,
			ismEngine_RemoteServer_PendingUpdateHandle_t  *phPendingUpdateHandle);

	/*
	 * @see RemoteTopicTreeSubscriptionEvents
	 */
	virtual int addSubscriptions(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			char                             **pSubscriptions,
			size_t                             subscriptionsLength);

	/*
	 * @see RemoteTopicTreeSubscriptionEvents
	 */
	virtual int removeSubscriptions(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			char                             **pSubscriptions,
			size_t                             subscriptionsLength);

	/*
	 * @see EngineEventCallback
	 */
	virtual int reportEngineStatistics(ismCluster_EngineStatistics_t * pEngineStatistics);

	/**
	 * Unregistering the callback causes the adapter to close effectively preventing further callback invocations.
	 */
	void close();


private:
	ismEngine_RemoteServerCallback_t   remoteServerEventCallback;
	void* pCtx;
	boost::recursive_mutex mutex;
	bool closed;
};

} /* namespace mcp */

#endif /* SERVERREGISTRATIONCADAPTER_H_ */
