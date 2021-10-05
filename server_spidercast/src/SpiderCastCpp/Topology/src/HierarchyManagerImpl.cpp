// Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
/*
 * HierarchyManagerImpl.cpp
 *
 *  Created on: Oct 4, 2010
 */

#include "HierarchyManagerImpl.h"

namespace spdr
{
using namespace trace;

ScTraceComponent* HierarchyManagerImpl::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy, "HierarchyManagerImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchyManagerImpl::HierarchyManagerImpl(const String& instID,
		const SpiderCastConfigImpl& config, NodeIDCache& nodeIDCache, VirtualIDCache& vidCache,
		CoreInterface& coreInterface) :
	ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
			instID_(instID),
			config_(config),
			coreInterface_(coreInterface),
			nodeIDCache_(nodeIDCache),
			viewKeeper_SPtr(new HierarchyViewKeeper(instID_, config_, *this)),
			delegate_(instID, config, nodeIDCache, vidCache, coreInterface, viewKeeper_SPtr),
			supervisor_(instID, config, nodeIDCache, coreInterface, viewKeeper_SPtr),
			memMngr_SPtr(),
			taskSchedule_SPtr(),
			periodicTask_SPtr(),
			managementZone(false),
			isDelegateCandidate(false),
			started_(false),
			closed_(false),
			close_done_(false),
			close_soft_(false),
			first_periodic_task_(true)
{
	Trace_Entry(this, "HierarchyManagerImpl()");

	//determine whether this instance is a supervisor or a delegate candidate
	if (config_.getBusName_SPtr()->getLevel() == 1)
	{

		managementZone = true;
	}
	else if (!config_.getSupervisorBootstrapSet().empty())
	{
		Trace_Event(this, "HierarchyManagerImpl()",
				"In base zone, delegate candidate");
		isDelegateCandidate = true;
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,
				"HierarchyManagerImpl()");
		buffer->addProperty<bool> ("managementZone", managementZone);
		buffer->addProperty<bool> ("isDelegateCandidate", isDelegateCandidate);
		buffer->invoke();
	}
}

HierarchyManagerImpl::~HierarchyManagerImpl()
{
	Trace_Entry(this, "~HierarchyManagerImpl()");
}

void HierarchyManagerImpl::init()
{
	Trace_Entry(this, "init()");

	memMngr_SPtr = coreInterface_.getMembershipManager();
	taskSchedule_SPtr = coreInterface_.getTopoMemTaskSchedule();
	periodicTask_SPtr = AbstractTask_SPtr (
			new HierarchyPeriodicTask(coreInterface_));

	memMngr_SPtr->registerInternalMembershipConsumer(
			boost::static_pointer_cast<SCMembershipListener>(viewKeeper_SPtr),
			MembershipManager::IntMemConsumer_Hierarchy);

	delegate_.init();
	supervisor_.init();

	Trace_Exit(this, "init()");
}

void HierarchyManagerImpl::destroyCrossRefs()
{
	Trace_Entry(this, "destroyCrossRefs()");
	//reset all shared pointers to break cycles.

	delegate_.destroyCrossRefs();
	supervisor_.destroyCrossRefs();

	memMngr_SPtr.reset();
	taskSchedule_SPtr.reset();
	periodicTask_SPtr.reset();

	Trace_Exit(this, "destroyCrossRefs()");
}

/*
 * Threading: App-thread
 */
void HierarchyManagerImpl::start()
{
	Trace_Entry(this, "start()");

	{
		boost::recursive_mutex::scoped_lock lock(hier_mutex);
		started_ = true;
	}

	//TODO remove this once done
	taskSchedule_SPtr->scheduleDelay(periodicTask_SPtr,
			TaskSchedule::ZERO_DELAY);

	if (isDelegateCandidate)
	{
		AbstractTask_SPtr firstDelegateTask_SPtr =
				AbstractTask_SPtr (
						new HierarchyDelegateConnectTask(instID_, delegate_));
		taskSchedule_SPtr->scheduleDelay(firstDelegateTask_SPtr,
				TaskSchedule::ZERO_DELAY);
	}
	else if (managementZone)
	{
		//TODO
	}

	//TODO
	Trace_Exit(this, "start()");
}

void HierarchyManagerImpl::terminate(bool soft)
{
	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "terminate()");
		buffer->addProperty<bool> ("soft", soft);
		buffer->invoke();
	}

	bool do_termination_task = true;

	{
		boost::recursive_mutex::scoped_lock lock(hier_mutex);

		if (!closed_)
		{
			closed_ = true;
			close_done_ = false;
			close_soft_ = soft;

			delegate_.terminate();
			supervisor_.terminate();

			if (!started_)
			{
				do_termination_task = false;
			}
		}
		else
		{
			do_termination_task = false;
		}
	}

	if (do_termination_task)
	{
		periodicTask_SPtr->cancel();
		//schedule a termination task to send leave messages and clean-up
		taskSchedule_SPtr->scheduleDelay(AbstractTask_SPtr (
				new HierarchyTerminationTask(coreInterface_)),
				TaskSchedule::ZERO_DELAY);
	}

	Trace_Exit(this, "terminate()");
}

/*
 * @see spdr::HierarchyManager
 */
bool HierarchyManagerImpl::isManagementZone() const
{
	return managementZone;
}


void HierarchyManagerImpl::terminationTask()
{
	Trace_Entry(this, "terminationTask()");

	if (close_soft_)
	{
		if (isDelegateCandidate)
		{
			delegate_.sendLeave2All();
		}
		else if (managementZone)
		{
			supervisor_.sendLeaveMsg();
		}
	}

	{
		boost::recursive_mutex::scoped_lock lock(hier_mutex);
		close_done_ = true;
	}

	Trace_Exit(this, "terminationTask()");
}

void HierarchyManagerImpl::processIncomingHierarchyMessage(
		SCMessage_SPtr inHierarchyMsg)
{
	Trace_Entry(this, "processIncomingHierarchyMessage()");

	{
		boost::recursive_mutex::scoped_lock lock(hier_mutex);
		if (closed_)
		{
			Trace_Exit(this, "processIncomingHierarchyMessage()",
					"skipping, closed");
			return;
		}
	}

	if (managementZone)
	{
		supervisor_.processIncomingHierarchyMessage(inHierarchyMsg);
	}
	else if (isDelegateCandidate)
	{
		delegate_.processIncomingHierarchyMessage(inHierarchyMsg);
	}
	else
	{
		Trace_Event(this, "processIncomingHierarchyMessage()",
				"Orphan message", "message",
				(inHierarchyMsg ? inHierarchyMsg->toString() : "null"));
	}

	Trace_Exit(this, "processIncomingHierarchyMessage()");
}

void HierarchyManagerImpl::processIncomingCommEventMsg(
		SCMessage_SPtr incomingCommEvent)
{
	Trace_Entry(this, "processIncomingCommEventMsg()");

	{
		boost::recursive_mutex::scoped_lock lock(hier_mutex);
		if (closed_)
		{
			Trace_Exit(this, "processIncomingCommEventMsg()",
					"skipping, closed");
			return;
		}
	}

	if (managementZone)
	{
		supervisor_.processIncomingCommEventMsg(incomingCommEvent);
	}
	else if (isDelegateCandidate)
	{
		delegate_.processIncomingCommEventMsg(incomingCommEvent);
	}
	else
	{
		Trace_Event(this, "processIncomingCommEventMsg()", "Orphan message",
				"message", (incomingCommEvent ? incomingCommEvent->toString()
						: "null"));
	}

	Trace_Exit(this, "processIncomingHierarchyMessage()");
}

void HierarchyManagerImpl::periodicTask()
{
	Trace_Entry(this, "periodicTask()");

	memMngr_SPtr->getAttributeControl().setAttribute(".hier.enabled",
			std::make_pair((int) instID_.length()+1, instID_.c_str()));

	//TODO remove this once done.

	Trace_Exit(this, "periodicTask()");
}

void HierarchyManagerImpl::hierarchyViewChanged()
{
	Trace_Entry(this, "hierarchyViewChanged()");
	//TODO

	if (isDelegateCandidate)
	{
		delegate_.rescheduleConnectTask(0);
		if (config_.isRoutingEnabled())
		{
			delegate_.reschedulePubSubBridgeTask(0);
		}
	}
	else if (managementZone)
	{
		//TODO schedule a task
	}

	Trace_Exit(this, "hierarchyViewChanged()");
}

void HierarchyManagerImpl::globalViewChanged()
{
	Trace_Entry(this, "globalViewChanged()");

	if (isDelegateCandidate)
	{
		delegate_.globalViewChanged();
	}
	else if (managementZone)
	{
		//TODO
	}

	Trace_Exit(this, "globalViewChanged()");
}

void HierarchyManagerImpl::verifyIncomingMessageAddressing(
		const String& msgSenderName, const String& connSenderName,
		const String& msgTargetName) throw (SpiderCastRuntimeError)
{
	if (msgSenderName != connSenderName)
	{
		Trace_Event(this, "verifyIncomingMessageAddressing()",
				"Sender name different in message vs. connection", "msg",
				msgSenderName, "conn", connSenderName);
		String
				what(
						"HierarchyManager.verifyIncomingMessageAddressing(), Sender name different in message vs. connection");
		throw SpiderCastRuntimeError(what);
	}

	if (msgTargetName != config_.getMyNodeID()->getNodeName())
	{
		Trace_Event(this, "verifyIncomingMessageAddressing()",
				"Target name in message is not me", "msg", msgTargetName);
		String
				what(
						"HierarchyManager.verifyIncomingMessageAddressing(), Target name in message is not me");
		throw SpiderCastRuntimeError(what);
	}
}

int64_t HierarchyManagerImpl::queueForeignZoneMembershipRequest(
		BusName_SPtr zoneBusName, bool includeAttributes, int timeoutMillis)
		throw (SpiderCastLogicError, SpiderCastRuntimeError)
{
	Trace_Entry(this, "queueForeignZoneMembershipRequest()");

	if (config_.getBusName_SPtr()->getLevel() != 1)
	{
		std::ostringstream what;
		what << "This operation is not supported on a base-zone: "
				<< config_.getBusName();
		throw IllegalStateException(what.str());
	}

	if (config_.getBusName_SPtr()->isManaged(*zoneBusName))
	{
		std::ostringstream what;
		what << "zoneBusName must be of a managed base-zone: "
				<< zoneBusName->toString();
		throw IllegalArgumentException(what.str());
	}

	//TODO get it from the local supervisor or send a request to a remote supervisor
	int64_t requestId = supervisor_.queueForeignZoneMembershipRequest(
			zoneBusName, includeAttributes, timeoutMillis);

	Trace_Exit<int64_t> (this, "queueForeignZoneMembershipRequest()", requestId);
	return requestId;
}

int64_t HierarchyManagerImpl::queueZoneCensusRequest()
		throw (SpiderCastLogicError, SpiderCastRuntimeError)
{
	Trace_Entry(this, "queueZoneCensusRequest()");

	if (config_.getBusName_SPtr()->getLevel() != 1)
	{
		std::ostringstream what;
		what << "This operation is not supported on a base-zone: "
				<< config_.getBusName();
		throw IllegalStateException(what.str());
	}

	int64_t requestId = supervisor_.queueZoneCensusRequest();

	Trace_Exit<int64_t> (this, "queueZoneCensusRequest()", requestId);

	return requestId;
}

}
