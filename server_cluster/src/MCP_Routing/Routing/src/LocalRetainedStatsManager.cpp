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
 * LocalRetainedStatsManager.cpp
 *  Created on: May 21, 2015
 */

#include <LocalRetainedStatsManager.h>

namespace mcp
{

spdr::ScTraceComponent* LocalRetainedStatsManager::tc_ = spdr::ScTr::enroll(
        mcp::trace::Component_Name,
        mcp::trace::SubComponent_Local,
        spdr::trace::ScTrConstants::Layer_ID_App, "LocalRetainedStatsManager",
        spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

LocalRetainedStatsManager::LocalRetainedStatsManager(const std::string& inst_ID,
        const MCPConfig& mcpConfig, LocalSubManager& localSubManager) :
        spdr::ScTraceContext(tc_, inst_ID, ""),
        config(mcpConfig),
        localSubManager(localSubManager),
        filterPublisher(),
        m_started(false),
        m_closed(false),
        m_recovered(false),
		sqn(0)
{
    Trace_Entry(this, __FUNCTION__);
}

LocalRetainedStatsManager::~LocalRetainedStatsManager()
{
    Trace_Entry(this, __FUNCTION__);
	close();
}

MCPReturnCode LocalRetainedStatsManager::setSubCoveringFilterPublisher(
        SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher)
{
    Trace_Entry(this, __FUNCTION__);

    if (subCoveringFilterPublisher)
    {
        filterPublisher = subCoveringFilterPublisher;
        return ISMRC_OK;
    }
    else
    {
        Trace_Error(this, __FUNCTION__, "Error: subCoveringFilterPublisher is NULL", "RC", ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }
}

MCPReturnCode LocalRetainedStatsManager::start()
{
    Trace_Entry(this, __FUNCTION__);
    m_started = true;
    return ISMRC_OK;

}

MCPReturnCode LocalRetainedStatsManager::recoveryCompleted()
{
    Trace_Entry(this, __FUNCTION__);
	m_recovered = true;
	localSubManager.schedulePublishRetainedTask(0);
    return ISMRC_OK;

}

MCPReturnCode LocalRetainedStatsManager::close()
{
    Trace_Entry(this, __FUNCTION__);
	m_closed = true;
	for ( RetainedMap::iterator it = map.begin() ; it != map.end() ; it++ )
	{
		if ( it->second.pData )
			ism_common_free(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,34),it->second.pData);
	}
	map.clear();
    return ISMRC_OK;
}

MCPReturnCode LocalRetainedStatsManager::updateRetainedStats(
        const char *pServerUID, void *pData, uint32_t dataLength)
{
    Trace_Entry(this, __FUNCTION__);

	if ( !pServerUID )
	{
	    Trace_Error(this, __FUNCTION__, "Error: pServerUID is NULL", "RC", ISMRC_NullArgument);
		return ISMRC_NullArgument;
	}

    if ( !pData && dataLength )
    {
        Trace_Error(this, __FUNCTION__, "Error: pData is NULL, but length>0", "RC", ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    std::string uid = std::string(pServerUID);
	if (!pData )
	{
		RetainedMap::iterator it = map.find(uid);
		if ( it != map.end() )
		{
			ism_common_free(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,35),it->second.pData);
			map.erase(uid);
		}
		return ISMRC_OK ;
	}

    SubCoveringFilterPublisher::RetainedStatsValue val;
	val.pData = ism_common_malloc(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,36),dataLength);
	if (!val.pData)
	{
	    Trace_Error(this, __FUNCTION__, "Error: cannot allocate", "RC", ISMRC_AllocateError);
		return ISMRC_AllocateError;
	}
	memcpy(val.pData,pData,dataLength);
	val.dataLength = dataLength;
	std::pair<RetainedMap::iterator,bool> it = map.insert(std::make_pair(uid,val));
	if (!it.second )
	{
		ism_common_free(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,37),it.first->second.pData);
		it.first->second.pData = val.pData;
		it.first->second.dataLength = val.dataLength;
	}

	if (m_started && m_recovered && !m_closed)
	{
	    localSubManager.schedulePublishRetainedTask(config.getPublishRetainedStatsIntervalMillis());
	}

    return ISMRC_OK;
}

MCPReturnCode LocalRetainedStatsManager::publishRetainedStats()
{
    if (filterPublisher)
        return (MCPReturnCode)filterPublisher->publishRetainedStats(map,&sqn);
    else
        return ISMRC_NullPointer;
}

} /* namespace mcp */
