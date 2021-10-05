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

#include <boost/lexical_cast.hpp>
#include <LocalMonitoringManager.h>

namespace mcp
{

spdr::ScTraceComponent* LocalMonitoringManager::tc_ = spdr::ScTr::enroll(
        mcp::trace::Component_Name,
        mcp::trace::SubComponent_Local,
        spdr::trace::ScTrConstants::Layer_ID_App, "LocalMonitoringManager",
        spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

LocalMonitoringManager::LocalMonitoringManager(const std::string& inst_ID,
        const MCPConfig& mcpConfig, LocalSubManager& localSubManager) :
                spdr::ScTraceContext(tc_, inst_ID, ""),
                config_(mcpConfig),
                localSubManager_(localSubManager),
                filterPublisher_(),
                started_(false),
                closed_(false),
                recovered_(false),
                healthStatus_(ISM_CLUSTER_HEALTH_UNKNOWN),
                haStatus_(ISM_CLUSTER_HA_UNKNOWN),
                sqn_(0)
{
    Trace_Entry(this, __FUNCTION__);
}

LocalMonitoringManager::~LocalMonitoringManager()
{
    Trace_Entry(this, __FUNCTION__);
}

MCPReturnCode LocalMonitoringManager::setSubCoveringFilterPublisher(
        SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher)
{
    Trace_Entry(this, __FUNCTION__);

    if (subCoveringFilterPublisher)
    {
        filterPublisher_ = subCoveringFilterPublisher;
        return ISMRC_OK;
    }
    else
    {
        Trace_Error(this, __FUNCTION__, "Error: subCoveringFilterPublisher is NULL", "RC", ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }
}

MCPReturnCode LocalMonitoringManager::start()
{
    Trace_Entry(this, __FUNCTION__);
    started_ = true;
    return ISMRC_OK;
}


MCPReturnCode LocalMonitoringManager::recoveryCompleted()
{
    Trace_Entry(this, __FUNCTION__);
    recovered_ = true;
    localSubManager_.schedulePublishMonitoringTask(0);
    return ISMRC_OK;
}

MCPReturnCode LocalMonitoringManager::close()
{
    Trace_Entry(this, __FUNCTION__);
    closed_ = true;
    return ISMRC_OK;
}


MCPReturnCode LocalMonitoringManager::setHealthStatus(ismCluster_HealthStatus_t  healthStatus)
{
    Trace_Entry(this, __FUNCTION__, "status", boost::lexical_cast<std::string>(healthStatus));

    if (healthStatus_ != healthStatus)
    {
        Trace_Debug(this, __FUNCTION__, "changed",
                "new-healthStatus", boost::lexical_cast<std::string>(healthStatus),
                "old-healthStatus", boost::lexical_cast<std::string>(healthStatus_));
        healthStatus_ = healthStatus;
        if (started_ && recovered_ && !closed_)
        {
            localSubManager_.schedulePublishMonitoringTask(0);
        }
    }

    return ISMRC_OK;
}


MCPReturnCode LocalMonitoringManager::setHaStatus(ismCluster_HaStatus_t haStatus)
{
    Trace_Entry(this, __FUNCTION__, "status", boost::lexical_cast<std::string>(haStatus));

    if (haStatus_ != haStatus)
    {
        Trace_Debug(this, __FUNCTION__, "changed",
                "new-HaStatus", boost::lexical_cast<std::string>(haStatus),
                "old-HaStatus", boost::lexical_cast<std::string>(haStatus_));

        haStatus_ = haStatus;
        if (started_ && recovered_ && !closed_)
        {
            localSubManager_.schedulePublishMonitoringTask(0);
        }
    }

    return ISMRC_OK;
}

ismCluster_HealthStatus_t LocalMonitoringManager::getHealthStatus()
{
    return healthStatus_;
}

ismCluster_HaStatus_t LocalMonitoringManager::getHaStatus()
{
    return haStatus_;
}

MCPReturnCode LocalMonitoringManager::publishMonitoringStatus()
{
    int rc = ISMRC_OK;

    if (!closed_)
    {
        rc = filterPublisher_->publishMonitoringStatus(healthStatus_,haStatus_, &sqn_);
        if (rc != ISMRC_OK)
        {
            Trace_Error(this, __FUNCTION__, "Error: failed to publish monitoring status", "RC", rc);
        }
    }

    return static_cast<MCPReturnCode>(rc);
}

} /* namespace mcp */
