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
 * MembershipServiceImpl.cpp
 *
 *  Created on: 28/03/2010
 */

#include <boost/algorithm/string.hpp>

#include "MembershipServiceImpl.h"

namespace spdr
{

ScTraceComponent* MembershipServiceImpl::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"MembershipServiceImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

MembershipServiceImpl::MembershipServiceImpl(
		const String& instID,
		NodeIDImpl_SPtr myID,
		BusName_SPtr myBusName,
		MembershipManager_SPtr memMngr,
		HierarchyManager_SPtr hierMngr,
		const SpiderCastConfigImpl& spdrConfig,
		const PropertyMap& prop,
		MembershipListener& listener) :
	ScTraceContext(tc_,instID,myID->getNodeName()),
	myID_(myID),
	myBusName_(myBusName),
	memMngr_SPtr_(memMngr),
	hierMngr_SPtr_(hierMngr),
	spdrConfig_(spdrConfig),
	config_(prop), //may throw IllegalConfigException
	membershipListener_(listener),
	attributeManager_(memMngr->getAttributeControl()),
	closed_(false),
	firstViewDelivered_(false)
{
	Trace_Entry(this,"MembershipServiceImpl()");
}

MembershipServiceImpl::~MembershipServiceImpl()
{
	Trace_Entry(this,"~MembershipServiceImpl()");
	close(); //close the service if it is not closed already
}

/*
 * @see spdr::MembershipService
 */
void MembershipServiceImpl::close()
{
	Trace_Entry(this,"close()");

	bool destroy = false;

	{
		boost::mutex::scoped_lock lock(mutex_); //always most internal mutex
		if (!closed_)
		{
			destroy = true;
			closed_ = true;
		}
	}

	if (destroy)
	{
		memMngr_SPtr_->destroyMembershipService(); //takes a mutex, hence outside of lock
		memMngr_SPtr_.reset();
		hierMngr_SPtr_.reset();
	}

	Trace_Exit(this,"close()");
}

/*
 * Called when closing SpiderCast
 */
void MembershipServiceImpl::internalClose()
{
	Trace_Entry(this,"internalClose()");
	{
		boost::mutex::scoped_lock lock(mutex_); //always most internal mutex
		closed_ = true;
	}
	memMngr_SPtr_.reset();
	Trace_Exit(this,"internalClose()");
}

/*
 * @see spdr::MembershipService
 */
bool MembershipServiceImpl::isClosed()
{
	boost::mutex::scoped_lock lock(mutex_); //always most internal mutex
	return closed_;
}

void MembershipServiceImpl::queueForDelivery(event::MembershipEvent_SPtr event_SPtr)
{
	Trace_Entry(this,"queueForDelivery()");

	if (!firstViewDelivered_)
	{
		if ((*event_SPtr).getType() != event::View_Change)
		{
			String what("Error: First event must be a ViewChange, event=");
			what.append((*event_SPtr).toString());
			SpiderCastRuntimeError ex(what);

			Trace_Error(this,"queueForDelivery()",what);

			throw ex;
		}

		firstViewDelivered_ = true;
	}

	//TODO implement a delivery Q, currently deliver directly to app. using MemTopo-thread.
	deliverEventToListener(event_SPtr);

	Trace_Exit(this,"queueForDelivery()");
}


/*
 * @see spdr::MembershipService
 */
bool MembershipServiceImpl::setHighPriorityMonitor(bool value)
{
	Trace_Entry(this, "setHighPriorityMonitor()", "value", value?"true":"false");
	bool hpm = false;

	if (spdrConfig_.isHighPriorityMonitoringEnabled() && spdrConfig_.isUDPDiscovery())
	{
		String hpmKey(".hpm");

		if (value)
		{
			attributeManager_.setAttribute(hpmKey, EmptyAttributeValue);
			hpm = true;
			Trace_Event(this, "setHighPriorityMonitor()","set");
		}
		else
		{
			attributeManager_.removeAttribute(hpmKey);
			hpm = false;
			Trace_Event(this, "setHighPriorityMonitor()","reset");
		}

	}
	else
	{
		Trace_Event(this, "setHighPriorityMonitor()",
				"High priority monitoring not permitted by configuration.");
	}

	Trace_Exit<bool>(this,"setHighPriorityMonitor()",hpm);
	return hpm;
}

/*
 * @see spdr::MembershipService
 */
bool MembershipServiceImpl::isHighPriorityMonitor()
{
	Trace_Entry(this, "isHighPriorityMonitor()");

	bool rc = false;

	if (spdrConfig_.isHighPriorityMonitoringEnabled() && spdrConfig_.isUDPDiscovery())
	{
		String hpmKey(".hpm");
		std::pair<event::AttributeValue,bool> res = attributeManager_.getAttribute(hpmKey);
		rc = res.second;
	}
	else
	{
		Trace_Event(this, "isHighPriorityMonitor()",
				"High priority monitoring not permitted by configuration.");
	}

	Trace_Exit(this, "isHighPriorityMonitor()", rc);
	return rc;
}

void MembershipServiceImpl::deliverEventToListener(event::MembershipEvent_SPtr event_SPtr)
{
	TRACE_ENTRY3(this, "deliverEventToListener()", "event", event_SPtr->toString());

	if (isClosed())
	{
		Trace_Event(this, "deliverEventToListener()", "service closed, dropping event");
		return;
	}
	//TODO implement a delivery Q, currently deliver directly to app. using MemTopo-thread.
	try
	{
		membershipListener_.onMembershipEvent(event_SPtr);
	}
	catch (std::exception& e)
	{
		Trace_Error(this, "deliverEventToListener()", "Error: exception thrown in MembershipListener",
				"what", e.what(), "typeid", typeid(e).name());
		throw;
	}

	Trace_Exit(this, "deliverEventToListener()");
}

//=== Attribute manipulation ===

/*
 * @see spdr::MembershipService
 */
bool MembershipServiceImpl::setAttribute(const String& key, Const_Buffer value)
{
	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
	        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "setAttribute");
	        buffer->addProperty("key", key);
	        buffer->addProperty("value", binBufferToString(value));
	        buffer->invoke();
	}

	boost::mutex::scoped_lock lock(mutex_);
	if (closed_)
	{
		throw IllegalStateException("MembershipService closed.");
	}
	else
	{
		String key1 = boost::trim_copy(key);
		if (key1.length()>0 && isalnum(key1[0]))
		{
			bool result = attributeManager_.setAttribute(key1,value);

			//memMngr_SPtr_->scheduleChangeOfMetadataDeliveryTask(); //TODO move this to attribute manager

			Trace_Exit<bool>(this, "setAttribute()",result);
			return result;
		}
		else
		{
			String what("Illegal key: '");
			what.append(key).append("'");
			throw IllegalArgumentException(what);
		}
	}
}

/*
 * @see spdr::MembershipService
 */
std::pair<event::AttributeValue,bool> MembershipServiceImpl::getAttribute(
		const String& key)
{
	Trace_Entry(this, "getAttribute()", "key", key);

	boost::mutex::scoped_lock lock(mutex_);
	if (closed_)
	{
		throw IllegalStateException("MembershipService closed.");
	}
	else
	{
		std::pair<event::AttributeValue,bool> res = attributeManager_.getAttribute(key);

		if (ScTraceBuffer::isExitEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::exit(this, "getAttribute()");
		    buffer->addProperty<bool>("found", res.second);
		    buffer->addProperty("value", binBufferToString(
		    		Const_Buffer(res.first.getLength(),res.first.getBuffer().get())));
		    buffer->invoke();
		}

		return res;
	}
}

/*
 * @see spdr::MembershipService
 */
bool MembershipServiceImpl::removeAttribute(const String& key)
{
	Trace_Entry(this, "removeAttribute()", "key", key);

	boost::mutex::scoped_lock lock(mutex_);
	if (closed_)
	{
		throw IllegalStateException("MembershipService closed.");
	}
	else
	{
		String key1 = boost::trim_copy(key);
		if (key1.length()>0 && isalnum(key1[0]))
		{
			bool result = attributeManager_.removeAttribute(key1);
			//memMngr_SPtr_->scheduleChangeOfMetadataDeliveryTask(); //TODO move this to attribute manager
			Trace_Exit<bool>(this, "removeAttribute()",result);
			return result;
		}
		else
		{
			String what("Illegal key: '");
			what.append(key).append("'");
			throw IllegalArgumentException(what);
		}
	}
}

/*
 * @see spdr::MembershipService
 */
void MembershipServiceImpl::clearAttributeMap()
{
	Trace_Entry(this, "clearAttributeMap()");

	boost::mutex::scoped_lock lock(mutex_);
	if (closed_)
	{
		throw IllegalStateException("MembershipService closed.");
	}
	else
	{
		attributeManager_.clearExternalAttributeMap();
		//memMngr_SPtr_->scheduleChangeOfMetadataDeliveryTask(); //TODO move to attribute manager
	}
	Trace_Exit(this, "clearAttributeMap()");
}

/*
 * @see spdr::MembershipService
 */
bool MembershipServiceImpl::isEmptyAttributeMap()
{
	Trace_Entry(this, "isEmptyAttributeMap()");

	boost::mutex::scoped_lock lock(mutex_);
	if (closed_)
	{
		throw IllegalStateException("MembershipService closed.");
	}
	else
	{
		bool res = attributeManager_.isEmptyAttributeMap();
		Trace_Exit<bool>(this, "isEmptyAttributeMap()", res);
		return res;
	}
}

/*
 * @see spdr::MembershipService
 */
size_t MembershipServiceImpl::sizeOfAttributeMap()
{
	Trace_Entry(this, "sizeOfAttributeMap()");

	boost::mutex::scoped_lock lock(mutex_);
	if (closed_)
	{
		throw IllegalStateException("MembershipService closed.");
	}
	else
	{
		size_t res = attributeManager_.sizeOfAttributeMap();
		Trace_Exit<size_t>(this, "sizeOfAttributeMap()", res);
		return res;
	}
}

/*
 * @see spdr::MembershipService
 */
StringSet MembershipServiceImpl::getAttributeKeySet()
{
	Trace_Entry(this, "getAttributeKeySet()");

	boost::mutex::scoped_lock lock(mutex_);
	if (closed_)
	{
		throw IllegalStateException("MembershipService closed.");
	}
	else
	{
		StringSet set = attributeManager_.getAttributeKeySet();

		//Trace
		if (ScTraceBuffer::isExitEnabled(tc_))
		{
			std::ostringstream oss;
			oss << "[";
			size_t i=1;
			for (StringSet::const_iterator it = set.begin(); it != set.end(); ++it, ++i)
			{
				oss << (*it);
				if ( i < set.size())
					oss << ", ";
			}
			oss << "]";
		    ScTraceBufferAPtr buffer = ScTraceBuffer::exit(this, "getAttributeKeySet()", oss.str());
		    buffer->invoke();
		}

		return set;
	}
}

/*
 * @see spdr::MembershipService
 */
bool MembershipServiceImpl::containsAttribute(const String& key)
{
	Trace_Entry(this, "containsAttribute()", "key", key);

	boost::mutex::scoped_lock lock(mutex_);
	if (closed_)
	{
		throw IllegalStateException("MembershipService closed.");
	}
	else
	{
		bool res = attributeManager_.containsAttributeKey(key);
		Trace_Exit<bool>(this, "containsAttribute()",res);
		return res;
	}
}

bool MembershipServiceImpl::clearRemoteNodeRetainedAttributes(NodeID_SPtr target, int64_t incarnation)
{
	//TODO RetainAttr
	Trace_Entry(this, "clearRemoteNodeRetainedAttributes()", "target", spdr::toString(target), "inc", ScTraceBuffer::stringValueOf(incarnation));


	{
		boost::mutex::scoped_lock lock(mutex_);
		if (closed_)
		{
			throw IllegalStateException("MembershipService closed.");
		}
	}


	if (!target)
	{
		throw NullPointerException("NULL Target node");;
	}

	//takes the membership_mutex, hence outside of this->mutex_
	bool res = memMngr_SPtr_->clearRemoteNodeRetainedAttributes(target,incarnation);
	Trace_Exit<bool>(this, "clearRemoteNodeRetainedAttributes()",res);
	return res;
}

int64_t MembershipServiceImpl::getForeignZoneMembership(
	const String& zoneBusName, bool includeAttributes)
	throw(SpiderCastLogicError, SpiderCastRuntimeError)
{
	Trace_Entry(this, "getForeignZoneMembership()",
			"zoneBusName", zoneBusName,
			"includeAttributes", (includeAttributes?"true":"false"));

	BusName_SPtr busName(new BusName(zoneBusName.c_str())); // will throw IllegalArgumentException

	int64_t reqID = hierMngr_SPtr_->queueForeignZoneMembershipRequest(
			busName,includeAttributes, config_.getFZMRequestTimeoutMillis());

	Trace_Exit<int64_t>(this, "getForeignZoneMembership()", reqID);
	return reqID;
}

int64_t MembershipServiceImpl::getZoneCensus()
	throw(SpiderCastLogicError, SpiderCastRuntimeError)
{
	Trace_Entry(this, "getZoneCensus()");

	int64_t reqID = hierMngr_SPtr_->queueZoneCensusRequest();

	Trace_Exit<int64_t>(this, "getZoneCensus()", reqID);
	return (reqID);
}


//static
String MembershipServiceImpl::binBufferToString(Const_Buffer buffer)
{
	std::ostringstream oss;
	if (buffer.first < 0)
	{
		oss << "B=Missing";
	}
	else if (buffer.first == 0)
	{
		oss << "B=Empty";
	}
	else
	{
		oss << "B(" << buffer.first << ")=" << std::hex;
		const int lim = std::min(8*1024,buffer.first);
		for (int i=0; i<lim; ++i)
		{
			oss << ((unsigned int)(*(buffer.second+i)) & 0x000000FF);
			if (i<lim-1)
			{
				oss << ',';
			}
		}
		if (lim<buffer.first)
			oss << ",... Too long, truncated";
	}

	return oss.str();
}

}
