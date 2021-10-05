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
 * GlobalRetainedStatsManager.cpp
 *  Created on: May 25, 2015
 */

#include <GlobalRetainedStatsManager.h>

namespace mcp
{


spdr::ScTraceComponent* GlobalRetainedStatsManager::tc_ = spdr::ScTr::enroll(
        spdr::trace::ScTrConstants::ScTr_Component_Name,
        spdr::trace::ScTrConstants::ScTr_SubComponent_App,
        spdr::trace::ScTrConstants::Layer_ID_App, "GlobalRetainedStatsManager",
        spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

GlobalRetainedStatsManager::GlobalRetainedStatsManager(const std::string& inst_ID, const MCPConfig& mcpConfig) :
        spdr::ScTraceContext(tc_, inst_ID, mcpConfig.getServerUID())
{
    Trace_Entry(this, "GlobalRetainedStatsManager()");
}

GlobalRetainedStatsManager::~GlobalRetainedStatsManager()
{
    Trace_Entry(this, "~GlobalRetainedStatsManager()");

    for (RetainedMap::iterator it = map.begin(); it != map.end(); ++it)
    {
        delete it->second;
    }
	map.clear();
}

int GlobalRetainedStatsManager::onRetainedStatsChange(
        ismCluster_RemoteServerHandle_t node, const std::string& serverUID,
        SubCoveringFilterEventListener::RetainedStatsVector* retainedStats)
{
    Trace_Entry(this, "onRetainedStatsChange()", "uid", serverUID);

	std::pair<RetainedMap::iterator,bool> it = map.insert(std::make_pair(serverUID,retainedStats));
	if (!it.second )
	{
		SubCoveringFilterEventListener::RetainedStatsVector *v = it.first->second;
		delete v;
		it.first->second = retainedStats;
	}

    Trace_Debug(this, "onRetainedStatsChange()",(it.second ? "added" : "replaced"),
            "uid", serverUID);

	Trace_Exit(this, "onRetainedStatsChange()");
    return ISMRC_OK;
}

int GlobalRetainedStatsManager::onRetainedStatsRemove(
        ismCluster_RemoteServerHandle_t node, const std::string& serverUID)
{
    Trace_Entry(this, "onRetainedStatsRemove()", "uid", serverUID);

	RetainedMap::iterator it = map.find(serverUID);
	if ( it != map.end() )
	{
		SubCoveringFilterEventListener::RetainedStatsVector *v = it->second;
		map.erase(it);
		delete v;
		Trace_Debug(this, "onRetainedStatsRemove()","found", "uid", serverUID);
	}
	else
	{
	    Trace_Debug(this, "onRetainedStatsRemove()","not found", "uid", serverUID);
	}

	Trace_Exit(this, "onRetainedStatsRemove()");
    return ISMRC_OK;
}

MCPReturnCode GlobalRetainedStatsManager::lookupRetainedStats(
        const char *pServerUID,
        ismCluster_LookupRetainedStatsInfo_t **pLookupInfo)
{
    using namespace spdr;

    Trace_Entry(this, "lookupRetainedStats()", "uid", pServerUID);

	char *p;
	ismCluster_LookupRetainedStatsInfo_t *li;
	ismCluster_RetainedStats_t *rs;
	SubCoveringFilterEventListener::RetainedStatsVector *v;
	SubCoveringFilterEventListener::RetainedStatsItem *si;
	size_t s=0, n;
	RetainedMap::iterator it = map.find(std::string(pServerUID));

	if ( it == map.end() )
	{
		*pLookupInfo = NULL;
		Trace_Exit(this, "lookupRetainedStats()", "not found");
		return ISMRC_OK;
	}

	v = it->second ;
	n = v->size();

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
	    ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,"lookupRetainedStats()", "found");
	    buffer->addProperty("uid", pServerUID);
        buffer->addProperty("#items", n);
	    buffer->invoke();
	}

	s += sizeof(ismCluster_LookupRetainedStatsInfo_t) ;
	s += (n * sizeof(ismCluster_RetainedStats_t)) ;
	for ( size_t i=0 ; i<n ; i++)
	{
		s = (s+7)&(~7) ;
		si = &(v->at(i)) ;
		s += si->dataLen + si->uid.size()+1 ;
	}
	if (!(li = (ismCluster_LookupRetainedStatsInfo_t *)ism_common_malloc(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,38),s)))
	{
	    Trace_Error(this, "lookupRetainedStats()", "Error: failed to allocate", "size", s);
		return ISMRC_AllocateError;
	}
	li->numStats = n;
	li->pStats = rs = (ismCluster_RetainedStats_t *)(li+1);
	p = (char *)(rs+li->numStats) ;
	for ( size_t i=0 ; i<n ; i++)
	{
		si = &(v->at(i));
		p = (char *)(((uintptr_t)p+7)&(~7)) ; //align the (void*), it main contain a structure
		rs->dataLength = si->dataLen ;
		rs->pData = p ;
		p += rs->dataLength ;
		rs->pServerUID = p ;
		memcpy(rs->pData, si->data.get(), rs->dataLength);
		memcpy(p,si->uid.data(),si->uid.size());
		p += si->uid.size() ;
		*p++ =0;
		rs++ ;
	}
	*pLookupInfo = li;

	Trace_Exit(this, "lookupRetainedStats()");
    return ISMRC_OK;
}

MCPReturnCode GlobalRetainedStatsManager::freeRetainedStats(
        ismCluster_LookupRetainedStatsInfo_t *pLookupInfo)
{
    if ( pLookupInfo ) ism_common_free(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,39),pLookupInfo);
    return ISMRC_OK;
}

} /* namespace mcp */
