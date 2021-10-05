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

#ifndef MCP_LOCALMONITORINGMANAGER_H_
#define MCP_LOCALMONITORINGMANAGER_H_


#include "MCPConfig.h"
#include "LocalSubManager.h"
#include "SubCoveringFilterPublisher.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

class LocalMonitoringManager: public spdr::ScTraceContext
{
private:
    static spdr::ScTraceComponent* tc_;

public:
    LocalMonitoringManager(const std::string& inst_ID,
            const MCPConfig& mcpConfig, LocalSubManager& localSubManager);

    virtual ~LocalMonitoringManager();

    MCPReturnCode setSubCoveringFilterPublisher(
            SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher);

    MCPReturnCode start();

    MCPReturnCode recoveryCompleted();

    MCPReturnCode close();

    MCPReturnCode setHealthStatus(ismCluster_HealthStatus_t  healthStatus);

    MCPReturnCode setHaStatus(ismCluster_HaStatus_t haStatus);

    ismCluster_HealthStatus_t getHealthStatus();

    ismCluster_HaStatus_t getHaStatus();

    MCPReturnCode publishMonitoringStatus();

private:
    const MCPConfig& config_;
    LocalSubManager& localSubManager_;
    SubCoveringFilterPublisher_SPtr filterPublisher_;

    //State
    bool started_;
    bool closed_;
    bool recovered_;

    //Status
    ismCluster_HealthStatus_t  healthStatus_;
    ismCluster_HaStatus_t haStatus_;

    uint64_t sqn_;
};

} /* namespace mcp */

#endif /* LOCALMONITORINGMANAGER_H_ */
