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
 * LocalRetainedStatsManager.h
 *  Created on: May 21, 2015
 */

#ifndef MCP_LOCALRETAINEDMSGMANAGER_H_
#define MCP_LOCALRETAINEDMSGMANAGER_H_

#include "MCPConfig.h"
#include "LocalSubManager.h"
#include "SubCoveringFilterPublisher.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

class LocalRetainedStatsManager : public spdr::ScTraceContext
{
private:
    static spdr::ScTraceComponent* tc_;

public:
    LocalRetainedStatsManager(const std::string& inst_ID,
            const MCPConfig& mcpConfig, LocalSubManager& localSubManager);
    virtual ~LocalRetainedStatsManager();

    MCPReturnCode setSubCoveringFilterPublisher(
            SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher);

    MCPReturnCode start();

    MCPReturnCode recoveryCompleted();

    MCPReturnCode close();

    MCPReturnCode updateRetainedStats(const char *pServerUID,
               void *pData, uint32_t dataLength);

    MCPReturnCode publishRetainedStats();

private:
    const MCPConfig& config;
    LocalSubManager& localSubManager;
    SubCoveringFilterPublisher_SPtr filterPublisher;

    //State
    bool m_started;
    bool m_closed;
    bool m_recovered;

    typedef SubCoveringFilterPublisher::RetainedStatsMap RetainedMap;
    RetainedMap map;
    uint64_t sqn;

};

} /* namespace mcp */

#endif /* MCP_LOCALRETAINEDMSGMANAGER_H_ */
