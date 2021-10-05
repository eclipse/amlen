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

#ifndef FORWARDINGCONTROLCADAPTER_H_
#define FORWARDINGCONTROLCADAPTER_H_

#include <boost/thread/recursive_mutex.hpp>

#include <cluster.h>
#include <ForwardingControl.h>

namespace mcp
{

class ForwardingControlCAdapter: public ForwardingControl
{
public:
	ForwardingControlCAdapter(ismProtocol_RemoteServerCallback_t callback, void *pContext);

	virtual ~ForwardingControlCAdapter();

	int add(
			const char                        *pServerName,
			const char                        *pServerUID,
			const char                        *pRemoteServerAddress,
			int                                remoteServerPort,
			uint8_t                            fUseTLS,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			ismEngine_RemoteServerHandle_t     hEngineHandle,
			ismProtocol_RemoteServerHandle_t  *phProtocolHandle);

	int connect(
			ismProtocol_RemoteServerHandle_t   hRemoteServer,
			const char                        *pServerName,
			const char                        *pServerUID,
			const char                        *pRemoteServerAddress,
			int                                remoteServerPort,
			uint8_t                            fUseTLS,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			ismEngine_RemoteServerHandle_t     hEngineHandle);

	int disconnect(
			ismProtocol_RemoteServerHandle_t   hRemoteServer,
			const char                        *pServerName,
			const char                        *pServerUID,
			const char                        *pRemoteServerAddress,
			int                                remoteServerPort,
			uint8_t                            fUseTLS,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			ismEngine_RemoteServerHandle_t     hEngineHandle);


	int remove(
			ismProtocol_RemoteServerHandle_t   hRemoteServer,
			const char                        *pServerName,
			const char                        *pServerUID,
			const char                        *pRemoteServerAddress,
			int                                remoteServerPort,
			uint8_t                            fUseTLS,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			ismEngine_RemoteServerHandle_t     hEngineHandle);

	int term();

	/**
	 * Unregistering the callback causes the adapter to close effectively preventing further callback invocations.
	 */
	void close();

private:
	ismProtocol_RemoteServerCallback_t protocolCallback;
	void* pCtx;
	boost::recursive_mutex mutex;
	bool closed;
};

} /* namespace mcp */

#endif /* FORWARDINGCONTROLCADAPTER_H_ */
