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

#ifndef MCP_MCPTYPES_H_
#define MCP_MCPTYPES_H_

#include <string>
#include <set>
#include <map>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

/* Forward declaration taken from cluster.h */
typedef struct ismCluster_RemoteServer_t  * ismCluster_RemoteServerHandle_t;

/* Forward declaration taken from cluster.h */
typedef struct ismEngine_RemoteServer_t   * ismEngine_RemoteServerHandle_t;

/* Forward declaration taken from cluster.h */
typedef struct ismEngine_RemoteServer_PendingUpdate_t  * ismEngine_RemoteServer_PendingUpdateHandle_t;

/* Forward declaration taken from cluster.h */
typedef struct ismProtocol_RemoteServer_t * ismProtocol_RemoteServerHandle_t;

namespace mcp {

/**
 * An index that uniquely identifies a remote server within the context of the local server.
 *
 * Indexes of nodes removed from the cluster may be recycled.
 * The index range is compact - it can be used to address arrays.
 *
 * @see ServerRegistration
 */
//typedef void* ServerIndex;
typedef uint16_t ServerIndex;

/**
 * Another name for std::string.
 */
typedef std::string String;

/**
 * Another name for a shared pointer to std::string.
 */
typedef boost::shared_ptr<String> String_SPtr;

enum MCPState
{
	STATE_INIT,
	STATE_STARTED,
	STATE_RECOVERED,
	STATE_ACTIVE,
	STATE_CLOSED,
	STATE_CLOSED_DETACHED,
	STATE_ERROR
};


} /* namespace mcp */

#endif /* MCP_MCPTYPES_H_ */
