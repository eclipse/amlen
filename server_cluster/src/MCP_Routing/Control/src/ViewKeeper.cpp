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

#include <new>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include <ViewKeeper.h>
#include <FilterTags.h>
#include "PublishRemovedServersTask.h"
#include "AdminDeleteRemovedServersTask.h"
#include "PublishRestoredNotInViewTask.h"
#include "RequestAdminMaintenanceModeTask.h"

#include "SpiderCastFactory.h"
#include "SubCoveringFilterWireFormat.h"

namespace mcp
{
spdr::ScTraceComponent* ViewKeeper::tc_ = spdr::ScTr::enroll(
        mcp::trace::Component_Name,
        mcp::trace::SubComponent_Control,
		spdr::trace::ScTrConstants::Layer_ID_App,
		"ViewKeeper",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

ViewKeeper::ViewKeeper(
		const std::string& inst_ID,
		const MCPConfig& mcpConfig,
		spdr::NodeID_SPtr myNodeID,
		SubCoveringFilterEventListener& updatelistener,
		LocalSubManager& subscriptionStatsListener,
		TaskExecutor& taskExecutor,
		ControlManager& controlManager) :
		spdr::ScTraceContext(tc_, inst_ID, ""),
		mcpConfig_(mcpConfig),
		my_nodeID(myNodeID),
		my_ServerName(mcpConfig.getServerName()),
		my_ServerUID(mcpConfig.getServerUID()),
		my_ClusterName(mcpConfig.getClusterName()),
		filterUpdatelistener(updatelistener),
		subscriptionStatsListener(subscriptionStatsListener),
		taskExecutor_(taskExecutor),
		controlManager_(controlManager),
		fatalErrorHandler_(NULL),
		state_(STATE_STARTED),
		engineServerRegisteration(NULL),
		forwardingControl(NULL),
		incarnationNumber(-1),
		serverIndex_max(0),
		storeRecoveryState_ByteBuffer_(ByteBuffer::createByteBuffer(128)),
		storePatternsPending_(false),
		storeFirstTime_(true),
		storeRemovedServersPending_(false),
		selfNode_ClusterHandle_(),
		selfNodePrev_(false),
		selfNodePrev_UID_(),
		selfNodePrev_Name_(),
		pSubs_array_(),
		pSubs_array_length_(0)
{
	selfNode_ClusterHandle_.index = 0;
	selfNode_ClusterHandle_.engineHandle = NULL;
	selfNode_ClusterHandle_.protocolHandle = NULL;
	selfNode_ClusterHandle_.deletedFlag = 0;
}

ViewKeeper::~ViewKeeper()
{
}

MCPReturnCode ViewKeeper::registerEngineEventCallback(EngineEventCallback* engineEventCallBack)
{
	boost::recursive_mutex::scoped_lock lock(view_mutex);
	if (engineEventCallBack != NULL)
	{
		this->engineServerRegisteration = engineEventCallBack;
		return ISMRC_OK;
	}
	else
	{
		return ISMRC_NullArgument;
	}
}

MCPReturnCode ViewKeeper::registerProtocolEventCallback(ForwardingControl* forwardingControl)
{
	boost::recursive_mutex::scoped_lock lock(view_mutex);
	if (forwardingControl != NULL)
	{
		this->forwardingControl = forwardingControl;
		return ISMRC_OK;
	}
	else
	{
		return ISMRC_NullArgument;
	}
}

void ViewKeeper::setFatalErrorHandler(FatalErrorHandler* pFatalErrorHandler)
{
	if (pFatalErrorHandler == NULL)
	{
		std::string what = "ViewKeeper::setFatalErrorHandler Null handler";
		throw MCPRuntimeError(what, ISMRC_NullArgument);
	}

	fatalErrorHandler_ = pFatalErrorHandler;
}

MCPReturnCode ViewKeeper::restoreRemoteServers(
		const ismCluster_RemoteServerData_t *pServersData, int numServers, int64_t& storedIncarnationNumber)
{
    using namespace spdr;
	Trace_Entry(this, "restoreRemoteServers()", "numServers", spdr::stringValueOf(numServers));

    int rc = ISMRC_OK;

    {
        boost::recursive_mutex::scoped_lock lock(view_mutex);

        if (state_ == STATE_STARTED)
        {
            for (int s = 0; s < numServers; ++s)
            {
                const ismCluster_RemoteServerData_t *pServerDataItem =
                        pServersData + s;

                String bogus_id_str = String(pServerDataItem->pRemoteServerUID)
					                + ",,,0";
                spdr::NodeID_SPtr bogus_id =
                        spdr::SpiderCastFactory::getInstance().createNodeID_SPtr(
                                bogus_id_str);

                Trace_Event(this, "restoreRemoteServers()", String("#") + spdr::stringValueOf(s+1),
                        "UID", bogus_id_str,
                        "Name", spdr::stringValueOf(pServerDataItem->pRemoteServerName),
                        "isLocalServer", (pServerDataItem->fLocalServer?"T":"F"));

                if (pServerDataItem->fLocalServer)
                {
                    if (*my_nodeID == *bogus_id) //Normal recovery
                    {
                        std::vector<SubscriptionPattern_SPtr> recoveredPatterns;
                        std::string clusterName;
                        rc = readRecoverySelfRecord(static_cast<const char*>(pServerDataItem->pData),
                                pServerDataItem->dataLength, storedIncarnationNumber, recoveredPatterns, removedServers_, clusterName);
                        if (rc != ISMRC_OK)
                        {
                            Trace_Debug(this,"restoreRemoteServers()", "Error restoring self record");
                            Trace_Exit(this,"restoreRemoteServers()", rc);
                            return static_cast<MCPReturnCode>(rc);
                        }

                        *(pServerDataItem->phClusterHandle) = &selfNode_ClusterHandle_;
                        selfNode_ClusterHandle_.engineHandle = pServerDataItem->hServerHandle;
                        rc = subscriptionStatsListener.restoreSubscriptionPatterns(recoveredPatterns);
                        if (rc != ISMRC_OK)
                        {
                            Trace_Debug(this,"restoreRemoteServers()", "Error restoring subscription patterns from the self record");
                            Trace_Exit(this,"restoreRemoteServers()", rc);
                            return static_cast<MCPReturnCode>(rc);
                        }

                        Trace_Event(this,"restoreRemoteServers()", "Recovered self-record",
                                "storedIncarnationNumber", boost::lexical_cast<std::string>(storedIncarnationNumber),
                                "#patterns", boost::lexical_cast<std::string>(recoveredPatterns.size()));
                        if (ScTraceBuffer::isDumpEnabled(tc_) && recoveredPatterns.size()>0)
                        {
                            ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this,"restoreRemoteServers()", "recovered patterns");
                            for (std::size_t i=0; i<recoveredPatterns.size(); ++i)
                            {
                                std::ostringstream key("P");
                                key << (i+1);
                                buffer->addProperty(key.str(),spdr::toString(recoveredPatterns[i]));
                            }
                            buffer->invoke();
                        }

                        if (!clusterName.empty() && my_ClusterName != clusterName)
                        {
                            Trace_Warning(this,"restoreRemoteServers()", "Warning: Recovered self-record contains a different cluster name than current configuration.",
                                    "config-ClusterName", my_ClusterName,
                                    "restored-ClusterName", clusterName);
                        }
                    }
                    else
                    {
                        if (!mcpConfig_.isRecoveryWithUIDChangeAllowed())
                        {
                            std::ostringstream what;
                            what << "Error: restoring self record, UID changed. "
                                    << "The recovery self-record is associated with UID=" << bogus_id_str << ", but the current UID=" << my_nodeID->getNodeName() <<", which is forbidden.";
                            rc = ISMRC_ClusterStoreOwnershipConflict;
                            Trace_Error(this, "restoreRemoteServers()", what.str(),"RC",rc);
                            return static_cast<MCPReturnCode>(rc);
                        }

                        //Recovery with a UID change
                        std::vector<SubscriptionPattern_SPtr> recoveredPatterns;
                        int64_t tmpIncNum;
                        std::string clusterName;
                        rc = readRecoverySelfRecord(static_cast<const char*>(pServerDataItem->pData),
                                pServerDataItem->dataLength, tmpIncNum, recoveredPatterns, removedServers_, clusterName);
                        if (rc != ISMRC_OK)
                        {
                            Trace_Error(this,"restoreRemoteServers()", "Error restoring self record, changed UID","RC",rc);
                            return static_cast<MCPReturnCode>(rc);
                        }

                        {
                            boost::recursive_mutex::scoped_lock lock(storeSelfRecord_mutex_);
                            selfNodePrev_ = true;
                            selfNodePrev_UID_ = spdr::stringValueOf(pServerDataItem->pRemoteServerUID);
                            selfNodePrev_Name_ = spdr::stringValueOf(pServerDataItem->pRemoteServerName);
                        }

                        *(pServerDataItem->phClusterHandle) = &selfNode_ClusterHandle_;
                        selfNode_ClusterHandle_.engineHandle = pServerDataItem->hServerHandle;

                        Trace_Event(this,"restoreRemoteServers()", "Recovered self-record, changed UID. Ignoring previous incarnation number and subscription patterns.",
                                "prev-UID",selfNodePrev_UID_,
                                "prev-Name", selfNodePrev_Name_,
                                "current-UID", my_nodeID->getNodeName(),
                                "stored-IncarnationNumber", boost::lexical_cast<std::string>(tmpIncNum),
                                "#patterns", boost::lexical_cast<std::string>(recoveredPatterns.size()));

                        if (!clusterName.empty() && my_ClusterName != clusterName)
                        {
                            Trace_Warning(this,"restoreRemoteServers()", "Warning: Recovered self-record contains a different cluster name than current configuration.",
                                    "config-ClusterName", my_ClusterName,
                                    "restored-ClusterName", clusterName);
                        }
                    }
                }
                else
                {
                    if (*my_nodeID != *bogus_id) //Normal recovery of a remote server
                    {
                        RecoveryFilterState recoveryState;
                        rc = readRecoveryFilterState(
                                pServerDataItem->pRemoteServerUID,
                                static_cast<const char*>(pServerDataItem->pData),
                                pServerDataItem->dataLength,
                                recoveryState);
                        if (rc != ISMRC_OK)
                        {
                            Trace_Error(this,"restoreRemoteServers()", "Error: calling readRecoveryFilterState()", "RC", rc);
                            return static_cast<MCPReturnCode>(rc);
                        }

                        Trace_Event(this,"restoreRemoteServers()", "Recovered remote-server record",
                                "state", recoveryState.toString());

                        ServerIndex serverIndex = allocate_ServerIndex();

                        RemoteServerStatus_SPtr status(
                                new RemoteServerStatus(
                                        String(pServerDataItem->pRemoteServerName),
                                        String(pServerDataItem->pRemoteServerUID),
                                        serverIndex,
                                        recoveryState.incarnation_number,
                                        false,
                                        false));
                        *(pServerDataItem->phClusterHandle) = &(status->info);
                        status->info.engineHandle = pServerDataItem->hServerHandle;
                        status->engineAdded = true;
                        serverRegistryMap[bogus_id] = status;

                        recoveryFilterState_Map_[serverIndex] = recoveryState;

                        if (recoveryState.isNonZero())
                        {
                            //PersistentRemove - set route all only if the recovered filter version is non zero
                            status->routeAll = true;
                            rc = static_cast<MCPReturnCode>(filterUpdatelistener.setRouteAll(&(status->info), 1));
                            if (rc != ISMRC_OK)
                            {
                                Trace_Error(this,"restoreRemoteServers()", "Error: calling setRouteAll()", "RC", rc);
                                return static_cast<MCPReturnCode>(rc);
                            }

                            Trace_Event(this,"restoreRemoteServers()", "ROUTE_ALL ON", "uid", status->uid);
                        }

                        Trace_Event(this,"restoreRemoteServers()", "Restored OK", "status", status->toString());
                    }
                    else
                    {
                        std::ostringstream what;
                        what << "Error: A recovery record for a remote server has the same UID is the current local node: UID="
                                << bogus_id_str << "; Cannot to recover from a store that belongs to another server.";
                        rc = ISMRC_ClusterStoreOwnershipConflict;
                        Trace_Error(this, "restoreRemoteServers()", what.str(), "RC", rc);
                        return static_cast<MCPReturnCode>(rc);
                    }
                }
            } //for server
        }
        else if (state_ == STATE_RECOVERED || state_ == STATE_ACTIVE)
        {
            rc = ISMRC_Error;
            Trace_Error(this,"restoreRemoteServers()", "Error: ViewKeeper is in an illegal state for this operation.",
                    "state", spdr::stringValueOf(state_), "RC",rc);
        }
        else
        {
            rc = ISMRC_Error;
            Trace_Error(this,"restoreRemoteServers()", "Error: ViewKeeper is in an illegal state for this operation.",
                    "state", spdr::stringValueOf(state_), "RC",rc);
        }
    }

	Trace_Exit(this, "restoreRemoteServers()",rc);
	return static_cast<MCPReturnCode>(rc);
}

MCPReturnCode ViewKeeper::readRecoveryFilterState(const char* uid, const char* pData, uint32_t dataLength,
		RecoveryFilterState& recoveryState)
{
    using namespace spdr;
    MCPReturnCode rc = ISMRC_OK;

	if (pData != NULL && dataLength > 0)
	{
	    try
	    {
	        ByteBufferReadOnlyWrapper bb(pData, dataLength);
	        uint16_t storeVer = static_cast<uint16_t>(bb.readShort());
	        if (storeVer == SubCoveringFilterWireFormat::STORE_VERSION)
	        {
	            SubCoveringFilterWireFormat::StoreRecordType type = static_cast<SubCoveringFilterWireFormat::StoreRecordType>(bb.readChar());
	            if (type == SubCoveringFilterWireFormat::Store_Remote_Server_Record)
	            {
	                recoveryState.incarnation_number = bb.readLong();
	                recoveryState.bf_exact_lastUpdate = static_cast<uint64_t>(bb.readLong());
	                recoveryState.bf_wildcard_lastUpdate =
	                        static_cast<uint64_t>(bb.readLong());
	                recoveryState.bf_wcsp_lastUpdate = static_cast<uint64_t>(bb.readLong());
	                recoveryState.rcf_lastUpdate = static_cast<uint64_t>(bb.readLong());
	            }
	            else
                {
                    std::ostringstream what;
                    what << "Error: Incompatible store record type on remote-server record; remote server UID="
                            << uid << " has type=" << type;
                    if (type == SubCoveringFilterWireFormat::Store_Local_Server_Record)
                    {
                        what << " (Store_Local_Server_Record), ";
                    }
                    else
                    {
                        what << " (Illegal type), ";
                    }
                    what << "expected type="
                            << SubCoveringFilterWireFormat::Store_Remote_Server_Record
                            << " (Store_Remote_Server_Record); ";
                    rc = ISMRC_Error;

                    Trace_Error(this, "readRecoveryFilterState()", what.str(), "RC", rc);
                }
	        }
	        else
	        {
	            rc = ISMRC_ClusterStoreVersionConflict;
	            Trace_Error(this,"readRecoveryFilterState()",
	                    "Error while recovering remote server data, stored version not compatible with current version. The store was generated with an incompatible version of the server software.",
	                    "stored-ver", boost::lexical_cast<std::string>(storeVer),
	                    "current-ver", boost::lexical_cast<std::string>(SubCoveringFilterWireFormat::STORE_VERSION));
	        }
	    }
	    catch (MCPIndexOutOfBoundsError& ob )
	    {
	        rc = ob.getReturnCode();
	        Trace_Error(this,"readRecoveryFilterState()", "Error reading from ByteBuffer",
	                "what",ob.what(), "RC", boost::lexical_cast<std::string>(rc));
	    }
	    catch (std::bad_alloc& ba)
	    {
	        rc = ISMRC_AllocateError;
	        Trace_Error(this,"readRecoveryFilterState()", "Error reading from ByteBuffer",
	                "what",ba.what(), "RC", boost::lexical_cast<std::string>(rc));

	    }
	    catch (std::exception& e)
	    {
	        rc = ISMRC_Error;
	        Trace_Error(this,"readRecoveryFilterState()", "Error reading from ByteBuffer",
	                "what",e.what(), "RC", boost::lexical_cast<std::string>(rc));
	    }
	    catch (...)
	    {
	        rc = ISMRC_Error;
	        Trace_Error(this,"readRecoveryFilterState()", "Error reading from ByteBuffer, untyped exception");
	    }
	}
	else
	{
		recoveryState.incarnation_number = 0;
		recoveryState.bf_exact_lastUpdate = 0;
		recoveryState.bf_wildcard_lastUpdate = 0;
		recoveryState.bf_wcsp_lastUpdate = 0;
		recoveryState.rcf_lastUpdate = 0;
	}

	return rc;
}

int ViewKeeper::readRecoverySelfRecord(const char* pData, uint32_t dataLength,
        int64_t& incNum, std::vector<SubscriptionPattern_SPtr>& recoveredPatterns, RemovedServers& removedServers, std::string& clusterName)
{
    int rc = ISMRC_OK;

    if (pData != NULL)
    {
        try
        {
            ByteBufferReadOnlyWrapper bb(pData, dataLength);
            uint16_t storeVer = static_cast<uint16_t>(bb.readShort());
            if (storeVer == SubCoveringFilterWireFormat::STORE_VERSION)
            {
                SubCoveringFilterWireFormat::StoreRecordType type = static_cast<SubCoveringFilterWireFormat::StoreRecordType>(bb.readChar());
                if (type == SubCoveringFilterWireFormat::Store_Local_Server_Record)
                {
                    incNum = bb.readLong();
                    uint32_t num_patterns = static_cast<uint32_t>(bb.readInt());
                    while (num_patterns-- > 0)
                    {
                        SubscriptionPattern_SPtr pattern;
                        SubCoveringFilterWireFormat::readSubscriptionPattern(SubCoveringFilterWireFormat::ATTR_VERSION, bb, pattern);
                        recoveredPatterns.push_back(pattern);
                    }

                    /*Optional:
                     * uint32_t num_records
                     * {
                     *     String  uid
                     *     int64_t incNum
                     * } x num_records
                     */

                    if (bb.getPosition() < bb.getDataLength())
                    {
                        removedServers.readAdd(SubCoveringFilterWireFormat::ATTR_VERSION,bb);
                        Trace_Event(this,"readRecoverySelfRecord()", "Restored removed servers",
                                "removed-servers", removedServers.toString());

                        /*
                         * Optional:
                         * String cluster-name
                         */
                        if (bb.getPosition() < bb.getDataLength())
                        {
                            clusterName = bb.readString();
                            Trace_Event(this,"readRecoverySelfRecord()", "Restored cluster name", "name", clusterName);
                        }
                    }
                }
                else
                {
                    std::ostringstream what;
                    what << "Error: Incompatible store record type on a record for a local server, type=" << type;
                    if (type == SubCoveringFilterWireFormat::Store_Remote_Server_Record)
                    {
                        what << " (Store_Remote_Server_Record), ";
                    }
                    else
                    {
                        what << " (Illegal type), ";
                    }
                    what << "expected type="
                            << SubCoveringFilterWireFormat::Store_Local_Server_Record
                            << " (Store_Local_Server_Record)";
                    rc = ISMRC_Error;
                    Trace_Error(this,"readRecoverySelfRecord()",what.str(), "RC", rc);
                }
            }
            else
            {
                std::ostringstream what;
                what << "Error: Incompatible store version on self-record=" << storeVer
                        << ", expected version=" << SubCoveringFilterWireFormat::STORE_VERSION
                        << ". The store was generated with an incompatible version of the server software.";
                rc = ISMRC_ClusterStoreVersionConflict;
                Trace_Error(this,"readRecoverySelfRecord()",what.str(), "RC", rc);
            }
        }
        catch (MCPIndexOutOfBoundsError& ob )
        {
            rc = ob.getReturnCode();
            Trace_Error(this,"readRecoverySelfRecord()", "Error reading from ByteBuffer",
                    "what",ob.what(), "RC", boost::lexical_cast<std::string>(rc));
        }
        catch (std::bad_alloc& ba)
        {
            rc = ISMRC_AllocateError;
            Trace_Error(this,"readRecoverySelfRecord()", "Error reading from ByteBuffer",
                    "what",ba.what(), "RC", boost::lexical_cast<std::string>(rc));

        }
        catch (std::exception& e)
        {
            rc = ISMRC_Error;
            Trace_Error(this,"readRecoverySelfRecord()", "Error reading from ByteBuffer",
                    "what",e.what(), "RC", boost::lexical_cast<std::string>(rc));
        }
        catch (...)
        {
            rc = ISMRC_Error;
            Trace_Error(this,"readRecoverySelfRecord()", "Error reading from ByteBuffer, untyped exception",
                    "RC", rc);
        }
    }

    return rc;
}

MCPReturnCode ViewKeeper::recoveryCompleted(int64_t incNum)
{
    using namespace spdr;
	boost::recursive_mutex::scoped_lock lock(view_mutex);

	if (forwardingControl == NULL || engineServerRegisteration == NULL)
	{
	    Trace_Error(this, "recoveryCompleted()", "Error: callback missing",
	            "RC", boost::lexical_cast<std::string>(ISMRC_Error),
	            "ProtocolEventCallback", boost::lexical_cast<std::string>(forwardingControl),
	            "EngineEventCallback", boost::lexical_cast<std::string>(engineServerRegisteration));
		return ISMRC_Error;
	}
	else
	{
		state_ = STATE_RECOVERED;
		incarnationNumber = incNum;
		Trace_Event(this, "recoveryCompleted()", "OK");
		return ISMRC_OK;
	}
}

void ViewKeeper::onMembershipEvent(spdr::event::MembershipEvent_SPtr event)
{
	using namespace std;
	using namespace spdr;
	using namespace spdr::event;

	Trace_Debug(this, "onMembershipEvent()",  spdr::toString(event));

	if (event)
	{
	    if (event->getType() == event::View_Change)
	    {
	        //The ViewChange is the first event from SpiderCast that happens only once
	        ism_engine_threadInit(0);
	        Trace_Event(this, "onMembershipEvent()", "Registered SpiderCast thread", "id", boost::this_thread::get_id());

	    }

	    int rc = storeRecoverySelfRecord();
	    if (rc != ISMRC_OK)
	    {
	        onFatalError(this->getMemberName(), "Fatal Error in cluster component. Local server will leave the cluster.", rc);
	        return;
	    }

	    switch (event->getType())
	    {
	    case event::View_Change: //This event happens only once, at start
	        try
	        {
	            onViewChangeEvent(event);
	        }
	        catch (std::exception& e)
	        {
	            Trace_Error(this,"onMembershipEvent()","Error: exception from onViewChangeEvent",
	                    "what", e.what(), "typeid", typeid(e).name(),"event", spdr::toString(event));
	            throw;
	        }
	        break;

	    case event::Node_Join:
	        try
	        {
	            onNodeJoinEvent(event);
	        }
	        catch (std::exception& e)
	        {
	            Trace_Error(this,"onMembershipEvent()","Error: exception from onNodeJoinEvent",
	                    "what", e.what(), "typeid", typeid(e).name(),"event", spdr::toString(event));
	            throw;
	        }
	        break;

	    case event::Node_Leave:
	        try
	        {
	            onNodeLeaveEvent(event);
	        }
	        catch (std::exception& e)
	        {
	            Trace_Error(this,"onMembershipEvent()","Error: exception from onNodeLeaveEvent",
	                    "what", e.what(), "typeid", typeid(e).name(),"event", spdr::toString(event));
	            throw;
	        }
	        break;

	    case event::Change_of_Metadata:
	        try
	        {
	            onChangeOfMetadataEvent(event);
	        }
	        catch (std::exception& e)
	        {
	            Trace_Error(this,"onMembershipEvent()","Error: exception from onChangeOfMetadataEvent",
	                    "what", e.what(), "typeid", typeid(e).name(),"event", spdr::toString(event));
	            throw;
	        }
	        break;

	    default:
	    {
	        Trace_Error(this, "onMembershipEvent()", "Error: Unexpected event", "event", spdr::toString(event));
	        onFatalError(this->getMemberName(), "Fatal Error in cluster component. Local server will leave the cluster.", ISMRC_ClusterInternalError);
	    }
	    }
	}
	else
	{
        Trace_Error(this, "onMembershipEvent()", "Error: NULL event");
        onFatalError(this->getMemberName(), "Fatal Error in cluster component. Local server will leave the cluster.", ISMRC_ClusterInternalError);
	}
}

void ViewKeeper::onViewChangeEvent(spdr::event::MembershipEvent_SPtr event)
{
	using namespace std;
	using namespace spdr;
	using namespace spdr::event;

	event::ViewChangeEvent_SPtr vc = boost::static_pointer_cast<
			event::ViewChangeEvent>(event);
	ViewMap_SPtr view = vc->getView();

	Trace_Event(this, "onViewChangeEvent()", spdr::toString(vc));

	int fatal_rc = ISMRC_OK;

	{
	    boost::recursive_mutex::scoped_lock lock(view_mutex);

	    if (state_ == STATE_CLOSED_DETACHED)
	    {
	        Trace_Event(this,"onViewChangeEvent","in state=STATE_CLOSED_DETACHED, after notifyTerm(), ignoring all events.");
	        return;
	    }
        else if (state_ == STATE_CLOSED)
        {
            Trace_Event(this,__FUNCTION__,"in state=STATE_CLOSED, after close(), ignoring all events.");
            return;
        }

	    if (removedServers_.contains(my_nodeID->getNodeName()))
	    {
	        int rc = ISMRC_ClusterLocalServerRemoved;
	        std::string what("Error: discovered local UID on the persisted removed server list, going to shut down.");
	        what = what + " This server will restart in maintenance mode."
	                + " The local server's UID may have been removed from the cluster while local server was down or behind a network partition.";
	        Trace_Error(this, "deliver_RemovedServersList_changes", what,
	                "uid", my_nodeID->getNodeName(),
	                "RC", spdr::stringValueOf(rc));
	        mcp::AbstractTask_SPtr task(new RequestAdminMaintenanceModeTask(controlManager_, rc,1));
	        taskExecutor_.scheduleDelay(task, TaskExecutor::ZERO_DELAY);
	        return;
	    }

	    for (ServerRegistryMap::const_iterator it = serverRegistryMap.begin(); it != serverRegistryMap.end(); ++it)
	    {
	        ServerIndex index = it->second->info.index;
	        if (recoveryFilterState_Map_.count(index) > 0 && it->second->routeAll)
	        {
	            //ConsistentView - set route all only if the recovered filter version is non zero
	            int rc = engineServerRegisteration->route(it->second->info.engineHandle,
	                    &(it->second->info), it->second->name.c_str(),
	                    it->second->uid.c_str(), 1);
	            if (rc != ISMRC_OK && rc != ISMRC_Closed)
	            {
	                Trace_Error(this, "onViewChangeEvent()", "Error calling Engine callback route()", "RC", rc);
	                fatal_rc = rc;
	                goto vc_exit_label;
	            }
	            else if (rc == ISMRC_Closed)
	            {
	                Trace_Event(this, "onViewChangeEvent()", "Engine callback route() returned Closed, probably termination, ignoring",
	                        "uid",it->second->uid);
	            }
	            else
	            {
	                Trace_Event(this, "onViewChangeEvent()", "Engine callback route(), ROUTE_ALL ON",
	                        "name", it->second->name, "uid", it->second->uid);
	            }
	        }

	        if (it->second->engineAdded)
	        {
	            //add every recovered server to the forwarder
	            RemoteServerStatus_SPtr status_fwd = it->second;
	            status_fwd->forwardingAdded = true;
	            status_fwd->forwardingAddress = "";
	            status_fwd->forwardingPort = 0;
	            status_fwd->forwardingUseTLS = 0;

	            int rc2 = forwardingControl->add(
	                    status_fwd->name.c_str(),
	                    status_fwd->uid.c_str(),
	                    status_fwd->forwardingAddress.c_str(),
	                    status_fwd->forwardingPort,
	                    status_fwd->forwardingUseTLS,
	                    &(status_fwd->info),
	                    status_fwd->info.engineHandle,
	                    &(status_fwd->info.protocolHandle));

	            if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
	            {
	                Trace_Error(this, "onViewChangeEvent()", "Error: calling Forwarding callback add()",
	                        "RC", rc2);
	                fatal_rc = rc2;
	                goto vc_exit_label;
	            }
	            else if (rc2 == ISMRC_Closed)
	            {
	                Trace_Event(this, "onViewChangeEvent()", "Forwarding callback add() returned Closed, probably termination, ignoring");
	            }

	            Trace_Event(this, "onViewChangeEvent()", "Forwarding callback add (recovered) node",
	                    "index", boost::lexical_cast<string>(status_fwd->info.index),
	                    "uid", status_fwd->uid,
	                    "addr", status_fwd->forwardingAddress,
	                    "port", boost::lexical_cast<string>(status_fwd->forwardingPort),
	                    "useTLS", (status_fwd->forwardingUseTLS?"True":"False"));
	        }
	    }

	    for (ViewMap::iterator view_it = view->begin(); view_it != view->end(); ++view_it)
	    {
	        if (*(view_it->first) != *my_nodeID)
	        {
	            if (view_it->second) //MetaData_SPtr not null
	            {
	                Trace_Event(this,"onViewChangeEvent()", "Processing view", "NodeID", spdr::toString(view_it->first));

	                if (removedServers_.contains(view_it->first->getNodeName()))
	                {
	                    Trace_Event(this, "onViewChangeEvent", "Remote server is contained in the removed-servers list, ignoring",
	                            "uid", view_it->first->getNodeName(),
	                            "inc", spdr::stringValueOf(view_it->second->getIncarnationNumber()),
	                            "removed-servers", removedServers_.toString());
	                    continue; //for
	                }

	                RemoteServerStatus_SPtr status;

	                //lookup - whether exists from recovery
	                ServerRegistryMap::iterator reg_it = serverRegistryMap.find(view_it->first);

	                if (reg_it == serverRegistryMap.end()) //new node
	                {
	                    Trace_Event(this,"onViewChangeEvent()", "New node", "NodeID", spdr::toString(view_it->first));

	                    if (view_it->second->getNodeStatus() != spdr::event::STATUS_REMOVE)
	                    {
	                        ServerIndex serverIndex = allocate_ServerIndex();

	                        status.reset(new RemoteServerStatus(
	                                String(), //Extract name later
	                                view_it->first->getNodeName(),
	                                serverIndex,
	                                view_it->second->getIncarnationNumber(),
	                                true,
	                                (view_it->second->getNodeStatus() == spdr::event::STATUS_ALIVE))); //controlConnected: with retained attributes, this may be a suspect node


	                        serverRegistryMap[view_it->first] = status;

	                        int rc2 = ISMRC_OK;
	                        if (status->controlConnected)
	                        {
	                            rc2 = subscriptionStatsListener.connected(&(status->info), status->uid.c_str());
	                        }
	                        else
	                        {
	                            rc2 = subscriptionStatsListener.disconnected(&(status->info), status->uid.c_str());
	                        }

	                        if (rc2 != ISMRC_OK)
	                        {
	                            Trace_Error(this, "onViewChangeEvent()", "Error calling SubscriptionStatsListener.connected()/disconnected()",
	                                    "RC", rc2);
	                            fatal_rc = rc2;
	                            goto vc_exit_label;
	                        }

	                        Trace_Event(this,"onViewChangeEvent()", "A new node", "Status", spdr::toString(status));
	                    }
	                    else
	                    {
	                        addToRemovedServersMap(view_it->first->getNodeName(), view_it->second->getIncarnationNumber());
	                        Trace_Event(this, "onViewChangeEvent()",
	                                "Remote server removed before it was added to this server, nothing to do",
	                                "NodeID", spdr::toString(view_it->first),
	                                "incarnation", boost::lexical_cast<String>(view_it->second->getIncarnationNumber()));
	                    }
	                }
	                else //A node restored by recovery
	                {
	                    Trace_Event(this,"onViewChangeEvent()", "Existing node",
	                            "registered-NodeID", spdr::toString(reg_it->first),
	                            "new-NodeID", spdr::toString(view_it->first));

	                    status = reg_it->second;
	                    status->controlAdded = true;

	                    if (view_it->second->getNodeStatus() != spdr::event::STATUS_REMOVE)
	                    {
	                        status->incarnation = view_it->second->getIncarnationNumber();
	                        bool prev_control_connected = status->controlConnected;
	                        status->controlConnected = (view_it->second->getNodeStatus() == spdr::event::STATUS_ALIVE);
	                        if (prev_control_connected != status->controlConnected)
	                        {
	                            status->connectivityChangeTime = ism_common_currentTimeNanos();
	                        }
	                        status->engineConnected = false;
	                        status->forwardingConnected = false;
	                        status->forwardingAddress.clear();
	                        status->forwardingPort = 0;
	                        status->forwardingUseTLS = 0;

	                        Trace_Event(this, "onViewChangeEvent()", "Discovered a recovered remote server",
	                                "node", view_it->first->getNodeName(),
	                                "index", boost::lexical_cast<string>(status->info.index),
	                                "engineHandle", boost::lexical_cast<string>(status->info.engineHandle));

	                        //replace the key NodeID as well
	                        serverRegistryMap.erase(reg_it);
	                        serverRegistryMap[view_it->first] = status;

	                        Trace_Dump(this, "onViewChangeEvent()", "Discovered a recovered remote server",
	                                "Node", spdr::toString(view_it->first),
	                                "Status", spdr::toString(status));
	                    }
	                    else
	                    {
	                        Trace_Event(this, "onViewChangeEvent()", "A recovered remote server was deleted while local server was down",
	                                "node", view_it->first->getNodeName(),
	                                "index", boost::lexical_cast<string>(status->info.index),
	                                "engineHandle", boost::lexical_cast<string>(status->info.engineHandle));
	                        fatal_rc = deleteNode(reg_it, true);
	                        if (fatal_rc != ISMRC_OK)
	                        {
	                            goto vc_exit_label;
	                        }
	                    }
	                }

	                //===
	                event::AttributeMap_SPtr attr =	view_it->second->getAttributeMap(); //May be NULL

	                if (attr)
	                {
	                    uint16_t protoVer = 0;
	                    uint16_t protoVerUsed = 0;
	                    string serverName;

	                    if (extractServerInfo(*attr,protoVer,protoVerUsed,serverName))
	                    {
	                        if (status->protoVer_supported != protoVer || status->protoVer_used != protoVerUsed || status->name != serverName)
	                        {
	                            status->protoVer_supported = protoVer;
	                            status->protoVer_used = protoVerUsed;
	                            status->name = serverName;
	                            Trace_Event(this, "onViewChangeEvent()", "Discovered remote server name and protocol-version",
	                                    "uid", view_it->first->getNodeName(),
	                                    "index", boost::lexical_cast<string>(status->info.index),
	                                    "name", serverName,
	                                    "proto-ver", boost::lexical_cast<string>(protoVer),
	                                    "proto-ver-used", boost::lexical_cast<string>(protoVerUsed));

	                        }

	                        if (SubCoveringFilterWireFormat::ATTR_VERSION < status->protoVer_used)
	                        {
	                            Trace_Event(this, "onViewChangeEvent()",
	                                    "Remote server is using a protocol-version incompatible with local server, ignoring remote server",
	                                    "uid", view_it->first->getNodeName(),
	                                    "index", boost::lexical_cast<string>(status->info.index),
	                                    "name", status->name,
	                                    "remote proto-ver-used", boost::lexical_cast<string>(status->protoVer_used),
	                                    "local proto-ver", boost::lexical_cast<string>(SubCoveringFilterWireFormat::ATTR_VERSION));
	                            return;
	                        }

	                        if (!status->engineAdded)
	                        {
	                            //add in a disconnected state
	                            int rc1 = engineServerRegisteration->add(
	                                    &(status->info),
	                                    status->name.c_str(),
	                                    status->uid.c_str(),
	                                    &(status->info.engineHandle));
	                            if (rc1 != ISMRC_OK && rc1 != ISMRC_Closed)
	                            {
	                                Trace_Error(this, "onViewChangeEvent()", "Error: calling Engine callback add()",
	                                        "RC", rc1);
	                                fatal_rc = rc1;
	                                goto vc_exit_label;
	                            }
	                            else if (rc1 == ISMRC_Closed)
	                            {
	                                Trace_Event(this, "onViewChangeEvent()", "Engine callback add() returned Closed, probably termination, ignoring");
	                            }

	                            status->engineAdded = true;
	                            Trace_Event(this, "onViewChangeEvent()", "Engine callback add()",
	                                    "name", status->name,
	                                    "uid", status->uid,
	                                    "index", boost::lexical_cast<string>(status->info.index),
	                                    "engineHandle", boost::lexical_cast<string>(status->info.engineHandle));
	                        }

	                        if (!status->forwardingAdded)
	                        {
	                            status->forwardingAddress = "";
	                            status->forwardingPort = 0;
	                            status->forwardingUseTLS = 0;

	                            int rc2 = forwardingControl->add(
	                                    status->name.c_str(),
	                                    status->uid.c_str(),
	                                    status->forwardingAddress.c_str(),
	                                    status->forwardingPort,
	                                    status->forwardingUseTLS,
	                                    &(status->info),
	                                    status->info.engineHandle,
	                                    &(status->info.protocolHandle));

	                            if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
	                            {
	                                Trace_Error(this, "onViewChangeEvent()", "Error: calling Forwarding callback add()",
	                                        "RC", rc2);
	                                fatal_rc = rc2;
	                                goto vc_exit_label;
	                            }
	                            else if (rc2 == ISMRC_Closed)
	                            {
	                                Trace_Event(this, "onViewChangeEvent()", "Forwarding callback add() returned Closed, probably termination, ignoring");
	                            }

	                            status->forwardingAdded = true;
	                            Trace_Event(this, "onViewChangeEvent()", "Forwarding callback add node (disconnected)",
	                                    "index", boost::lexical_cast<string>(status->info.index),
	                                    "uid", view_it->first->getNodeName());
	                        }
	                    }
	                    else
	                    {
	                        Trace_Event(this,  "onViewChangeEvent()", "ServerInfo not found yet");
	                        return;
	                    }

	                    string fwdAddr("");
	                    uint16_t fwdPort(0);
	                    uint8_t fUseTLS(0);

	                    if (status->controlConnected && extractFwdEndPoint(*attr,fwdAddr,fwdPort, fUseTLS))
	                    {
	                        status->forwardingAddress = fwdAddr;
	                        status->forwardingPort = fwdPort;
	                        status->forwardingUseTLS = fUseTLS;

	                        int rc3 = forwardingControl->connect(
	                                status->info.protocolHandle,
	                                status->name.c_str(),
	                                status->uid.c_str(),
	                                fwdAddr.c_str(),
	                                fwdPort,
	                                fUseTLS,
	                                &(status->info),
	                                status->info.engineHandle);

	                        if (rc3 != ISMRC_OK && rc3 != ISMRC_Closed)
	                        {
	                            Trace_Error(this, "onViewChangeEvent()", "Error: calling Forwarding callback connect()",
	                                    "RC", rc3);
                                fatal_rc = rc3;
                                goto vc_exit_label;	                            
	                        }
	                        else if (rc3 == ISMRC_Closed)
	                        {
	                            Trace_Event(this, "onViewChangeEvent()", "Forwarding callback connect() returned Closed, probably termination, ignoring");
	                        }

	                        Trace_Event(this, "onViewChangeEvent()", "Forwarding callback connect node",
	                                "index", boost::lexical_cast<string>(status->info.index),
	                                "uid", view_it->first->getNodeName(),
	                                "addr", fwdAddr,
	                                "port", boost::lexical_cast<string>(fwdPort),
	                                "useTLS", (fUseTLS?"True":"False"));
	                    }
	                }


	                if (attr)
	                {
	                    RecoveryFilterState recoveryState = getRecoveryFilterState(status);
	                    fatal_rc = deliver_filter_changes(attr,status,recoveryState);
	                    if (fatal_rc != ISMRC_OK)
	                    {
	                        goto vc_exit_label;
	                    }

	                    fatal_rc = deliver_retained_changes(attr,status);
	                    if (fatal_rc != ISMRC_OK)
	                    {
	                        goto vc_exit_label;
	                    }

	                    deliver_monitoring_changes(attr,status);
	                    deliver_WCSubStats_Update(*attr,status);
	                    fatal_rc = deliver_RemovedServersList_changes(attr,status);
	                    if (fatal_rc != ISMRC_OK)
	                    {
	                        goto vc_exit_label;
	                    }
	                    fatal_rc = deliver_RestoredNotInView_changes(attr,status);
                        if (fatal_rc != ISMRC_OK)
                        {
                            goto vc_exit_label;
                        }
	                }

	                //===
	                //check reconciliation over
	                if (status && status->info.deletedFlag == 0)
	                {
	                    fatal_rc = reconcileRecoveryState(status);
	                    if (fatal_rc != ISMRC_OK)
	                    {
	                        goto vc_exit_label;
	                    }
	                }

	                Trace_Dump(this, "onViewChangeEvent()", "Done processing remote server",
	                        "Node", spdr::toString(view_it->first),
	                        "Status", spdr::toString(status));

	            }//Metadata valid
	        } //not my ID
	    } //for

	    mcp::AbstractTask_SPtr taskPRNV(new PublishRestoredNotInViewTask(controlManager_));
	    taskExecutor_.scheduleDelay(taskPRNV, TaskExecutor::ZERO_DELAY);
	    Trace_Event(this, "onViewChangeEvent", "Scheduled PublishRestoredNotInViewTask");

	} //mutex

	vc_exit_label:
    if (fatal_rc == ISMRC_ClusterLocalServerRemoved)
    {
        Trace_Error(this,__FUNCTION__, "Error: Requested restart in maintenance mode.", "RC", fatal_rc);
    }
    else if (fatal_rc != ISMRC_OK)
	{
	    onFatalError(this->getMemberName(),
	            "Fatal Error in cluster component. Local server will leave the cluster.", fatal_rc);
	}

}

void ViewKeeper::onNodeJoinEvent(spdr::event::MembershipEvent_SPtr event)
{
	using namespace std;
	using namespace spdr;
	using namespace spdr::event;

	event::NodeJoinEvent_SPtr nj = boost::static_pointer_cast<
			event::NodeJoinEvent>(event);

    Trace_Event(this,"onNodeJoinEvent", "",
            "event", spdr::toString(nj),
            "NodeID", spdr::toString(nj->getNodeID()));

    int fatal_rc = ISMRC_OK;

	if (*(nj->getNodeID()) != *my_nodeID)
	{
		//Filter update
		if (nj->getMetaData())
		{
			boost::recursive_mutex::scoped_lock lock(view_mutex);

		    if (state_ == STATE_CLOSED_DETACHED)
		    {
		        Trace_Event(this,"onNodeJoinEvent","in state=STATE_CLOSED_DETACHED, after notifyTerm(), ignoring all events.");
		        return;
		    }
		    else if (state_ == STATE_CLOSED)
		    {
		        Trace_Event(this,__FUNCTION__,"in state=STATE_CLOSED, after close(), ignoring all events.");
		        return;
		    }

	        if (removedServers_.contains(nj->getNodeID()->getNodeName()))
	        {
	            Trace_Event(this, "onNodeJoinEvent", "Remote server is contained in the removed-servers list, ignoring",
	                    "uid", nj->getNodeID()->getNodeName(),
	                    "inc", spdr::stringValueOf(nj->getMetaData()->getIncarnationNumber()),
	                    "removed-servers", removedServers_.toString());
	            return;
	        }

			//Server registration
			RemoteServerStatus_SPtr status;

			//Check if it is new or existing disconnected
			ServerRegistryMap::iterator pos = serverRegistryMap.find(nj->getNodeID());
			if (pos != serverRegistryMap.end())
			{
				if (!pos->second->controlConnected)
				{
					//A disconnected known node,
					//save the index, check incarnation, save the forwadingAdded
					status = pos->second;
					status->controlAdded = true;
					status->controlConnected = true;
					status->connectivityChangeTime = ism_common_currentTimeNanos();
					status->engineConnected = false;
					status->forwardingConnected = false;
					status->forwardingAddress.clear();
					status->forwardingPort = 0;

                    Trace_Event(this,"onNodeJoinEvent","A disconnected known node",
                            "uid", status->uid,"registered-name", status->name,
                            "index", boost::lexical_cast<std::string>(status->info.index),
                            "registered-NodeID", spdr::toString(pos->first));

					if (status->incarnation < nj->getMetaData()->getIncarnationNumber())
					{
					    Trace_Event(this,"onNodeJoinEvent","A new incarnation, resetting sequence numbers",
					            "old-inc", boost::lexical_cast<std::string>(status->incarnation),
					            "new-inc", boost::lexical_cast<std::string>(nj->getMetaData()->getIncarnationNumber()));
						//New sequence numbers, the old stuff does not apply
						status->incarnation = nj->getMetaData()->getIncarnationNumber();
						status->sqn_bf_exact_base = 0;
						status->sqn_bf_exact_last_update = 0;
						status->sqn_bf_wildcard_base = 0;
						status->sqn_bf_wildcard_last_update = 0;
						status->sqn_bf_wcsp_base = 0;
						status->sqn_bf_wcsp_last_update = 0;
						status->sqn_rcf_base = 0;
						status->sqn_rcf_last_update = 0;
						status->sqn_sub_stats = 0;
						status->sqn_retained_stats_last_updated = 0;
					}
					else if (status->incarnation > nj->getMetaData()->getIncarnationNumber())
					{
						//should never happen, should be filtered by SpiderCast history
						Trace_Error(this, "onNodeJoinEvent()", "Error: incarnation number from the past",
						        "status-inc", boost::lexical_cast<std::string>(status->incarnation),
						        "event-inc",  boost::lexical_cast<std::string>(nj->getMetaData()->getIncarnationNumber()));
                        fatal_rc = ISMRC_ClusterInternalError;
                        goto nj_exit_label;
					}

					serverRegistryMap.erase(pos);
					serverRegistryMap[nj->getNodeID()] = status; //To update the NodeID as well


	                int rc2 = subscriptionStatsListener.connected(&(status->info), status->uid.c_str());
	                if (rc2 != ISMRC_OK)
	                {
	                    Trace_Error(this, "onNodeJoinEvent()", "Error: calling SubscriptionStatsListener.connected() existing node",
	                            "uid", status->uid, "RC", rc2);
                        fatal_rc = rc2;
                        goto nj_exit_label;
	                }
				}
				else
				{
					//error in ViewKeeper logic
					Trace_Error(this, "onNodeJoinEvent()", "Error: NJ but controlConnected==true");
                    fatal_rc = ISMRC_ClusterInternalError;
                    goto nj_exit_label;
				}

				Trace_Dump(this,"onNodeJoinEvent", "A disconnected known node",
				          "Node", spdr::toString(nj->getNodeID()),
				          "Status", spdr::toString(status));
			}
			else
			{
				//A new node

				//reset the forwarding address & port
				//in order to distinguish between forwarding add/update
				//port==0 => add, port!=0 => update
				status.reset(new RemoteServerStatus(
						String(), //Extract name later
						nj->getNodeID()->getNodeName(),
						allocate_ServerIndex(),
						nj->getMetaData()->getIncarnationNumber(),
						true,
						true));

				serverRegistryMap[nj->getNodeID()] = status;

				int rc2 = subscriptionStatsListener.connected(&(status->info), status->uid.c_str());
				if (rc2 != ISMRC_OK)
				{
					Trace_Error(this, "onNodeJoinEvent()", "Error: calling SubscriptionStatsListener.connected() new node",
					        "uid", status->uid, "RC", rc2);
                    fatal_rc = rc2;
                    goto nj_exit_label;
				}

				Trace_Event(this,"onNodeJoinEvent","A new node","status", status->toString());
			}

			event::AttributeMap_SPtr attr = nj->getMetaData()->getAttributeMap();
			if (attr)
			{
				uint16_t protoVer = 0;
				uint16_t protoVerUsed = 0;
				string serverName;

				if (extractServerInfo(*attr,protoVer,protoVerUsed,serverName))
				{
					if (status->protoVer_supported != protoVer || status->protoVer_used != protoVerUsed || status->name != serverName)
					{
						status->protoVer_supported = protoVer;
						status->protoVer_used = protoVerUsed;
						status->name = serverName;
						Trace_Event(this, "onNodeJoinEvent()", "Discovered remote server name and wire-format-version",
								"uid", nj->getNodeID()->getNodeName(),
								"index", boost::lexical_cast<string>(status->info.index),
								"name", serverName,
								"proto-ver", boost::lexical_cast<string>(protoVer),
								"proto-ver-used", boost::lexical_cast<string>(protoVerUsed));
					}

					if (SubCoveringFilterWireFormat::ATTR_VERSION < status->protoVer_used)
					{
					    Trace_Event(this, "onNodeJoinEvent()",
					            "Remote server is using a protocol-version incompatible with local server, ignoring remote server",
					            "uid", nj->getNodeID()->getNodeName(),
					            "index", boost::lexical_cast<string>(status->info.index),
					            "name", status->name,
					            "remote proto-ver-used", boost::lexical_cast<string>(status->protoVer_used),
					            "local proto-ver", boost::lexical_cast<string>(SubCoveringFilterWireFormat::ATTR_VERSION));
					    return;
					}

					if (!status->engineAdded)
					{
						int rc = engineServerRegisteration->add(
								&(status->info),
								status->name.c_str(),
								status->uid.c_str(),
								&(status->info.engineHandle));
						if (rc != ISMRC_OK && rc != ISMRC_Closed)
						{
							Trace_Error(this, "onNodeJoinEvent()", "Error: calling Engine callback add()",
							        "RC", boost::lexical_cast<std::string>(rc));
	                        fatal_rc = rc;
	                        goto nj_exit_label;
						}
			            else if (rc == ISMRC_Closed)
			            {
			                Trace_Event(this, "onNodeJoinEvent()", "Engine callback add() returned Closed, probably termination, ignoring");
			            }

						status->engineAdded = true;
						Trace_Event(this, "onNodeJoinEvent()", "Engine callback add()",
								"name", status->name,
								"uid", status->uid,
								"index", boost::lexical_cast<string>(status->info.index),
								"engineHandle", boost::lexical_cast<string>(status->info.engineHandle));
					}

					if (!status->forwardingAdded)
					{
					    status->forwardingAddress.clear();
					    status->forwardingPort = 0;
					    status->forwardingUseTLS = 0;

					    int rc2 = forwardingControl->add(
					            status->name.c_str(),
					            status->uid.c_str(),
					            status->forwardingAddress.c_str(),
					            status->forwardingPort,
					            status->forwardingUseTLS,
					            &(status->info),
					            status->info.engineHandle,
					            &(status->info.protocolHandle));
					    if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
					    {
					        Trace_Error(this, "onNodeJoinEvent()", "Error: calling Protocol callback add()",
					                "RC", rc2);
					        fatal_rc = rc2;
					        goto nj_exit_label;
					    }
					    else if (rc2 == ISMRC_Closed)
					    {
					        Trace_Event(this, "onNodeJoinEvent()", "Protocol callback add() returned Closed, probably termination, ignoring");
					    }

					    status->forwardingAdded = true;

					    Trace_Event(this, "onNodeJoinEvent()", "Protocol callback add() (disconnected) node",
					            "index", boost::lexical_cast<string>(status->info.index),
					            "uid", status->uid);
					}
				}
				else
				{
				    Trace_Event(this,  "onNodeJoinEvent()", "ServerInfo not found yet");
				    return;
				}				

				string fwdAddr("");
				uint16_t fwdPort(0);
				uint8_t fUseTLS(0);

				if (extractFwdEndPoint(*attr,fwdAddr,fwdPort,fUseTLS))
				{
					status->forwardingAddress = fwdAddr;
					status->forwardingPort = fwdPort;
					status->forwardingUseTLS = fUseTLS;

					if (!status->forwardingAdded)
					{
						int rc2 = forwardingControl->add(
								status->name.c_str(),
								status->uid.c_str(),
								fwdAddr.c_str(),
								fwdPort,
								fUseTLS,
								&(status->info),
								status->info.engineHandle,
								&(status->info.protocolHandle));
						if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
						{
							Trace_Error(this, "onNodeJoinEvent()", "Error: calling Protocol callback add()",
							        "RC", rc2);
	                        fatal_rc = rc2;
	                        goto nj_exit_label;
						}
						else if (rc2 == ISMRC_Closed)
						{
						    Trace_Event(this, "onNodeJoinEvent()", "Protocol callback add() returned Closed, probably termination, ignoring");
						}

						status->forwardingAdded = true;

						Trace_Event(this, "onNodeJoinEvent()", "Protocol callback add()",
						        "index", boost::lexical_cast<string>(status->info.index),
						        "uid", status->uid,
						        "addr", fwdAddr,
						        "port", boost::lexical_cast<string>(fwdPort),
						        "useTLS", (fUseTLS?"True":"False"));
					}
					else
					{
						int rc2 = forwardingControl->connect(
								status->info.protocolHandle,
								status->name.c_str(),
								status->uid.c_str(),
								fwdAddr.c_str(),
								fwdPort,
								fUseTLS,
								&(status->info),
								status->info.engineHandle);
						if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
						{
							Trace_Error(this, "onNodeJoinEvent()", "Error: calling Protocol callback connect()",
							        "RC", rc2);
	                        fatal_rc = rc2;
	                        goto nj_exit_label;
						}
						else if (rc2 == ISMRC_Closed)
						{
						    Trace_Event(this, "onNodeJoinEvent()", "Protocol callback connect() returned Closed, probably termination, ignoring");
						}

						Trace_Event(this, "onNodeJoinEvent()", "Protocol callback connect()",
						        "index", boost::lexical_cast<string>(status->info.index),
						        "uid", status->uid,
						        "addr", fwdAddr,
						        "port", boost::lexical_cast<string>(fwdPort),
						        "useTLS", (fUseTLS?"True":"False"));
					}
				}

				RecoveryFilterState recoveryState = getRecoveryFilterState(status);

				if (attr)
				{
				    fatal_rc = deliver_filter_changes(attr, status, recoveryState);
                    if (fatal_rc != ISMRC_OK)
                    {
                        goto nj_exit_label;
                    }

					fatal_rc = deliver_retained_changes(attr,status);
                    if (fatal_rc != ISMRC_OK)
                    {
                        goto nj_exit_label;
                    }

					deliver_monitoring_changes(attr,status);
					deliver_WCSubStats_Update(*attr,status);
					fatal_rc = deliver_RemovedServersList_changes(attr,status);
					if (fatal_rc != ISMRC_OK)
					{
					    goto nj_exit_label;
					}
                    fatal_rc = deliver_RestoredNotInView_changes(attr,status);
                    if (fatal_rc != ISMRC_OK)
                    {
                        goto nj_exit_label;
                    }

				}

				//check reconciliation over
				fatal_rc = reconcileRecoveryState(status);
                if (fatal_rc != ISMRC_OK)
                {
                    goto nj_exit_label;
                }

			}
			else
			{
			    Trace_Event(this, "onNodeJoinEvent()", "No attributes");
			}

			mcp::AbstractTask_SPtr taskPRNV(new PublishRestoredNotInViewTask(controlManager_));
			taskExecutor_.scheduleDelay(taskPRNV, TaskExecutor::ZERO_DELAY);
			Trace_Debug(this, "onNodeJoinEvent", "Scheduled PublishRestoredNotInViewTask");

			Trace_Dump(this, "onNodeJoinEvent()", "Done processing remote server",
			        "Node", spdr::toString(nj->getNodeID()),
			        "Status", spdr::toString(status));
		}//Metadata valid
	}//not my ID

	nj_exit_label:
    if (fatal_rc == ISMRC_ClusterLocalServerRemoved)
    {
        Trace_Error(this,__FUNCTION__, "Error: Requested restart in maintenance mode.", "RC", fatal_rc);
    }
    else if (fatal_rc != ISMRC_OK)
	{
	    onFatalError(this->getMemberName(),
	            "Fatal Error in cluster component. Local server will leave the cluster.", fatal_rc);
	}
}

void ViewKeeper::onNodeLeaveEvent(spdr::event::MembershipEvent_SPtr event)
{
	using namespace std;
	using namespace spdr;
	using namespace spdr::event;

	event::NodeLeaveEvent_SPtr nl = boost::static_pointer_cast<
			event::NodeLeaveEvent>(event);

	Trace_Event(this,"onNodeLeaveEvent", "",
	        "event", spdr::toString(nl),
	        "NodeID", spdr::toString(nl->getNodeID()));

	int fatal_rc = ISMRC_OK;

	if (*(nl->getNodeID()) != *my_nodeID)
	{
		if (nl->getMetaData())
		{
			boost::recursive_mutex::scoped_lock lock(view_mutex);

			if (state_ == STATE_CLOSED_DETACHED)
			{
			    Trace_Event(this,"onNodeLeaveEvent","in state=STATE_CLOSED_DETACHED, after notifyTerm(), ignoring all events.");
			    return;
			}
            else if (state_ == STATE_CLOSED)
            {
                Trace_Event(this,__FUNCTION__,"in state=STATE_CLOSED, after close(), ignoring all events.");
                return;
            }

			if (removedServers_.contains(nl->getNodeID()->getNodeName()))
			{
			    Trace_Event(this, "onNodeLeaveEvent", "Remote server is contained in the removed-servers list, ignoring",
			            "uid", nl->getNodeID()->getNodeName(),
			            "inc", spdr::stringValueOf(nl->getMetaData()->getIncarnationNumber()),
			            "removed-servers", removedServers_.toString());
			    return;
			}

			RemoteServerStatus_SPtr status;

			//Server registration
			ServerRegistryMap::iterator pos = serverRegistryMap.find(nl->getNodeID());

			if (pos == serverRegistryMap.end())
			{
				if (nl->getMetaData()->getNodeStatus() == spdr::event::STATUS_LEAVE ||
						nl->getMetaData()->getNodeStatus() == spdr::event::STATUS_SUSPECT ||
						nl->getMetaData()->getNodeStatus() == spdr::event::STATUS_SUSPECT_DUPLICATE_NODE)
				{
					//reset the forwarding address & port
					//in order to distinguish between forwarding add/update
					//port==0 => add, port!=0 => update
					status.reset(new RemoteServerStatus(
							String(), //Extract name later
							nl->getNodeID()->getNodeName(),
							allocate_ServerIndex(),
							nl->getMetaData()->getIncarnationNumber(),
							true,
							false));

                    Trace_Event(this,"onNodeLeaveEvent", "New disconnected node", "status", status->toString());

					std::pair<ServerRegistryMap::iterator,bool> res = serverRegistryMap.insert(std::make_pair(nl->getNodeID(), status));
					if (res.second)
					{
						pos = res.first;
					}
					else
					{
						Trace_Error(this, "onNodeLeaveEvent()", "Error: failed inserting to registry");
                        fatal_rc = ISMRC_ClusterInternalError;
                        goto nl_exit_label;
					}

				}
				else if (nl->getMetaData()->getNodeStatus() == spdr::event::STATUS_REMOVE)
				{
				    addToRemovedServersMap(nl->getNodeID()->getNodeName(), nl->getMetaData()->getIncarnationNumber());
					Trace_Event(this, "onNodeLeaveEvent()", "node remove, not in registry, nothing to do",
							"node", nl->getNodeID()->getNodeName(),
							"inc",spdr::stringValueOf(nl->getMetaData()->getIncarnationNumber()));
				}
				else
				{
					Trace_Error(this, "onNodeLeaveEvent()", "Error: unexpected NodeStatus",
					        "NodeStatus", boost::lexical_cast<std::string>(nl->getMetaData()->getNodeStatus()),
					        "RC", boost::lexical_cast<std::string>(ISMRC_ClusterInternalError));
					fatal_rc = ISMRC_ClusterInternalError;
					goto nl_exit_label;
				}
			}

			if (pos != serverRegistryMap.end())
			{
				//check return code, do leave or delete
				//in case of leave, fire disconnect and do not delete the routing info from filters
				if (nl->getMetaData()->getNodeStatus() == spdr::event::STATUS_LEAVE ||
						nl->getMetaData()->getNodeStatus() == spdr::event::STATUS_SUSPECT ||
						nl->getMetaData()->getNodeStatus() == spdr::event::STATUS_SUSPECT_DUPLICATE_NODE)
				{
					status = pos->second;
					status->controlAdded = true;
					status->controlConnected = false;
					status->connectivityChangeTime = ism_common_currentTimeNanos();

					//signals that the forwarding EP needs updating in future join/view-change
					status->forwardingAddress.clear();
					status->forwardingPort = 0;
					//keep the the forwardingAdded intact
					//in order to distinguish between forwarding add/update

					if (status->engineAdded && status->engineConnected)
					{
					    status->engineConnected = false;
					    int rc = engineServerRegisteration->disconnected(
					            status->info.engineHandle, &(status->info),
					            status->name.c_str(),
					            status->uid.c_str());
					    if (rc != ISMRC_OK && rc != ISMRC_Closed)
					    {
					        Trace_Error(this, "onNodeLeaveEvent()",	"Error: calling ServerRegistration.disconnected()",
					                "RC", rc);
		                    fatal_rc = rc;
		                    goto nl_exit_label;
					    }
					    else if (rc == ISMRC_Closed)
					    {
					        Trace_Event(this, "onNodeLeaveEvent()", "Engine callback disconnected() returned Closed, probably termination, ignoring");
					    }

						Trace_Event(this, "onNodeLeaveEvent()", "Engine callback disconnected()",
								"name", status->name,
								"uid", status->uid,
								"index", boost::lexical_cast<string>(status->info.index));
					}

					int rc2 = subscriptionStatsListener.disconnected(&(status->info), status->uid.c_str());
					if (rc2 != ISMRC_OK)
					{
					    Trace_Error(this, "onNodeLeaveEvent()", "Error: calling SubscriptionStatsListener.disconnected(),", "RC", rc2);
                        fatal_rc = rc2;
                        goto nl_exit_label;
					}

					if (status->forwardingAdded) // && status->forwardingConnected) //Never drops a disconnect
					{
					    status->forwardingConnected = false;
					    int rc2 = forwardingControl->disconnect(
					            status->info.protocolHandle,
					            status->name.c_str(),
					            status->uid.c_str(),
					            status->forwardingAddress.c_str(),
					            status->forwardingPort,
					            status->forwardingUseTLS,
					            &(status->info),
					            status->info.engineHandle);
					    if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
					    {
					        Trace_Error(this, "onNodeLeaveEvent()", "Error: calling Protocol callback disconnect()", "RC", rc2);
                            fatal_rc = rc2;
                            goto nl_exit_label;
					    }
					    else if (rc2 == ISMRC_Closed)
					    {
					        Trace_Event(this, "onNodeLeaveEvent()", "Protocol callback disconnect() returned Closed, probably termination, ignoring");
					    }

						Trace_Event(this, "onNodeLeaveEvent()",
								"Protocol callback disconnect node", "node",
								pos->first->getNodeName(), "index",
								boost::lexical_cast<string>(
										status->info.index));
					}

					event::AttributeMap_SPtr attr = nl->getMetaData()->getAttributeMap();
					if (attr)
					{
						uint16_t protoVer = 0;
						uint16_t protoVerUsed = 0;
						string serverName;

						if (extractServerInfo(*attr,protoVer,protoVerUsed,serverName))
						{
							if (status->protoVer_supported != protoVer || status->protoVer_used != protoVerUsed || status->name != serverName)
							{
								status->protoVer_supported = protoVer;
								status->protoVer_used = protoVerUsed;
								status->name = serverName;
								Trace_Event(this, "onNodeLeaveEvent()", "Discovered remote server name and wire-format-version",
										"uid", nl->getNodeID()->getNodeName(),
										"index", boost::lexical_cast<string>(status->info.index),
										"name", serverName,
										"proto-ver", boost::lexical_cast<string>(protoVer),
										"proto-ver-used", boost::lexical_cast<string>(protoVerUsed));
							}

							if (SubCoveringFilterWireFormat::ATTR_VERSION < status->protoVer_used)
							{
							    Trace_Event(this, "onNodeLeaveEvent()",
							            "Remote server is using a protocol-version incompatible with local server, ignoring remote server",
							            "uid", nl->getNodeID()->getNodeName(),
							            "index", boost::lexical_cast<string>(status->info.index),
							            "name", status->name,
							            "remote proto-ver-used", boost::lexical_cast<string>(status->protoVer_used),
							            "local proto-ver", boost::lexical_cast<string>(SubCoveringFilterWireFormat::ATTR_VERSION));
							    return;
							}

							if (!status->engineAdded)
							{
								int rc = engineServerRegisteration->add(
										&(status->info),
										status->name.c_str(),
										status->uid.c_str(),
										&(status->info.engineHandle));
								if (rc != ISMRC_OK && rc != ISMRC_Closed)
								{
									Trace_Error(this, "onNodeLeaveEvent()", "Error: calling ServerRegistration.add()", "RC", rc);
		                            fatal_rc = rc;
		                            goto nl_exit_label;
								}
		                        else if (rc == ISMRC_Closed)
		                        {
		                            Trace_Event(this, "onNodeLeaveEvent()", "Engine callback add() returned Closed, probably termination, ignoring");
		                        }


								status->engineAdded = true;
								Trace_Event(this, "onNodeLeaveEvent()", "Engine callback add()",
								        "name", status->name,
								        "uid", status->uid,
										"index", boost::lexical_cast<string>(status->info.index) );
							}

		                    if (!status->forwardingAdded)
		                    {
		                        status->forwardingAddress.clear();
		                        status->forwardingPort = 0;
		                        status->forwardingUseTLS = 0;

		                        int rc2 = forwardingControl->add(
		                                status->name.c_str(),
		                                status->uid.c_str(),
		                                status->forwardingAddress.c_str(),
		                                status->forwardingPort,
		                                status->forwardingUseTLS,
		                                &(status->info),
		                                status->info.engineHandle,
		                                &(status->info.protocolHandle));
		                        if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
		                        {
		                            Trace_Error(this, "onNodeLeaveEvent()", "Error: calling Protocol callback add()",
		                                    "RC", rc2);
		                            fatal_rc = rc2;
		                            goto nl_exit_label;
		                        }
		                        else if (rc2 == ISMRC_Closed)
		                        {
		                            Trace_Event(this, "onNodeLeaveEvent()", "Protocol callback add() returned Closed, probably termination, ignoring");
		                        }

		                        status->forwardingAdded = true;

		                        Trace_Event(this, "onNodeLeaveEvent()", "Protocol callback add() (disconnected) node",
		                                "index", boost::lexical_cast<string>(status->info.index),
		                                "uid", status->uid);
		                    }
						}
						else
						{
						    Trace_Event(this,  "onNodeLeaveEvent()", "ServerInfo not found yet");
						    return;

						}

						RecoveryFilterState recoveryState = getRecoveryFilterState(status);
						fatal_rc = deliver_filter_changes(attr,status,recoveryState);
                        if (fatal_rc != ISMRC_OK)
                        {
                            goto nl_exit_label;
                        }

						fatal_rc = deliver_retained_changes(attr,status);
	                    if (fatal_rc != ISMRC_OK)
	                    {
	                        goto nl_exit_label;
	                    }

						deliver_monitoring_changes(attr,status);
						deliver_WCSubStats_Update(*attr,status);
						fatal_rc = deliver_RemovedServersList_changes(attr,status);
						if (fatal_rc != ISMRC_OK)
						{
						    goto nl_exit_label;
						}
                        fatal_rc = deliver_RestoredNotInView_changes(attr,status);
                        if (fatal_rc != ISMRC_OK)
                        {
                            goto nl_exit_label;
                        }


						//check reconciliation over
						fatal_rc = reconcileRecoveryState(status);
                        if (fatal_rc != ISMRC_OK)
                        {
                            goto nl_exit_label;
                        }
					}
				}
				else if (nl->getMetaData()->getNodeStatus() == spdr::event::STATUS_REMOVE)
				{
					Trace_Event(this, "onNodeLeaveEvent()", "node remove",
							"uid", pos->first->getNodeName(),
							"index", boost::lexical_cast<string>(pos->second->info.index));
					fatal_rc = deleteNode(pos, false);
					if (fatal_rc != ISMRC_OK)
					{
					    goto nl_exit_label;
					}
				}
				else
				{
					Trace_Error(this, "onNodeLeaveEvent()", "Error: unexpected NodeStatus", "NodeStatus", nl->getMetaData()->getNodeStatus());
                    fatal_rc = ISMRC_ClusterInternalError;
                    goto nl_exit_label;
				}
			}

			mcp::AbstractTask_SPtr taskPRNV(new PublishRestoredNotInViewTask(controlManager_));
			taskExecutor_.scheduleDelay(taskPRNV, TaskExecutor::ZERO_DELAY);
			Trace_Debug(this, "onNodeLeaveEvent", "Scheduled PublishRestoredNotInViewTask");

			Trace_Dump(this, "onNodeLeaveEvent()", "Done processing remote server",
			        "Node", spdr::toString(nl->getNodeID()),
			        "Status", spdr::toString(status));
		}
	}

	nl_exit_label:
    if (fatal_rc == ISMRC_ClusterLocalServerRemoved)
    {
        Trace_Error(this,__FUNCTION__, "Error: Requested restart in maintenance mode.", "RC", fatal_rc);
    }
    else if (fatal_rc != ISMRC_OK)
	{
	    onFatalError(this->getMemberName(),
	            "Fatal Error in cluster component. Local server will leave the cluster.", fatal_rc);
	}
}

void ViewKeeper::onChangeOfMetadataEvent(spdr::event::MembershipEvent_SPtr event)
{
    using namespace std;
    using namespace spdr;
    using namespace spdr::event;

    event::ChangeOfMetaDataEvent_SPtr cm = boost::static_pointer_cast<
            event::ChangeOfMetaDataEvent>(event);
    ViewMap_SPtr view = cm->getView();

    int fatal_rc = ISMRC_OK;

    {
        boost::recursive_mutex::scoped_lock lock(view_mutex);

        if (state_ == STATE_CLOSED_DETACHED)
        {
            Trace_Event(this,"onChangeOfMetadataEvent","in state=STATE_CLOSED_DETACHED, after notifyTerm(), ignoring all events.");
            return;
        }
        else if (state_ == STATE_CLOSED)
        {
            Trace_Event(this,__FUNCTION__,"in state=STATE_CLOSED, after close(), ignoring all events.");
            return;
        }

        for (ViewMap::iterator it = view->begin(); it != view->end(); ++it)
        {
            if (*(it->first) != *my_nodeID)
            {
                if (it->second) //MetaData_SPtr not null
                {
                    if (removedServers_.contains(it->first->getNodeName()))
                    {
                        Trace_Event(this, "onChangeOfMetadataEvent", "Remote server is contained in the removed-servers list, ignoring",
                                "uid", it->first->getNodeName(),
                                "inc", spdr::stringValueOf(it->second->getIncarnationNumber()),
                                "removed-servers", removedServers_.toString());
                        continue; //for
                    }

                    if (serverRegistryMap.count(it->first)==0)
                    {
                        Trace_Error(this, __FUNCTION__, "Error: remote server not in ServerRegistryMap",
                                "server", spdr::toString(it->first));
                        fatal_rc = ISMRC_ClusterInternalError;
                        goto cm_exit_label;
                    }

                    RemoteServerStatus_SPtr status = serverRegistryMap[it->first];
                    if (!status->controlAdded)
                    {
                        status->controlAdded = true;
                        mcp::AbstractTask_SPtr taskPRNV(new PublishRestoredNotInViewTask(controlManager_));
                        taskExecutor_.scheduleDelay(taskPRNV, TaskExecutor::ZERO_DELAY);
                        Trace_Debug(this, "onChangeOfMetadataEvent", "Scheduled PublishRestoredNotInViewTask");
                    }
                    ServerIndex serverIndex = status->info.index;
                    event::AttributeMap_SPtr attr =	it->second->getAttributeMap(); //May be NULL

                    if (attr)
                    {

                        uint16_t protoVer = 0;
                        uint16_t protoVerUsed = 0;
                        string serverName;

                        if (extractServerInfo(*attr,protoVer,protoVerUsed,serverName))
                        {
                            if (status->protoVer_supported != protoVer || status->protoVer_used != protoVerUsed || status->name != serverName)
                            {
                                status->protoVer_supported = protoVer;
                                status->protoVer_used = protoVerUsed;
                                status->name = serverName;
                                Trace_Event(this, "onChangeOfMetadataEvent()", "Discovered remote server name and protocol-version",
                                        "uid", it->first->getNodeName(),
                                        "index", boost::lexical_cast<string>(status->info.index),
                                        "name", serverName,
                                        "proto-ver", boost::lexical_cast<string>(protoVer),
                                        "proto-ver-used", boost::lexical_cast<string>(protoVer));
                            }

                            if (SubCoveringFilterWireFormat::ATTR_VERSION < status->protoVer_used)
                            {
                                Trace_Event(this, "onChangeOfMetadataEvent()",
                                        "Remote server is using a protocol-version incompatible with local server, ignoring remote server",
                                        "uid", it->first->getNodeName(),
                                        "index", boost::lexical_cast<string>(status->info.index),
                                        "name", status->name,
                                        "remote proto-ver-used", boost::lexical_cast<string>(status->protoVer_used),
                                        "local proto-ver", boost::lexical_cast<string>(SubCoveringFilterWireFormat::ATTR_VERSION));
                                return;
                            }

                            if (!status->engineAdded)
                            {
                                //add in a disconnected state
                                int rc1 = engineServerRegisteration->add(
                                        &(status->info),
                                        status->name.c_str(),
                                        status->uid.c_str(),
                                        &(status->info.engineHandle));
                                if (rc1 != ISMRC_OK && rc1 != ISMRC_Closed)
                                {
                                    Trace_Error(this, "onChangeOfMetadataEvent()", "Error: calling ServerRegistration.add()","RC", rc1);
                                    fatal_rc = rc1;
                                    goto cm_exit_label;
                                }
                                else if (rc1 == ISMRC_Closed)
                                {
                                    Trace_Event(this, "onChangeOfMetadataEvent()", "Engine callback add() returned Closed, probably termination, ignoring");
                                }

                                status->engineAdded = true;
                                Trace_Event(this, "onChangeOfMetadataEvent()", "Engine callback add()",
                                        "name", status->name,
                                        "uid", status->uid,
                                        "index", boost::lexical_cast<string>(status->info.index),
                                        "engineHandle", boost::lexical_cast<string>(status->info.engineHandle));
                            }

                            if (!status->forwardingAdded)
                            {
                                status->forwardingAddress.clear();
                                status->forwardingPort = 0;
                                status->forwardingUseTLS = 0;

                                int rc2 = forwardingControl->add(
                                        status->name.c_str(),
                                        status->uid.c_str(),
                                        status->forwardingAddress.c_str(),
                                        status->forwardingPort,
                                        status->forwardingUseTLS,
                                        &(status->info),
                                        status->info.engineHandle,
                                        &(status->info.protocolHandle));
                                if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
                                {
                                    Trace_Error(this, "onChangeOfMetadataEvent()", "Error: calling Protocol callback add()",
                                            "RC", rc2);
                                    fatal_rc = rc2;
                                    goto cm_exit_label;
                                }
                                else if (rc2 == ISMRC_Closed)
                                {
                                    Trace_Event(this, "onChangeOfMetadataEvent()", "Protocol callback add() returned Closed, probably termination, ignoring");
                                }

                                status->forwardingAdded = true;
                                Trace_Event(this, "onChangeOfMetadataEvent()", "Protocol callback add() (disconnected) node",
                                        "index", boost::lexical_cast<string>(status->info.index),
                                        "uid", status->uid);
                            }
                        }
                        else
                        {
                            Trace_Event(this,  "onChangeOfMetadataEvent()", "ServerInfo not found yet");
                            return;
                        }

                        string fwdAddr("");
                        uint16_t fwdPort(0);
                        uint8_t fUseTLS(0);

                        //avoid extracting every time an attribute changes
                        if (serverRegistryMap[it->first]->controlConnected && serverRegistryMap[it->first]->forwardingPort == 0)
                        {
                            if (extractFwdEndPoint(*attr,fwdAddr,fwdPort,fUseTLS))
                            {
                                string desc("ChangeOfMetadata: forwarding EP ");
                                serverRegistryMap[it->first]->forwardingAddress = fwdAddr;
                                serverRegistryMap[it->first]->forwardingPort = fwdPort;
                                serverRegistryMap[it->first]->forwardingUseTLS = fUseTLS;

                                if (!serverRegistryMap[it->first]->forwardingAdded)
                                {
                                    int rc2 = forwardingControl->add(
                                            status->name.c_str(),
                                            status->uid.c_str(),
                                            fwdAddr.c_str(),
                                            fwdPort,
                                            fUseTLS,
                                            &(status->info),
                                            status->info.engineHandle,
                                            &(status->info.protocolHandle));
                                    if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
                                    {
                                        Trace_Error(this, "onChangeOfMetadataEvent()", "Error: calling ForwardingControl.add()","RC", rc2);
                                        fatal_rc = rc2;
                                        goto cm_exit_label;
                                    }
                                    else if (rc2 == ISMRC_Closed)
                                    {
                                        Trace_Event(this, "onChangeOfMetadataEvent()", "Protocol callback add() returned Closed, probably termination, ignoring");
                                    }

                                    serverRegistryMap[it->first]->forwardingAdded = true;
                                    desc += string("add node");
                                }
                                else
                                {
                                    int rc2 = forwardingControl->connect(
                                            status->info.protocolHandle,
                                            status->name.c_str(),
                                            status->uid.c_str(),
                                            fwdAddr.c_str(),
                                            fwdPort,
                                            fUseTLS,
                                            &(status->info),
                                            status->info.engineHandle);
                                    if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
                                    {
                                        Trace_Error(this, "onChangeOfMetadataEvent()", "Error: calling ForwardingControl.update()","RC", rc2);
                                        fatal_rc = rc2;
                                        goto cm_exit_label;
                                    }
                                    else if (rc2 == ISMRC_Closed)
                                    {
                                        Trace_Event(this, "onChangeOfMetadataEvent()", "Protocol callback connect() returned Closed, probably termination, ignoring");
                                    }
                                    desc += string("connect node");
                                }

                                Trace_Event(this, "onChangeOfMetadataEvent()", desc,
                                        "index", boost::lexical_cast<string>(serverIndex),
                                        "uid", it->first->getNodeName(),
                                        "addr", fwdAddr,
                                        "port", boost::lexical_cast<string>(fwdPort));
                            }
                        }
                    }

                    RecoveryFilterState recoveryState = getRecoveryFilterState(status);

                    if (attr)
                    {
                        fatal_rc = deliver_filter_changes(attr,status,recoveryState);
                        if (fatal_rc != ISMRC_OK)
                        {
                            goto cm_exit_label;
                        }

                        fatal_rc = deliver_retained_changes(attr,status);
                        if (fatal_rc != ISMRC_OK)
                        {
                            goto cm_exit_label;
                        }

                        deliver_monitoring_changes(attr,status);
                        deliver_WCSubStats_Update(*attr,status);
                        fatal_rc = deliver_RemovedServersList_changes(attr,status);
                        if (fatal_rc != ISMRC_OK)
                        {
                            goto cm_exit_label;
                        }
                        fatal_rc = deliver_RestoredNotInView_changes(attr,status);
                        if (fatal_rc != ISMRC_OK)
                        {
                            goto cm_exit_label;
                        }

                    }

                    //check reconciliation over
                    fatal_rc = reconcileRecoveryState(status);
                    if (fatal_rc != ISMRC_OK)
                    {
                        goto cm_exit_label;
                    }


                    Trace_Dump(this, "onChangeOfMetadataEvent()", "Done processing remote server",
                            "Node", spdr::toString(it->first),
                            "Status", spdr::toString(status));

                }
            }//not my ID
        }//for
    }//mutex

    cm_exit_label:
    if (fatal_rc == ISMRC_ClusterLocalServerRemoved)
    {
        Trace_Error(this,__FUNCTION__, "Error: Requested restart in maintenance mode.", "RC", fatal_rc);
    }
    else if (fatal_rc != ISMRC_OK)
    {
        onFatalError(this->getMemberName(),
                "Fatal Error in cluster component. Local server will leave the cluster.", fatal_rc);
    }
}


MCPReturnCode ViewKeeper::nodeForwardingConnected(const ismCluster_RemoteServerHandle_t phServerHandle)
{
	using namespace std;

	Trace_Entry(this, "nodeForwardingConnected()", "handle", spdr::stringValueOf(phServerHandle));

	int fatal_rc = ISMRC_OK;

	{
	    boost::recursive_mutex::scoped_lock lock(view_mutex);

	    if (state_ == STATE_CLOSED_DETACHED)
	    {
	        Trace_Event(this,"nodeForwardingConnected","in state=STATE_CLOSED_DETACHED, after notifyTerm(), ignoring all events.");
	        return ISMRC_OK;
	    }
        else if (state_ == STATE_CLOSED)
        {
            Trace_Event(this,__FUNCTION__,"in state=STATE_CLOSED, after close(), ignoring all events.");
            return ISMRC_OK;
        }

	    if (phServerHandle->deletedFlag)
	    {
	        Trace_Event(this, "nodeForwardingConnected()", "node deleted, ignored",
	                "index", spdr::stringValueOf(phServerHandle->index),
	                "handle", spdr::stringValueOf(phServerHandle));
	        return ISMRC_OK;
	    }

	    //find the status by linear search
	    ServerRegistryMap::iterator pos = serverRegistryMap.begin();
	    for ( ; pos != serverRegistryMap.end(); ++pos)
	    {
	        if (&(pos->second->info) == phServerHandle)
	        {
	            break;
	        }
	    }

	    if (pos != serverRegistryMap.end())
	    {
	        RemoteServerStatus_SPtr status = pos->second;
	        status->forwardingConnected = true;

	        if (status->controlConnected)
	        {
	            status->engineConnected = true;
	            int rc = engineServerRegisteration->connected(
	                    status->info.engineHandle,
	                    phServerHandle,
	                    status->name.c_str(),
	                    status->uid.c_str());
	            if (rc != ISMRC_OK && rc != ISMRC_Closed)
	            {
	                Trace_Error(this, "nodeForwardingConnected()", "Error: calling ServerRegistration.connected()","RC", rc);
	                fatal_rc = rc;
	                goto nfc_exit_label;
	            }
	            else if (rc == ISMRC_Closed)
	            {
	                Trace_Event(this, "nodeForwardingConnected()", "Engine callback connected() returned Closed, probably termination, ignoring");
	            }

	            status->connectivityChangeTime = ism_common_currentTimeNanos();

	            Trace_Event(this, "nodeForwardingConnected()", "Engine callback connected()",
	                    "name", status->name, "uid", status->uid,
	                    "index", boost::lexical_cast<string>(status->info.index) );
	        }
	        else
	        {
	            Trace_Event(this, "nodeForwardingConnected()", "Control not connected, Forwarding is connected, Engine not called",
	                                    "Status", spdr::toString(status));
	        }
	    }
	    else
	    {
	        Trace_Event(this, "nodeForwardingConnected()", "cannot find node in registry, ignored",
	                "handle", boost::lexical_cast<string>(phServerHandle) );
	    }

	} //mutex

    nfc_exit_label:
    if (fatal_rc != ISMRC_OK)
    {
        onFatalError(this->getMemberName(),
                "Fatal Error in cluster component. Local server will leave the cluster.", fatal_rc);
    }

	Trace_Exit(this, "nodeForwardingConnected()",fatal_rc);
	return static_cast<MCPReturnCode>(fatal_rc);
}

MCPReturnCode ViewKeeper::nodeForwardingDisconnected(const ismCluster_RemoteServerHandle_t phServerHandle)
{
	using namespace std;

	Trace_Entry(this, "nodeForwardingDisconnected()",
	            "index", boost::lexical_cast<string>(phServerHandle->index),
	            "handle", boost::lexical_cast<string>(phServerHandle) );

	int fatal_rc = ISMRC_OK;

	{
	    boost::recursive_mutex::scoped_lock lock(view_mutex);

	    if (state_ == STATE_CLOSED_DETACHED)
	    {
	        Trace_Event(this,"nodeForwardingDisconnected","in state=STATE_CLOSED_DETACHED, after notifyTerm(), ignoring all events.");
	        return ISMRC_OK;
	    }
        else if (state_ == STATE_CLOSED)
        {
            Trace_Event(this,__FUNCTION__,"in state=STATE_CLOSED, after close(), ignoring all events.");
            return ISMRC_OK;
        }

	    if (phServerHandle->deletedFlag)
	    {
	        Trace_Event(this, "nodeForwardingDisconnected()", "node deleted, ignored",
	                "index", boost::lexical_cast<string>(phServerHandle->index),
	                "handle", boost::lexical_cast<string>(phServerHandle) );
	        return ISMRC_OK;
	    }

	    ServerRegistryMap::iterator pos = serverRegistryMap.begin();
	    for ( ; pos != serverRegistryMap.end(); ++pos)
	    {
	        if (&(pos->second->info) == phServerHandle)
	        {
	            break;
	        }
	    }

	    if (pos != serverRegistryMap.end())
	    {
	        RemoteServerStatus_SPtr status = pos->second;
	        status->forwardingConnected = false;
	        if (status->engineAdded && status->engineConnected)
	        {
	            status->engineConnected = false;
	            status->connectivityChangeTime = ism_common_currentTimeNanos();
	            int rc = engineServerRegisteration->disconnected(
	                    status->info.engineHandle,
	                    phServerHandle,
	                    status->name.c_str(),
	                    status->uid.c_str());
	            if (rc != ISMRC_OK && rc != ISMRC_Closed)
	            {
	                Trace_Error(this, "nodeForwardingDisconnected()", "Error: calling ServerRegistration.disconnected()","RC", rc);
	                fatal_rc = rc;
	                goto nfd_exit_label;
	            }
	            else if (rc == ISMRC_Closed)
	            {
	                Trace_Event(this, "nodeForwardingDisconnected()", "Engine callback disconnected() returned Closed, probably termination, ignoring");
	            }

	            Trace_Event(this, "nodeForwardingDisconnected()", "Engine callback disconnected()",
	                    "name", status->name, "uid", status->uid,
	                    "index", boost::lexical_cast<string>(status->info.index) );
	        }
	        else
	        {
	            Trace_Event(this, "nodeForwardingDisconnected()", "Engine not added or connected, not called",
	                    "Status", spdr::toString(status));
	        }
	    }
	    else
	    {
	        Trace_Event(this, "nodeForwardingDisconnected()", "Warning: cannot find node in registry, ignored",
	                "handle", boost::lexical_cast<string>(phServerHandle) );
	    }
	}//mutex

    nfd_exit_label:
    if (fatal_rc != ISMRC_OK)
    {
        onFatalError(this->getMemberName(),
                "Fatal Error in cluster component. Local server will leave the cluster.", fatal_rc);
    }

	Trace_Exit(this, "nodeForwardingDisconnected()",fatal_rc);
	return static_cast<MCPReturnCode>(fatal_rc);
}

MCPReturnCode ViewKeeper::adminDeleteNodeFromList(const std::string& nodeUID, spdr::NodeID_SPtr& nodeID, int64_t* incarnation_num)
{
    using namespace std;
    int rc = ISMRC_OK;
    String bogus_id_str = String(nodeUID)+ ",,,0";
    spdr::NodeID_SPtr bogus_id = spdr::SpiderCastFactory::getInstance().createNodeID_SPtr(bogus_id_str);

    {
        boost::recursive_mutex::scoped_lock lock(view_mutex);
        ServerRegistryMap::iterator pos = serverRegistryMap.find(bogus_id);
        if (pos != serverRegistryMap.end())
        {
            nodeID = pos->first;
            *incarnation_num = pos->second->incarnation;
            rc = deleteNode(pos, true);
            Trace_Event(this, "adminDeleteNodeFromList()", "node from RemovedServers list, delete immediately.",
                    "node", spdr::toString(nodeID),
                    "incarnation", spdr::stringValueOf(*incarnation_num),
                    "success", (rc == ISMRC_OK?"True":"False"));
        }
        else
        {
            Trace_Debug(this, "adminDeleteNodeFromList()", "node not found in registry, ignoring",
                       "nodeUID", nodeUID, "inc", spdr::stringValueOf(incarnation_num));
        }
    }

    return static_cast<MCPReturnCode>(rc);
}

MCPReturnCode ViewKeeper::adminDeleteNode(const ismCluster_RemoteServerHandle_t phServerHandle, spdr::NodeID_SPtr& nodeID, int64_t* incarnation_num)
{
    //if this is on a node in the recovery list, i.e. it is not in the SpiderCast view, remove directly

	using namespace std;

	{
		boost::recursive_mutex::scoped_lock lock(view_mutex);

		if (phServerHandle->deletedFlag == 0)
		{
			ServerRegistryMap::iterator pos = serverRegistryMap.begin();
			while (pos != serverRegistryMap.end())
			{
				if (phServerHandle == &(pos->second->info))
				{
					break;
				}
				++pos;
			}

			if (pos != serverRegistryMap.end())
			{
				if (pos->second->info.index != phServerHandle->index)
				{
					MCPReturnCode rc = ISMRC_ArgNotValid;
					ostringstream what;
					what << "internal index=" << pos->second->info.index << "; handle index=" << phServerHandle->index;
					Trace_Error(this, "adminDeleteNode()", "Error: mismatch between handle data and internal data-structure",
					        "what", what.str(),"RC", rc);
					return rc;
				}

				if (pos->second->controlConnected)
				{
					MCPReturnCode rc = ISMRC_ClusterRemoveRemoteServerStillAlive;
					Trace_Warning(this, "adminDeleteNode()", "Warning: cannot delete a live node, shutdown remote server first",
					        "node", pos->first->getNodeName(),
					        "RC", boost::lexical_cast<std::string>(rc));
					return rc;
				}

				nodeID = pos->first;
				*incarnation_num = pos->second->incarnation;

				MCPReturnCode rc1 = ISMRC_OK;
				if (recoveryFilterState_Map_.count(pos->second->info.index))
				{
					spdr::NodeID_SPtr nodeID = pos->first;
					int64_t inc_num = pos->second->incarnation;
					rc1 = deleteNode(pos, true);
					Trace_Event(this, "adminDeleteNode()", "node still in recovery, delete immediately.",
							"node", spdr::toString(nodeID),
							"incarnation", spdr::stringValueOf(inc_num),
							"success", (rc1 == ISMRC_OK?"True":"False"));
				}
				else
				{
				    Trace_Debug(this, "adminDeleteNode()", "node after recovery, SpiderCast event will delete it.",
                            "node", spdr::toString(pos->first),
                            "incarnation", spdr::stringValueOf(pos->second->incarnation));
				}

				return rc1;
			}
			else
			{
				Trace_Debug(this, "adminDeleteNode()", "node not found in registry, ignoring",
				        "phServerHandle", spdr::stringValueOf(phServerHandle));
				return ISMRC_OK;
			}

		}
		else
		{
			Trace_Debug(this, "adminDeleteNode()", "node already deleted, ignoring");
			return ISMRC_OK;
		}
	}
}

MCPReturnCode ViewKeeper::storeSubscriptionPatterns(
			const std::vector<SubscriptionPattern_SPtr>& patterns)
{
    using namespace spdr;
    Trace_Entry(this,"storeSubscriptionPatterns()","#patt", stringValueOf(patterns.size()));

    {
        boost::recursive_mutex::scoped_lock lock(storeSelfRecord_mutex_);
        storePatterns_.clear();
        storePatterns_.assign(patterns.begin(),patterns.end());
        storePatternsPending_ = true;
    }

    Trace_Exit(this,"storeSubscriptionPatterns()");
    return ISMRC_OK;
}

MCPReturnCode ViewKeeper::deleteNode(ServerRegistryMap::iterator pos, bool recovery)
{
	using namespace std;

	if (pos != serverRegistryMap.end())
	{
	    RemoteServerStatus_SPtr status = pos->second;

	    //Filter update, will remove all kinds (RouteAll, BF, RCF) and also set ismCluster_RemoteServer_t.deletedFlag=1
	    int rc3 = filterUpdatelistener.onServerDelete(&(status->info), recovery);
	    if (rc3 == ISMRC_NotFound)
	    {
	        Trace_Event(this, "deleteNode()", "Not found in GlobalSubManager, ignoring", "status", status->toString());
	    }
	    else if (rc3 != ISMRC_OK )
	    {
	        Trace_Error(this, "deleteNode()", "Error: calling onServerDelete()", "RC", rc3);
	        return static_cast<MCPReturnCode>(rc3);
	    }

	    int rc4 = filterUpdatelistener.onRetainedStatsRemove(&(status->info),status->uid);
	    if (rc4 != ISMRC_OK)
	    {
	        Trace_Error(this, "deleteNode()", "Error: calling onRetainedStatsRemove()", "RC", rc4);
	        return static_cast<MCPReturnCode>(rc4);
	    }

	    if (!recovery)
	    {
	        int rc1 = subscriptionStatsListener.remove(&(status->info), status->uid.c_str());
	        if (rc1 != ISMRC_OK)
	        {
	            Trace_Error(this, "deleteNode()", "Error: calling SubscriptionStatsListener.remove()","RC", rc1);
	            return static_cast<MCPReturnCode>(rc1);
	        }
	    }

	    //Remote servers are removed first from forwarder then from engine because engineHandle is destroyed after removing from engine
	    if (status->forwardingAdded)
	    {
	        int rc2 = forwardingControl->remove(
	                status->info.protocolHandle,
	                status->name.c_str(),
	                status->uid.c_str(),
	                status->forwardingAddress.c_str(),
	                status->forwardingPort,
	                status->forwardingUseTLS,
	                &(status->info),
	                status->info.engineHandle);

	        if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
	        {
	            Trace_Error(this, "deleteNode()", "Error: calling ForwardingControl.remove()","RC", rc2);
	            return static_cast<MCPReturnCode>(rc2);
	        }
	        else if (rc2 == ISMRC_Closed)
	        {
	            Trace_Event(this, "deleteNode()", "Protocol callback remove() returned Closed, probably termination, ignoring");
	        }

	        Trace_Config(this, "deleteNode()", "Protocol callback remove()",
	                "uid", status->uid,
	                "index", boost::lexical_cast<string>(status->info.index) );
	    }

        if (status->engineAdded)
        {
            int rc = engineServerRegisteration->remove(
                    status->info.engineHandle,
                    &(status->info),
                    status->name.c_str(),
                    status->uid.c_str());
            if (rc != ISMRC_OK && rc != ISMRC_Closed)
            {
                Trace_Error(this, "deleteNode()", "Error: calling ServerRegistration.remove()","RC", rc);
                return static_cast<MCPReturnCode>(rc);
            }
            else if (rc == ISMRC_Closed)
            {
                Trace_Event(this, "deleteNode()", "Engine callback remove() returned Closed, probably termination, ignoring");
            }

            Trace_Config(this, "deleteNode()", "Engine callback remove()",
                    "uid", status->uid,
                    "index", boost::lexical_cast<string>(status->info.index) );
        }


	    int rc = addToRemovedServersMap(status->uid, status->incarnation);
	    if (rc != ISMRC_OK)
	    {
	    	Trace_Error(this, "deleteNode()", "Error: calling addToRemovedServersMap()",
	    			"status", status->toString(), "RC", rc);
	    	return static_cast<MCPReturnCode>(rc);
	    }

	    serverRegistryMap.erase(pos);

	    recoveryFilterState_Map_.erase(status->info.index);
	    deletedNodes.push_back(std::make_pair(status, boost::posix_time::second_clock::universal_time()));
	    cleanDeletedNodesList(); //move to a task?

	    return ISMRC_OK;
	}
	else
	{
	    Trace_Error(this, "deleteNode()", "Error: invalid iterator to delete position","RC", ISMRC_Error);
		return ISMRC_Error;
	}
}

int ViewKeeper::addToRemovedServersMap(const std::string& uid, int64_t incNum)
{
    int rc = ISMRC_OK;
    bool changed = removedServers_.add(uid,incNum);

    if (changed)
    {
        Trace_Event(this, "addToRemovedServersMap", "Added to RemovedServersMap",
                "uid",uid,"inc",spdr::stringValueOf(incNum));
        {
            boost::recursive_mutex::scoped_lock lock(storeSelfRecord_mutex_);
            storeRemovedServersPending_ = true;
        }

        if ((rc = storeRecoverySelfRecord()) != ISMRC_OK)
        {
            Trace_Error(this, "addToRemovedServersMap", "Error: failed to persist removed servers list to the store", "RC",rc);
            return rc;
        }
        else
        {
            Trace_Event(this, "addToRemovedServersMap", "Stored to RecoverySelfRecord");
        }

        mcp::AbstractTask_SPtr task(new PublishRemovedServersTask(controlManager_));
        taskExecutor_.scheduleDelay(task, TaskExecutor::ZERO_DELAY);
        Trace_Event(this, "addToRemovedServersMap", "Scheduled PublishRemovedServersTask");
    }

    return rc;
}

void ViewKeeper::cleanDeletedNodesList()
{
    boost::posix_time::ptime threshold =
            boost::posix_time::second_clock::universal_time()
                    - boost::posix_time::seconds(
                            mcpConfig_.getDeletedNodeListCleanIntervalSec());
    DeletedNodesList::iterator it = deletedNodes.begin();
    while (it != deletedNodes.end() && it->second < threshold)
    {
        free_ServerIndex(it->first->info.index);
        it = deletedNodes.erase(it);
    }
}

uint64_t ViewKeeper::getSQN_from_BFAttVal(
		const spdr::event::AttributeValue& attVal)
{
	using namespace spdr;

	Trace_Entry(this,"getSQN_from_BFAttVal()");

	ByteBufferReadOnlyWrapper bb(attVal.getBuffer().get(), attVal.getLength());
	uint64_t sqn = static_cast<uint64_t>(bb.readLong());

	Trace_Exit(this,"getSQN_from_BFAttVal()",sqn);

	return sqn;
}

bool ViewKeeper::filterSort_attribute_map(
		const spdr::event::AttributeMap& attr_map,
		uint64_t bf_exact_last_update,
		uint64_t bf_wildcard_last_update,
		uint64_t bf_wcsp_last_update,
		uint64_t rcf_last_update,
		uint64_t& bf_exact_last_update_new,
		uint64_t& bf_wildcard_last_update_new,
		uint64_t& bf_wcsp_last_update_new,
		uint64_t& rcf_last_update_new,
		Sorted_FilterValue_Map& sorted_map)
{
	using namespace std;
	Trace_Entry(this, "filterSort_attribute_map()");

	bool change = false;

	uint64_t exact_base_sqn_max = 0;
	uint64_t wc_base_sqn_max = 0;
	uint64_t wcsp_base_sqn_max = 0;
	uint64_t rcf_base_sqn_max = 0;

	sorted_map.clear();

	bf_exact_last_update_new = bf_exact_last_update;
	bf_wildcard_last_update_new = bf_wildcard_last_update;
	bf_wcsp_last_update_new = bf_wcsp_last_update;
	rcf_last_update_new = rcf_last_update;

	for (spdr::event::AttributeMap::const_iterator it = attr_map.begin();
			it != attr_map.end(); ++it)
	{
		Trace_Debug(this, "filterSort_attribute_map()", "",
				"key", it->first);

		if (boost::starts_with(it->first, FilterTags::BF_ExactSub_Base))
		{
			uint64_t sqn = ViewKeeper::getSQN_from_BFAttVal( it->second );
			if (sqn > bf_exact_last_update && sqn >= exact_base_sqn_max)
			{
				sorted_map[sqn] = make_pair(VALUE_TYPE_BF_E_BASE,it->second);
			}

			if (sqn > exact_base_sqn_max)
			{
				exact_base_sqn_max = sqn;
			}

			if (sqn > bf_exact_last_update_new)
			{
				bf_exact_last_update_new = sqn;
				change = true;
			}
		}
		else if (boost::starts_with(it->first, FilterTags::BF_WildcardSub_Base))
		{
			uint64_t sqn = ViewKeeper::getSQN_from_BFAttVal( it->second );
			if (sqn > bf_wildcard_last_update && sqn >= wc_base_sqn_max)
			{
				sorted_map[sqn] = make_pair(VALUE_TYPE_BF_W_BASE,it->second);
			}

			if (sqn > wc_base_sqn_max)
			{
				wc_base_sqn_max = sqn;
			}

			if (sqn > bf_wildcard_last_update_new)
			{
				bf_wildcard_last_update_new = sqn;
				change = true;
			}
		}
		else if (boost::starts_with(it->first, FilterTags::BF_ExactSub_Update))
		{
			uint64_t sqn = ViewKeeper::getSQN_from_BFAttVal( it->second );
			if (sqn > bf_exact_last_update && sqn > exact_base_sqn_max)
			{
				sorted_map[sqn] = make_pair(VALUE_TYPE_BF_E_UPDT,it->second);
			}

			if (sqn > bf_exact_last_update_new)
			{
				bf_exact_last_update_new = sqn;
				change = true;
			}
		}
		else if (boost::starts_with(it->first, FilterTags::BF_WildcardSub_Update))
		{
			uint64_t sqn = ViewKeeper::getSQN_from_BFAttVal( it->second );
			if (sqn > bf_wildcard_last_update && sqn > wc_base_sqn_max)
			{
				sorted_map[sqn] = make_pair(VALUE_TYPE_BF_W_UPDT,it->second);
			}

			if (sqn > bf_wildcard_last_update_new)
			{
				bf_wildcard_last_update_new = sqn;
				change = true;
			}
		}
		else if (boost::starts_with(it->first, FilterTags::RCF_Base))
		{
			uint64_t sqn = ViewKeeper::getSQN_from_BFAttVal( it->second );
			if (sqn > rcf_last_update && sqn >= rcf_base_sqn_max)
			{
				sorted_map[sqn] = make_pair(VALUE_TYPE_RCF_BASE,it->second);
			}

			if (sqn > rcf_base_sqn_max)
			{
				rcf_base_sqn_max = sqn;
			}

			if (sqn > rcf_last_update_new)
			{
				rcf_last_update_new = sqn;
				change = true;
			}
		}
		else if (boost::starts_with(it->first, FilterTags::RCF_Update))
		{
			uint64_t sqn = ViewKeeper::getSQN_from_BFAttVal( it->second );
			if (sqn > rcf_last_update && sqn >= rcf_base_sqn_max)
			{
				sorted_map[sqn] = make_pair(VALUE_TYPE_RCF_UPDT,it->second);
			}

			if (sqn > rcf_last_update_new)
			{
				rcf_last_update_new = sqn;
				change = true;
			}
		}
		else if (boost::starts_with(it->first, FilterTags::BF_Wildcard_SubscriptionPattern_Base))
		{
			uint64_t sqn = ViewKeeper::getSQN_from_BFAttVal( it->second );
			if (sqn > bf_wcsp_last_update && sqn >= wcsp_base_sqn_max)
			{
				sorted_map[sqn] = make_pair(VALUE_TYPE_WCSP_BASE,it->second);
			}

			if (sqn > wcsp_base_sqn_max)
			{
				wcsp_base_sqn_max = sqn;
			}

			if (sqn > bf_wcsp_last_update_new)
			{
				bf_wcsp_last_update_new = sqn;
				change = true;
			}
		}
		else if (boost::starts_with(it->first, FilterTags::BF_Wildcard_SubscriptionPattern_Update))
		{
			uint64_t sqn = ViewKeeper::getSQN_from_BFAttVal( it->second );
			if (sqn > bf_wcsp_last_update && sqn >= wcsp_base_sqn_max)
			{
				sorted_map[sqn] = make_pair(VALUE_TYPE_WCSP_UPDT,it->second);
			}

			if (sqn > bf_wcsp_last_update_new)
			{
				bf_wcsp_last_update_new = sqn;
				change = true;
			}
		}
	}

	//remove everything below max_base
	Sorted_FilterValue_Map::iterator it2 = sorted_map.begin();
	while ( it2 != sorted_map.end())
	{
		Sorted_FilterValue_Map::iterator pos = it2++;

		if (pos->first >= exact_base_sqn_max
				&& pos->first >= wc_base_sqn_max
				&& pos->first >= rcf_base_sqn_max
				&& pos->first >= wcsp_base_sqn_max)
			break; //out of while

		switch (pos->second.first)
		{
		case VALUE_TYPE_BF_E_BASE:
		case VALUE_TYPE_BF_E_UPDT:
			if (pos->first < exact_base_sqn_max)
			{
				sorted_map.erase(pos);
			}
			break;

		case VALUE_TYPE_BF_W_BASE:
		case VALUE_TYPE_BF_W_UPDT:
			if (pos->first < wc_base_sqn_max)
			{
				sorted_map.erase(pos);
			}
			break;

		case VALUE_TYPE_RCF_BASE:
		case VALUE_TYPE_RCF_UPDT:
			if (pos->first < rcf_base_sqn_max)
			{
				sorted_map.erase(pos);
			}
			break;

		case VALUE_TYPE_WCSP_BASE:
		case VALUE_TYPE_WCSP_UPDT:
			if (pos->first < wcsp_base_sqn_max)
			{
				sorted_map.erase(pos);
			}
			break;

		default:
			break;
		} //switch
	}//while

	Trace_Exit(this, "filterSort_attribute_map()",change);
	return change;
}



int ViewKeeper::deliver_retained_changes(
            spdr::event::AttributeMap_SPtr attr_map,
            RemoteServerStatus_SPtr status)
{
    Trace_Entry(this, "deliver_retained_changes()");

    spdr::event::AttributeMap::const_iterator pos = attr_map->find(FilterTags::RetainedStats);
    if (pos != attr_map->end())
    {
        ByteBufferReadOnlyWrapper bb(pos->second.getBuffer().get(), pos->second.getLength());
        uint64_t sqn = static_cast<uint64_t>(bb.readLong());
        if (sqn > status->sqn_retained_stats_last_updated)
        {
            int rc = ISMRC_OK;

            SubCoveringFilterEventListener::RetainedStatsVector* retainedVec = new SubCoveringFilterEventListener::RetainedStatsVector;
            uint32_t numServers = static_cast<uint32_t>(bb.readInt());
            while (numServers > 0)
            {
                SubCoveringFilterEventListener::RetainedStatsItem stats;
                stats.uid = bb.readString();
                stats.dataLen = static_cast<uint32_t>(bb.readInt());
                if (stats.dataLen > 0)
                {
                    stats.data.reset(new char[stats.dataLen]);
                    bb.readByteArray(stats.data.get(), stats.dataLen);
                }

                retainedVec->push_back(stats);
                numServers--;
            }

            rc = filterUpdatelistener.onRetainedStatsChange(&(status->info), status->uid, retainedVec);
            if  (rc != ISMRC_OK)
            {
                Trace_Error(this, "deliver_retained_changes()", "Error: calling onRetainedStatsChange()","RC", rc);
                return rc;
            }

            status->sqn_retained_stats_last_updated = sqn;
        }
    }

    Trace_Exit(this, "deliver_retained_changes()");
    return ISMRC_OK;
}

void ViewKeeper::deliver_monitoring_changes(
    spdr::event::AttributeMap_SPtr attr_map,
    RemoteServerStatus_SPtr status)
{
    Trace_Entry(this, "deliver_monitoring_changes()");

    spdr::event::AttributeMap::const_iterator pos = attr_map->find(FilterTags::MonitoringStatus);
    if (pos != attr_map->end())
    {
        ByteBufferReadOnlyWrapper bb(pos->second.getBuffer().get(), pos->second.getLength());
        uint64_t sqn = static_cast<uint64_t>(bb.readLong());
        if (sqn > status->sqn_monitoring_status_last_update)
        {
            status->sqn_monitoring_status_last_update = sqn;
            status->healthStatus = static_cast<ismCluster_HealthStatus_t>(bb.readChar());
            status->haStatus = static_cast<ismCluster_HaStatus_t>(bb.readChar());
            Trace_Debug(this, "deliver_monitoring_changes()", "Updated",
                    "sqn", boost::lexical_cast<std::string>(sqn),
                    "Health", boost::lexical_cast<std::string>(status->healthStatus),
                    "HA", boost::lexical_cast<std::string>(status->haStatus));
        }
    }
    else
    {
        Trace_Debug(this, "deliver_monitoring_changes()", "attribute key not found, setting to UNKNOWN");
        status->healthStatus = ISM_CLUSTER_HEALTH_UNKNOWN;
        status->haStatus = ISM_CLUSTER_HA_UNKNOWN;
    }

    Trace_Exit(this, "deliver_monitoring_changes()");
}

int ViewKeeper::deliver_RemovedServersList_changes(
        spdr::event::AttributeMap_SPtr attr_map,
        RemoteServerStatus_SPtr status)
{
    Trace_Entry(this, "deliver_RemovedServersList_changes()");
    int rc = ISMRC_OK;
    spdr::event::AttributeMap::const_iterator pos = attr_map->find(FilterTags::RemovedServersList);
    if (pos != attr_map->end())
    {
        ByteBufferReadOnlyWrapper bb(pos->second.getBuffer().get(), pos->second.getLength());
        uint64_t sqn = static_cast<uint64_t>(bb.readLong());
        if (sqn > status->sqn_removed_servers_last_update)
        {
            status->sqn_removed_servers_last_update = sqn;
            RemoteServerVector newServers;
            bool changed = removedServers_.readMerge(SubCoveringFilterWireFormat::ATTR_VERSION, bb, newServers);

            if (changed)
            {
                Trace_Event(this, "deliver_RemovedServersList_changes", "RemovedServers changed",
                        "removed-servers", removedServers_.toString());

                {
                    boost::recursive_mutex::scoped_lock lock(storeSelfRecord_mutex_);
                    storeRemovedServersPending_ = true;
                }

                if ((rc = storeRecoverySelfRecord()) != ISMRC_OK)
                {
                    Trace_Error(this, "deliver_RemovedServersList_changes", "Error: failed to persist removed servers list to the store", "RC",rc);
                    return rc;
                }

                mcp::AbstractTask_SPtr task(new PublishRemovedServersTask(controlManager_));
                taskExecutor_.scheduleDelay(task, TaskExecutor::ZERO_DELAY);
            }
            else
            {
                Trace_Event(this, "deliver_RemovedServersList_changes", "RemovedServers unchanged");
            }


            RemoteServerVector pendingRemovals;
            for (RemoteServerVector::iterator it = newServers.begin(); it != newServers.end(); ++it)
            {
                String bogus_id_str = String((*it)->serverUID) + ",,,0";
                spdr::NodeID_SPtr bogus_id =
                        spdr::SpiderCastFactory::getInstance().createNodeID_SPtr(bogus_id_str);

                if (*bogus_id != *my_nodeID)
                {
                    Trace_Event(this, "deliver_RemovedServersList_changes", "new Removed Server",
                            "removed-server", bogus_id_str,
                            "inc", spdr::stringValueOf((*it)->incarnationNumber));
                    pendingRemovals.push_back((*it));
                }
                else
                {
                    rc = ISMRC_ClusterLocalServerRemoved;
                    std::string what("Error: discovered local UID on the removed server list, going to shut down.");
                    what = what + " This server will restart in maintenance mode."
                            + " The local server's UID may have been removed from the cluster while local server was down or behind a network partition.";
                    Trace_Error(this, "deliver_RemovedServersList_changes", what,
                            "removed-server", bogus_id_str,
                            "inc", spdr::stringValueOf((*it)->incarnationNumber),
                            "RC", spdr::stringValueOf(rc));
                    mcp::AbstractTask_SPtr task(new RequestAdminMaintenanceModeTask(controlManager_, ISMRC_ClusterLocalServerRemoved,1));
                    taskExecutor_.scheduleDelay(task, TaskExecutor::ZERO_DELAY);
                    return rc;
                }
            }

            if (!pendingRemovals.empty())
            {
                mcp::AbstractTask_SPtr task(new AdminDeleteRemovedServersTask(controlManager_, pendingRemovals));
                taskExecutor_.scheduleDelay(task, TaskExecutor::ZERO_DELAY);
            }
        }
    }
    else
    {
        Trace_Debug(this, "deliver_RemovedServersList_changes()", "attribute key not found, ignoring");
    }

    Trace_Exit(this, "deliver_RemovedServersList_changes()",rc);
    return rc;
}

int ViewKeeper::deliver_RestoredNotInView_changes(
                spdr::event::AttributeMap_SPtr attr_map,
                RemoteServerStatus_SPtr status)
{
    Trace_Entry(this, "deliver_RestoredNotInView_changes()");
    int rc = ISMRC_OK;
    spdr::event::AttributeMap::const_iterator pos = attr_map->find(FilterTags::RestoredNotInView);
    if (pos != attr_map->end())
    {
        ByteBufferReadOnlyWrapper bb(pos->second.getBuffer().get(), pos->second.getLength());
        uint64_t sqn = static_cast<uint64_t>(bb.readLong());
        if (sqn > status->sqn_restored_not_in_view_last_update)
        {
            status->sqn_restored_not_in_view_last_update = sqn;
            RemoteServerVector new_servers;
            const int32_t num_records = bb.readInt();
            std::ostringstream oss;
            oss << "sqn=" << sqn << " #records=" << num_records << ", ";
            for (int i=0; i<num_records; ++i)
            {
                std::string uid = bb.readString();
                std::string name = bb.readString();
                int64_t inc = bb.readLong();

                String bogus_id_str = uid + ",,,0";
                spdr::NodeID_SPtr bogus_id = spdr::SpiderCastFactory::getInstance().createNodeID_SPtr(bogus_id_str);

                bool is_new = false;
                if (uid != my_nodeID->getNodeName())
                {
                    if (serverRegistryMap.count(bogus_id) == 0)
                    {
                        RemoteServerRecord_SPtr server(new RemoteServerRecord(uid,name,inc));
                        new_servers.push_back(server);
                        is_new = true;
                    }

                    if (spdr::ScTraceBuffer::isDebugEnabled(tc_))
                    {
                        oss << uid << "," << name << "," << inc << (is_new ? ",New; " : "; ");
                    }
                }
                else
                {
                    oss << uid << "," << name << "," << inc << ",Me-ignored; ";
                }
            }

            oss << " #new=" << new_servers.size() << "; ";
            for (std::size_t i=0; i<new_servers.size(); ++i)
            {
                int rc1 = addUpdate_RestoredNotInView(new_servers[i]);
                if (rc1 != ISMRC_OK)
                {
                    Trace_Error(this, "deliver_RestoredNotInView_changes()", "Error: failure to addUpdate_RestoredNotInView",
                            "server", new_servers[i]->toString(), "record", oss.str(), "RC", spdr::stringValueOf(rc1));
                    return rc1;
                }
            }

            Trace_Debug(this, "deliver_RestoredNotInView_changes()", oss.str());
        }
    }
    else
    {
        Trace_Debug(this, "deliver_RestoredNotInView_changes()", "attribute key not found, ignoring");
    }

    Trace_Exit(this, "deliver_RestoredNotInView_changes()",rc);
    return rc;
}

int ViewKeeper::addUpdate_RestoredNotInView(RemoteServerRecord_SPtr server)
{
    using namespace std;

    String bogus_id_str = server->serverUID + ",,,0";
    spdr::NodeID_SPtr bogus_id = spdr::SpiderCastFactory::getInstance().createNodeID_SPtr(bogus_id_str);

    //Add to engine
    ServerIndex serverIndex = allocate_ServerIndex();
    RemoteServerStatus_SPtr status(
            new RemoteServerStatus(
                    server->serverName,
                    server->serverUID,
                    serverIndex,
                    server->incarnationNumber,
                    false,
                    false));
    serverRegistryMap[bogus_id] = status;

    int rc1 = engineServerRegisteration->add(
            &(status->info),
            status->name.c_str(),
            status->uid.c_str(),
            &(status->info.engineHandle));
    if (rc1 != ISMRC_OK && rc1 != ISMRC_Closed)
    {
        Trace_Error(this, "addUpdate_RestoredNotInView()", "Error: calling Engine callback add()",
                "RC", rc1);
        return rc1;
    }
    else if (rc1 == ISMRC_Closed)
    {
        Trace_Event(this, "addUpdate_RestoredNotInView()", "Engine callback add() returned Closed, probably termination, ignoring");
    }

    status->engineAdded = true;
    Trace_Event(this, "addUpdate_RestoredNotInView()", "Engine callback add()",
            "name", status->name,
            "uid", status->uid,
            "index", boost::lexical_cast<string>(status->info.index),
            "engineHandle", boost::lexical_cast<string>(status->info.engineHandle));

    //Update engine with recovery filter, all zeros but incarnation
    RecoveryFilterState recoveryState;
    recoveryState.incarnation_number = status->incarnation;
    rc1 = storeRecoveryFilterState(status, recoveryState);
    if  (rc1 != ISMRC_OK)
    {
        Trace_Error(this, "addUpdate_RestoredNotInView()", "Error: calling storeRecoveryFilterState()", "RC", rc1);
        return rc1;
    }

    //Add to forwarding
    status->forwardingAddress = "";
    status->forwardingPort = 0;
    status->forwardingUseTLS = 0;

    rc1 = forwardingControl->add(
            status->name.c_str(),
            status->uid.c_str(),
            status->forwardingAddress.c_str(),
            status->forwardingPort,
            status->forwardingUseTLS,
            &(status->info),
            status->info.engineHandle,
            &(status->info.protocolHandle));

    if (rc1 != ISMRC_OK && rc1 != ISMRC_Closed)
    {
        Trace_Error(this, "addUpdate_RestoredNotInView()", "Error: calling Forwarding callback add()",
                "RC", rc1);
        return rc1;
    }
    else if (rc1 == ISMRC_Closed)
    {
        Trace_Event(this, "addUpdate_RestoredNotInView()", "Forwarding callback add() returned Closed, probably termination, ignoring");
    }

    status->forwardingAdded = true;
    Trace_Event(this, "addUpdate_RestoredNotInView()", "Forwarding callback add node (disconnected)",
            "index", boost::lexical_cast<string>(status->info.index),
            "uid", status->uid);

    return ISMRC_OK;
}

int ViewKeeper::deliver_filter_changes(
		spdr::event::AttributeMap_SPtr attr_map,
		RemoteServerStatus_SPtr status,
		RecoveryFilterState& recoveryState)
{
	using namespace std;

	Trace_Entry(this, "deliver_filter_changes()", "node", status->uid);

	if (attr_map)
	{
		Sorted_FilterValue_Map sorted_map;
		bool change = filterSort_attribute_map(*attr_map,
				status->sqn_bf_exact_last_update,
				status->sqn_bf_wildcard_last_update,
				status->sqn_bf_wcsp_last_update,
				status->sqn_rcf_last_update,
				recoveryState.bf_exact_lastUpdate,
				recoveryState.bf_wildcard_lastUpdate,
				recoveryState.bf_wcsp_lastUpdate,
				recoveryState.rcf_lastUpdate,
				sorted_map);

		if (change)
		{
			int rc = storeRecoveryFilterState(status, recoveryState);
			if  (rc != ISMRC_OK)
			{
			    Trace_Error(this, "deliver_filter_changes()", "Error: calling storeRecoveryFilterState()", "RC", rc);
				return rc;
			}

			//deliver base-update stuff in order
			for (Sorted_FilterValue_Map::iterator it = sorted_map.begin(); it != sorted_map.end(); ++it)
			{
			    switch (it->second.first)
			    {
			    case VALUE_TYPE_BF_E_BASE:
			    {
			        Trace_Debug(this, "deliver_filter_changes()","bf Exact Base");
			        int rc = deliver_BF_Base(&(status->info), it->second.second, FilterTags::BF_ExactSub);
			        if (rc != ISMRC_OK)
			        {
			            Trace_Error(this, "deliver_filter_changes()", "Error: calling deliver_BF_Base() Exact", "RC", rc);
			            return rc;
			        }
			        status->sqn_bf_exact_base = it->first;
			        status->sqn_bf_exact_last_update = it->first;
			        break;
			    }

			    case VALUE_TYPE_BF_E_UPDT:
			    {
			        Trace_Debug(this, "deliver_filter_changes()","bf Exact Update");
			        int rc = deliver_BF_Update(&(status->info), it->second.second, FilterTags::BF_ExactSub);
			        if (rc != ISMRC_OK)
			        {
			            Trace_Error(this, "deliver_filter_changes()", "Error: calling deliver_BF_Update() Exact", "RC", rc);
			            return rc;
			        }
			        status->sqn_bf_exact_last_update = it->first;
			        break;
			    }

			    case VALUE_TYPE_BF_W_BASE:
			    {
			        int rc = deliver_BF_Base(&(status->info), it->second.second, FilterTags::BF_WildcardSub);
			        if (rc != ISMRC_OK)
			        {
			            Trace_Error(this, "deliver_filter_changes()", "Error: calling deliver_BF_Base() Wildcard", "RC", rc);
			            return rc;
			        }
			        status->sqn_bf_wildcard_base = it->first;
			        status->sqn_bf_wildcard_last_update = it->first;
			        break;
			    }

			    case VALUE_TYPE_BF_W_UPDT:
			    {
			        int rc = deliver_BF_Update(&(status->info), it->second.second, FilterTags::BF_WildcardSub);
			        if (rc != ISMRC_OK)
			        {
			            Trace_Error(this, "deliver_filter_changes()", "Error: calling deliver_BF_Update() Wildcard", "RC", rc);
			            return rc;
			        }
			        status->sqn_bf_wildcard_last_update = it->first;
			        break;
			    }

			    case VALUE_TYPE_RCF_BASE:
			    {
			        int rc = deliver_RCF_Base(status, it->second.second);
			        if (rc != ISMRC_OK)
			        {
			            Trace_Error(this, "deliver_filter_changes()", "Error: calling deliver_RCF_Base()", "RC", rc);
			            return rc;
			        }
			        status->sqn_rcf_base = it->first;
			        status->sqn_rcf_last_update = it->first;
			        break;
			    }

			    case VALUE_TYPE_RCF_UPDT:
			    {
			        int rc = deliver_RCF_Update(status, it->second.second);
			        if (rc != ISMRC_OK)
			        {
			            Trace_Error(this, "deliver_filter_changes()", "Error: calling deliver_RCF_Update()", "RC", rc);
			            return rc;
			        }
			        status->sqn_rcf_last_update = it->first;
			        break;
			    }

			    case VALUE_TYPE_WCSP_BASE:
			    {
			        Trace_Debug(this, "deliver_filter_changes()","wcsp base");
			        int rc = deliver_WCSP_Base(&(status->info), it->second.second);
			        if (rc != ISMRC_OK)
			        {
			            Trace_Error(this, "deliver_filter_changes()", "Error: calling deliver_WCSP_Base()", "RC", rc);
			            return rc;
			        }
			        status->sqn_bf_wcsp_base = it->first;
			        status->sqn_bf_wcsp_last_update = it->first;
			        break;
			    }

			    case VALUE_TYPE_WCSP_UPDT:
			    {
			        Trace_Debug(this, "deliver_filter_changes()","wcsp update");
			        int rc = deliver_WCSP_Update(&(status->info), it->second.second);
			        if (rc != ISMRC_OK)
			        {
			            Trace_Error(this, "deliver_filter_changes()", "Error: calling deliver_WCSP_Update()", "RC", rc);
			            return rc;
			        }
			        status->sqn_bf_wcsp_last_update = it->first;
			        break;
			    }

			    default:
			        break;
			    }
			}
		}
		else
		{
			Trace_Debug(this, "deliver_filter_changes()", "no change");
		}
	}
	else
	{
		Trace_Debug(this, "deliver_filter_changes()", "null attributes");

		if (status->sqn_bf_exact_base > 0 || status->sqn_bf_wildcard_base > 0
				|| status->sqn_bf_wcsp_last_update > 0 || status->sqn_rcf_base > 0)
		{
			//This should not happen
		    Trace_Error(this, "deliver_filter_changes()", "Error: Null BF definition but existing SqnInfo entry");
			return ISMRC_ClusterInternalError;
		}
	}

	Trace_Exit(this, "deliver_filter_changes()");
	return ISMRC_OK;
}

int ViewKeeper::deliver_BF_Base(ismCluster_RemoteServerHandle_t clusterHandle,
		const spdr::event::AttributeValue& attrVal, const std::string& filterTag)
{
	Trace_Entry(this,"deliver_BF_Base()", "tag", filterTag);
	ByteBufferReadOnlyWrapper bb(attrVal.getBuffer().get(), attrVal.getLength());
	//the value wire format (network byte order / big-endian) :
	// (uint64_t) sqnNum
	// (int16_t) bfType,
	// (int16_t) numHash,
	// (int32_t) numBins, (must be a multiple of 8)
	// buffer

	bb.setPosition(8); //skip the sqnNum
	mcc_hash_HashType_t type = static_cast<mcc_hash_HashType_t>(bb.readShort());
	if (type != ISM_HASH_TYPE_NONE)
	{
		int16_t numHash = bb.readShort();
		int32_t numBins = bb.readInt();
		const char* buffp = (bb.getBuffer() + bb.getPosition());
		int rc = filterUpdatelistener.onBloomFilterBase(clusterHandle, filterTag,
				type, numHash, numBins, buffp);
		if (rc != ISMRC_OK)
		{
		    Trace_Error(this, "deliver_BF_Base()", "Error: calling onBloomFilterBase()","RC", rc);
			return rc;
		}
		Trace_Debug(this,"deliver_BF_Base()","regular base delivered");
	}
	else
	{
		int rc = filterUpdatelistener.onBloomFilterRemove(clusterHandle,filterTag);
		if (rc != ISMRC_OK)
		{
		    Trace_Error(this, "deliver_BF_Base()", "Error: calling onBloomFilterRemove()", "RC", rc);
			return rc;
		}
		Trace_Debug(this,"deliver_BF_Base()","empty base, remove delivered");
	}

	Trace_Exit(this,"deliver_BF_Base()");
	return ISMRC_OK;
}

int ViewKeeper::deliver_BF_Update(ismCluster_RemoteServerHandle_t clusterHandle,
		const spdr::event::AttributeValue& attrVal, const std::string& filterTag)
{
	Trace_Entry(this,"deliver_BF_Update()");
	//it is an update
	ByteBufferReadOnlyWrapper bb(attrVal.getBuffer().get(), attrVal.getLength());
	bb.setPosition(8); //skip the sqnNum

	//the value wire format (network byte order / big-endian) :
	// (uint64_t) sqnNum
	// (int32_t) numUpdates,
	// ((int32_t) update)  x numUpdates
	int32_t numUpdates = bb.readInt();
	std::vector<int32_t> updates;

	for (int i = 0; i < numUpdates; ++i)
	{
		updates.push_back(bb.readInt());
	}

	int rc = filterUpdatelistener.onBloomFilterUpdate(clusterHandle,
			filterTag, updates);
	if (rc != ISMRC_OK)
	{
	    Trace_Error(this, "deliver_BF_Update()", "Error: calling onBloomFilterUpdate()","RC", rc);
		return rc;
	}

	Trace_Exit(this,"deliver_BF_Update()");
	return ISMRC_OK;
}

//int ViewKeeper::deliver_RCF_Base(
//		RemoteServerStatus_SPtr status,
//		const spdr::event::AttributeValue& attrVal)
//{
//	using namespace std;
//
//	RemoteServerStatus::RCF_Map rcf_map_new;
//	vector<String_SPtr> rcf_vec_add;
//	vector<String_SPtr> rcf_vec_remove;
//
//	//Format:
//	// uint64_t SQN
//	// uint32_t #filters
//	//  {
//	//    (uint64_t) RegularCoveringFilterID
//	//	  (string) (length, data)
//	//  } x numFilters
//
//	ByteBufferReadOnlyWrapper bb(attrVal.getBuffer().get(), attrVal.getLength());
//	bb.setPosition(8); //skip SQN
//	uint32_t num_RCFs = static_cast<uint32_t>(bb.readInt());
//	for (uint32_t i=0; i<num_RCFs; ++i)
//	{
//		uint64_t rcf_id = static_cast<uint64_t>(bb.readLong());
//		String_SPtr rcf = bb.readStringSPtr();
//		rcf_map_new[rcf_id] = rcf;
//	}
//
//	RemoteServerStatus::RCF_Map::const_iterator first1 = status->rcf_map.begin();
//	RemoteServerStatus::RCF_Map::const_iterator last1 = status->rcf_map.end();
//	RemoteServerStatus::RCF_Map::const_iterator first2 = rcf_map_new.begin();
//	RemoteServerStatus::RCF_Map::const_iterator last2 = rcf_map_new.end();
//
//	while (first1 != last1 && first2 != last2)
//	{
//		if (first1->first < first2->first)
//		{
//			rcf_vec_remove.push_back(first1->second);
//			++first1;
//		}
//		else if (first2->first < first1->first)
//		{
//			rcf_vec_add.push_back(first2->second);
//			++first2;
//		}
//		else
//		{
//			++first1;
//			++first2;
//		}
//	}
//
//	while (first1 != last1)
//	{
//		rcf_vec_remove.push_back(first1->second);
//		++first1;
//	}
//
//	while (first2 != last2)
//	{
//		rcf_vec_add.push_back(first2->second);
//		++first2;
//	}
//
//	int num_changes = std::max(rcf_vec_add.size(), rcf_vec_remove.size());
//	char** pSubs_array = new char*[num_changes];
//
//	//First remove then add, to address the case when the same string is in base1 one ID and in base2 with another ID
//    if (rcf_vec_remove.size() > 0)
//    {
//        for (uint32_t i=0; i<rcf_vec_remove.size(); ++i)
//        {
//            pSubs_array[i] = const_cast<char*>( rcf_vec_remove[i]->c_str() );
//        }
//
//        int rc = serverRegisteration->removeSubscriptions(
//                status->info.engineHandle,
//                &(status->info),
//                status->name.c_str(),
//                status->uid.c_str(),
//                pSubs_array, rcf_vec_remove.size());
//
//        if (rc != ISMRC_OK && rc != ISMRC_Closed)
//        {
//            delete[] pSubs_array;
//
//            Trace_Error(this, "deliver_RCF_Base()", "Error: calling ServerRegistration.removeSubscriptions()", "RC", rc);
//            return rc;
//        }
//        else if (rc == ISMRC_Closed)
//        {
//            Trace_Event(this, "deliver_RCF_Base()", "Engine callback removeSubscriptions() returned Closed, probably termination, ignoring");
//        }
//
//        Trace_Event(this, "deliver_RCF_Base()", "Engine callback removeSubscriptions()",
//                "name", status->name, "uid", status->uid,
//                "#subscriptions", boost::lexical_cast<std::string>(rcf_vec_remove.size()));
//    }
//
//	if (rcf_vec_add.size() > 0)
//	{
//		for (uint32_t i=0; i<rcf_vec_add.size(); ++i)
//		{
//			pSubs_array[i] = const_cast<char*>( rcf_vec_add[i]->c_str() );
//		}
//
//		int rc = serverRegisteration->addSubscriptions(
//				status->info.engineHandle,
//				&(status->info),
//				status->name.c_str(),
//				status->uid.c_str(),
//				pSubs_array, rcf_vec_add.size());
//
//		if (rc != ISMRC_OK && rc != ISMRC_Closed)
//		{
//			delete[] pSubs_array;
//
//			Trace_Error(this, "deliver_RCF_Base()", "Error: calling ServerRegistration.addSubscriptions()", "RC", rc);
//			return rc;
//		}
//        else if (rc == ISMRC_Closed)
//        {
//            Trace_Event(this, "deliver_RCF_Base()", "Engine callback addSubscriptions() returned Closed, probably termination, ignoring");
//        }
//
//		Trace_Event(this, "deliver_RCF_Base()", "Engine callback addSubscriptions()",
//		        "name", status->name, "uid", status->uid,
//		        "#subscriptions", boost::lexical_cast<std::string>(rcf_vec_add.size()));
//	}
//
//	delete[] pSubs_array;
//
//	status->rcf_map.swap(rcf_map_new);
//
//	return ISMRC_OK;
//}

int ViewKeeper::deliver_RCF_Base(
        RemoteServerStatus_SPtr status,
        const spdr::event::AttributeValue& attrVal)
{
    using namespace std;

    RemoteServerStatus::RCF_Map rcf_map_new;
    RemoteServerStatus::RCF_Map rcf_map_add;
    RemoteServerStatus::RCF_Map rcf_map_remove;

    //Format:
    // uint64_t SQN
    // uint32_t #filters
    //  {
    //    (uint64_t) RegularCoveringFilterID
    //    (string) (length, data)
    //  } x numFilters

    ByteBufferReadOnlyWrapper bb(attrVal.getBuffer().get(), attrVal.getLength());
    bb.setPosition(8); //skip SQN
    uint32_t num_RCFs = static_cast<uint32_t>(bb.readInt());
    for (uint32_t i=0; i<num_RCFs; ++i)
    {
        uint64_t rcf_id = static_cast<uint64_t>(bb.readLong());
        String_SPtr rcf = bb.readStringSPtr();
        rcf_map_new[rcf_id] = rcf;
    }

    RemoteServerStatus::RCF_Map::const_iterator first1 = status->rcf_map.begin();
    RemoteServerStatus::RCF_Map::const_iterator last1 = status->rcf_map.end();
    RemoteServerStatus::RCF_Map::const_iterator first2 = rcf_map_new.begin();
    RemoteServerStatus::RCF_Map::const_iterator last2 = rcf_map_new.end();

    while (first1 != last1 && first2 != last2)
    {
        if (first1->first < first2->first)
        {
            rcf_map_remove[first1->first] = first1->second;
            ++first1;
        }
        else if (first2->first < first1->first)
        {
            rcf_map_add[first2->first] = first2->second;
            ++first2;
        }
        else
        {
            ++first1;
            ++first2;
        }
    }

    while (first1 != last1)
    {
        rcf_map_remove[first1->first] = first1->second;
        ++first1;
    }

    while (first2 != last2)
    {
        rcf_map_add[first2->first] = first2->second;
        ++first2;
    }

    unsigned int num_changes = std::max(rcf_map_add.size(), rcf_map_remove.size());
    if (num_changes > pSubs_array_length_)
    {
        pSubs_array_.reset(new char*[num_changes]);
        pSubs_array_length_ = num_changes;
    }

    //First remove then add, to address the case when the same string is in base1 one ID and in base2 with another ID
    if (rcf_map_remove.size() > 0)
    {
        int rc = deliver_RCF_Sequence(status,rcf_map_remove,-1);
        if (rc != ISMRC_OK)
        {
            Trace_Error(this, "deliver_RCF_Base()", "Error: calling deliver_RCF_Sequence remove", "RC", rc);
            return rc;
        }
        Trace_Event(this, "deliver_RCF_Base()", "Removed",
                "name", status->name, "uid", status->uid,
                "#subscriptions", spdr::stringValueOf(rcf_map_remove.size()));
    }

    if (rcf_map_add.size() > 0)
    {
        int rc = deliver_RCF_Sequence(status,rcf_map_add,1);
        if (rc != ISMRC_OK)
        {
            Trace_Error(this, "deliver_RCF_Base()", "Error: calling deliver_RCF_Sequence add", "RC", rc);
            return rc;
        }
        Trace_Event(this, "deliver_RCF_Base()", "Added",
                "name", status->name, "uid", status->uid,
                "#subscriptions", spdr::stringValueOf(rcf_map_add.size()));
    }

    status->rcf_map.swap(rcf_map_new);

    return ISMRC_OK;
}

int ViewKeeper::deliver_RCF_Update(
		RemoteServerStatus_SPtr status,
		const spdr::event::AttributeValue& attrVal)
{
    using namespace std;

    RemoteServerStatus::RCF_Map rcf_map_add;
    RemoteServerStatus::RCF_Map rcf_map_remove;

    //Format:
    // uint64_t SQN
    // uint32_t #filters
    //  {
    //    (uint64_t) RegularCoveringFilterID
    //    (string) (length, data)
    //  } x numFilters

    ByteBufferReadOnlyWrapper bb(attrVal.getBuffer().get(), attrVal.getLength());
    bb.setPosition(8); //skip SQN
    uint32_t num_RCFs = static_cast<uint32_t>(bb.readInt());

    if (pSubs_array_length_ < num_RCFs)
    {
        pSubs_array_.reset(new char*[num_RCFs]);
        pSubs_array_length_ = num_RCFs;
    }

    //deliver series of adds and removes, never change order of add vs. remove
    int action = 0; //-1 remove, 0 - none, 1 - add

    for (uint32_t i=0; i<num_RCFs; ++i)
    {
        uint64_t rcf_id = static_cast<uint64_t>(bb.readLong());
        String_SPtr rcf = bb.readStringSPtr();

        if (rcf && !rcf->empty()) //Add RCF
        {
            if (rcf_map_remove.empty())
            {
                action = 0;
            }
            else if (!rcf_map_remove.empty() && rcf_map_add.empty())
            {
                action = -1; //deliver remove
            }
            else
            {
                //should never happen
                throw MCPRuntimeError("Error in RCF update delivery, add RCF", ISMRC_Error);
            }

            rcf_map_add[rcf_id] = rcf;
            status->rcf_map[rcf_id] = rcf;
        }
        else //Remove RCF
        {
            if (rcf_map_add.empty())
            {
                action = 0;
            }
            else if (!rcf_map_add.empty() && rcf_map_remove.empty())
            {
                action = 1; //deliver add
            }
            else
            {
                //should never happen
                throw MCPRuntimeError("Error in RCF update delivery, remove RCF", ISMRC_Error);
            }

            rcf_map_remove[rcf_id] = status->rcf_map[rcf_id];
            status->rcf_map.erase(rcf_id);
        }

        int rc = ISMRC_OK;
        if (action > 0)
        {
            rc = deliver_RCF_Sequence(status,rcf_map_add,action);
            rcf_map_add.clear();
            action = 0;
        }
        else if (action < 0)
        {
            rc = deliver_RCF_Sequence(status,rcf_map_remove,action);
            rcf_map_remove.clear();
            action = 0;
        }

        if (rc != ISMRC_OK)
        {
            Trace_Error(this, "deliver_RCF_Update()", "Error: calling deliver_RCF_Update_Sequence()", "RC", rc);
            return rc;
        }

        //last round
        if (action==0 && i==(num_RCFs-1) && (!rcf_map_remove.empty() || !rcf_map_add.empty()))
        {
            action = (rcf_map_remove.empty()? 1 : -1);
            if (action > 0)
            {
                rc = deliver_RCF_Sequence(status,rcf_map_add,action);
                rcf_map_add.clear();
                action = 0;
            }
            else if (action < 0)
            {
                rc = deliver_RCF_Sequence(status,rcf_map_remove,action);
                rcf_map_remove.clear();
                action = 0;
            }

            if (rc != ISMRC_OK)
            {
                Trace_Error(this, "deliver_RCF_Update()", "Error: calling deliver_RCF_Update_Sequence()", "RC", rc);
                return rc;
            }

        }
    }//for

    return ISMRC_OK;
}

int ViewKeeper::deliver_RCF_Sequence(
        RemoteServerStatus_SPtr status,
        const RemoteServerStatus::RCF_Map& rcf_map,
        int action)
{
    Trace_Entry(this, __FUNCTION__, "action", spdr::stringValueOf(action), "#subs", spdr::stringValueOf(rcf_map.size()));

    int rc = ISMRC_OK;

    std::ostringstream subs_oss("{");
    uint32_t i=0;
    for (RemoteServerStatus::RCF_Map::const_iterator it = rcf_map.begin(); it != rcf_map.end(); ++it)
    {
        pSubs_array_[i++] = const_cast<char*>( it->second->c_str() );
        if (spdr::ScTraceBuffer::isDumpEnabled(tc_))
        {
            subs_oss << "id=" << it->first << " " <<it->second->c_str() << " ";
        }
    }
    subs_oss << "}";

    if (action > 0)
    {
        Trace_Event(this, __FUNCTION__, "Engine callback addSubscriptions()",
                "name", status->name, "uid", status->uid, "#subscriptions", spdr::stringValueOf(rcf_map.size()));
        Trace_Dump(this, __FUNCTION__, "Engine callback addSubscriptions()",
                "name", status->name, "uid", status->uid, "subscriptions", subs_oss.str());

        int rc = engineServerRegisteration->addSubscriptions(
                status->info.engineHandle,
                &(status->info),
                status->name.c_str(),
                status->uid.c_str(),
                pSubs_array_.get(), rcf_map.size());

        if (rc != ISMRC_OK && rc != ISMRC_Closed)
        {
            Trace_Error(this, __FUNCTION__, "Error: calling ServerRegistration.addSubscriptions()", "RC", rc);
            return rc;
        }
        else if (rc == ISMRC_Closed)
        {
            Trace_Event(this, __FUNCTION__, "Engine callback addSubscriptions() returned Closed, probably termination, ignoring");
            rc = ISMRC_OK;
        }
    }
    else if (action < 0)
    {
        Trace_Event(this, __FUNCTION__, "Engine callback removeSubscriptions()",
                "name", status->name, "uid", status->uid, "#subscriptions", spdr::stringValueOf(rcf_map.size()));
        Trace_Dump(this, __FUNCTION__, "Engine callback removeSubscriptions()",
                "name", status->name, "uid", status->uid, "subscriptions", subs_oss.str());

        int rc = engineServerRegisteration->removeSubscriptions(
                status->info.engineHandle,
                &(status->info),
                status->name.c_str(),
                status->uid.c_str(),
                pSubs_array_.get(), rcf_map.size());

        if (rc != ISMRC_OK && rc != ISMRC_Closed)
        {
            Trace_Error(this, __FUNCTION__, "Error: calling ServerRegistration.removeSubscriptions()", "RC", rc);
        }
        else if (rc == ISMRC_Closed)
        {
            Trace_Event(this, __FUNCTION__, "Engine callback removeSubscriptions() returned Closed, probably termination, ignoring");
            rc = ISMRC_OK;
        }
    }

    Trace_Exit(this, __FUNCTION__, rc);
    return rc;
}

int ViewKeeper::deliver_WCSP_Base(
		ismCluster_RemoteServerHandle_t clusterHandle,
		const spdr::event::AttributeValue& attrVal)
{
	using namespace std;
	Trace_Entry(this, "deliver_WCSP_Base()");

	vector<SubCoveringFilterPublisher::SubscriptionPatternUpdate> pattern_vec;

	//Format:
	// uint64_t SQN
	// uint32_t #patterns
	ByteBufferReadOnlyWrapper bb(attrVal.getBuffer().get(), attrVal.getLength());
	bb.setPosition(8); //skip SQN
	uint32_t num_patterns = static_cast<uint32_t>(bb.readInt());

	for (uint32_t i=0; i<num_patterns; ++i)
	{
		/* Format:
		 * uint64_t ID
		 * SupscriptionPattern (not empty)
		 */
		uint64_t id = static_cast<uint64_t>(bb.readLong());
		SubscriptionPattern_SPtr pattern;
		SubCoveringFilterWireFormat::readSubscriptionPattern(SubCoveringFilterWireFormat::ATTR_VERSION, bb,pattern);

		if (!pattern || pattern->empty())
		{
			Trace_Error(this, "deliver_RCF_Update()", "Error: WC subscription pattern is empty in base");
			return ISMRC_ClusterInternalError;
		}
		else
		{
			pattern_vec.push_back(std::make_pair(id, pattern));
		}
	}

	int rc  = filterUpdatelistener.onWCSubscriptionPatternBase(clusterHandle,pattern_vec);
	if (rc != ISMRC_OK)
	{
		Trace_Error(this, "deliver_WCSP_Base()", "Error: calling onWCSubscriptionPatternBase()", "RC", rc);
		return rc;
	}

	Trace_Exit(this, "deliver_WCSP_Base()");
	return ISMRC_OK;
}

int ViewKeeper::deliver_WCSP_Update(
		ismCluster_RemoteServerHandle_t clusterHandle,
		const spdr::event::AttributeValue& attrVal)
{
	using namespace std;

	Trace_Entry(this, "deliver_WCSP_Update()");

	vector<SubCoveringFilterPublisher::SubscriptionPatternUpdate> pattern_vec;

	//Format:
	// uint64_t SQN
	// uint32_t #patterns
	ByteBufferReadOnlyWrapper bb(attrVal.getBuffer().get(), attrVal.getLength());
	bb.setPosition(8); //skip SQN
	uint32_t num_patterns = static_cast<uint32_t>(bb.readInt());

	for (uint32_t i=0; i<num_patterns; ++i)
	{
		SubCoveringFilterPublisher::SubscriptionPatternUpdate id_pattern;

		/* Format:
		 * uint64_t ID
		 * SubscriptionPattern
		 */
		uint64_t id = static_cast<uint64_t>(bb.readLong());
		SubscriptionPattern_SPtr pattern;
		SubCoveringFilterWireFormat::readSubscriptionPattern(SubCoveringFilterWireFormat::ATTR_VERSION, bb,pattern);

		SubCoveringFilterPublisher::SubscriptionPatternUpdate update;
		if (!pattern || pattern->empty())
		{
			update.first = id;
		}
		else
		{
			update.first = id;
			update.second = pattern;
		}
		pattern_vec.push_back(update);
	}

	int rc = filterUpdatelistener.onWCSubscriptionPatternUpdate(clusterHandle,pattern_vec);
	if (rc != ISMRC_OK)
	{
		Trace_Error(this, "deliver_WCSP_Update()", "Error: onWCSubscriptionPatternUpdate()", "RC", rc);
	}

	Trace_Exit(this, "deliver_WCSP_Update()");
	return rc;
}

ServerIndex ViewKeeper::allocate_ServerIndex()
{
	if (serverIndex_gaps.empty())
	{
		return (++serverIndex_max);
	}
	else
	{
		ServerIndex index = *(serverIndex_gaps.begin());
		serverIndex_gaps.erase(serverIndex_gaps.begin());
		return index;
	}
}

void ViewKeeper::free_ServerIndex(ServerIndex index)
{
	serverIndex_gaps.insert(index);
}

bool ViewKeeper::extractFwdEndPoint(const spdr::event::AttributeMap& attr_map, std::string& addr, uint16_t& port, uint8_t& fUseTLS)
{
    Trace_Entry(this, "extractFwdEndPoint()");

    bool res = false;

	spdr::event::AttributeMap::const_iterator pos = attr_map.find(FilterTags::Fwd_Endpoint);
	if (pos != attr_map.end())
	{
		ByteBufferReadOnlyWrapper bb(pos->second.getBuffer().get(), pos->second.getLength());
		/* Format:
		 * string Address
		 * uint16_t Port
		 * boolean fUseTLS
		 */
		addr = bb.readString();
		port = static_cast<uint16_t>(bb.readShort());
		if (bb.readBoolean())
			fUseTLS = 1;
		else
			fUseTLS = 0;

		res = true;
	}
	else
	{
		res = false;
	}

	Trace_Exit(this, "extractFwdEndPoint()",res);
	return res;
}

bool ViewKeeper::extractServerInfo(const spdr::event::AttributeMap& attr_map, uint16_t& protoVerSupported,  uint16_t& protoVerUsed, std::string& serverName)
{
    Trace_Entry(this, "extractServerInfo");
    bool res = false;

	spdr::event::AttributeMap::const_iterator pos = attr_map.find(FilterTags::LocalServerInfo);
	if (pos != attr_map.end())
	{
		ByteBufferReadOnlyWrapper bb(pos->second.getBuffer().get(), pos->second.getLength());
		/* Format:
		 * uint16_t wire format supported-version
		 * uint16_t wire format used-version
		 * string ServerName
		 */
		protoVerSupported = static_cast<uint16_t>(bb.readShort());
		protoVerUsed = static_cast<uint16_t>(bb.readShort());
		serverName = bb.readString();
		res = true;
	}
	else
	{
		res = false;
	}

	Trace_Exit(this, "extractServerInfo",res);
	return res;
}


int ViewKeeper::deliver_WCSubStats_Update(const spdr::event::AttributeMap& attr_map, RemoteServerStatus_SPtr status)
{
	int rc = ISMRC_OK;

	spdr::event::AttributeMap::const_iterator pos = attr_map.find(FilterTags::WCSub_Stats);
	if (pos != attr_map.end())
	{
		ByteBufferReadOnlyWrapper bb(pos->second.getBuffer().get(), pos->second.getLength());
		uint64_t sqn = static_cast<uint64_t>(bb.readLong());
		if (sqn > status->sqn_sub_stats)
		{
			RemoteSubscriptionStats stats;
			rc = SubCoveringFilterWireFormat::readSubscriptionStats(SubCoveringFilterWireFormat::ATTR_VERSION, bb,&stats);
			if (rc != ISMRC_OK)
			{
				return rc;
			}
			rc = subscriptionStatsListener.update(&(status->info), status->uid.c_str(), stats);
			if (rc != ISMRC_OK)
			{
				return rc;
			}

			status->sqn_sub_stats = sqn;
		}
	}

	return rc;
}

int ViewKeeper::reconcileRecoveryState(RemoteServerStatus_SPtr status)
{
	ServerIndex index = status->info.index;
	if (recoveryFilterState_Map_.count(index) > 0)
	{
		bool reconciliation_over = true;

		if (status->incarnation
				== recoveryFilterState_Map_[index].incarnation_number)
		{
			if (recoveryFilterState_Map_[index].bf_exact_lastUpdate
					> status->sqn_bf_exact_last_update)
			{
				reconciliation_over = false;
			}

			if (recoveryFilterState_Map_[index].bf_wildcard_lastUpdate
					> status->sqn_bf_wildcard_last_update)
			{
				reconciliation_over = false;
			}

			if (recoveryFilterState_Map_[index].bf_wcsp_lastUpdate
					> status->sqn_bf_wcsp_last_update)
			{
				reconciliation_over = false;
			}

			if (recoveryFilterState_Map_[index].rcf_lastUpdate
					> status->sqn_rcf_last_update)
			{
				reconciliation_over = false;
			}
		}
		else if (status->incarnation
				> recoveryFilterState_Map_[index].incarnation_number)
		{
			if (recoveryFilterState_Map_[index].bf_exact_lastUpdate > 0
					&& status->sqn_bf_exact_last_update == 0)
			{
				reconciliation_over = false;
			}

			if (recoveryFilterState_Map_[index].bf_wildcard_lastUpdate > 0
					&& status->sqn_bf_wildcard_last_update == 0)
			{
				reconciliation_over = false;
			}

			if (recoveryFilterState_Map_[index].bf_wcsp_lastUpdate > 0
					&& status->sqn_bf_wcsp_last_update == 0)
			{
				reconciliation_over = false;
			}

			if (recoveryFilterState_Map_[index].rcf_lastUpdate > 0
					&& status->sqn_rcf_last_update == 0)
			{
				reconciliation_over = false;
			}
		}

		if (reconciliation_over)
		{
		    if (status->routeAll)
		    {
		        status->routeAll = false;
		        int rc = filterUpdatelistener.setRouteAll(&(status->info), 0);
		        if (rc != ISMRC_OK)
		        {
		            Trace_Error(this, "reconcileRecoveryState()", "Error: setRouteAll()", "RC", rc);
		            return rc;
		        }
		        else
		        {
		            Trace_Event(this, "reconcileRecoveryState()", "ROUTR_ALL OFF", "uid", status->uid);
		        }

		        rc = engineServerRegisteration->route(status->info.engineHandle,
		                &(status->info), status->name.c_str(), status->uid.c_str(), 0);
		        if (rc != ISMRC_OK && rc != ISMRC_Closed)
		        {
		            Trace_Error(this, "reconcileRecoveryState()", "Error calling serverRegisteration.route()", "RC", rc);
		            return rc;
		        }
		        else if (rc == ISMRC_Closed)
		        {
		            Trace_Event(this, "reconcileRecoveryState()", "Engine callback route() returned Closed, probably termination, ignoring");
		        }

		        Trace_Event(this, "reconcileRecoveryState()", "Engine callback route(), ROUTR_ALL OFF",
		                "name", status->name, "uid", status->uid,
		                "index", boost::lexical_cast<std::string>(status->info.index));
		    }
		    recoveryFilterState_Map_.erase(index);
		}
	}

	return ISMRC_OK;
}

ViewKeeper::RecoveryFilterState ViewKeeper::getRecoveryFilterState(RemoteServerStatus_SPtr status)
{
	RecoveryFilterState recovery_state;
	recovery_state.incarnation_number = status->incarnation;
	recovery_state.bf_exact_lastUpdate = status->sqn_bf_exact_last_update;
	recovery_state.bf_wildcard_lastUpdate = status->sqn_bf_wildcard_last_update;
	recovery_state.bf_wcsp_lastUpdate = status->sqn_bf_wcsp_last_update;
	recovery_state.rcf_lastUpdate = status->sqn_rcf_last_update;

	return recovery_state;
}

int ViewKeeper::storeRecoveryFilterState(
		RemoteServerStatus_SPtr status,
		RecoveryFilterState& filter_sqn_vector )
{
	Trace_Entry(this, "storeRecoveryFilterState()", "node", status->uid);

	storeRecoveryState_ByteBuffer_->reset();
	storeRecoveryState_ByteBuffer_->writeShort(static_cast<int16_t>(SubCoveringFilterWireFormat::STORE_VERSION));
	storeRecoveryState_ByteBuffer_->writeChar(static_cast<int8_t>(SubCoveringFilterWireFormat::Store_Remote_Server_Record));
	storeRecoveryState_ByteBuffer_->writeLong(filter_sqn_vector.incarnation_number);
	storeRecoveryState_ByteBuffer_->writeLong(filter_sqn_vector.bf_exact_lastUpdate);
	storeRecoveryState_ByteBuffer_->writeLong(filter_sqn_vector.bf_wildcard_lastUpdate);
	storeRecoveryState_ByteBuffer_->writeLong(filter_sqn_vector.bf_wcsp_lastUpdate);
	storeRecoveryState_ByteBuffer_->writeLong(filter_sqn_vector.rcf_lastUpdate);

	ismEngine_RemoteServer_PendingUpdateHandle_t  hPendingUpdateHandle = NULL;

	int rc = engineServerRegisteration->update(
			status->info.engineHandle,
			&(status->info),
			status->name.c_str(),
			status->uid.c_str(),
			static_cast<void*>(const_cast<char*>(storeRecoveryState_ByteBuffer_->getBuffer())),
			storeRecoveryState_ByteBuffer_->getDataLength(),
			1, //commit
			&hPendingUpdateHandle);

	if (rc == ISMRC_Closed)
	{
	    Trace_Event(this, "storeRecoveryFilterState()", "Engine callback update() returned Closed, probably termination, ignoring");
	    rc = ISMRC_OK;
	}

	if (spdr::ScTraceBuffer::isEventEnabled(tc_))
	{
	    spdr::ScTraceBufferAPtr buffer = spdr::ScTraceBuffer::event(this, __FUNCTION__, "Engine callback update(), COMMIT");
	    buffer->addProperty("name", status->name);
	    buffer->addProperty("uid", status->uid);
	    buffer->addProperty("size", storeRecoveryState_ByteBuffer_->getDataLength());
	    buffer->invoke();
	}

	Trace_Exit(this, "storeRecoveryFilterState()", rc);
	return rc;
}

int ViewKeeper::storeRecoverySelfRecord()
{
	Trace_Entry(this, "storeRecoverySelfRecord()");

	bool do_store = false;
	std::vector<SubscriptionPattern_SPtr> store_patterns;

	{
	    boost::recursive_mutex::scoped_lock lock(storeSelfRecord_mutex_);
	    if (storePatternsPending_ || storeRemovedServersPending_ || storeFirstTime_ || selfNodePrev_)
	    {
	        store_patterns.assign(storePatterns_.begin(),storePatterns_.end());
	        storePatternsPending_ = false;
	        storeRemovedServersPending_ = false;
	        storeFirstTime_ = false;
	        do_store = true;
	    }
	}

	if (do_store)
	{
	    boost::recursive_mutex::scoped_lock lock(view_mutex);

	    if (selfNodePrev_)
	    {
            Trace_Event(this, "storeRecoverySelfRecord()", "Previous self UID record",
                    "name", selfNodePrev_Name_, "uid", selfNodePrev_UID_);
	        int rc = engineServerRegisteration->remove(
	                selfNode_ClusterHandle_.engineHandle,
	                &selfNode_ClusterHandle_,
	                selfNodePrev_Name_.c_str(),
	                selfNodePrev_UID_.c_str());
	        if (rc != ISMRC_OK && rc != ISMRC_Closed)
	        {
	            Trace_Error(this, "storeRecoverySelfRecord()", "Error calling Engine callback remove()", "RC", rc);
	            return rc;
	        }
	        else if (rc == ISMRC_Closed)
	        {
	            Trace_Event(this, "storeRecoverySelfRecord()", "Engine callback remove() returned Closed, probably termination, ignoring");
	        }
	        Trace_Event(this, "storeRecoverySelfRecord()", "Engine callback remove()");
	        selfNode_ClusterHandle_.engineHandle = NULL;
	        selfNodePrev_ = false;
	    }

	    if (selfNode_ClusterHandle_.engineHandle == NULL)
	    {
	        int rc = engineServerRegisteration->createLocal(
	                &selfNode_ClusterHandle_,
	                my_ServerName.c_str(), //mcpConfig_.getServerName().c_str(),
	                my_ServerUID.c_str(), //mcpConfig_.getServerUID().c_str(),
	                &(selfNode_ClusterHandle_.engineHandle));
	        if (rc != ISMRC_OK && rc != ISMRC_Closed)
	        {
	            Trace_Error(this, "storeRecoverySelfRecord()", "Error calling Engine callback createLocal()", "RC", rc);
	            return rc;
	        }
	        else if (rc == ISMRC_Closed)
	        {
	            Trace_Event(this, "storeRecoverySelfRecord()", "Engine callback createLocal() returned Closed, probably termination, ignoring");
	        }

	        Trace_Event(this, "storeRecoverySelfRecord()", "Engine callback createLocal()",
	                "name", my_ServerName, "uid", my_ServerUID);
	    }

	    storeRecoveryState_ByteBuffer_->reset();
	    storeRecoveryState_ByteBuffer_->writeShort(static_cast<int16_t>(SubCoveringFilterWireFormat::STORE_VERSION));
	    storeRecoveryState_ByteBuffer_->writeChar(static_cast<int8_t>(SubCoveringFilterWireFormat::Store_Local_Server_Record));
	    storeRecoveryState_ByteBuffer_->writeLong(incarnationNumber);
	    storeRecoveryState_ByteBuffer_->writeInt(static_cast<int32_t>(store_patterns.size()));
	    for (uint32_t i=0; i<store_patterns.size(); ++i)
	    {
	        SubCoveringFilterWireFormat::writeSubscriptionPattern(SubCoveringFilterWireFormat::ATTR_VERSION, *store_patterns[i],storeRecoveryState_ByteBuffer_);
	    }

	    removedServers_.write(SubCoveringFilterWireFormat::ATTR_VERSION, storeRecoveryState_ByteBuffer_);
	    storeRecoveryState_ByteBuffer_->writeString(my_ClusterName);

	    ismEngine_RemoteServer_PendingUpdateHandle_t  hPendingUpdateHandle = NULL;

	    int rc1 = engineServerRegisteration->update(
	            selfNode_ClusterHandle_.engineHandle,
	            &selfNode_ClusterHandle_,
	            my_ServerName.c_str(), //mcpConfig_.getServerName().c_str(),
	            my_ServerUID.c_str(), //mcpConfig_.getServerUID().c_str(),
	            static_cast<void*>(const_cast<char*>(storeRecoveryState_ByteBuffer_->getBuffer())),
	            storeRecoveryState_ByteBuffer_->getDataLength(),
	            1, //commit
	            &hPendingUpdateHandle);

	    if (rc1 != ISMRC_OK && rc1 != ISMRC_Closed)
	    {
	        Trace_Error(this, "storeRecoverySelfRecord()", "Error calling Engine callback update()", "RC", rc1);
	        return rc1;
	    }
        else if (rc1 == ISMRC_Closed)
        {
            Trace_Event(this, "storeRecoverySelfRecord()", "Engine callback update() returned Closed, probably termination, ignoring");
        }

	    Trace_Event(this, "storeRecoverySelfRecord()", "Engine callback update(), COMMIT",
	            "name", my_ServerName,
	            "uid", my_ServerUID,
	            "size", boost::lexical_cast<std::string>(storeRecoveryState_ByteBuffer_->getDataLength()));


	    Trace_Debug(this,  "storeRecoverySelfRecord()", "stored patterns",
	            "num", boost::lexical_cast<std::string>(store_patterns.size()));
        Trace_Debug(this, "storeRecoverySelfRecord()", "stored RemovedServers list",
                "removed-servers", removedServers_.toString());

	}

	Trace_Exit(this, "storeRecoverySelfRecord()");
	return ISMRC_OK;
}

int ViewKeeper::onFatalError(const std::string& component, const std::string& errorMessage, int rc)
{
	if (fatalErrorHandler_ == NULL)
	{   //Only in unit test when we don't have a handler
		std::string what(component);
		what += ": " + errorMessage;
		throw MCPRuntimeError(what, static_cast<MCPReturnCode>(rc));
	}
	else
	{
		return fatalErrorHandler_->onFatalError(component, errorMessage, rc);
	}
}

bool ViewKeeper::isReconciliationFinished() const
{
	boost::recursive_mutex::scoped_lock lock(view_mutex);
	return recoveryFilterState_Map_.empty();
}

MCPReturnCode ViewKeeper::getStatistics(ismCluster_Statistics_t *pStatistics) const
{
	boost::recursive_mutex::scoped_lock lock(view_mutex);

	pStatistics->connectedServers = 0;
	pStatistics->disconnectedServers = 0;
	for (ServerRegistryMap::const_iterator it = serverRegistryMap.begin(); it != serverRegistryMap.end(); ++it)
	{
		if (it->second->controlConnected && it->second->forwardingConnected)
		{
			pStatistics->connectedServers++;
		}
		else
		{
			pStatistics->disconnectedServers++;
		}
	}

	return ISMRC_OK;
}

int ViewKeeper::reportEngineStatistics(ismCluster_EngineStatistics_t *pEngineStatistics)
{
    Trace_Entry(this, "reportEngineStatistics");

    int rc = ISMRC_OK;

    {
        boost::recursive_mutex::scoped_lock lock(view_mutex);
        if (state_ == STATE_RECOVERED || state_ == STATE_ACTIVE)
        {
            rc = engineServerRegisteration->reportEngineStatistics(pEngineStatistics);
            if (rc != ISMRC_OK)
            {
                Trace_Error(this, __FUNCTION__, "Error: failure calling Engine callback reportEngineStatistics()", "RC", rc);
            }
        }
        else
        {
            Trace_Event(this, __FUNCTION__, "State is not STATE_RECOVERED | STATE_ACTIVE, skipping task", "state", spdr::stringValueOf(state_));
            rc = ISMRC_ClusterNotAvailable;
        }
    }

    Trace_Exit(this, "reportEngineStatistics", rc);
    return rc;
}


MCPReturnCode ViewKeeper::close()
{
    Trace_Entry(this, "close");

    {
        boost::recursive_mutex::scoped_lock lock(view_mutex);
        state_ = STATE_CLOSED;
    }

    Trace_Exit(this, "notifyTerm");
    return static_cast<MCPReturnCode>(ISMRC_OK);
}


MCPReturnCode ViewKeeper::notifyTerm()
{
    Trace_Entry(this, "notifyTerm");

    int rc = ISMRC_OK;

    {
        boost::recursive_mutex::scoped_lock lock(view_mutex);

        //set a flag that will prevent future invocations of the callbacks
        state_ = STATE_CLOSED_DETACHED;

        //first notify forwarder then engine because engineHandle(s) are destroyed after calling term() on engine
        int rc2 = forwardingControl->term();
        if (rc2 != ISMRC_OK && rc2 != ISMRC_Closed)
        {
            Trace_Warning(this, "notifyTerm()", "Error calling forwardingControl.term(), ignoring",
                    "RC", boost::lexical_cast<std::string>(rc2));
            rc = rc2;
        }
        else if (rc2 == ISMRC_Closed)
        {
            Trace_Event(this, "notifyTerm()", "Protocol callback term() returned Closed, probably termination, ignoring");
        }

        Trace_Config(this, "notifyTerm()", "Protocol callback term()");

        int rc1 = engineServerRegisteration->term();
        if (rc1 != ISMRC_OK && rc1 != ISMRC_Closed)
        {
            Trace_Warning(this, "notifyTerm()", "Error calling serverRegisteration.term(), ignoring",
                    "RC", boost::lexical_cast<std::string>(rc1));
            rc = rc1;
        }
        else if (rc1 == ISMRC_Closed)
        {
            Trace_Event(this, "notifyTerm()", "Engine callback term() returned Closed, probably termination, ignoring");
        }

        Trace_Config(this, "notifyTerm()", "Engine callback term()");
    }

    Trace_Exit(this, "notifyTerm",rc);
    return static_cast<MCPReturnCode>(rc);
}

MCPReturnCode ViewKeeper::getView(ismCluster_ViewInfo_t **pView)
{
    Trace_Entry(this, "getView()");
    MCPReturnCode rc = ISMRC_OK;

    {
        boost::recursive_mutex::scoped_lock lock(view_mutex);
        try
        {
            (*pView)->pLocalServer->phServerHandle = &selfNode_ClusterHandle_;
            (*pView)->numRemoteServers = serverRegistryMap.size();
            (*pView)->pRemoteServers = NULL;
            if (serverRegistryMap.size()>0)
            {
                (*pView)->pRemoteServers =
                        new ismCluster_RSViewInfo_t[serverRegistryMap.size()];
                ismCluster_RSViewInfo_t* pRS_view_info = (*pView)->pRemoteServers;
                int actualNumRemoteServers = 0;
                for (ServerRegistryMap::const_iterator it = serverRegistryMap.begin();
                        it != serverRegistryMap.end(); ++it)
                {
                    // forwarding is the monitoring hub, all valid remote servers are added to it
                    // therefore do not report servers that were not added to forwarding
                    if (!it->second->forwardingAdded)
                    {
                    	Trace_Dump(this,__FUNCTION__, "Node not added to forwarding, Skipped.",
                    			"name", it->second->name, "uid", it->second->uid);
                        continue;
                    }

                    pRS_view_info->pServerName = ism_common_strdup(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,1000),it->second->name.c_str());
                    if (pRS_view_info->pServerName == NULL)
                    {
                        return ISMRC_AllocateError;
                        //throw std::bad_alloc();
                    }

                    pRS_view_info->pServerUID = ism_common_strdup(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,1000),it->second->uid.c_str());
                    if (pRS_view_info->pServerUID == NULL)
                    {
                        return ISMRC_AllocateError;
                    }

                    pRS_view_info->phServerHandle = &(it->second->info);

                    if (!it->second->controlConnected)
                    {
                        pRS_view_info->state = ISM_CLUSTER_RS_STATE_INACTIVE;
                        pRS_view_info->healthStatus = ISM_CLUSTER_HEALTH_UNKNOWN;
                        pRS_view_info->haStatus = ISM_CLUSTER_HA_UNKNOWN;
                    }
                    else
                    {
                        pRS_view_info->state = (it->second->forwardingConnected) ?
                                ISM_CLUSTER_RS_STATE_ACTIVE :
                                ISM_CLUSTER_RS_STATE_CONNECTING;
                        pRS_view_info->healthStatus = it->second->healthStatus;
                        pRS_view_info->haStatus = it->second->haStatus;
                    }

                    if (spdr::ScTraceBuffer::isDumpEnabled(tc_))
                    {
                        spdr::ScTraceBufferAPtr buffer = spdr::ScTraceBuffer::dump(this,__FUNCTION__, "prepared RSViewInfo");
                        buffer->addProperty("state", spdr::stringValueOf(pRS_view_info->state));
                        buffer->addProperty("healthStatus", spdr::stringValueOf(pRS_view_info->healthStatus));
                        buffer->addProperty("haStatus", spdr::stringValueOf(pRS_view_info->healthStatus)),
                                buffer->addProperty("Status", spdr::toString(it->second));
                        buffer->invoke();
                    }

                    pRS_view_info->stateChangeTime = it->second->connectivityChangeTime;

                    pRS_view_info++;
                    actualNumRemoteServers++;
                } //for

                (*pView)->numRemoteServers = actualNumRemoteServers;
                if (actualNumRemoteServers == 0)
                {
                    delete[] (*pView)->pRemoteServers;
                    (*pView)->pRemoteServers = NULL;
                }
            } //size>0
            else
            {
            	Trace_Dump(this,__FUNCTION__, "serverRegistryMap size=0");
            }


        }
        catch (std::bad_alloc& ba)
        {
            return ISMRC_AllocateError;
        }
    }

    Trace_Exit(this, "getView()", rc);
    return rc;
}

MCPReturnCode ViewKeeper::freeView(ismCluster_ViewInfo_t *pView)
{
    MCPReturnCode rc = ISMRC_OK;

    if (pView)
    {
        if (pView->pLocalServer)
        {
            ism_common_free(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,40),(void *) pView->pLocalServer->pServerName); //allocated with strdup
            ism_common_free(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,41),(void *) pView->pLocalServer->pServerUID); //allocated with strdup
            delete pView->pLocalServer;
        }

        if (pView->pRemoteServers)
        {
            for (int i=0; i< pView->numRemoteServers; ++i)
            {
                ism_common_free(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,42),(void*)(pView->pRemoteServers+i)->pServerName); //allocated with strdup
                ism_common_free(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,43),(void*)(pView->pRemoteServers+i)->pServerUID); //allocated with strdup
            }

            delete[] pView->pRemoteServers;
        }

        delete pView;
    }

    return rc;
}

String ViewKeeper::toString_RSViewInfo(ismCluster_RSViewInfo_t* rsViewInfo)
{
    if (rsViewInfo)
    {
        std::ostringstream oss;
        oss << "{uid=" << (rsViewInfo->pServerUID ? rsViewInfo->pServerUID : "NULL");
        oss << " name=" << (rsViewInfo->pServerName ? rsViewInfo->pServerName : "NULL");
        oss << " state=" << rsViewInfo->state
                << " healthStatus=" << rsViewInfo->healthStatus
                << " haStatus=" << rsViewInfo->haStatus
                << " stateChangeTime=" << rsViewInfo->stateChangeTime
                << " handle=" << rsViewInfo->phServerHandle << "}";

        return oss.str();
    }
    else
        return "NULL";
}

String ViewKeeper::toString_ViewInfo(ismCluster_ViewInfo_t* viewInfo)
{
    if (viewInfo)
    {
        std::ostringstream oss;
        oss << "#RS=" << viewInfo->numRemoteServers << " ";
        for (int i=0; i<viewInfo->numRemoteServers; ++i)
        {
            oss << "RS#" << (i+1) << "=" << toString_RSViewInfo(viewInfo->pRemoteServers+i) << " ";
        }
        oss << " Local=" << toString_RSViewInfo(viewInfo->pLocalServer);
        return oss.str();
    }
    else
        return "NULL";
}

ViewKeeper::RecoveryFilterState::RecoveryFilterState() :
        incarnation_number(-1), bf_exact_lastUpdate(0),
        bf_wildcard_lastUpdate(0), bf_wcsp_lastUpdate(0),
        rcf_lastUpdate(0)
{
}

bool ViewKeeper::RecoveryFilterState::isNonZero() const
{
    return (bf_exact_lastUpdate !=0 || bf_wildcard_lastUpdate != 0 || bf_wcsp_lastUpdate != 0 || rcf_lastUpdate != 0);
}
std::string ViewKeeper::RecoveryFilterState::toString() const
{
    std::ostringstream s;
    s << "I=" << incarnation_number << " x=" << bf_exact_lastUpdate
            << " w=" << bf_wildcard_lastUpdate << " p=" << bf_wcsp_lastUpdate
            << " r=" << rcf_lastUpdate;
    return s.str();
}

void ViewKeeper::getRemovedServers(RemoteServerVector& serverVector) const
{
    Trace_Entry(this,"getRemovedServers");
    {
        boost::recursive_mutex::scoped_lock lock(view_mutex);
        removedServers_.exportTo(serverVector);
    }
    Trace_Exit(this,"getRemovedServers");
}

void ViewKeeper::getRestoredNotInViewServers(RemoteServerVector& serverVector) const
{
    {
        boost::recursive_mutex::scoped_lock lock(view_mutex);

        for (ServerRegistryMap::const_iterator it = serverRegistryMap.begin(); it != serverRegistryMap.end(); ++it)
        {
            if (!it->second->controlAdded)
            {
                RemoteServerRecord_SPtr server(new RemoteServerRecord(it->second->uid,it->second->name, it->second->incarnation));
                serverVector.push_back(server);
            }
        }
    }

    std::ostringstream oss;

    if (spdr::ScTraceBuffer::isDebugEnabled(tc_))
    {
        oss << "#servers=" << serverVector.size() << ", {";
        for (std::size_t i=0; i<serverVector.size(); ++i)
        {
            oss << serverVector[i]->toString() << (i<serverVector.size()-1 ? ", " : "");
        }
        oss << "}";
    }

    Trace_Debug(this,"getRestoredNotInViewServers()", oss.str());
}

} /* namespace mcp */
