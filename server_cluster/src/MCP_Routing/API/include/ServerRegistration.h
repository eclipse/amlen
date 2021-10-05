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

#ifndef MCP_SERVERREGISTRATION_H_
#define MCP_SERVERREGISTRATION_H_

#include "NodeID.h"
#include "MCPTypes.h"

namespace mcp
{

/******************************************************************************
 * An interface for registering remote servers into the engine.
 *
 * Each newly discovered remote server will be assigned a cluster handle,
 * and added into the engine, where it is assigned a engine handle. The pair of handles will be
 * used in all subsequent interaction between MCP and the engine, in particular,
 * during route lookup.
 *
 * A newly discovered server is in a "disconnected" state.
 * Routing will then wait for the forwarding component to signal the node is
 * connected to forwarding address of that node. Routing will then invoke the
 * "connected" call to the engine.
 *
 * When either the control unit discovers a "leave" event, or the forwarding
 * unit discovers a disruption, the "disconnected" method will be invoked.
 *
 * If the disconnection is due to the forwarding unit, then if the forwarding
 * unit regains connectivity, it will notify MCPRouting, and the "connected"
 * will be invoked.
 *
 * When a remote server that was already added (and thus has an index) rejoins
 * the cluster, a "connected" will be called, with the same UID and index.
 *
 * Remove means removing the server permanently form the cluster.
 *
 * Summary:
 * Add - join of a new node
 * Connect - after both control and forwarding are connected
 * Disconnect - if control and/or forwarding are disconnected
 * Remove - permanent leave (currently not implemented)
 */
class ServerRegistration
{
public:
	ServerRegistration();
	virtual ~ServerRegistration();

	virtual int createLocal(
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			ismEngine_RemoteServerHandle_t    *phEngineHandle) = 0;

	/**
	 * A remote server joins the cluster for the first time.
	 *
	 * The remote server is uniquely identified by the node name string.
	 * The Routing component provides the ismCluster_RemoteServerHandle_t, which is used in all subsequent interactions with the engine.
	 * The Engine provides the ismEngine_RemoteServerHandle_t, which is used in all subsequent interactions with the engine.
	 *
	 * This method is called on the first time a server joins.
	 *
	 * @param hClusterHandle a cluster handle to the remote server
	 * @param pServerName remote server name
	 * @param phEngineHandle (output) an engine handle to the remote server
	 * @return ISMRC_OK if success, error code from MCPReturnCode otherwise
	 */
	virtual int add(
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			ismEngine_RemoteServerHandle_t    *phEngineHandle) = 0;

	/**
	 * When a remote server is connected via control and forwarding
	 *
	 * @param hRemoteServer
	 * @param hClusterHandle
	 * @param pServerName
	 * @return ISMRC_OK if success, error code from MCPReturnCode otherwise
	 */
	virtual int connected(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
		    ismCluster_RemoteServerHandle_t    hClusterHandle,
		    const char                        *pServerName,
		    const char                        *pServerUID) = 0;

	/**
	 * When a remote server is disconnected from control and/or forwarding, temporarily.
	 *
	 * A remote server leaves the cluster temporarily.
	 * The server is expected to return, engine should keep the state associated with this server.
	 *
	 * @param hRemoteServer
	 * @param hClusterHandle
	 * @param pServerName
	 * @return ISMRC_OK if success, error code from MCPReturnCode otherwise
	 */
	virtual int disconnected(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
		    ismCluster_RemoteServerHandle_t    hClusterHandle,
		    const char                        *pServerName,
		    const char                        *pServerUID) = 0;

	/**
	 * Remove a server from the cluster permanently, deleting all associated state.
	 *
	 * @param hRemoteServer
	 * @param hClusterHandle
	 * @param pServerName
	 * @return ISMRC_OK if success, error code from MCPReturnCode otherwise
	 */
	virtual int remove(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
		    ismCluster_RemoteServerHandle_t    hClusterHandle,
		    const char                        *pServerName,
		    const char                        *pServerUID) = 0;

	/**
	 * Signal the engine whether to route all publications to this remote server.
	 *
	 * @param hRemoteServer
	 * @param hClusterHandle
	 * @param pServerName
	 * @param pServerUID
	 * @param fIsRoutAll
	 * @return
	 */
	virtual int route(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
		    ismCluster_RemoteServerHandle_t    hClusterHandle,
		    const char                        *pServerName,
		    const char                        *pServerUID,
		    uint8_t                            fIsRoutAll) = 0;

	/**
	 * Signal the engine that the cluster components was terminated.
	 *
	 * @return
	 */
	virtual int term() = 0;
};

}

#endif /* MCP_SERVERREGISTRATION_H_ */
