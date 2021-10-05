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

#include <ControlManagerImpl.h>
#include <RequestAdminMaintenanceModeTask.h>

namespace mcp
{
spdr::ScTraceComponent* ControlManagerImpl::tc_ = spdr::ScTr::enroll(
		mcp::trace::Component_Name,
		mcp::trace::SubComponent_Control,
		spdr::trace::ScTrConstants::Layer_ID_App,
		"ControlManagerImpl",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

const int ControlManagerImpl::adminDetachFromCluster_timeoutMillis_ = 5000;

ControlManagerImpl::ControlManagerImpl(
		const std::string& inst_ID,
		const MCPConfig& mcp_config,
		const spdr::PropertyMap& spidercastProps,
		const std::vector<spdr::NodeID_SPtr>& bootstrapSet,
		TaskExecutor& taskExecutor)
	throw (spdr::SpiderCastLogicError, spdr::SpiderCastRuntimeError) :
		spdr::ScTraceContext(tc_, inst_ID, ""),
		mcpConfig(mcp_config),
		spidercastProperties(spidercastProps),
		spidercastBootstrapSet(bootstrapSet),
		taskExecutor(taskExecutor),
		spidercastConfig(),
		closed(false),
		started(false),
		recovered(false),
		spidercast(),
		nodeID(),
		filterUpdatelistener(),
		recoveredIncarnationNumber(-1),
		forwardingAddress(),
		forwardingPort(-1),
		forwardingUseTLS(0),
		fatalErrorHandler_(NULL),
		viewNotifyEventQ_(),
		viewNotifyTask_scheduled_(false),
		viewNotifyEventQ_mutex_()
{
	using namespace spdr;

	Trace_Entry(this, "ControlManagerImpl()");

	//Set MCP defaults. Some defaults may be overridden by the tuning property file
	spidercastProperties.setProperty( spdr::config::HierarchyEnabled_PROP_KEY, std::string("false") );
	spidercastProperties.setProperty( spdr::config::RoutingEnabled_PROP_KEY, std::string("false") );
	spidercastProperties.setProperty( spdr::config::RetainAttributesOnSuspectNodesEnabled_PROP_KEY, std::string("true") );

	if (spidercastProperties.count(spdr::config::MembershipGossipIntervalMillis_PROP_KEY) == 0)
	    spidercastProperties.setProperty( spdr::config::MembershipGossipIntervalMillis_PROP_KEY,
			boost::lexical_cast<std::string>(mcpConfig.getPublishLocalBFTaskIntervalMillis()) );

    if (spidercastProperties.count(spdr::config::StatisticsEnabled_PROP_KEY) == 0)
        spidercastProperties.setProperty( spdr::config::StatisticsEnabled_PROP_KEY, std::string("false") );

	spidercastConfig = SpiderCastFactory::getInstance().createSpiderCastConfig(spidercastProperties,spidercastBootstrapSet);
	//The incarnation number is created here, from the clock
	spidercast = SpiderCastFactory::getInstance().createSpiderCast(*spidercastConfig, *this);

	nodeID = spidercast->getNodeID();

	forwardingAddress = mcpConfig.getLocalForwardingAddress();
	forwardingPort = mcpConfig.getLocalForwardingPort();
	forwardingUseTLS = mcpConfig.getLocalForwardingUseTLS();
}

ControlManagerImpl::~ControlManagerImpl()
{
    using namespace spdr;
	Trace_Entry(this, "~ControlManagerImpl()");
	this->close();
	localSubManager_.reset();
}

SubCoveringFilterPublisher_SPtr ControlManagerImpl::getSubCoveringFilterPublisher()
{
	boost::recursive_mutex::scoped_lock lock(control_mutex);

	if (closed)
	{
		throw MCPIllegalStateError("ControlManager is closed",
				ISMRC_ClusterNotAvailable);
	}

	if (!started)
	{
		throw MCPIllegalStateError("ControlManager not started",
				ISMRC_ClusterNotAvailable);
	}

	return boost::static_pointer_cast<SubCoveringFilterPublisher>(
			filterPublisher);
}

MCPReturnCode ControlManagerImpl::registerEngineEventCallback(EngineEventCallback* engineEventCallBack)
{
	if (viewKeeper)
		return viewKeeper->registerEngineEventCallback(engineEventCallBack);
	else
		return ISMRC_NullPointer;
}

MCPReturnCode ControlManagerImpl::registerProtocolEventCallback(ForwardingControl* forwardingControl)
{
	if (viewKeeper)
		return viewKeeper->registerProtocolEventCallback(forwardingControl);
	else
		return ISMRC_NullPointer;
}

MCPReturnCode ControlManagerImpl::storeSubscriptionPatterns(
				const std::vector<SubscriptionPattern_SPtr>& patterns)
{
    using namespace spdr;
    Trace_Event(this, "storeSubscriptionPatterns()","Entry");

    int rc;

	if (viewKeeper)
	{
	    rc = viewKeeper->storeSubscriptionPatterns(patterns);
	}
	else
	{
	    rc = ISMRC_NullPointer;
	}

	Trace_Event(this, "storeSubscriptionPatterns()","Exit");
	return static_cast<MCPReturnCode>(rc);
}

void ControlManagerImpl::setSubCoveringFilterEventListener(
		boost::shared_ptr<SubCoveringFilterEventListener> listener) throw (MCPRuntimeError)
{
	if (!listener)
	{
		std::string what = "ControlManagerImpl::setSubCoveringFilterEventListener Null SubCoveringFilterEventListener";
		throw MCPRuntimeError(what, ISMRC_NullArgument);
	}

	filterUpdatelistener = listener;
}

void ControlManagerImpl::setLocalSubManager(
		boost::shared_ptr<LocalSubManager> listener)
				throw (MCPRuntimeError)
{
    using namespace spdr;

	if (!listener)
	{
		std::string what = "ControlManagerImpl::setRemoteSubscriptionStatsListener Null RemoteSubscriptionStatsListener";
		throw MCPRuntimeError(what, ISMRC_NullArgument);
	}
	localSubManager_ = listener;
	viewKeeper.reset(new ViewKeeper(
			this->getInstanceID(), mcpConfig, nodeID, *filterUpdatelistener,
			*localSubManager_, taskExecutor, dynamic_cast<ControlManager&>(*this)));
}

void ControlManagerImpl::setFatalErrorHandler(FatalErrorHandler* pFatalErrorHandler)
{
	if (pFatalErrorHandler == NULL)
	{
		std::string what = "ControlManagerImpl::setFatalErrorHandler Null handler";
		throw MCPRuntimeError(what, ISMRC_NullArgument);
	}

	fatalErrorHandler_ = pFatalErrorHandler;
	viewKeeper->setFatalErrorHandler(pFatalErrorHandler);
}

void ControlManagerImpl::start() throw (spdr::SpiderCastLogicError,
		spdr::SpiderCastRuntimeError, MCPIllegalStateError, MCPRuntimeError)
{

	{
		boost::recursive_mutex::scoped_lock lock(control_mutex);

		if (closed)
		{
			throw MCPIllegalStateError("ControlManager is closed", ISMRC_ClusterNotAvailable);
		}

		if (started)
		{
			throw MCPIllegalStateError("ControlManager already started", ISMRC_Error);
		}

		if (!filterUpdatelistener)
		{
			throw MCPRuntimeError("SubCoveringFilterEventListener cannot be null", ISMRC_NullPointer);
		}

		//TODO try/catch
		spidercast->start();

		started = true;
	}
}

MCPReturnCode ControlManagerImpl::restoreRemoteServers(
			const ismCluster_RemoteServerData_t *pServersData, int numServers)
{
	if (numServers == 0)
		return ISMRC_OK;

	MCPReturnCode rc = ISMRC_OK;

	{
		boost::recursive_mutex::scoped_lock lock(control_mutex);

		if (closed)
		{
			return ISMRC_ClusterNotAvailable;
		}

		if (!started)
		{
			return ISMRC_ClusterNotAvailable;
		}

		if (recovered)
		{
			return ISMRC_Error;
		}

		rc = viewKeeper->restoreRemoteServers(pServersData, numServers, recoveredIncarnationNumber);
	}

	return rc;
}

int64_t ControlManagerImpl::getRecoveredIncarnationNumber() const
{
    boost::recursive_mutex::scoped_lock lock(control_mutex);
    return recoveredIncarnationNumber;
}

int64_t ControlManagerImpl::getCurrentIncarnationNumber() const
{
    boost::recursive_mutex::scoped_lock lock(control_mutex);
    if (spidercast)
    {
        return spidercast->getIncarnationNumber();
    }
    else
    {
        return -1L;
    }
}


MCPReturnCode ControlManagerImpl::recoveryCompleted()
{
    using namespace spdr;

	MCPReturnCode rc = ISMRC_OK;

	{
		boost::recursive_mutex::scoped_lock lock(control_mutex);

		if (closed)
		{
			return ISMRC_ClusterNotAvailable;
		}

		if (!started)
		{
			return ISMRC_ClusterNotAvailable;
		}

		if (recovered)
		{
			return ISMRC_Error;
		}

		if (forwardingAddress.empty() || forwardingPort <= 0)
		{
		    Trace_Error(this, "recoveryCompleted()", "Error: LocalForwardingInfo missing",
		            "RC", boost::lexical_cast<std::string>(ISMRC_Error));
			return ISMRC_Error;
		}

		try
		{
	        int64_t current_inc_num = spidercast->getIncarnationNumber();
	        bool restart_SpiderCast = false;
	        if (current_inc_num <= recoveredIncarnationNumber)
	        {
	            Trace_Event(this, "recoveryCompleted()",
	                    "Current incarnation number smaller or equal than recovered incarnation number. Will restart SpiderCast with a higher incarnation.",
	                    "current-inc", boost::lexical_cast<std::string>(current_inc_num),
	                    "recovered-inc", boost::lexical_cast<std::string>(recoveredIncarnationNumber));
	            restart_SpiderCast = true;
	        }

	        spdr::SpiderCast::NodeState spdr_state = spidercast->getNodeState();
	        if (spdr_state != spdr::SpiderCast::Started)
	        {
                Trace_Event(this, "recoveryCompleted()",
                        "SpiderCast is not in a 'Started' state. Will restart SpiderCast.",
                        "state", spdr::SpiderCast::getNodeStateName(spdr_state));
                restart_SpiderCast = true;
	        }

	        if (restart_SpiderCast)
	        {
                spidercast->close(false);
                spidercastProperties.setProperty( spdr::config::ForceIncarnationNumber_PROP_KEY, "-1" );
                spidercastProperties.setProperty( spdr::config::ChooseIncarnationNumberHigherThan_PROP_KEY,
                        boost::lexical_cast<std::string>(recoveredIncarnationNumber));
                spidercastConfig = spdr::SpiderCastFactory::getInstance().createSpiderCastConfig(spidercastProperties,spidercastBootstrapSet);
                spidercast = spdr::SpiderCastFactory::getInstance().createSpiderCast(*spidercastConfig,*this);
                spidercast->start();
                Trace_Event(this, "recoveryCompleted()", "Restarted SpiderCast.");
	        }

	        current_inc_num = spidercast->getIncarnationNumber();
	        spdr_state = spidercast->getNodeState();

	        rc = viewKeeper->recoveryCompleted(current_inc_num);
	        if (rc != ISMRC_OK)
	        {
	            return rc;
	        }

            if (current_inc_num > recoveredIncarnationNumber && spdr_state == spdr::SpiderCast::Started )
            {
                membershipService = spidercast->createMembershipService(spidercastProperties, *viewKeeper); //FIXME
                filterPublisher.reset(new SubCoveringFilterPublisherImpl(this->getInstanceID(), *membershipService));

                filterPublisher->publishLocalServerInfo(mcpConfig.getServerName());
                filterPublisher->publishForwardingAddress(forwardingAddress,forwardingPort,forwardingUseTLS);
                executePublishRemovedServersTask();
                recovered = true;

                Trace_Event(this, "recoveryCompleted()", "Published LocalServerInfo & ForwardingAddress",
                        "current-inc", boost::lexical_cast<std::string>(current_inc_num),
                        "recovered-inc", boost::lexical_cast<std::string>(recoveredIncarnationNumber));
            }
            else
            {
                Trace_Error(this,"recoveryCompleted()", "Failed to re-start SpiderCast with higher incarnation number",
                        "current-inc", boost::lexical_cast<std::string>(current_inc_num),
                        "recovered-inc", boost::lexical_cast<std::string>(recoveredIncarnationNumber),
                        "spidercast-state", spdr::SpiderCast::getNodeStateName(spdr_state));
                return ISMRC_Error;
            }
		}
		catch (MCPIllegalStateError& ise)
		{
			Trace_Error(this,"recoveryCompleted()", "MCPIllegalStateError",
					"what",ise.what(), "stacktrace", ise.getStackTrace());
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",ise.getReturnCode());
			return ise.getReturnCode();
		}
		catch (MCPRuntimeError& rte)
		{
		    Trace_Error(this,"recoveryCompleted()","MCPRuntimeError",
					"what", rte.what(), "stacktrace", rte.getStackTrace());
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",rte.getReturnCode());
			return rte.getReturnCode();
		}
		catch (spdr::IllegalStateException& ilse)
		{
		    Trace_Warning(this,"recoveryCompleted()","Warning: spdr::IllegalStateException, FilterPublisher already closed, ignoring",
		            "what",ilse.what(), "stacktrace", ilse.getStackTrace() );
		    Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",ISMRC_OK);
		    return ISMRC_OK;
		}
		catch (spdr::SpiderCastLogicError& le)
		{
		    Trace_Error(this,"recoveryCompleted()","spdr::SpiderCastLogicError",
					"what",le.what(), "stacktrace", le.getStackTrace() );
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",ISMRC_ClusterInternalError);
			ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
			return ISMRC_ClusterInternalError;
		}
		catch (spdr::SpiderCastRuntimeError& re)
		{
		    Trace_Error(this,"recoveryCompleted()","spdr::SpiderCastRuntimeError",
					"what",re.what(), "stacktrace", re.getStackTrace());
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",ISMRC_ClusterInternalError);
			ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
			return ISMRC_ClusterInternalError;
		}
		catch (std::bad_alloc& ba)
		{
		    Trace_Error(this, "recoveryCompleted()", "std::bad_alloc", "what", ba.what());
		    Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",ISMRC_AllocateError);
		    return ISMRC_AllocateError;
		}
		catch (std::exception& e)
		{
		    Trace_Error(this,"recoveryCompleted()",e.what());
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",ISMRC_Error);
			return ISMRC_Error;
		}
		catch (...)
		{
		    Trace_Error(this,"recoveryCompleted()","catch (...) unspecified error");
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",ISMRC_Error);
			return ISMRC_Error;
		}
	}

	Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",rc);
	return rc;
}

MCPReturnCode ControlManagerImpl::notifyTerm()
{
    if (viewKeeper)
    {
        return viewKeeper->notifyTerm();
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

void ControlManagerImpl::close(bool soft) throw (spdr::SpiderCastRuntimeError)
{
	{
		boost::recursive_mutex::scoped_lock lock(control_mutex);

		if (closed)
		{
			return;
		}

		closed = true;

		if (membershipService)
		{
			membershipService->close();
		}

		viewKeeper->close();
		spidercast->close(soft);

	}
}

int ControlManagerImpl::setLocalForwardingInfo(
		const char *pServerName,
		const char *pServerUID,
		const char *pServerAddress,
		int serverPort,
		uint8_t fUseTLS)
{
	boost::recursive_mutex::scoped_lock lock(control_mutex);
	if (closed)
	{
		return ISMRC_ClusterNotAvailable;
	}

	if (recovered)
	{
		return ISMRC_Error;
	}

	if (pServerAddress == NULL)
	{
		return ISMRC_NullArgument;
	}

	if (serverPort < 1 || serverPort > 65535)
	{
		return ISMRC_ArgNotValid;
	}

	forwardingAddress = pServerAddress;
	forwardingPort = serverPort;
	forwardingUseTLS = fUseTLS;

	Trace_Event(this, "setLocalForwardingInfo()", "Forwarding info set",
	        "address", forwardingAddress,
	        "port", boost::lexical_cast<std::string>(forwardingPort),
	        "useTLS", (forwardingUseTLS ? "True" : "False"));

	return ISMRC_OK;
}

/*
 * @see LocalForwardingEvents
 *
 * Threads: IOP, SpiderCast.MemTopo
 */
int ControlManagerImpl::nodeForwardingConnected(
		const ismCluster_RemoteServerHandle_t phServerHandle)
{
    if (phServerHandle == NULL)
    {
        Trace_Error(this, "nodeForwardingConnected()", "Error: NULL cluster handle", "RC", ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

//    {
//        boost::recursive_mutex::scoped_lock lock(control_mutex);
//        if (closed)
//        {
//            Trace_Error(this, "nodeForwardingConnected()", "Error: cluster closed", "RC", ISMRC_ClusterNotAvailable);
//            return ISMRC_ClusterNotAvailable;
//        }
//    }

    if (viewKeeper)
    {
        boost::recursive_mutex::scoped_lock lock(viewNotifyEventQ_mutex_);
        ViewNotifyEvent_SPtr vne = ViewNotifyEvent::createInProtoConnected(phServerHandle,viewKeeper);
        viewNotifyEventQ_.push_back(vne);
        Trace_Event(this, "nodeForwardingConnected()", "Queued",
                "handle", boost::lexical_cast<std::string>(phServerHandle));
        if (!viewNotifyTask_scheduled_)
        {
            viewNotifyTask_scheduled_ = true;
            mcp::AbstractTask_SPtr task(new ViewNotifyTask(dynamic_cast<ControlManager&>(*this)));
            taskExecutor.scheduleDelay(task, TaskExecutor::ZERO_DELAY);
            Trace_Event(this, "nodeForwardingConnected()", "scheduled a ViewNotifyTask");
        }

        return ISMRC_OK;
    }
    else
    {
        Trace_Error(this, "nodeForwardingConnected()", "Error: viewKeeper NULL", "RC", ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }
}

/*
 * @see LocalForwardingEvents
 *
 * Threads: IOP, SpiderCast.MemTopo
 */
int ControlManagerImpl::nodeForwardingDisconnected(
		const ismCluster_RemoteServerHandle_t phServerHandle)
{
    if (phServerHandle == NULL)
    {
        Trace_Error(this, "nodeForwardingDisconnected()", "Error: NULL cluster handle", "RC", ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

//    {
//        boost::recursive_mutex::scoped_lock lock(control_mutex);
//        if (closed)
//        {
//            Trace_Error(this, "nodeForwardingDisconnected()", "Error: cluster closed", "RC", ISMRC_ClusterNotAvailable);
//            return ISMRC_ClusterNotAvailable;
//        }
//    }

	if (viewKeeper)
	{
	    boost::recursive_mutex::scoped_lock lock(viewNotifyEventQ_mutex_);
	    ViewNotifyEvent_SPtr vne = ViewNotifyEvent::createInProtoDisconnected(phServerHandle,viewKeeper);
	    viewNotifyEventQ_.push_back(vne);
	    Trace_Event(this, "nodeForwardingDisconnected()", "Queued",
	            "handle", boost::lexical_cast<std::string>(phServerHandle));
	    if (!viewNotifyTask_scheduled_)
	    {
	        viewNotifyTask_scheduled_ = true;
	        mcp::AbstractTask_SPtr task(new ViewNotifyTask(dynamic_cast<ControlManager&>(*this)));
	        taskExecutor.scheduleDelay(task, TaskExecutor::ZERO_DELAY);
	        Trace_Event(this, "nodeForwardingDisconnected()", "scheduled a ViewNotifyTask");
	    }

	    return ISMRC_OK;
	}
	else
	{
	    Trace_Error(this, "nodeForwardingDisconnected()", "Error: viewKeeper NULL", "RC", ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}
}

/*
 * @param nodeIndex
 * @param nodeName
 */
MCPReturnCode ControlManagerImpl::adminDeleteNode(const ismCluster_RemoteServerHandle_t phServerHandle)
{
    //TODO PersistentRemove

    using namespace spdr;
	Trace_Entry(this, "adminDeleteNode()");

	boost::recursive_mutex::scoped_lock lock(control_mutex);
	if (closed)
	{
		return ISMRC_ClusterNotAvailable;
	}

	if (!recovered)
	{
		return ISMRC_Error;
	}

	spdr::NodeID_SPtr id;
	int64_t incurnation_num = 0;
	MCPReturnCode rc = viewKeeper->adminDeleteNode(phServerHandle, id, &incurnation_num);
	if (rc == ISMRC_OK && id)
	{
	    try
	    {
	        bool res = membershipService->clearRemoteNodeRetainedAttributes(id, incurnation_num);

	        Trace_Event(this, "adminDeleteNode()",
	                (res ? std::string("clear retained success") : std::string("clear retained failed, target still alive")),
	                "node", id->getNodeName());

	        if (!res)
	        {
	            rc = ISMRC_ClusterRemoveRemoteServerStillAlive;
	            Trace_Error(this, "adminDeleteNode()", "Error: cannot remove remote server, server still alive",
	                    "uid",id->getNodeName(), "RC", rc);
	        }
	    }
	    catch (spdr::SpiderCastLogicError& le)
	    {
	        rc = ISMRC_ClusterRemoveRemoteServerFailed;
            Trace_Error(this, "adminDeleteNode()", "Error: failed to remove remote server, SpiderCastLogicError",
                    "uid",id->getNodeName(), "what", le.what(), "RC", boost::lexical_cast<std::string>(rc));
	    }
	    catch (spdr::SpiderCastRuntimeError& re)
	    {
	        rc = ISMRC_ClusterRemoveRemoteServerFailed;
            Trace_Error(this, "adminDeleteNode()", "Error: failed to remove remote server, SpiderCastRuntimeError",
                    "uid",id->getNodeName(), "what", re.what(), "RC", boost::lexical_cast<std::string>(rc));
	    }
	}

	Trace_Exit(this, "adminDeleteNode()",rc);
	return rc;
}

MCPReturnCode ControlManagerImpl::adminDetachFromCluster()
{
    using namespace spdr;

	boost::recursive_mutex::scoped_lock lock(control_mutex);

	if (closed)
	{
		return ISMRC_ClusterNotAvailable;
	}

	closed = true;

	if (membershipService)
	{
		membershipService->close();
	}

	try
	{
		bool res = spidercast->closeAndRemove(5000); //TODO config parameter
		if (res)
		{
			return ISMRC_OK;
		}
		else
		{
		    Trace_Warning(this,"adminDetachFromCluster()", "Warning: No Ack was received from cluster. This may be OK when this is the last server removed." );
			return ISMRC_ClusterRemoveLocalServerNoAck;
		}
	}
	catch (spdr::SpiderCastLogicError& le)
	{
		Trace_Debug(this,"adminDetachFromCluster()","spdr::SpiderCastLogicError",
				"what",le.what(), "stacktrace", le.getStackTrace() );
		Trace_Exit<MCPReturnCode>(this,"adminDetachFromCluster()",ISMRC_ClusterInternalError);
		ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
		return ISMRC_ClusterInternalError;
	}
	catch (spdr::SpiderCastRuntimeError& re)
	{
		Trace_Debug(this,"adminDetachFromCluster()","spdr::SpiderCastRuntimeError",
				"what",re.what(), "stacktrace", re.getStackTrace());
		Trace_Exit<MCPReturnCode>(this,"adminDetachFromCluster()",ISMRC_ClusterInternalError);
		ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
		return ISMRC_ClusterInternalError;
	}
	catch (std::bad_alloc& ba)
	{
	    Trace_Debug(this, "adminDetachFromCluster()", "std::bad_alloc", "what", ba.what());
	    Trace_Exit<MCPReturnCode>(this,"adminDetachFromCluster()",ISMRC_AllocateError);
	    return ISMRC_AllocateError;
	}
	catch (std::exception& e)
	{
		Trace_Debug(this,"adminDetachFromCluster()",e.what());
		Trace_Exit<MCPReturnCode>(this,"adminDetachFromCluster()",ISMRC_Error);
		return ISMRC_Error;
	}
	catch (...)
	{
		Trace_Debug(this,"adminDetachFromCluster()","catch (...) unspecified error");
		Trace_Exit<MCPReturnCode>(this,"adminDetachFromCluster()",ISMRC_Error);
		return ISMRC_Error;
	}
}

/**
 * Set only the number of connected and disconnected
 * @param pStatistics
 * @return
 */
MCPReturnCode ControlManagerImpl::getStatistics(ismCluster_Statistics_t *pStatistics)
{
    if (viewKeeper)
        return viewKeeper->getStatistics(pStatistics);
    else
        return ISMRC_NullPointer;
}

void ControlManagerImpl::onEvent(spdr::event::SpiderCastEvent_SPtr event)
{
    using namespace spdr;
	Trace_Debug(this, "onEvent()", "SpiderCast Event", "event", event->toString());

	if (event->getType() == event::Fatal_Error)
	{
	    event::FatalErrorEvent_SPtr fe = boost::static_pointer_cast<event::FatalErrorEvent,event::SpiderCastEvent>(event);

	    {
	        boost::recursive_mutex::scoped_lock lock(control_mutex);
	        if (started && !closed && !recovered)
	        {
	            if (fe->getErrorCode() == spdr::event::Duplicate_Local_Node_Detected)
	            {
	                std::string what("Warning: SpiderCast encountered a problem before recovery completed. ");
	                what += "Cluster will restart SpiderCast automatically after recovery completed.";
	                Trace_Warning(this, "onEvent()", what, "event", event->toString());
	                return;
	            }
	        }
	        else if (closed)
	        {
	            std::string what("Warning: SpiderCast encountered a problem while the server is closing. Ignored.");
	            Trace_Warning(this, "onEvent()", what, "event", event->toString());
	            return;
	        }
	    }

	    if (fe->getErrorCode() == spdr::event::Duplicate_Local_Node_Detected)
        {
            std::ostringstream what;
            what << "Error: Duplicate local node detected."
                    <<  " The local server discovered another server with the same UID, the local server will shut down."
                    << " The local server will restart in maintenance mode, RC=" << ISMRC_ClusterDuplicateServerUID << ".";
            Trace_Error(this, "onEvent()", what.str());
            mcp::AbstractTask_SPtr task(new RequestAdminMaintenanceModeTask(*this, ISMRC_ClusterDuplicateServerUID, 1));
            taskExecutor.scheduleDelay(task, TaskExecutor::ZERO_DELAY);
            return;
        }
	    else
	    {
	        Trace_Error(this, "onEvent()", "Error: SpiderCast FatalError, Cluster will terminate.", "event", event->toString());
	        onFatalError(this->getMemberName(), "Fatal Error in cluster component. Local server will leave the cluster.", ISMRC_ClusterInternalError);
	        return;
	    }

	}
}

int ControlManagerImpl::onFatalError(const std::string& component, const std::string& errorMessage, int rc)
{
	if (fatalErrorHandler_ == NULL)
	{   //Only in unit test when we don't have a handler
		std::string what(component);
		what += ": " + errorMessage;
		Trace_Warning(this, "onFatalError()", "Warning: no fatalErrorHandler, going to throw", "what", what.c_str());
		throw MCPRuntimeError(what, static_cast<MCPReturnCode>(rc));
	}
	else
	{
		return fatalErrorHandler_->onFatalError(component, errorMessage, rc);
	}
}

int ControlManagerImpl::onFatalError_MaintenanceMode(const std::string& component, const std::string& errorMessage, int rc, int restartFlag)
{
    if (fatalErrorHandler_ == NULL)
    {   //Only in unit test when we don't have a handler
        std::ostringstream what;
        what << "MaintenanceMode: " << component << ": " << errorMessage << ", RC=" << rc;
        Trace_Warning(this, "onFatalError_MaintenanceMode()", "Warning: no fatalErrorHandler, going to throw", "what", what.str());
        throw MCPRuntimeError(what.str(), static_cast<MCPReturnCode>(rc));
    }
    else
    {
        return fatalErrorHandler_->onFatalError_MaintenanceMode(component, errorMessage, rc, restartFlag);
    }
}

bool ControlManagerImpl::isReconciliationFinished() const
{
	return viewKeeper->isReconciliationFinished();
}

MCPReturnCode ControlManagerImpl::getView(ismCluster_ViewInfo_t **pView)
{
    if (viewKeeper)
        return viewKeeper->getView(pView);
    else
        return ISMRC_NullPointer;
}

void ControlManagerImpl::executeViewNotifyTask()
{
    using namespace spdr;
    Trace_Entry(this, "executeViewNotifyTask()");


    int num_events = 0;

    {
        boost::recursive_mutex::scoped_lock lock(viewNotifyEventQ_mutex_);
        num_events = viewNotifyEventQ_.size();
        viewNotifyTask_scheduled_ = false;
    }

    while (num_events > 0)
    {
        ViewNotifyEvent_SPtr event;

        {
            boost::recursive_mutex::scoped_lock lock(viewNotifyEventQ_mutex_);
            if (!viewNotifyEventQ_.empty())
            {
                event = viewNotifyEventQ_.front();
                viewNotifyEventQ_.pop_front();
                num_events--;
            }
        }

        if (event)
        {
            int rc = event->deliver();
            if (rc != ISMRC_OK)
            {
                Trace_Error(this,"executeViewNotifyTask()","Error: ViewNotifyEvent delivery failed",
                        "event", spdr::toString(event),
                        "RC", boost::lexical_cast<std::string>(rc));
            }
            else
            {
                Trace_Debug(this,"executeViewNotifyTask()","delivered",
                        "event",  spdr::toString(event),
                        "num_events",boost::lexical_cast<std::string>(num_events));
            }
        }
        else
        {
            Trace_Debug(this,"executeViewNotifyTask()","NULL event, break out of loop",
                    "num_events remaining",boost::lexical_cast<std::string>(num_events));
            break;
        }
    }

    Trace_Exit(this, "executeViewNotifyTask()");
}

void ControlManagerImpl::executePublishRemovedServersTask()
{
    using namespace spdr;
    Trace_Entry(this, "executePublishRemovedServersTask()");

    if (viewKeeper)
    {
        RemoteServerVector removed;
        viewKeeper->getRemovedServers(removed);
        uint64_t sqn;
        int rc = filterPublisher->publishRemovedServers(removed, &sqn);
        if (rc == ISMRC_Closed)
        {
            Trace_Warning(this, "executePublishRemovedServersTask()", "Warning: FilterPublisher already closed, ignored",
                    "RC", spdr::stringValueOf(rc));
        }
        else if (rc != ISMRC_OK)
        {
            Trace_Error(this, "executePublishRemovedServersTask()", "Error: failed to publish", "RC",rc);
            onFatalError(this->getMemberName(), "Fatal Error in cluster component. Local server will leave the cluster.", ISMRC_ClusterInternalError);
        }
        else
        {
            Trace_Debug(this, "executePublishRemovedServersTask()", "published",
                "SQN", spdr::stringValueOf(sqn), "num-servers", spdr::stringValueOf(removed.size()));
        }
    }
    else
    {
        Trace_Warning(this, "executePublishRemovedServersTask()", "Warning: ViewKeeper null, ignored");
    }
}

void ControlManagerImpl::executePublishRestoredNotInViewTask()
{
    using namespace spdr;
    Trace_Entry(this, __FUNCTION__);

    {
    	boost::recursive_mutex::scoped_lock lock(control_mutex);
    	if (closed)
    	{
    		Trace_Event(this, __FUNCTION__, "closed, ignored");
    		return;
    	}

    	if (!recovered)
    	{
    		Trace_Warning(this, __FUNCTION__, "Warning: not recovered, ignored");
    		return;
    	}

    	if (viewKeeper)
    	{
    		RemoteServerVector restoredNotInView;
    		viewKeeper->getRestoredNotInViewServers(restoredNotInView);
    		uint64_t sqn;
    		int rc = filterPublisher->publishRestoredNotInView(restoredNotInView, &sqn);
    		if (rc == ISMRC_Closed)
    		{
    			Trace_Warning(this, __FUNCTION__, "Warning: FilterPublisher already closed, ignored",
    					"RC", spdr::stringValueOf(rc));
    		}
    		else if (rc != ISMRC_OK)
    		{
    			Trace_Error(this, __FUNCTION__, "Error: failed to publish", "RC",rc);
    			onFatalError(this->getMemberName(), "Fatal Error in cluster component. Local server will leave the cluster.", ISMRC_ClusterInternalError);
    		}
    		else
    		{
    			Trace_Debug(this, __FUNCTION__, "published",
    					"SQN", spdr::stringValueOf(sqn), "num-servers", spdr::stringValueOf(restoredNotInView.size()));
    		}
    	}
    	else
    	{
    		Trace_Warning(this, __FUNCTION__, "Warning: ViewKeeper null, ignored");
    	}
    }
}

void ControlManagerImpl::executeAdminDeleteRemovedServersTask(RemoteServerVector& pendingRemovals)
{
    using namespace std;
    using namespace spdr;
    Trace_Entry(this, "executeAdminDeleteRemovedServersTask()");

    for (RemoteServerVector::iterator it = pendingRemovals.begin(); it != pendingRemovals.end(); ++it)
    {

        boost::recursive_mutex::scoped_lock lock(control_mutex);
        if (closed)
        {
            return;
        }

        if (!recovered)
        {
            return;
        }

        if (viewKeeper)
        {
            NodeID_SPtr nodeID;
            int64_t incNum = -1;
            Trace_Event(this, "executeAdminDeleteRemovedServersTask", "going to delete",
                    "uid", (*it)->serverUID, "inc", spdr::stringValueOf((*it)->incarnationNumber));
            MCPReturnCode rc = viewKeeper->adminDeleteNodeFromList((*it)->serverUID,nodeID,&incNum);
            if (rc == ISMRC_OK && nodeID)
            {
                try
                {
                    bool res = membershipService->clearRemoteNodeRetainedAttributes(nodeID, std::max((*it)->incarnationNumber,incNum));

                    Trace_Event(this, "executeAdminDeleteRemovedServersTask()",
                            (res ? std::string("clear retained success") : std::string("clear retained failed, target still alive")),
                            "node", spdr::toString(nodeID));

                    if (!res)
                    {
                        Trace_Event(this, "executeAdminDeleteRemovedServersTask()", "cannot remove remote server, server still alive, ignored, remote server will shut down.",
                                "node", spdr::toString(nodeID));
                    }
                }
                catch (spdr::SpiderCastLogicError& le)
                {
                    rc = ISMRC_ClusterRemoveRemoteServerFailed;
                    Trace_Error(this, "adminDeleteNode()", "Error: failed to remove remote server, SpiderCastLogicError",
                            "node", spdr::toString(nodeID), "what", le.what(), "RC", boost::lexical_cast<std::string>(rc));
                }
                catch (spdr::SpiderCastRuntimeError& re)
                {
                    rc = ISMRC_ClusterRemoveRemoteServerFailed;
                    Trace_Error(this, "adminDeleteNode()", "Error: failed to remove remote server, SpiderCastRuntimeError",
                            "node", spdr::toString(nodeID), "what", re.what(), "RC", boost::lexical_cast<std::string>(rc));
                }
            }
            else
            {
                Trace_Event(this, "executeAdminDeleteRemovedServersTask", "delete from view-keeper not OK",
                                    "NodeID", spdr::toString(nodeID), "inc", spdr::stringValueOf(incNum));
            }
        }

    } //for

    Trace_Exit(this, "executeAdminDeleteRemovedServersTask()");
}

void ControlManagerImpl::executeRequestAdminMaintenanceModeTask(int errorRC, int restartFlag)
{
    Trace_Entry(this, __FUNCTION__,
            "RC", spdr::stringValueOf(errorRC),"restartFlag", spdr::stringValueOf(restartFlag));
    onFatalError_MaintenanceMode(this->getMemberName(),
            "Fatal Error in cluster component. Local server will restart in maintenance mode.", errorRC, restartFlag);
    Trace_Exit(this, __FUNCTION__);
}

void ControlManagerImpl::onTaskFailure(const std::string& component, const std::string& errorMessage, int rc)
{
    Trace_Error(this, __FUNCTION__, "Error: critical task failed to execute.");
    onFatalError(component,errorMessage,rc);
}

int ControlManagerImpl::reportEngineStatistics(ismCluster_EngineStatistics_t *pEngineStatistics)
{
    if (viewKeeper)
    {
        return viewKeeper->reportEngineStatistics(pEngineStatistics);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

} /* namespace mcp */


