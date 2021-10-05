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

/*
 * GlobalRetainedStatsManager.h
 *  Created on: May 25, 2015
 */

#ifndef MCP_GLOBALRETAINEDSTATSMANAGER_H_
#define MCP_GLOBALRETAINEDSTATSMANAGER_H_

#include <string>
#include "cluster.h"
#include "SubCoveringFilterEventListener.h"
#include "MCPConfig.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

class GlobalRetainedStatsManager : public spdr::ScTraceContext
{
private:
    static spdr::ScTraceComponent* tc_;

public:
    GlobalRetainedStatsManager(const std::string& inst_ID, const MCPConfig& mcpConfig);
    virtual ~GlobalRetainedStatsManager();

    int onRetainedStatsChange(ismCluster_RemoteServerHandle_t node,
            const std::string& serverUID,
            SubCoveringFilterEventListener::RetainedStatsVector* retainedStats);

    int onRetainedStatsRemove(ismCluster_RemoteServerHandle_t node,
            const std::string& serverUID);

    MCPReturnCode lookupRetainedStats(const char *pServerUID,
            ismCluster_LookupRetainedStatsInfo_t **pLookupInfo);

    static MCPReturnCode freeRetainedStats(ismCluster_LookupRetainedStatsInfo_t *pLookupInfo);

private:
    typedef std::map<std::string,SubCoveringFilterEventListener::RetainedStatsVector* > RetainedMap;
    RetainedMap map;

};

} /* namespace mcp */

#endif /* MCP_GLOBALRETAINEDSTATSMANAGER_H_ */
