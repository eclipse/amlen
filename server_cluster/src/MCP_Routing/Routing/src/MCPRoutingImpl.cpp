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

#include "MCPRoutingImpl.h"
#include "ismutil.h"
#include "admin.h"

namespace mcp
{

spdr::ScTraceComponent* MCPRoutingImpl::tc_ = spdr::ScTr::enroll(
        mcp::trace::Component_Name,
        mcp::trace::SubComponent_Core,
        spdr::trace::ScTrConstants::Layer_ID_App,
		"MCPRoutingImpl",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

const boost::posix_time::time_duration MCPRoutingImpl::DISCOVERY_TIMEOUT_TASK_INTERVAL_MS = boost::posix_time::milliseconds(500);
const boost::posix_time::time_duration MCPRoutingImpl::TRACE_LEVEL_MONITOR_TASK_INTERVAL_MS = boost::posix_time::milliseconds(5000);

MCPRoutingImpl::MCPRoutingImpl(
		const std::string& inst_ID,
		const spdr::PropertyMap& mcpProps,
		const spdr::PropertyMap& spidercastProps,
		const std::vector<spdr::NodeID_SPtr>& bootstrapSet)
		throw (MCPRuntimeError, spdr::SpiderCastLogicError, spdr::SpiderCastRuntimeError) :
		spdr::ScTraceContext(tc_, inst_ID, ""),
		mcpConfig_(mcpProps),
		my_ClusterName(mcpConfig_.getClusterName()),
		my_ServerName(mcpConfig_.getServerName()),
		my_ServerUID(mcpConfig_.getServerUID()),
		state_(STATE_INIT),
		stateChangeTime_(ism_common_currentTimeNanos()),
		stateFailure_(false),
		localSubManager_SPtr(),
		globalSubManager_SPtr(),
		controlManager_SPtr(),
		taskExecutor_SPtr(),
		discoveryTimeoutTask_(),
		discoveryTimeoutTask_deadline_(),
		traceLevelMonitorTask_(),
		cluster_trace_level_(-1),
		spidercast_trace_level_(-1),
        engineStatisticsTask_()
{
    using namespace spdr;

	Trace_Entry(this,"MCPRoutingImpl()");

	Trace_Config(this,"MCPRoutingImpl()","MCP Cluster Configuration", "Props", mcpConfig_.toString());

	taskExecutor_SPtr.reset(new TaskExecutor(this->getInstanceID(),"TaskExecutor"));

	controlManager_SPtr.reset(new ControlManagerImpl(this->getInstanceID(),mcpConfig_, spidercastProps, bootstrapSet, *taskExecutor_SPtr));

	localSubManager_SPtr.reset(new LocalSubManagerImpl(this->getInstanceID(),mcpConfig_,
			controlManager_SPtr->getNodeID()->getNodeName(), *taskExecutor_SPtr,
			*boost::static_pointer_cast<ControlManager,ControlManagerImpl>(controlManager_SPtr)));

	globalSubManager_SPtr.reset(new GlobalSubManagerImpl(this->getInstanceID(),mcpConfig_));

	controlManager_SPtr->setSubCoveringFilterEventListener(
			boost::static_pointer_cast<SubCoveringFilterEventListener,GlobalSubManagerImpl>(globalSubManager_SPtr));
	controlManager_SPtr->setLocalSubManager(
			boost::static_pointer_cast<LocalSubManager,LocalSubManagerImpl>(localSubManager_SPtr));

	controlManager_SPtr->setFatalErrorHandler(static_cast<FatalErrorHandler*>(this));
	taskExecutor_SPtr->setFatalErrorHandler(static_cast<FatalErrorHandler*>(this));
	localSubManager_SPtr->setFatalErrorHandler(static_cast<FatalErrorHandler*>(this));

	traceLevelMonitorTask_.reset(new TraceLevelMonitorTask(dynamic_cast<RoutingTasksHandler&>(*this)));
	engineStatisticsTask_.reset(new EngineStatisticsTask(dynamic_cast<RoutingTasksHandler&>(*this)));
}

MCPRoutingImpl::~MCPRoutingImpl()
{
    using namespace spdr;
	Trace_Entry(this,"~MCPRoutingImpl()");
	this->internalClose();
}


MCPReturnCode MCPRoutingImpl::start()
{
    using namespace spdr;
	Trace_Entry(this,"start()");

	MCPReturnCode rc = ISMRC_OK;

	boost::recursive_mutex::scoped_lock lock(state_mutex);

	if (state_ == STATE_INIT)
	{
		//Start task executor thread
		try
		{
			taskExecutor_SPtr->start();
			taskExecutor_SPtr->scheduleDelay(traceLevelMonitorTask_, TaskExecutor::ZERO_DELAY);
		}
		catch (MCPRuntimeError& e)
		{
			Trace_Debug(this,"start()", "MCPRuntimeError",
					"what",e.what(), "stacktrace", e.getStackTrace());
			internalClose(false,true);
			rc = e.getReturnCode();
			Trace_Exit<MCPReturnCode>(this,"start()",rc);
			return rc;
		}

		// Start GlobalSubManager
		rc = globalSubManager_SPtr->start();
		if (rc != ISMRC_OK)
		{
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"start()",rc);
			return rc;
		}

		//Start control (may throw)
		try
		{
			controlManager_SPtr->start();
		}
		catch (MCPIllegalStateError& ise)
		{
			Trace_Debug(this,"start()", "MCPIllegalStateError",
					"what",ise.what(), "stacktrace", ise.getStackTrace());
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"start()",ise.getReturnCode());
			return ise.getReturnCode();
		}
		catch (MCPRuntimeError& rte)
		{
			Trace_Debug(this,"start()","MCPRuntimeError",
					"what", rte.what(), "stacktrace", rte.getStackTrace());
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"start()",rte.getReturnCode());
			return rte.getReturnCode();
		}
		catch (spdr::BindException& be)
		{
		    Trace_Debug(this, "start()","spdr::BindException",
                    "what",be.what(), "stacktrace", be.getStackTrace() );
		    internalClose(false,true);
		    MCPReturnCode rc = ISMRC_ClusterInternalError;
		    if (be.getErrorCode() == spdr::event::Bind_Error_Port_Busy)
		    {
		        ism_common_setErrorData(ISMRC_ClusterBindErrorPortBusy,
		        		"%u", be.getPort());
		        rc = ISMRC_ClusterBindErrorPortBusy;
		    }
		    else if (be.getErrorCode() == spdr::event::Bind_Error_Cannot_Assign_Local_Interface)
		    {
		        ism_common_setErrorData(ISMRC_ClusterBindErrorLocalAddress,
		        		"%s", be.getAddress().c_str());
		        rc = ISMRC_ClusterBindErrorLocalAddress;
		    }
		    Trace_Exit<MCPReturnCode>(this,"start()",rc);
		    return rc;
		}
		catch (spdr::SpiderCastLogicError& le)
		{
			Trace_Debug(this,"start()","spdr::SpiderCastLogicError",
					"what",le.what(), "stacktrace", le.getStackTrace() );
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"start()",ISMRC_ClusterInternalError);
			ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
			return ISMRC_ClusterInternalError;
		}
		catch (spdr::SpiderCastRuntimeError& re)
		{
			Trace_Debug(this,"start()","spdr::SpiderCastRuntimeError",
					"what",re.what(), "stacktrace", re.getStackTrace());
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"start()",ISMRC_ClusterInternalError);
			ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
			return ISMRC_ClusterInternalError;
		}
		catch (std::bad_alloc& ba)
		{
		    Trace_Debug(this,"start()",ba.what());
		    internalClose(false,true);
		    Trace_Exit<MCPReturnCode>(this,"start()",ISMRC_AllocateError);
		    return ISMRC_AllocateError;
		}
		catch (std::exception& e)
		{
			Trace_Debug(this,"start()",e.what());
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"start()",ISMRC_Error);
			return ISMRC_Error;
		}
		catch (...)
		{
			Trace_Debug(this,"start()","catch (...) unspecified error");
			internalClose(false,true);
			return ISMRC_Error;
		}

		rc = localSubManager_SPtr->start();
		if (rc != ISMRC_OK)
		{
			Trace_Debug(this,"start()","localSubManager_SPtr->start() failed");
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"start()",rc);
			return rc;
		}

		state_ = STATE_STARTED;
		stateChangeTime_ = ism_common_currentTimeNanos();
	}
	else if (state_ == STATE_CLOSED)
	{
		rc = ISMRC_ClusterNotAvailable;
	}
	else if (state_ == STATE_CLOSED_DETACHED)
	{
	    rc = ISMRC_ClusterDisabled;
	}
	else if (state_ == STATE_STARTED || state_ == STATE_RECOVERED || state_ == STATE_ACTIVE)
	{
		rc = ISMRC_Error;
	}
	else if (state_ == STATE_ERROR)
	{
		rc = ISMRC_ClusterInternalErrorState;
	}
	else
	{
		rc = ISMRC_Error;
	}

	Trace_Exit<MCPReturnCode>(this,"start()", rc);
	return rc;
}

MCPReturnCode MCPRoutingImpl::restoreRemoteServers(
				const ismCluster_RemoteServerData_t *pServersData,
		        int numServers)
{
    using namespace spdr;
	Trace_Entry(this,"restoreRemoteServers()");

	MCPReturnCode rc = ISMRC_OK;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state_ == STATE_STARTED)
		{
			rc = controlManager_SPtr->restoreRemoteServers(pServersData,numServers);
			if (rc != ISMRC_OK)
			{
				internalClose(false,true);
			}
		}
		else if (state_ == STATE_INIT)
		{
			rc = ISMRC_ClusterNotAvailable;
		}
		else if (state_ == STATE_CLOSED)
		{
			rc = ISMRC_ClusterNotAvailable;
		}
	    else if (state_ == STATE_CLOSED_DETACHED)
	    {
	        rc = ISMRC_ClusterDisabled;
	    }
		else if (state_ == STATE_RECOVERED || state_ == STATE_ACTIVE)
		{
			rc = ISMRC_Error;
		}
		else if (state_ == STATE_ERROR)
		{
			rc = ISMRC_ClusterInternalErrorState;
		}
		else
		{
			rc =  ISMRC_Error;
		}
	}

	Trace_Exit(this,"restoreRemoteServers()",rc);
	return rc;
}

MCPReturnCode MCPRoutingImpl::recoveryCompleted()
{
    using namespace spdr;
	Trace_Entry(this,"recoveryCompleted()");
	MCPReturnCode rc = ISMRC_OK;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state_ == STATE_CLOSED)
		{
			rc = ISMRC_ClusterNotAvailable;
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",rc);
			return rc;
		}
	    else if (state_ == STATE_CLOSED_DETACHED)
	    {
	        rc = ISMRC_ClusterDisabled;
	        Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",rc);
	        return rc;
	    }
		else if (state_ == STATE_INIT)
		{
			rc = ISMRC_ClusterNotAvailable;
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",rc);
			return rc;
		}
		else if (state_ == STATE_ERROR)
		{
			rc = ISMRC_ClusterInternalErrorState;
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",rc);
			return rc;
		}

		rc = globalSubManager_SPtr->recoveryCompleted();
		if (rc != ISMRC_OK)
		{
			Trace_Debug(this,"recoveryCompleted()", "GlobalSubManager failed");
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",rc);
			return rc;
		}

		rc = controlManager_SPtr->recoveryCompleted();
		if (rc != ISMRC_OK)
		{
			Trace_Debug(this,"recoveryCompleted()", "ControlManager failed");
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",rc);
			return rc;
		}

		// connecting LocalSubManager with ControlManager
		rc = localSubManager_SPtr->setSubCoveringFilterPublisher(
				controlManager_SPtr->getSubCoveringFilterPublisher());
		if (rc != ISMRC_OK)
		{
			Trace_Debug(this,"start()","localSubManager_SPtr->setSubCoveringFilterPublisher failed");
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"start()",rc);
			return rc;
		}

		rc = localSubManager_SPtr->recoveryCompleted();
		if (rc != ISMRC_OK)
		{
			internalClose(false,true);
			Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()",rc);
			return rc;
		}

		state_ = STATE_RECOVERED;
		stateChangeTime_ = ism_common_currentTimeNanos();

		discoveryTimeoutTask_.reset(new DiscoveryTimeoutTask(dynamic_cast<RoutingTasksHandler&>(*this)));
		discoveryTimeoutTask_deadline_ = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(mcpConfig_.getDiscoveryTimeoutMillis());
	}

	discoveryTimeoutTask();

    if (rc == ISMRC_OK)
    {
        boost::recursive_mutex::scoped_lock lock(state_mutex);
        taskExecutor_SPtr->scheduleDelay(engineStatisticsTask_,
                boost::posix_time::seconds(mcpConfig_.getEngineStatsIntervalSec()));
        Trace_Debug(this, __FUNCTION__, "scheduled the EngineStatisticsTask for the first time");
    }

	Trace_Exit<MCPReturnCode>(this,"recoveryCompleted()", rc);
	return rc;
}

MCPReturnCode MCPRoutingImpl::startMessaging()
{
    using namespace spdr;
    Trace_Entry(this,"startMessaging()");
    MCPReturnCode rc = ISMRC_OK;
    bool done = false;

    while (!done)
    {
        {
            boost::recursive_mutex::scoped_lock lock(state_mutex);

            if (state_ == STATE_CLOSED)
            {
                rc = ISMRC_ClusterNotAvailable;
            }
            else if (state_ == STATE_CLOSED_DETACHED)
            {
                rc = ISMRC_ClusterDisabled;
            }
            else if (state_ == STATE_INIT)
            {
                rc = ISMRC_ClusterNotAvailable;
            }
            else if (state_ == STATE_ERROR)
            {
                rc = ISMRC_ClusterInternalErrorState;
            }
            else if (state_ == STATE_STARTED)
            {
                rc = ISMRC_Error;
                Trace_Event(this,"startMessaging()","Error: startMessaging called before recoveryCompleted");
            }

            if (rc != ISMRC_OK)
            {
                Trace_Exit<MCPReturnCode>(this,"startMessaging()",rc);
                return rc;
            }

            if (state_ == STATE_ACTIVE)
            {
                done = true;
                Trace_Event(this, "startMessaging()", "Detected cluster ACTIVE state, discovery finished.");
            }
        }

        if (!done)
        {
            Trace_Event(this, "startMessaging()", "Cluster still in discovery mode, going to sleep...");
            boost::this_thread::sleep( DISCOVERY_TIMEOUT_TASK_INTERVAL_MS );
        }
    }

    Trace_Exit<MCPReturnCode>(this,"startMessaging()", rc);
    return rc;
}


/*
 * @see MCPRouting
 */
MCPReturnCode MCPRoutingImpl::registerEngineEventCallback(EngineEventCallback* engineEventCallBack)
{

	boost::recursive_mutex::scoped_lock lock(state_mutex);
	MCPReturnCode rc = ISMRC_OK;

	if (state_ == STATE_INIT || state_ == STATE_STARTED)
	{
		if (controlManager_SPtr)
		{
			rc = controlManager_SPtr->registerEngineEventCallback(engineEventCallBack);
		}
		else
		{
			rc = ISMRC_NullPointer;
		}
	}
	else if (state_ == STATE_CLOSED)
	{
		rc = ISMRC_ClusterNotAvailable;
	}
    else if (state_ == STATE_CLOSED_DETACHED)
    {
        rc = ISMRC_ClusterDisabled;
    }
	else if (state_ == STATE_ERROR)
	{
		rc = ISMRC_ClusterInternalErrorState;
	}

	return rc;
}

/*
 * @see MCPRouting
 */
MCPReturnCode MCPRoutingImpl::registerProtocolEventCallback(ForwardingControl* forwardingControl)
{
	boost::recursive_mutex::scoped_lock lock(state_mutex);
	MCPReturnCode rc = ISMRC_OK;

	if (state_ == STATE_INIT || state_ == STATE_STARTED)
	{
		if (controlManager_SPtr)
		{
			rc = controlManager_SPtr->registerProtocolEventCallback(forwardingControl);
		}
		else
		{
			rc = ISMRC_NullPointer;
		}
	}
	else if (state_ == STATE_CLOSED)
	{
		rc = ISMRC_ClusterNotAvailable;
	}
    else if (state_ == STATE_CLOSED_DETACHED)
    {
        rc = ISMRC_ClusterDisabled;
    }
	else if (state_ == STATE_ERROR)
	{
		rc = ISMRC_ClusterInternalErrorState;
	}

	return rc;
}


MCPReturnCode MCPRoutingImpl::stop()
{
    using namespace spdr;
	Trace_Entry(this, "stop()");
	MCPReturnCode rc = internalClose();
	Trace_Exit<MCPReturnCode>(this, "stop()", rc);
	return rc;
}

MCPReturnCode MCPRoutingImpl::getStatistics(const ismCluster_Statistics_t *pStatistics)
{
    using namespace spdr;
	Trace_Entry(this, "getStatistics()");
	MCPReturnCode rc = ISMRC_OK;

	ismCluster_Statistics_t* pStats = const_cast<ismCluster_Statistics_t *>(pStatistics);

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

        pStats->pClusterName = my_ClusterName.c_str();
        pStats->pServerName = my_ServerName.c_str();
        pStats->pServerUID = my_ServerUID.c_str();
        pStats->connectedServers = 0;
        pStats->disconnectedServers = 0;
        pStats->healthStatus = ISM_CLUSTER_HEALTH_UNKNOWN;
        pStats->haStatus = ISM_CLUSTER_HA_UNKNOWN;

		if (state_ == STATE_INIT)
		{
			pStats->state = ISM_CLUSTER_LS_STATE_INIT;
			if (localSubManager_SPtr && localSubManager_SPtr->getHaStatus() == ISM_CLUSTER_HA_STANDBY)
			{
				pStats->state = ISM_CLUSTER_LS_STATE_STANDBY;	
			}			
		}
		else if (state_ == STATE_ERROR)
		{
			pStats->state = ISM_CLUSTER_LS_STATE_ERROR;
			rc = ISMRC_ClusterInternalErrorState;
		}
		else if (state_ == STATE_CLOSED)
		{
			//state is undefined
			rc = ISMRC_ClusterNotAvailable;
		}
		else if (state_ == STATE_CLOSED_DETACHED)
		{
			pStats->state = ISM_CLUSTER_LS_STATE_REMOVED;
			rc = ISMRC_OK;
		}
		else
		{
			if (state_ == STATE_ACTIVE)
			{
				pStats->state = ISM_CLUSTER_LS_STATE_ACTIVE;
			}
			else
			{
				pStats->state = ISM_CLUSTER_LS_STATE_DISCOVER;
			}

			if (localSubManager_SPtr)
			{
			   pStats->healthStatus = localSubManager_SPtr->getHealthStatus();
			   pStats->haStatus = localSubManager_SPtr->getHaStatus();
			}
			else
			{
			    rc = ISMRC_NullPointer;
			}

			if (rc == ISMRC_OK)
			{
			    if (controlManager_SPtr)
			    {
			        rc = controlManager_SPtr->getStatistics(pStats);
			    }
			    else
			    {
			        rc = ISMRC_NullPointer;
			    }
			}
		}
	}

	Trace_Exit<MCPReturnCode>(this, "getStatistics()", rc);
	return rc;
}

int MCPRoutingImpl::setLocalForwardingInfo(
		const char *pServerName,
		const char *pServerUID,
		const char *pServerAddress,
		int serverPort,
		uint8_t fUseTLS)
{
	boost::recursive_mutex::scoped_lock lock(state_mutex);

	MCPReturnCode rc = ISMRC_Error;

	if (state_ == STATE_INIT || state_ == STATE_STARTED)
	{
		if (controlManager_SPtr)
		{
			return controlManager_SPtr->setLocalForwardingInfo(
					pServerName,pServerUID,pServerAddress,serverPort,fUseTLS);
		}
		else
		{
			return ISMRC_NullPointer;
		}
	}
	else if (state_ == STATE_RECOVERED || state_ == STATE_ACTIVE)
	{
		rc = ISMRC_Error;
	}
	else if (state_ == STATE_CLOSED)
	{
		rc = ISMRC_ClusterNotAvailable;
	}
    else if (state_ == STATE_CLOSED_DETACHED)
    {
        rc = ISMRC_ClusterDisabled;
    }
	else if (state_ == STATE_ERROR)
	{
		rc = ISMRC_ClusterInternalErrorState;
	}

	return rc;
}

int MCPRoutingImpl::nodeForwardingConnected(const ismCluster_RemoteServerHandle_t phServerHandle)
{
	int rc = ISMRC_Error;

//	bool notify = false;
//
//	{
//	    boost::recursive_mutex::scoped_lock lock(state_mutex);
//
//	    if (state_ == STATE_RECOVERED || state_ == STATE_ACTIVE)
//	    {
//	        notify = true;
//	    }
//	    else if (state_ == STATE_CLOSED)
//	    {
//	        rc = ISMRC_ClusterNotAvailable;
//	    }
//	    else if (state_ == STATE_CLOSED_DETACHED)
//	    {
//	        rc = ISMRC_ClusterDisabled;
//	    }
//	    else if (state_ == STATE_STARTED || state_ == STATE_INIT)
//	    {
//	        rc = ISMRC_Error;
//	    }
//	    else if (state_ == STATE_ERROR)
//	    {
//	        rc = ISMRC_ClusterInternalErrorState;
//	    }
//	}
//
//	if (notify)
	{
        if (controlManager_SPtr)
            rc = controlManager_SPtr->nodeForwardingConnected(phServerHandle);
        else
            rc = ISMRC_NullPointer;
	}

	return rc;
}


int MCPRoutingImpl::nodeForwardingDisconnected(const ismCluster_RemoteServerHandle_t phServerHandle)
{
    int rc = ISMRC_Error;

//    bool notify = false;
//
//    {
//        boost::recursive_mutex::scoped_lock lock(state_mutex);
//        if (state_ == STATE_RECOVERED || state_ == STATE_ACTIVE)
//        {
//            notify = true;
//        }
//        else if (state_ == STATE_CLOSED)
//        {
//            rc = ISMRC_ClusterNotAvailable;
//        }
//        else if (state_ == STATE_CLOSED_DETACHED)
//        {
//            rc = ISMRC_ClusterDisabled;
//        }
//        else if (state_ == STATE_STARTED || state_ == STATE_INIT)
//        {
//            rc = ISMRC_Error;
//        }
//        else if (state_ == STATE_ERROR)
//        {
//            rc = ISMRC_ClusterInternalErrorState;
//        }
//    }
//
//    if (notify)
    {
        if (controlManager_SPtr)
            rc = controlManager_SPtr->nodeForwardingDisconnected(phServerHandle);
        else
            rc = ISMRC_NullPointer;
    }

	return rc;

}



MCPReturnCode MCPRoutingImpl::lookup(
		ismCluster_LookupInfo_t* pLookupInfo)
{
	if (globalSubManager_SPtr)
		return globalSubManager_SPtr->lookup(pLookupInfo);
	else
		return ISMRC_NullPointer;
}

MCPReturnCode MCPRoutingImpl::addSubscriptions(
		const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs)
{
	boost::recursive_mutex::scoped_lock lock(state_mutex);

	MCPReturnCode rc = ISMRC_Error;

	if (state_ == STATE_RECOVERED || state_ == STATE_STARTED || state_ == STATE_ACTIVE)
	{
		if (localSubManager_SPtr)
			return localSubManager_SPtr->addSubscriptions(pSubInfo,numSubs);
		else
			return ISMRC_NullPointer;
	}
	else if (state_ == STATE_CLOSED)
	{
		rc = ISMRC_ClusterNotAvailable;
	}
    else if (state_ == STATE_CLOSED_DETACHED)
    {
        rc = ISMRC_ClusterDisabled;
    }
	else if (state_ == STATE_INIT)
	{
		rc = ISMRC_ClusterNotAvailable;
	}
	else if (state_ == STATE_ERROR)
	{
		rc = ISMRC_ClusterInternalErrorState;
	}

	return rc;
}

MCPReturnCode MCPRoutingImpl::removeSubscriptions(
		const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs)
{
	boost::recursive_mutex::scoped_lock lock(state_mutex);

	MCPReturnCode rc = ISMRC_Error;

	if (state_ == STATE_RECOVERED || state_ == STATE_ACTIVE)
	{
		if (localSubManager_SPtr)
			return localSubManager_SPtr->removeSubscriptions(pSubInfo,numSubs);
		else
			return ISMRC_NullPointer;
	}
	else if (state_ == STATE_CLOSED)
	{
		rc = ISMRC_ClusterNotAvailable;
	}
    else if (state_ == STATE_CLOSED_DETACHED)
    {
        rc = ISMRC_ClusterDisabled;
    }
	else if (state_ == STATE_INIT || state_ == STATE_STARTED)
	{
		rc = ISMRC_Error;
	}
	else if (state_ == STATE_ERROR)
	{
		rc = ISMRC_ClusterInternalErrorState;
	}

	return rc;
}

int MCPRoutingImpl::onFatalError(const std::string& component, const std::string& errorMessage, int rc)
{
    using namespace spdr;

    {
        boost::recursive_mutex::scoped_lock lock(state_mutex);
        if (stateFailure_)
        {
            Trace_Warning(this,"onFatalError()", "Warning: onFatalError already called! ignoring this call. The previous call is going to close cluster component and shutdown server",
                    "component", component, "msg",errorMessage);
            return ISMRC_OK;

        }
        else
        {
            Trace_Error(this,"onFatalError()", "Going to close cluster component and shutdown server", "component", component, "msg",errorMessage);
            stateFailure_ = true;
        }
    }

    MCPReturnCode ret = internalClose(false,true);

    //let the logs get written
    sleep(5);
    //this normally does not return (abort)
    ism_common_shutdown(1);


    Trace_Exit(this,"onFatalError()", ret);
    return ret;
}

int MCPRoutingImpl::onFatalError_MaintenanceMode(const std::string& component, const std::string& errorMessage, int rc, int restartFlag)
{
    using namespace spdr;

    {
        boost::recursive_mutex::scoped_lock lock(state_mutex);
        if (stateFailure_)
        {
            Trace_Warning(this,"onFatalError_MaintenanceMode()",
                    "Warning: onFatalError or onRequestMaintenanceMode already called! ignoring this call.",
                    "component", component, "msg",errorMessage, "RC", spdr::stringValueOf(rc));
            return ISMRC_OK;

        }
        else
        {
            Trace_Error(this,"onFatalError_MaintenanceMode()", "Going to close cluster component and restart server in maintenance mode",
                    "component", component, "msg",errorMessage, "RC", spdr::stringValueOf(rc));
            stateFailure_ = true;
        }
    }

    MCPReturnCode ret1 = internalClose(false,true);
    if (ret1 != ISMRC_OK)
    {
        std::ostringstream what;
        what << "Error: Failure while calling internalClose, RC=" << ret1 << ", ignored, continue to restart in maintenance mode";
        Trace_Error(this,"onFatalError_MaintenanceMode()", what.str());
    }

    int ret2 = ism_admin_setMaintenanceMode(rc, restartFlag);
    if (ret2 != ISMRC_OK)
    {
        std::ostringstream what;
        what << "Error: Failure while calling ism_admin_setMaintenanceMode, RC=" << ret2;
        Trace_Error(this,"onFatalError_MaintenanceMode()", what.str());
        sleep(5);               //let the logs get written
        ism_common_shutdown(1); //this normally does not return (abort)
    }

    Trace_Exit(this,"onFatalError_MaintenanceMode()", ret2);
    return ret2;
}

void MCPRoutingImpl::discoveryTimeoutTask()
{
    using namespace spdr;
	Trace_Entry(this,"discoveryTimeoutTask()");

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);
		boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
		if (controlManager_SPtr->isReconciliationFinished())
		{
			state_ = STATE_ACTIVE;
			stateChangeTime_ = ism_common_currentTimeNanos();
			Trace_Event(this,"discoveryTimeoutTask()","Discovery ended, reconciliation finished");
		}
		else if (now > discoveryTimeoutTask_deadline_)
		{
			state_ = STATE_ACTIVE;
			stateChangeTime_ = ism_common_currentTimeNanos();
			Trace_Event(this,"discoveryTimeoutTask()","Discovery ended, timeout expired");
		}
		else
		{
			taskExecutor_SPtr->scheduleDelay(discoveryTimeoutTask_, DISCOVERY_TIMEOUT_TASK_INTERVAL_MS);
			Trace_Debug(this,"discoveryTimeoutTask()","rescheduled");
		}
	}

	Trace_Exit(this,"discoveryTimeoutTask()");
}

void MCPRoutingImpl::engineStatisticsTask()
{
    using namespace spdr;
    Trace_Entry(this,"engineStatisticsTask()");

    if (controlManager_SPtr && localSubManager_SPtr)
    {
        ismCluster_EngineStatistics_t stats;
        memset(&stats,0,sizeof(ismCluster_EngineStatistics_t));
        int rc1 = controlManager_SPtr->reportEngineStatistics(&stats);
        if (rc1 == ISMRC_OK)
        {
            int rc2 = localSubManager_SPtr->reportEngineStatistics(&stats);
            if (rc2 != ISMRC_OK)
            {
                Trace_Warning(this, __FUNCTION__, "Warning: Could not process report by localSubManager, skipping task",
                        "RC",spdr::stringValueOf(rc2));
            }
            else
            {
                boost::recursive_mutex::scoped_lock lock(state_mutex);
                if (state_ == STATE_ACTIVE || state_ == STATE_RECOVERED)
                {
                    taskExecutor_SPtr->scheduleDelay(engineStatisticsTask_,
                            boost::posix_time::seconds(mcpConfig_.getEngineStatsIntervalSec()));
                    Trace_Debug(this,"engineStatisticsTask()","rescheduled");
                }
                else
                {
                    Trace_Debug(this, __FUNCTION__, "State not STATE_RECOVERED | STATE_ACTIVE, task not rescheduled.");
                }
            }
        }
        else
        {
            Trace_Warning(this, __FUNCTION__, "Warning: Could not get report from engine, skipping task",
                    "RC",spdr::stringValueOf(rc1));
        }
    }
    else
    {
        std::ostringstream what;
        what << "Error: NULL pointer to manager, "
                << "local=" << (localSubManager_SPtr ? "Valid" : "NULL")
                << ", control=" << (controlManager_SPtr ? "Valid" : "NULL");
        Trace_Error(this, __FUNCTION__, what.str());
        onFatalError("Cluster", what.str(), ISMRC_NullPointer);
    }

    Trace_Exit(this,"engineStatisticsTask()");
}

void MCPRoutingImpl::traceLevelMonitorTask()
{
    using namespace spdr;

    int ism_trace_level = TRACE_LEVELX(Cluster);
    if (ism_trace_level != cluster_trace_level_)
    {
        spdr::log::Level spdr_trace_level = MCPRouting::ismLogLevel_to_spdrLogLevel(ism_trace_level);
        spdr::SpiderCastFactory::getInstance().changeLogLevel(
                spdr_trace_level, mcp::trace::Component_Name);
        cluster_trace_level_ = ism_trace_level;
        Trace_Event(this,"traceLevelMonitorTask","Changed Cluster component trace level", "ism-trace-level", ism_trace_level);
    }

    ism_trace_level = TRACE_LEVELX(SpiderCast);
    if (ism_trace_level != spidercast_trace_level_)
    {
        spdr::log::Level spdr_trace_level = MCPRouting::ismLogLevel_to_spdrLogLevel(ism_trace_level);
        spdr::SpiderCastFactory::getInstance().changeLogLevel(
                spdr_trace_level, spdr::trace::ScTrConstants::ScTr_Component_Name);
        spdr::SpiderCastFactory::getInstance().changeRUMLogLevel(ism_trace_level);
        spidercast_trace_level_ = ism_trace_level;
        Trace_Event(this,"traceLevelMonitorTask","Changed SpiderCast component trace level", "ism-trace-level", ism_trace_level);
    }

    taskExecutor_SPtr->scheduleDelay(traceLevelMonitorTask_, TRACE_LEVEL_MONITOR_TASK_INTERVAL_MS);
}

MCPReturnCode MCPRoutingImpl::internalClose(bool remove_self, bool leave_state_error)
{
    using namespace spdr;
	Trace_Entry(this,"internalClose()",
			"remove_self", boost::lexical_cast<std::string>(remove_self),
			"leave_state_error", boost::lexical_cast<std::string>(leave_state_error));

	MCPReturnCode rc = ISMRC_OK;
	bool do_close = false;

	{
	    boost::recursive_mutex::scoped_lock lock(state_mutex);

	    if (state_ == STATE_INIT || state_ == STATE_STARTED || state_ == STATE_RECOVERED || state_ == STATE_ACTIVE)
	    {
	        if (leave_state_error)
	        {
	            state_ = STATE_ERROR;
	            stateChangeTime_ = ism_common_currentTimeNanos();
	        }
	        else if (remove_self)
	        {
	            state_ = STATE_CLOSED_DETACHED;
	            stateChangeTime_ = ism_common_currentTimeNanos();
	            if (controlManager_SPtr)
	            {
	                rc = controlManager_SPtr->notifyTerm();
	                if (rc != ISMRC_OK)
	                {
	                    Trace_Event(this, "internalClose()", "Failed to notify TERM to engine or protocol, ignoring and continuing removal sequence.");
	                }
	            }
	        }
	        else
	        {
	            state_ = STATE_CLOSED;
	            stateChangeTime_ = ism_common_currentTimeNanos();
	        }
	        do_close = true;
	    }
	    else if (state_ == STATE_ERROR)
	    {
	        rc = ISMRC_ClusterInternalErrorState;
	    }
	    else if (state_ == STATE_CLOSED || state_ == STATE_CLOSED_DETACHED)
	    {
	        if (!remove_self)
	            rc = ISMRC_OK;
	        else
	            rc = ISMRC_ClusterNotAvailable;
	    }
	}

	if (do_close)
	{
		//The taskExecutor thread may call internaClose()
		MCPReturnCode rc_tmp0 = ISMRC_OK;;
		try
		{
			taskExecutor_SPtr->finish();
			if (boost::this_thread::get_id() != taskExecutor_SPtr->getThreadID())
			{
				taskExecutor_SPtr->join();
			}
			else
			{
				Trace_Event(this,"internalClose()","closing thread is TaskExecutor");
			}
		}
		catch (std::exception& e)
		{
			Trace_Debug(this,"internalClose()","Error joining taskExecutor thread",
					"what",e.what());
			rc_tmp0 = ISMRC_Error;
		}
		catch (...)
		{
		    Trace_Debug(this, "internalClose()", "Error: catch (...) unspecified error");
		    rc_tmp0 = ISMRC_Error;
		}

		MCPReturnCode rc_tmp1 = globalSubManager_SPtr->close(leave_state_error);
		MCPReturnCode rc_tmp2 = localSubManager_SPtr->close(leave_state_error);

		/* return the last error code that went wrong */
		if (rc_tmp0 != ISMRC_OK)
		{
			rc = rc_tmp0;
		}

		if (rc_tmp1 != ISMRC_OK)
		{
			Trace_Debug(this, "internalClose()", "Error closing GlobalSubManager");
			rc = rc_tmp1;
		}

		if (rc_tmp2 != ISMRC_OK)
		{
			Trace_Debug(this, "internalClose()", "Error closing LocalSubManager");
			rc = rc_tmp2;
		}

		if (!leave_state_error && remove_self)
		{
			MCPReturnCode rc_tmp3 = controlManager_SPtr->adminDetachFromCluster();
			if (rc_tmp3 != ISMRC_OK)
			{
				Trace_Debug(this, "internalClose()", "Error closing ControlManager, detach from cluster");
				rc = rc_tmp3;
			}
		}
		else
		{
			try
			{   //close soft only when not in error path
				controlManager_SPtr->close(!leave_state_error);
			}
			catch (spdr::SpiderCastRuntimeError& re)
			{
				Trace_Debug(this, "internalClose()", String("Error: ") + re.what());
				rc = ISMRC_ClusterInternalError;
				ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
			}
			catch (std::exception& e)
			{
				Trace_Debug(this, "internalClose()", String("Error: ") + e.what());
				rc = ISMRC_Error;
			}
			catch (...)
			{
				Trace_Debug(this, "internalClose()", "Error: catch (...) unspecified error");
				rc = ISMRC_Error;
			}
		}
	}

	Trace_Exit<MCPReturnCode>(this,"internalClose()",rc);
	return rc;
}

spdr::NodeID_SPtr MCPRoutingImpl::getNodeID()
{
	if (controlManager_SPtr)
	{
		return controlManager_SPtr->getNodeID();
	}
	else
	{
		return spdr::NodeID_SPtr();
	}
}

MCPReturnCode MCPRoutingImpl::adminDeleteNode(const ismCluster_RemoteServerHandle_t phServerHandle)
{
	if (controlManager_SPtr)
	{
		return controlManager_SPtr->adminDeleteNode(phServerHandle);
	}
	else
	{
		return ISMRC_NullPointer;
	}
}

MCPReturnCode MCPRoutingImpl::adminDetachFromCluster()
{
	return internalClose(true);
}

MCPReturnCode MCPRoutingImpl::updateRetainedStats(const char *pServerUID,
        void *pData, uint32_t dataLength)
{
    if (localSubManager_SPtr)
    {
        return localSubManager_SPtr->updateRetainedStats(pServerUID,pData,dataLength);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

MCPReturnCode MCPRoutingImpl::lookupRetainedStats(const char *pServerUID,
        ismCluster_LookupRetainedStatsInfo_t **pLookupInfo)
{
    if (globalSubManager_SPtr)
    {
        return globalSubManager_SPtr->lookupRetainedStats(pServerUID,pLookupInfo);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

MCPReturnCode MCPRoutingImpl::getView(ismCluster_ViewInfo_t **pView)
{
    using namespace spdr;
    Trace_Entry(this, "getView");

    MCPReturnCode rc = ISMRC_OK;

    {
        boost::recursive_mutex::scoped_lock lock(state_mutex);

        try
        {
            ismCluster_ViewInfo_t* view = new ismCluster_ViewInfo_t;
            view->pRemoteServers = NULL;
            view->numRemoteServers = 0;

            view->pLocalServer = new ismCluster_RSViewInfo_t;
            view->pLocalServer->pServerName = ism_common_strdup(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,1000),my_ServerName.c_str());
            if (view->pLocalServer->pServerName == NULL)
            {
                return ISMRC_AllocateError;
                //throw std::bad_alloc();
            }

            view->pLocalServer->pServerUID = ism_common_strdup(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,1000),my_ServerUID.c_str());
            if (view->pLocalServer->pServerUID == NULL)
            {
                return ISMRC_AllocateError;
                //throw std::bad_alloc();
            }


            view->pLocalServer->healthStatus = ISM_CLUSTER_HEALTH_UNKNOWN;
            view->pLocalServer->haStatus = ISM_CLUSTER_HA_UNKNOWN;
            view->pLocalServer->stateChangeTime = stateChangeTime_;

            view->pLocalServer->phServerHandle = NULL;

            *pView = view;

            if (state_ == STATE_INIT)
            {
                view->pLocalServer->state = ISM_CLUSTER_LS_STATE_INIT;
                if (localSubManager_SPtr && localSubManager_SPtr->getHaStatus() == ISM_CLUSTER_HA_STANDBY)
				{
					view->pLocalServer->state = ISM_CLUSTER_LS_STATE_STANDBY;	
				}			
            }
            else if (state_ == STATE_ERROR)
            {
                view->pLocalServer->state = ISM_CLUSTER_LS_STATE_ERROR;
                rc = ISMRC_ClusterInternalErrorState;
            }
            else if (state_ == STATE_CLOSED)
            {
                //state is undefined
                rc = ISMRC_ClusterNotAvailable;
            }
            else if (state_ == STATE_CLOSED_DETACHED)
            {
                view->pLocalServer->state = ISM_CLUSTER_LS_STATE_REMOVED;
            }
            else
            {
                if (state_ == STATE_ACTIVE)
                {
                    view->pLocalServer->state = ISM_CLUSTER_LS_STATE_ACTIVE;
                }
                else
                {
                    view->pLocalServer->state = ISM_CLUSTER_LS_STATE_DISCOVER;
                }

                if (localSubManager_SPtr)
                {
                    view->pLocalServer->healthStatus = localSubManager_SPtr->getHealthStatus();
                    view->pLocalServer->haStatus = localSubManager_SPtr->getHaStatus();
                }
                else
                {
                    rc = ISMRC_NullPointer;
                }

                if (rc == ISMRC_OK)
                {
                    if (controlManager_SPtr)
                    {
                        rc = controlManager_SPtr->getView(pView);
                    }
                    else
                    {
                        rc = ISMRC_NullPointer;
                    }
                }
            }
        }
        catch (std::bad_alloc& ba)
        {
            rc = ISMRC_AllocateError;
        }
    }

    Trace_Exit(this, "getView",rc);
    return rc;
}

MCPReturnCode MCPRoutingImpl::setHealthStatus(ismCluster_HealthStatus_t  healthStatus)
{
    if (localSubManager_SPtr)
    {
       return localSubManager_SPtr->setHealthStatus(healthStatus);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

MCPReturnCode MCPRoutingImpl::setHaStatus(ismCluster_HaStatus_t haStatus)
{
    if (localSubManager_SPtr)
    {
       return localSubManager_SPtr->setHaStatus(haStatus);
    }
    else
    {
        return ISMRC_NullPointer;
    }
}

} /* namespace mcp */
