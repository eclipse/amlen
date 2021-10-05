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

#include "LocalSubManagerImpl.h"
#include "MCPReturnCode.h"

namespace mcp
{

spdr::ScTraceComponent* LocalSubManagerImpl::tc_ = spdr::ScTr::enroll(
		mcp::trace::Component_Name,
		mcp::trace::SubComponent_Local,
		spdr::trace::ScTrConstants::Layer_ID_App, "LocalSubManagerImpl",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

LocalSubManagerImpl::LocalSubManagerImpl(
		const std::string& inst_ID,
		const MCPConfig& mcpConfig,
		const std::string& server_name,
		TaskExecutor& taskExecutor,
		ControlManager& controlManager) :
		spdr::ScTraceContext(tc_, inst_ID, ""),
		config(mcpConfig),
		taskExecutor(taskExecutor),
		m_started(false),
		m_closed(false),
		m_recovered(false),
		m_error(false),
		publishTask(new PublishLocalBFTask( dynamic_cast<LocalSubManager&>(*this))),
		publishTask_scheduled(false),
		retainTask(new PublishRetainedTask( dynamic_cast<LocalSubManager&>(*this))),
		retainTask_scheduled(false),
		monitoringTask(new PublishMonitoringTask(  dynamic_cast<LocalSubManager&>(*this))),
		monitoringTask_scheduled(false),
		exactManager( new LocalExactSubManager(inst_ID,mcpConfig, dynamic_cast<LocalSubManager&>(*this)) ),
		wildcardManager( new LocalWildcardSubManager(inst_ID,mcpConfig, server_name, dynamic_cast<LocalSubManager&>(*this), controlManager)),
		retainedManager( new LocalRetainedStatsManager(inst_ID,mcpConfig, dynamic_cast<LocalSubManager&>(*this))),
		monitoringManager( new LocalMonitoringManager(inst_ID,mcpConfig, dynamic_cast<LocalSubManager&>(*this))),
		fatalErrorHandler_(NULL),
		engineStatsNumPeriods_(30) //30*10s = 5min
{
	Trace_Entry(this, "LocalSubManagerImpl()");
	ismCluster_EngineStatistics_t zeros;
	memset(&zeros,0,sizeof(ismCluster_EngineStatistics_t));
	engineStatistics_.push_front(zeros);
}

LocalSubManagerImpl::~LocalSubManagerImpl()
{
	Trace_Entry(this, "~LocalSubManagerImpl()");
}

MCPReturnCode LocalSubManagerImpl::addSubscriptions(
		const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs)
{
	Trace_Entry(this, "addSubscriptions()",
				"num", boost::lexical_cast<std::string>(numSubs));

	boost::recursive_mutex::scoped_lock lock(m_stateMutex);

	MCPReturnCode rc = ISMRC_OK;

	if (m_closed)
	{
		if (m_error)
		{
			rc = ISMRC_ClusterInternalErrorState;
			Trace_Error(this, "addSubscriptions()", "Error: in error state", "RC", rc);
			return rc;
		}
		else
		{
			rc = ISMRC_ClusterNotAvailable;
			Trace_Error(this, "addSubscriptions()", "Error: already closed", "RC", rc);
			return rc;
		}
	}

	for (int i=0; i<numSubs; ++i)
	{
		if (pSubInfo[i].fWildcard == 0)
		{
			rc = exactManager->subscribe(pSubInfo[i].pSubscription);
		}
		else
		{
			rc = wildcardManager->subscribe(pSubInfo[i].pSubscription);
		}

		if (rc != ISMRC_OK)
		{
		    Trace_Error(this, "addSubscriptions()", "Error: failure in exact/wildcard manager", "RC", rc);
			break;
		}
	}

	Trace_Exit(this, "addSubscriptions()",rc);
	return rc;
}

MCPReturnCode LocalSubManagerImpl::removeSubscriptions(
		const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs)
{
	Trace_Entry(this, "removeSubscriptions()", "num",
			boost::lexical_cast<std::string>(numSubs));

	boost::recursive_mutex::scoped_lock lock(m_stateMutex);

	MCPReturnCode rc = ISMRC_OK;

	if (m_closed)
	{
		if (m_error)
		{
			rc = ISMRC_ClusterInternalErrorState;
			Trace_Error(this, __FUNCTION__, "Error: in error state", "RC", rc);
			return rc;
		}
		else
		{
			rc = ISMRC_ClusterNotAvailable;
			Trace_Error(this, __FUNCTION__, "Error: already closed", "RC", rc);
			return rc;
		}
	}

	for (int i = 0; i < numSubs; ++i)
	{
		if (pSubInfo[i].fWildcard == 0)
		{
			rc = exactManager->unsubscribe(pSubInfo[i].pSubscription);
		}
		else
		{
			rc = wildcardManager->unsubscribe(pSubInfo[i].pSubscription);
		}

		if (rc != ISMRC_OK)
		{
		    Trace_Error(this, "removeSubscriptions()", "Error: failure in exact/wildcard manager", "RC", rc);
			break;
		}
	}

	Trace_Exit(this, "removeSubscriptions()", rc);
	return rc;
}

MCPReturnCode LocalSubManagerImpl::updateRetainedStats(const char *pServerUID,
                   void *pData, uint32_t dataLength)
{
    Trace_Entry(this, "updateRetainedStats()");
    MCPReturnCode rc = ISMRC_OK;

    boost::recursive_mutex::scoped_lock lock(m_stateMutex);
    if (m_closed)
    {
        if (m_error)
        {
            rc = ISMRC_ClusterInternalErrorState;
            Trace_Error(this, "updateRetainedStats()", "Error: in error state", "RC", rc);
            return rc;
        }
        else
        {
            rc = ISMRC_ClusterNotAvailable;
            Trace_Error(this, "updateRetainedStats()", "Error: already closed", "RC", rc);
            return rc;
        }
    }

    rc = retainedManager->updateRetainedStats(pServerUID,pData,dataLength);

    Trace_Exit(this, "updateRetainedStats()", rc);
    return rc;
}

MCPReturnCode LocalSubManagerImpl::setHealthStatus(ismCluster_HealthStatus_t  healthStatus)
{
    Trace_Entry(this, "setHealthStatus()",
            "status", boost::lexical_cast<std::string>(healthStatus));

    boost::recursive_mutex::scoped_lock lock(m_stateMutex);
    MCPReturnCode rc = monitoringManager->setHealthStatus(healthStatus);

    Trace_Exit(this, "setHealthStatus()",rc);
    return rc;
}

MCPReturnCode LocalSubManagerImpl::setHaStatus(ismCluster_HaStatus_t haStatus)
{
    Trace_Entry(this, "setHaStatus()",
            "status", boost::lexical_cast<std::string>(haStatus));

    boost::recursive_mutex::scoped_lock lock(m_stateMutex);
    MCPReturnCode rc = monitoringManager->setHaStatus(haStatus);

    Trace_Exit(this, "setHaStatus()", rc);
    return rc;
}

ismCluster_HealthStatus_t LocalSubManagerImpl::getHealthStatus()
{
    boost::recursive_mutex::scoped_lock lock(m_stateMutex);
    return monitoringManager->getHealthStatus();
}

ismCluster_HaStatus_t LocalSubManagerImpl::getHaStatus()
{
    boost::recursive_mutex::scoped_lock lock(m_stateMutex);
    return monitoringManager->getHaStatus();
}

MCPReturnCode LocalSubManagerImpl::setSubCoveringFilterPublisher(
		SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher)
{
    Trace_Entry(this, __FUNCTION__);

    boost::recursive_mutex::scoped_lock lock(m_stateMutex);

    MCPReturnCode rc = ISMRC_OK;

    rc = exactManager->setSubCoveringFilterPublisher(
            subCoveringFilterPublisher);
    if (rc != ISMRC_OK)
    {
        return rc;
    }

    rc = wildcardManager->setSubCoveringFilterPublisher(
            subCoveringFilterPublisher);
    if (rc != ISMRC_OK)
    {
        return rc;
    }

    rc = retainedManager->setSubCoveringFilterPublisher(
            subCoveringFilterPublisher);
    if (rc != ISMRC_OK)
    {
        return rc;
    }

    rc = monitoringManager->setSubCoveringFilterPublisher(
            subCoveringFilterPublisher);

    return rc;
}

MCPReturnCode LocalSubManagerImpl::start()
{
	Trace_Entry(this, "start()");

	boost::recursive_mutex::scoped_lock lock(m_stateMutex);

    MCPReturnCode rc = ISMRC_OK;

    if (m_closed)
    {
        if (m_error)
        {
            rc = ISMRC_ClusterInternalErrorState;
            Trace_Error(this, __FUNCTION__, "Error: in error state", "RC", rc);
            return rc;
        }
        else
        {
            rc = ISMRC_ClusterNotAvailable;
            Trace_Error(this, __FUNCTION__, "Error: already closed", "RC", rc);
            return rc;
        }
    }

	if (m_started)
	{
	    rc = ISMRC_Error;
	    Trace_Error(this, __FUNCTION__, "Error: already started", "RC", rc);
		return rc;
	}

	if (m_recovered)
	{
        rc = ISMRC_Error;
        Trace_Error(this, __FUNCTION__, "Error: already recovered", "RC", rc);
        return rc;
	}

	m_started = true;

	rc = exactManager->start();
	if (rc != ISMRC_OK)
	{
	    Trace_Exit(this, "start()",rc);
		return rc;
	}

	rc = wildcardManager->start();
	if (rc != ISMRC_OK)
	{
	    Trace_Exit(this, "start()",rc);
	    return rc;
	}

	rc = retainedManager->start();
	if (rc != ISMRC_OK)
	{
	    Trace_Exit(this, "start()",rc);
	    return rc;
	}

	rc = monitoringManager->start();

	Trace_Exit(this, "start()",rc);
	return rc;
}

MCPReturnCode LocalSubManagerImpl::restoreSubscriptionPatterns(const std::vector<SubscriptionPattern_SPtr>& patterns)
{
    Trace_Entry(this, "restoreSubscriptionPatterns()");

    boost::recursive_mutex::scoped_lock lock(m_stateMutex);

    MCPReturnCode rc = ISMRC_OK;
    if (m_closed)
    {
        if (m_error)
        {
            rc = ISMRC_ClusterInternalErrorState;
            Trace_Error(this, __FUNCTION__, "Error: in error state", "RC", rc);
            return rc;
        }
        else
        {
            rc = ISMRC_ClusterNotAvailable;
            Trace_Error(this, __FUNCTION__, "Error: already closed", "RC", rc);
            return rc;
        }
    }

    if (!m_started)
    {
        rc = ISMRC_Error;
        Trace_Error(this, __FUNCTION__, "Error: not started", "RC", rc);
        return rc;
    }

    if (m_recovered)
    {
        rc = ISMRC_Error;
        Trace_Error(this, __FUNCTION__, "Error: already recovered", "RC", rc);
        return rc;
    }

    rc = wildcardManager->restoreSubscriptionPatterns(patterns);

    Trace_Exit(this, "restoreSubscriptionPatterns()",rc);
    return rc;
}

MCPReturnCode LocalSubManagerImpl::recoveryCompleted()
{
	boost::recursive_mutex::scoped_lock lock(m_stateMutex);
	MCPReturnCode rc = ISMRC_OK;

	if (m_closed)
	{
	    if (m_error)
	    {
	        rc = ISMRC_ClusterInternalErrorState;
	        Trace_Error(this, __FUNCTION__, "Error: in error state", "RC", rc);
	        return rc;
	    }
	    else
	    {
	        rc = ISMRC_ClusterNotAvailable;
	        Trace_Error(this, __FUNCTION__, "Error: already closed", "RC", rc);
	        return rc;
	    }
	}

	if (!m_started)
	{
	    rc = ISMRC_Error;
	    Trace_Error(this, __FUNCTION__, "Error: not started", "RC", rc);
	    return rc;
	}

	if (m_recovered)
	{
	    rc = ISMRC_Error;
	    Trace_Error(this, __FUNCTION__, "Error: already recovered", "RC", rc);
	    return rc;
	}

	//m_republish_base = true;
	rc = exactManager->recoveryCompleted();
	if (rc != ISMRC_OK)
	{
		Trace_Exit(this, "recoveryCompleted()",rc);
		return rc;
	}

	rc = wildcardManager->recoveryCompleted();
	if (rc != ISMRC_OK)
	{
		Trace_Exit(this, "recoveryCompleted()",rc);
		return rc;
	}

	rc = retainedManager->recoveryCompleted();
	if (rc != ISMRC_OK)
	{
	    Trace_Exit(this, "recoveryCompleted()",rc);
	    return rc;
	}

	rc = monitoringManager->recoveryCompleted();
	if (rc != ISMRC_OK)
	{
	    Trace_Exit(this, "recoveryCompleted()",rc);
	    return rc;
	}

	schedulePublishLocalBFTask(0);

	m_recovered = true;

	Trace_Exit(this, "recoveryCompleted()",rc);
	return rc;
}

int LocalSubManagerImpl::reportEngineStatistics(ismCluster_EngineStatistics_t *pEngineStatistics)
{
    if (pEngineStatistics)
    {
        engineStatistics_.push_front(*pEngineStatistics);
        if (engineStatistics_.size() > (engineStatsNumPeriods_+1) )
        {
            engineStatistics_.pop_back();
        }

        //10sec stats
        if (engineStatistics_.size() >= 2)
        {
            double errorRate = 0;
            int period = config.getEngineStatsIntervalSec();
            ismCluster_EngineStatistics_t delta;
            delta.numFwdIn = engineStatistics_[0].numFwdIn - engineStatistics_[1].numFwdIn;
            delta.numFwdInNoConsumer = engineStatistics_[0].numFwdInNoConsumer - engineStatistics_[1].numFwdInNoConsumer;
            delta.numFwdInRetained = engineStatistics_[0].numFwdInRetained - engineStatistics_[1].numFwdInRetained;

            if (delta.numFwdIn > 0)
            {
                errorRate = (double)(delta.numFwdInNoConsumer)/(double)(delta.numFwdIn);
            }

            if (errorRate >  config.getBloomFilterErrorRate())
            {
                std::ostringstream what;
                what << "Alert: Bloom-filter actual false-positive rate is higher than expected in the last "
                        << period << " seconds";
                Trace_Event(this, __FUNCTION__,
                        what.str(), "actual-rate",
                        spdr::stringValueOf(errorRate), "configured-rate",
                        spdr::stringValueOf(config.getBloomFilterErrorRate()));
            }

            if (delta.numFwdIn > 0) //Trace only when something happens, can increase this threshold if needed
            {
                std::ostringstream what;
                what << "Bloom-filter false-positive rate in the last " << period << " seconds";
                Trace_Dump(this, __FUNCTION__, what.str(),
                        "actual-fp-rate", spdr::stringValueOf(errorRate),
                        "configured-fp-rate", spdr::stringValueOf(config.getBloomFilterErrorRate()),
                        "InFwd Msg/Sec", spdr::stringValueOf( (double)(delta.numFwdIn) / period ),
                        "InFwdRetained Msg/Sec", spdr::stringValueOf( (double)(delta.numFwdInRetained) / period ));
            }
        }

        //5min stats
        if (engineStatistics_.size() >= engineStatsNumPeriods_+1)
        {
            double errorRate = 0;
            int period = engineStatsNumPeriods_*config.getEngineStatsIntervalSec();
            ismCluster_EngineStatistics_t delta;
            delta.numFwdIn = engineStatistics_[0].numFwdIn - engineStatistics_[engineStatsNumPeriods_].numFwdIn;
            delta.numFwdInNoConsumer = engineStatistics_[0].numFwdInNoConsumer - engineStatistics_[engineStatsNumPeriods_].numFwdInNoConsumer;
            delta.numFwdInRetained = engineStatistics_[0].numFwdInRetained - engineStatistics_[engineStatsNumPeriods_].numFwdInRetained;

            if (delta.numFwdIn > 0)
            {
                errorRate = (double)(delta.numFwdInNoConsumer)/(double)(delta.numFwdIn);
            }

            if (errorRate >  config.getBloomFilterErrorRate())
            {
                std::ostringstream what;
                what << "Alert: Bloom-filter actual false-positive rate is higher than expected in the last "
                        << period << " seconds";
                Trace_Event(this,__FUNCTION__, what.str(),
                        "actual-rate", spdr::stringValueOf(errorRate),
                        "configured-rate", spdr::stringValueOf(config.getBloomFilterErrorRate()));
            }

            if (delta.numFwdIn > 0) //Trace only when something happens, can increase this threshold if needed
            {
                std::ostringstream what;
                what << "Bloom-filter false-positive rate in the last " << period << " seconds";
                Trace_Dump(this, __FUNCTION__, what.str(),
                        "actual-fp-rate", spdr::stringValueOf(errorRate),
                        "configured-fp-rate", spdr::stringValueOf(config.getBloomFilterErrorRate()),
                        "InFwd Msg/Sec", spdr::stringValueOf( (double)(delta.numFwdIn) / period ),
                        "InFwdRetained Msg/Sec", spdr::stringValueOf( (double)(delta.numFwdInRetained) / period ));
            }
        }
    }
    else
    {
        return ISMRC_NullArgument;
    }

    return ISMRC_OK;
}

MCPReturnCode LocalSubManagerImpl::close(bool leave_state_error)
{
    Trace_Entry(this, __FUNCTION__, "leave-state-error", (leave_state_error?"T":"F"));

    boost::recursive_mutex::scoped_lock lock(m_stateMutex);
    m_closed = true;
    m_error = leave_state_error;

    publishTask->cancel();
    retainTask->cancel();
    monitoringTask->cancel();


    MCPReturnCode rc1 = exactManager->close();
    MCPReturnCode rc_return = rc1;

    rc1 = wildcardManager->close();
    if (rc1 != ISMRC_OK)
    {
        rc_return = rc1;
    }

    rc1 = retainedManager->close();
    if (rc1 != ISMRC_OK)
    {
        rc_return = rc1;
    }

    rc1 = monitoringManager->close();
    if (rc1 != ISMRC_OK)
    {
        rc_return = rc1;
    }

    return rc_return;
}

MCPReturnCode LocalSubManagerImpl::publishLocalBFTask()
{
	using namespace std;

	Trace_Entry(this, "publishLocalBFTask()");

	MCPReturnCode rc = ISMRC_OK;

	{
	    boost::recursive_mutex::scoped_lock lock(m_stateMutex);

	    if (m_closed)
	    {
	        return ISMRC_OK;
	    }

	    publishTask_scheduled = false;

	    rc = exactManager->publishLocalExactBF();

	    if (rc == ISMRC_Closed)
	    {
	        Trace_Warning(this, "publishLocalBFTask()", "Warning: calling exactManager->publishLocalExactBF(), FilterPublisher already closed, ignoring");
	    }
	    else if (rc != ISMRC_OK)
	    {
	        Trace_Error(this, "publishLocalBFTask()", "Error: calling publishLocalExactBF()","RC", rc);
	    }

	    if (rc == ISMRC_OK)
	    {
	        rc = wildcardManager->publishLocalUpdates();

	        if (rc == ISMRC_Closed)
	        {
	            Trace_Warning(this, "publishLocalBFTask()", "Warning: calling wildcardManager->publishLocalUpdates(), FilterPublisher already closed, ignoring");
	        }
	        else if (rc != ISMRC_OK)
	        {
	            Trace_Error(this, "publishLocalBFTask()", "Error: calling LocalWildcardSubManager::publishLocalUpdates()","RC", rc);
	        }
	    }
	}

	if (rc == ISMRC_Closed)
	{
	    rc = ISMRC_OK;
	}
	else if (rc != ISMRC_OK)
	{
	    onFatalError(this->getMemberName(), "Fatal Error in cluster component. Local server will leave the cluster.", rc);
	}

	Trace_Exit(this, "publishLocalBFTask()",rc);
	return rc;
}

MCPReturnCode LocalSubManagerImpl::schedulePublishLocalBFTask(int delayMillis)
{
	Trace_Entry(this, "schedulePublishLocalBFTask()", "delay", boost::lexical_cast<std::string>(delayMillis));

	bool scheduled = false;

	if (!publishTask_scheduled) //TODO zero delay after long pause?
	{
		taskExecutor.scheduleDelay(publishTask, boost::posix_time::milliseconds(delayMillis));
		publishTask_scheduled = true;
		scheduled = true;
	}

	Trace_Exit(this, "schedulePublishLocalBFTask()", (scheduled ? "rescheduled" : "already scheduled"));
	return ISMRC_OK;
}

/*
 * @see LocalSubManager
 */
MCPReturnCode LocalSubManagerImpl::publishRetainedTask()
{
	using namespace std;

	Trace_Entry(this, "publishRetainedTask()");

	MCPReturnCode rc = ISMRC_OK;

	{
	    boost::recursive_mutex::scoped_lock lock(m_stateMutex);

	    if (m_closed)
	    {
	        return ISMRC_OK;
	    }

	    retainTask_scheduled = false;

	    rc = retainedManager->publishRetainedStats();
	}

	if (rc == ISMRC_Closed)
	{
	    Trace_Warning(this, "publishRetainedTask()", "Warning: calling publishRetainedStats(), FilterPublisher already closed, ignoring", "RC",spdr::stringValueOf(rc));
	    rc = ISMRC_OK;
	}
	else if (rc != ISMRC_OK)
    {
        Trace_Error(this, "publishRetainedTask()", "Error: calling publishRetainedStats()","RC", rc);
        onFatalError(this->getMemberName(), "Fatal Error in cluster component. Local server will leave the cluster.", rc);
    }

	Trace_Exit(this, "publishRetainedTask()",rc);
	return rc;
}


/*
 * @see LocalSubManager
 */
MCPReturnCode LocalSubManagerImpl::schedulePublishRetainedTask(int delayMillis)
{
	Trace_Entry(this, "schedulePublishRetaiendTask()", "delay", boost::lexical_cast<std::string>(delayMillis));

	bool scheduled = false;

	if (!retainTask_scheduled) //TODO zero delay after long pause?
	{
		taskExecutor.scheduleDelay(retainTask, boost::posix_time::milliseconds(delayMillis));
		retainTask_scheduled = true;
		scheduled = true;
	}

	Trace_Exit(this, "schedulePublishRetainTask()", (scheduled ? "rescheduled" : "already scheduled"));
    return ISMRC_OK;
}

MCPReturnCode LocalSubManagerImpl::publishMonitoringTask()
{
    using namespace std;

    Trace_Entry(this, "publishMonitoringTask()");

    MCPReturnCode rc = ISMRC_OK;

    {
        boost::recursive_mutex::scoped_lock lock(m_stateMutex);

        if (m_closed)
        {
            return ISMRC_OK;
        }

        monitoringTask_scheduled = false;

        rc = monitoringManager->publishMonitoringStatus();
    }

    if (rc == ISMRC_Closed)
    {
        Trace_Warning(this, "publishMonitoringTask()", "Warning: calling publishMonitoringTask(), FilterPublisher already closed, ignoring","RC", spdr::stringValueOf(rc));
        rc = ISMRC_OK;
    }
    else if (rc != ISMRC_OK)
    {
        Trace_Error(this, "publishMonitoringTask()", "Error: calling publishMonitoringTask()","RC", rc);
        onFatalError(this->getMemberName(), "Fatal Error in cluster component. Local server will leave the cluster.", rc);
    }

    Trace_Exit(this, "publishMonitoringTask()", rc);
    return rc;
}

MCPReturnCode LocalSubManagerImpl::schedulePublishMonitoringTask(int delayMillis)
{
    Trace_Entry(this, "schedulePublishMonitoringTask()", "delay", boost::lexical_cast<std::string>(delayMillis));

    bool scheduled = false;

    if (!monitoringTask_scheduled) //TODO zero delay after long pause?
    {
        taskExecutor.scheduleDelay(monitoringTask, boost::posix_time::milliseconds(delayMillis));
        monitoringTask_scheduled = true;
        scheduled = true;
    }

    Trace_Exit(this, "schedulePublishMonitoringTask()", (scheduled ? "rescheduled" : "already scheduled"));
    return ISMRC_OK;
}

int LocalSubManagerImpl::update(
		ismCluster_RemoteServerHandle_t hClusterHandle, const char* pServerUID,
		const RemoteSubscriptionStats& stats)
{
    boost::recursive_mutex::scoped_lock lock(m_stateMutex);
    return wildcardManager->update(hClusterHandle, pServerUID, stats);
}

int LocalSubManagerImpl::disconnected(
		ismCluster_RemoteServerHandle_t hClusterHandle, const char* pServerUID)
{
	boost::recursive_mutex::scoped_lock lock(m_stateMutex);
	return wildcardManager->disconnected(hClusterHandle, pServerUID);
}

int LocalSubManagerImpl::connected(
		ismCluster_RemoteServerHandle_t hClusterHandle, const char* pServerUID)
{
	boost::recursive_mutex::scoped_lock lock(m_stateMutex);
	return wildcardManager->connected(hClusterHandle, pServerUID);
}

int LocalSubManagerImpl::remove(
		ismCluster_RemoteServerHandle_t hClusterHandle, const char* pServerUID)
{
	boost::recursive_mutex::scoped_lock lock(m_stateMutex);
	return wildcardManager->remove(hClusterHandle, pServerUID);
}

int LocalSubManagerImpl::onFatalError(const std::string& component, const std::string& errorMessage, int rc)
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

void LocalSubManagerImpl::setFatalErrorHandler(FatalErrorHandler* pFatalErrorHandler)
{
	if (pFatalErrorHandler == NULL)
	{
		std::string what = "LocalSubManagerImpl::setFatalErrorHandler Null handler";
		throw MCPRuntimeError(what, ISMRC_NullArgument);
	}

	fatalErrorHandler_ = pFatalErrorHandler;
}


} /* namespace mcp */
