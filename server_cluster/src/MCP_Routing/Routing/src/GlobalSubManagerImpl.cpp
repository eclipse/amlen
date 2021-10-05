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

#include <GlobalSubManagerImpl.h>
#include <MCPReturnCode.h>


namespace mcp
{

spdr::ScTraceComponent* GlobalSubManagerImpl::tc_ = spdr::ScTr::enroll(
        mcp::trace::Component_Name,
        mcp::trace::SubComponent_Global,
        spdr::trace::ScTrConstants::Layer_ID_App, "GlobalSubManagerImpl",
        spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

GlobalSubManagerImpl::GlobalSubManagerImpl(const std::string& inst_ID,
        const MCPConfig& mcpConfig) :
		        spdr::ScTraceContext(tc_, inst_ID, mcpConfig.getServerUID()),
		        mcp_config(mcpConfig),
		        closed(false),
		        started(false),
		        error(false),
		        lus(NULL),
		        nl(1000),
		        retainedManager(inst_ID,mcpConfig)
{
    using namespace spdr;
    Trace_Entry(this, "GlobalSubManagerImpl()");
}

GlobalSubManagerImpl::~GlobalSubManagerImpl()
{
    using namespace spdr;
    Trace_Entry(this, "~GlobalSubManagerImpl()");
}

MCPReturnCode GlobalSubManagerImpl::start()
{
    using namespace spdr;
    Trace_Entry(this, "start()");

    MCPReturnCode rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);

        if (closed)
        {
            rc = ISMRC_ClusterNotAvailable;
            Trace_Error(this, __FUNCTION__, "Error: component closed", "RC", rc);
        }
        else if (started)
        {
            rc = ISMRC_Error;
            Trace_Error(this, __FUNCTION__, "Error: component not started", "RC", rc);
        }
        else
        {
            if ( (rc = mcc_lus_createLUSet(&lus)) == ISMRC_OK )
            {
                started = true;
            }
            else
            {
                Trace_Error(this, __FUNCTION__, "Error: failure to create LUSet", "RC", rc);
            }
        }
    }

    Trace_Exit(this, "start()", rc);
    return rc;
}

MCPReturnCode GlobalSubManagerImpl::recoveryCompleted()
{
    //nothing to do right now
    return ISMRC_OK;
}

MCPReturnCode GlobalSubManagerImpl::close(bool leave_state_error)
{
    using namespace spdr;
    Trace_Entry(this, "close()", "state-error",(leave_state_error ? "T" : "F"));

    MCPReturnCode rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);

        if (!closed)
        {
            closed = true;
            error = leave_state_error;
            if (started)
            {
                rc = mcc_lus_deleteLUSet(&lus);
                if (rc != ISMRC_OK)
                {
                    Trace_Error(this, __FUNCTION__, "Error: failure to delete LUSet", "RC", rc);
                }
            }
        }
    }

    Trace_Exit(this, "close()", rc);
    return rc;
}

MCPReturnCode GlobalSubManagerImpl::lookup(
        ismCluster_LookupInfo_t* pLookupInfo)
{
    using namespace spdr;
    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "lookup()");
        buffer->addProperty("topic", std::string(pLookupInfo->pTopic, pLookupInfo->topicLen));
        buffer->addProperty("dest-length", pLookupInfo->destsLen);
        buffer->addProperty("#dest", pLookupInfo->numDests);
        buffer->invoke();
    }

    MCPReturnCode rc = ISMRC_Error;

    {
        boost::shared_lock<boost::shared_mutex> read_lock(shared_mutex);

        if (!closed)
        {
            rc = mcc_lus_lookup(lus,pLookupInfo);
            if (rc != ISMRC_OK && rc != ISMRC_ClusterArrayTooSmall)
            {
                Trace_Error(this, __FUNCTION__, "Error: lookup into LUSet failed.", "RC", rc);
            }
            else if (ScTraceBuffer::isDumpEnabled(tc_))
            {
                ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this, "lookup()", "after");
                buffer->addProperty("topic", std::string(pLookupInfo->pTopic, pLookupInfo->topicLen));
                buffer->addProperty("#dest", pLookupInfo->numDests);
                buffer->addProperty("RC", rc);
                buffer->invoke();
            }
        }
        else
        {
            if (error)
            {
                rc = ISMRC_ClusterInternalErrorState;
            }
            else
            {
                rc = ISMRC_ClusterNotAvailable;
            }

            Trace_Error(this, __FUNCTION__, "Error: component is closed", "RC", rc);
        }
    }

    Trace_Exit<int>(this, "lookup()", rc);
    return rc;
}

int GlobalSubManagerImpl::onBloomFilterBase(ismCluster_RemoteServerHandle_t node,
        const std::string& tag, const mcc_hash_HashType_t bfType,
        const int16_t numHash, const int32_t numBins, const char* buffer)
{
    using namespace spdr;
    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,
                "onBloomFilterBase()");
        buffer->addProperty("node-index", (node ? node->index : (-1)));
        buffer->addProperty("tag", tag);
        buffer->addProperty("type", bfType);
        buffer->addProperty("#Hash", numHash);
        buffer->addProperty("#Bins", numBins);
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);
        if (!closed)
        {
            rc = mcc_lus_addBF(lus, node, buffer, numBins>>3, bfType, numHash,(tag == FilterTags::BF_WildcardSub));
            if (rc != ISMRC_OK)
            {
                Trace_Error(this, __FUNCTION__, "Error: add BF into LUSet failed", "RC", rc);
            }
        }
        else
        {
            Trace_Event(this, __FUNCTION__, "Component closed, ignoring.");
        }
    }

    Trace_Exit(this, "onBloomFilterBase()", rc);
    return rc;
}

int GlobalSubManagerImpl::onBloomFilterUpdate(ismCluster_RemoteServerHandle_t node,
        const std::string& tag, const std::vector<int32_t>& binUpdates)
{
    using namespace spdr;
    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,
                "onBloomFilterUpdate()");
        buffer->addProperty("node-index", (node ? node->index : (-1)));
        buffer->addProperty("tag", tag);
        buffer->addProperty("#updates", binUpdates.size());
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);
        if (!closed)
        {
            rc = mcc_lus_updateBF(lus, node, (tag == FilterTags::BF_WildcardSub), binUpdates.data(), binUpdates.size());
            if (rc != ISMRC_OK)
            {
                Trace_Error(this, __FUNCTION__, "Error: update BF on LUSet failed", "RC", rc);
            }
        }
        else
        {
            Trace_Event(this, __FUNCTION__, "Component closed, ignoring.");
        }
    }

    Trace_Exit(this, "onBloomFilterUpdate()",rc);
    return rc;
}

int GlobalSubManagerImpl::onBloomFilterRemove(ismCluster_RemoteServerHandle_t node,
        const std::string& tag)
{
    using namespace spdr;

    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,
                "onBloomFilterRemove()");
        buffer->addProperty("node-index", (node ? node->index : -1));
        buffer->addProperty("tag", tag);
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);
        if (!closed)
        {
            rc = mcc_lus_deleteBF(lus, node, (tag == FilterTags::BF_WildcardSub));
            if (rc != ISMRC_OK)
            {
                Trace_Error(this, __FUNCTION__, "Error: delete BF on LUSet failed", "RC", rc);
            }
        }
        else
        {
            Trace_Event(this, __FUNCTION__, "Component closed, ignoring.");
        }
    }

    Trace_Exit(this,"onBloomFilterRemove()",rc);
    return rc;
}

int GlobalSubManagerImpl::onWCSubscriptionPatternBase(
        ismCluster_RemoteServerHandle_t node,
        const std::vector<SubCoveringFilterPublisher::SubscriptionPatternUpdate>& subPatternBase)
{
    using namespace spdr;
    using namespace std;

    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,"onWCSubscriptionPatternBase()");
        buffer->addProperty("node-index", (node ? node->index : -1));
        buffer->addProperty("#patterns", subPatternBase.size());
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);
        if (!closed)
        {
            set<uint64_t> pattern_ids_new;

            Pattern_IDs_Nodes_Map::iterator it = pattern_ids_map.find(node->index);
            if (it == pattern_ids_map.end())
            {
                pair<Pattern_IDs_Nodes_Map::iterator,bool> res = pattern_ids_map.insert(std::make_pair(node->index, pattern_ids_new));
                it = res.first;
            }

            for (unsigned int i=0; i<subPatternBase.size(); ++i)
            {
                uint64_t id = subPatternBase[i].first;
                pattern_ids_new.insert(id);
                if (it->second.count(id) == 0)
                {
                    rc = onBloomFilterSubscriptionPatternAdd(
                            node,id,*(subPatternBase[i].second));
                }

                if (rc != ISMRC_OK)
                {
                    Trace_Error(this,"onWCSubscriptionPatternBase()","Error: failed to add BF subscription pattern", "RC", rc);
                    return rc;
                }
            }

            for (std::set<uint64_t>::const_iterator it_old = it->second.begin();
                    it_old != it->second.end(); ++it_old)
            {
                if (pattern_ids_new.count(*it_old) == 0 )
                {
                    rc = onBloomFilterSubscriptionPatternRemove(node,*it_old);
                }

                if (rc != ISMRC_OK)
                {
                    Trace_Error(this,"onWCSubscriptionPatternBase()","Error: failed to remove BF subscription pattern", "RC", rc);
                    break;
                }
            }

            it->second.swap(pattern_ids_new);
        }
    }

    Trace_Exit(this,"onWCSubscriptionPatternBase()",rc);
    return rc;
}

int GlobalSubManagerImpl::onWCSubscriptionPatternUpdate(
        ismCluster_RemoteServerHandle_t node,
        const std::vector<SubCoveringFilterPublisher::SubscriptionPatternUpdate>& subPatternUpdate)
{
    using namespace spdr;

    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,"onWCSubscriptionPatternUpdate()");
        buffer->addProperty("node-index", (node ? node->index : -1));
        buffer->addProperty("#patterns", subPatternUpdate.size());
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);
        if (!closed)
        {
            Pattern_IDs_Nodes_Map::iterator it = pattern_ids_map.find(node->index);
            if (it == pattern_ids_map.end())
            {
                Trace_Exit(this,"onWCSubscriptionPatternUpdate()",
                        "Error: Cannot find node index=" + ScTraceBuffer::stringValueOf(node->index));
                return ISMRC_Error;
            }

            for (unsigned int i=0; i<subPatternUpdate.size(); ++i)
            {
                uint64_t id = subPatternUpdate[i].first;
                size_t count = it->second.count(id);
                if (count==0 && subPatternUpdate[i].second)
                {
                    it->second.insert(id);
                    rc = onBloomFilterSubscriptionPatternAdd(
                            node,id, *(subPatternUpdate[i].second));
                }
                else if (count>0 && !subPatternUpdate[i].second)
                {
                    it->second.erase(id);
                    rc = onBloomFilterSubscriptionPatternRemove(node,id);
                }
                else
                {
                    Trace_Error(this,"onWCSubscriptionPatternUpdate()",	"Error: existing pattern ID with value",
                            "patternID", ScTraceBuffer::stringValueOf(id),
                            "pattern", subPatternUpdate[i].second->toString());

                    rc = ISMRC_Error;
                }

                if (rc != ISMRC_OK)
                {
                    Trace_Error(this, "onWCSubscriptionPatternUpdate()", "Error: failed to add/remove pattern", "RC", rc);
                    break;
                }
            }
        }
    }

    Trace_Exit(this,"onWCSubscriptionPatternUpdate()",rc);
    return rc;
}

int GlobalSubManagerImpl::onBloomFilterSubscriptionPatternAdd(
        ismCluster_RemoteServerHandle_t node, const uint64_t id,
        const SubscriptionPattern& pattern)
{
    using namespace spdr;
    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,
                "onBloomFilterSubscriptionPatternAdd()");
        buffer->addProperty("node-index", (node ? node->index : -1));
        buffer->addProperty("id", id);
        buffer->addProperty("pattern", pattern.toString());
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        mcc_lus_Pattern_t pat;
        pat.patternId = id;
        pat.numPluses = pattern.getPlusLocations().size() ;
        pat.pPlusLevels = pattern.getPlusLocations().data();
        pat.hashLevel = pattern.getHashLocation() ;
        pat.patternLen = pattern.getLastLevel() ;
        rc = mcc_lus_addPattern(lus, node, &pat);
    }

    Trace_Exit(this,"onBloomFilterSubscriptionPatternAdd()",rc);
    return rc;
}

int GlobalSubManagerImpl::onBloomFilterSubscriptionPatternRemove(
        ismCluster_RemoteServerHandle_t node, const uint64_t id)
{
    using namespace spdr;
    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,
                "onBloomFilterSubscriptionPatternRemove()");
        buffer->addProperty("node-index", (node ? node->index : -1));
        buffer->addProperty("id", id);
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        rc = mcc_lus_deletePattern(lus, node, id);
    }

    Trace_Exit(this,"onBloomFilterSubscriptionPatternRemove()", rc);
    return rc;
}

int GlobalSubManagerImpl::setRouteAll(ismCluster_RemoteServerHandle_t node,
        int fRoutAll)
{
    using namespace spdr;
    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,
                "setRouteAll()");
        buffer->addProperty("node-index", (node ? node->index : -1));
        buffer->addProperty("flag", fRoutAll);
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);
        if (!closed)
        {
            rc = mcc_lus_setRouteAll(lus,node,fRoutAll);
        }
    }

    Trace_Exit(this, "setRouteAll", rc);
    return rc;
}

int GlobalSubManagerImpl::onServerDelete(ismCluster_RemoteServerHandle_t node, bool recovery)
{
    using namespace spdr;
    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,
                "onServerDelete()");
        buffer->addProperty("node-index", (node ? node->index : -1));
        buffer->addProperty("recovery", (recovery ? "T":"F"));
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);
        node->deletedFlag = 1;
        if (!closed) // && !recovery)
        {
            pattern_ids_map.erase(node->index);
            rc = mcc_lus_deleteServer(lus, node);
        }
    }

    Trace_Exit(this, "onServerDelete()",rc);
    return rc;
}

int GlobalSubManagerImpl::onRetainedStatsChange(ismCluster_RemoteServerHandle_t node,
        const std::string& serverUID,
        RetainedStatsVector* retainedStats)
{
    using namespace spdr;

    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,"onRetainedStatsChange()");
        buffer->addProperty("node-index", (node ? node->index : -1));
        buffer->addProperty("uid", serverUID);
        buffer->addProperty("#stats", (retainedStats ? retainedStats->size() : -1));
        buffer->invoke();
    }

    int rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);
        if (!closed)
        {
            rc = retainedManager.onRetainedStatsChange(node, serverUID, retainedStats);
        }
    }

    Trace_Exit(this, "onRetainedStatsChange()",rc);
    return rc;

}

int GlobalSubManagerImpl::onRetainedStatsRemove(ismCluster_RemoteServerHandle_t node,
        const std::string& serverUID)
{
    using namespace spdr;
    if (ScTraceBuffer::isEntryEnabled(tc_))
    {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this,"onRetainedStatsRemove()");
        buffer->addProperty("node-index", (node ? node->index : -1));
        buffer->addProperty("uid", serverUID);
        buffer->invoke();
    }


    int rc = ISMRC_OK;

    {
        boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex);
        if (!closed)
        {
            rc = retainedManager.onRetainedStatsRemove(node, serverUID);
        }
    }

    Trace_Exit(this, "onRetainedStatsRemove()",rc);
    return rc;
}

MCPReturnCode GlobalSubManagerImpl::lookupRetainedStats(const char *pServerUID,
        ismCluster_LookupRetainedStatsInfo_t **pLookupInfo)
{
    using namespace spdr;
    Trace_Entry(this,"lookupRetainedStats()","uid",pServerUID);

    MCPReturnCode rc = ISMRC_OK;

    {
        boost::shared_lock<boost::shared_mutex> read_lock(shared_mutex);
        if (!closed)
        {
            rc = retainedManager.lookupRetainedStats(pServerUID, pLookupInfo);
        }
        else
        {
            rc = ISMRC_ClusterNotAvailable;
        }
    }

    Trace_Exit(this, "lookupRetainedStats()",rc);
    return rc;

}

} /* namespace mcp */
