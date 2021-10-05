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
 * AttributeManager.cpp
 *
 *  Created on: 10/06/2010
 */

#include "AttributeManager.h"

namespace spdr
{

ScTraceComponent* AttributeManager::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"AttributeManager",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

AttributeManager::AttributeManager(
		const String& instID,
		const SpiderCastConfigImpl& config,
		MembershipViewMap& viewMap,
		NodeHistoryMap& historyMap,
		NodeIDImpl_SPtr myID,
		NodeVersion& myVersion,
		NodeIDCache& nodeIDCache,
		CoreInterface& coreInterface) :
	ScTraceContext(tc_, instID, myID->getNodeName()),
	instID_(instID),
	config_(config),
	viewMap_(viewMap),
	historyMap_(historyMap),
	myID_(myID),
	myVersion_(myVersion),
	nodeIDCache_(nodeIDCache),
	coreInterface_(coreInterface),
	notifyTaskScheduled(false),
	crcEnabled_(config.isCRCMemTopoMsgEnabled())
{
	Trace_Entry(this,"AttributeManager()");
}

AttributeManager::~AttributeManager()
{
	Trace_Entry(this,"~AttributeManager()");
}

bool AttributeManager::testAndSetNotifyTaskScheduled()
{
	Trace_Entry(this,"testAndSetNotifyTaskScheduled()");

	bool res = false;

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		if (notifyTaskScheduled)
		{
			res = false;
		}
		else
		{
			notifyTaskScheduled = true;
			res = true;
		}
	}

	Trace_Exit<bool>(this,"testAndSetNotifyTaskScheduled()", res);
	return res;
}

void AttributeManager::resetNotifyTaskScheduled()
{
	Trace_Entry(this,"resetNotifyTaskScheduled()");
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		notifyTaskScheduled = false;
	}
	Trace_Exit(this,"resetNotifyTaskScheduled()");
}

std::pair<SCViewMap_SPtr, event::ViewMap_SPtr >  AttributeManager::prepareDifferentialNotification(
			bool internal, bool external)
{
	Trace_Entry(this,"prepareDifferentialNotification()");

	SCViewMap_SPtr scview_sptr;
	event::ViewMap_SPtr view_sptr;

	if (internal)
	{
		scview_sptr.reset(new SCViewMap);
	}

	if (external)
	{
		view_sptr.reset(new event::ViewMap);
	}


	//collect notification from all other tables but mine
	for (MembershipViewMap::const_iterator it = viewMap_.begin(); it != viewMap_.end(); ++it )
	{
		//no need to check (*(it->first) != *myID_) because the bogus map never needs notify
		if (it->second.attributeTable->isNotifyNeeded())
		{
			if (scview_sptr)
			{
				event::AttributeMap_SPtr att_map_SPtr = it->second.attributeTable->getAttributeMap4Notification();
				event::MetaData_SPtr metadata_SPtr(new event::MetaData(
						att_map_SPtr, it->second.nodeVersion.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
				(*scview_sptr)[it->first] = metadata_SPtr;
			}

			if (view_sptr)
			{
				NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(it->first);
				event::AttributeMap_SPtr att_map_SPtr = it->second.attributeTable->getAttributeMap4Notification();
				event::MetaData_SPtr metadata_SPtr(new event::MetaData(
						att_map_SPtr, it->second.nodeVersion.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
				(*view_sptr)[id_SPtr] = metadata_SPtr;
			}

			it->second.attributeTable->markLastNotify();
		}
	}

	//RetainAtt
	if (config_.isRetainAttributesOnSuspectNodesEnabled())
	{
		for (NodeHistoryMap::const_iterator it = historyMap_.begin();
				it != historyMap_.end(); ++it)
		{
			if (it->second.attributeTable
					&& it->second.attributeTable->isNotifyNeeded())
			{
				if (scview_sptr)
				{
					event::AttributeMap_SPtr att_map_SPtr =
							it->second.attributeTable->getAttributeMap4Notification();
					event::MetaData_SPtr metadata_SPtr(
							new event::MetaData(att_map_SPtr,
									it->second.nodeVersion.getIncarnationNumber(),
									it->second.status));
					(*scview_sptr)[it->first] = metadata_SPtr;
				}

				if (view_sptr)
				{
					NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(
							it->first);
					event::AttributeMap_SPtr att_map_SPtr =
							it->second.attributeTable->getAttributeMap4Notification();
					event::MetaData_SPtr metadata_SPtr(
							new event::MetaData(att_map_SPtr,
									it->second.nodeVersion.getIncarnationNumber(),
									it->second.status));
					(*view_sptr)[id_SPtr] = metadata_SPtr;
				}

				it->second.attributeTable->markLastNotify();
			}
		}
	}

	//collect notification from my table
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);

		if (myAttributeTable_.isNotifyNeeded())
		{
			if (scview_sptr)
			{
				event::AttributeMap_SPtr att_map_SPtr = myAttributeTable_.getAttributeMap4Notification();
				event::MetaData_SPtr metadata_SPtr(new event::MetaData(
						att_map_SPtr, myVersion_.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
				(*scview_sptr)[myID_] = metadata_SPtr;
			}

			if (view_sptr)
			{
				NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(myID_);
				event::AttributeMap_SPtr att_map_SPtr = myAttributeTable_.getAttributeMap4Notification();
				event::MetaData_SPtr metadata_SPtr(new event::MetaData(
						att_map_SPtr, myVersion_.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
				(*view_sptr)[id_SPtr] = metadata_SPtr;
			}

			myAttributeTable_.markLastNotify();
		}

		notifyTaskScheduled = false;
	}

	Trace_Exit(this,"prepareDifferentialNotification()");
	return std::make_pair(scview_sptr, view_sptr);
}

std::pair<SCViewMap_SPtr, event::ViewMap_SPtr >  AttributeManager::prepareFullViewNotification(
			bool intView, bool intDiff, bool extView, bool extDiff)
{
	Trace_Entry(this,"prepareFullViewNotification()");

	if (!intView && !extView)
	{
		String what("AttributeManager::prepareFullViewNotification at least one full view must be set");
		throw IllegalArgumentException(what);
	}

	if ((intView && intDiff) || (extView && extDiff))
	{
		String what("AttributeManager::prepareFullViewNotification cannot provide both full view and diff view");
		throw IllegalArgumentException(what);
	}

	SCViewMap_SPtr scview_sptr;
	event::ViewMap_SPtr view_sptr;

	if (intView || intDiff)
	{
		scview_sptr.reset(new SCViewMap);
	}

	if (extView || extDiff)
	{
		view_sptr.reset(new event::ViewMap);
	}

	//collect notification from all other tables but mine
	for (MembershipViewMap::const_iterator it = viewMap_.begin(); it != viewMap_.end(); ++it )
	{
		//no need to check (*(it->first) != *myID_) because the bogus map never needs notify
		if (it->second.attributeTable->isNotifyNeeded())
		{
			if (scview_sptr && intDiff)
			{
				event::AttributeMap_SPtr att_map_SPtr = it->second.attributeTable->getAttributeMap4Notification();
				event::MetaData_SPtr metadata_SPtr(new event::MetaData(
						att_map_SPtr, it->second.nodeVersion.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
				(*scview_sptr)[it->first] = metadata_SPtr;
			}

			if (view_sptr && extDiff)
			{
				NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(it->first);
				event::AttributeMap_SPtr att_map_SPtr = it->second.attributeTable->getAttributeMap4Notification();
				event::MetaData_SPtr metadata_SPtr(new event::MetaData(
						att_map_SPtr, it->second.nodeVersion.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
				(*view_sptr)[id_SPtr] = metadata_SPtr;
			}

			it->second.attributeTable->markLastNotify();
		}

		if (scview_sptr && intView)
		{
			event::AttributeMap_SPtr att_map_SPtr = it->second.attributeTable->getAttributeMap4Notification();
			event::MetaData_SPtr metadata_SPtr(new event::MetaData(
					att_map_SPtr, it->second.nodeVersion.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
			(*scview_sptr)[it->first] = metadata_SPtr;
		}

		if (view_sptr && extView)
		{
			NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(it->first);
			event::AttributeMap_SPtr att_map_SPtr = it->second.attributeTable->getAttributeMap4Notification();
			event::MetaData_SPtr metadata_SPtr(new event::MetaData(
					att_map_SPtr, it->second.nodeVersion.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
			(*view_sptr)[id_SPtr] = metadata_SPtr;
		}
	}

	//RetainAtt
	if (config_.isRetainAttributesOnSuspectNodesEnabled())
	{
		for (NodeHistoryMap::const_iterator it = historyMap_.begin(); it != historyMap_.end(); ++it )
		{
			if (it->second.attributeTable) 
			{
				if (it->second.attributeTable->isNotifyNeeded())
				{
					if (scview_sptr && intDiff)
					{
						event::AttributeMap_SPtr att_map_SPtr =
								it->second.attributeTable->getAttributeMap4Notification();
						event::MetaData_SPtr metadata_SPtr(
								new event::MetaData(att_map_SPtr,
										it->second.nodeVersion.getIncarnationNumber(),
										it->second.status));
						(*scview_sptr)[it->first] = metadata_SPtr;
					}

					if (view_sptr && extDiff)
					{
						NodeID_SPtr id_SPtr =
								boost::static_pointer_cast<NodeID>(it->first);
						event::AttributeMap_SPtr att_map_SPtr =
								it->second.attributeTable->getAttributeMap4Notification();
						event::MetaData_SPtr metadata_SPtr(
								new event::MetaData(att_map_SPtr,
										it->second.nodeVersion.getIncarnationNumber(),
										it->second.status));
						(*view_sptr)[id_SPtr] = metadata_SPtr;
					}

					it->second.attributeTable->markLastNotify();
				}
			}

			if (scview_sptr && intView)
			{
				event::AttributeMap_SPtr att_map_SPtr;
				if (it->second.attributeTable)
				{
					att_map_SPtr = it->second.attributeTable->getAttributeMap4Notification();
				}
				event::MetaData_SPtr metadata_SPtr(
						new event::MetaData(att_map_SPtr,
								it->second.nodeVersion.getIncarnationNumber(),
								it->second.status));
				(*scview_sptr)[it->first] = metadata_SPtr;
			}

			if (view_sptr && extView)
			{
				NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(it->first);
				event::AttributeMap_SPtr att_map_SPtr;
				if (it->second.attributeTable)
				{
					att_map_SPtr = it->second.attributeTable->getAttributeMap4Notification();
				}
				event::MetaData_SPtr metadata_SPtr(
						new event::MetaData(att_map_SPtr,
								it->second.nodeVersion.getIncarnationNumber(),
								it->second.status));
				(*view_sptr)[id_SPtr] = metadata_SPtr;
			}
		}
	}

	//collect notification from my table
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);

		if (myAttributeTable_.isNotifyNeeded())
		{
			if (scview_sptr && intDiff)
			{
				event::AttributeMap_SPtr att_map_SPtr = myAttributeTable_.getAttributeMap4Notification();
				event::MetaData_SPtr metadata_SPtr(new event::MetaData(
						att_map_SPtr, myVersion_.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
				(*scview_sptr)[myID_] = metadata_SPtr;
			}

			if (view_sptr && extDiff)
			{
				NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(myID_);
				event::AttributeMap_SPtr att_map_SPtr = myAttributeTable_.getAttributeMap4Notification();
				event::MetaData_SPtr metadata_SPtr(new event::MetaData(
						att_map_SPtr, myVersion_.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
				(*view_sptr)[id_SPtr] = metadata_SPtr;
			}

			myAttributeTable_.markLastNotify();
		}

		if (scview_sptr && intView)
		{
			scview_sptr->erase(myID_); //bogus entry
			event::AttributeMap_SPtr att_map_SPtr = myAttributeTable_.getAttributeMap4Notification();
			event::MetaData_SPtr metadata_SPtr(new event::MetaData(
					att_map_SPtr, myVersion_.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
			(*scview_sptr)[myID_] = metadata_SPtr;
		}

		if (view_sptr && extView)
		{
			NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(myID_);
			view_sptr->erase(id_SPtr); //bogus entry
			event::AttributeMap_SPtr att_map_SPtr = myAttributeTable_.getAttributeMap4Notification();
			event::MetaData_SPtr metadata_SPtr(new event::MetaData(
					att_map_SPtr, myVersion_.getIncarnationNumber(), spdr::event::STATUS_ALIVE));
			(*view_sptr)[id_SPtr] = metadata_SPtr;
		}

		notifyTaskScheduled = false;
	}

	Trace_Exit(this,"prepareFullViewNotification()");
	return std::make_pair(scview_sptr, view_sptr);
}

event::AttributeMap_SPtr AttributeManager::getMyNotifyAttributeMap(bool internal)
{
	Trace_Entry(this,"getMyNotifyAttributeMap()");

	boost::recursive_mutex::scoped_lock lock(mutex_);

	event::AttributeMap_SPtr att_map_SPtr;
	if (!myAttributeTable_.isEmpty())
	{
		att_map_SPtr = myAttributeTable_.getAttributeMap4Notification();
		if (internal)
		{
			myAttributeTable_.markLastNotifyInternal();
		}
		else
		{
			myAttributeTable_.markLastNotify();
		}
	}

	notifyTaskScheduled = false;

	Trace_Debug(this, "getMyNotifyAttributeMap()", "",
			"map", (att_map_SPtr ? att_map_SPtr->toString(event::AttributeValue::BIN) : "null"));

	Trace_Exit(this,"getMyNotifyAttributeMap()");

	return att_map_SPtr;
}

bool AttributeManager::isUpdateNeeded()
{
	//Trace_Entry(this,"isUpdateNeeded()");

	//is the (current-version > sent-version) on any of the tables?
	bool update_needed = false;

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		update_needed = myAttributeTable_.isUpdateNeeded();
	}

	if (!update_needed)
	{
		for (MembershipViewMap::const_iterator it = viewMap_.begin();
				it != viewMap_.end() && !update_needed; ++it)
		{
			//checking the bogus (my) map is faster then checking every ID
			//the bogus map will never need an update
			if (it->second.attributeTable->isUpdateNeeded())
			{
				update_needed = true;
				break;
			}
		}
	}

	//RetainAtt
	if (config_.isRetainAttributesOnSuspectNodesEnabled() && !update_needed)
	{
		for (NodeHistoryMap::const_iterator it = historyMap_.begin();
				it != historyMap_.end() && !update_needed; ++it)
		{
			if (it->second.attributeTable && it->second.attributeTable->isUpdateNeeded())
			{
				update_needed = true;
				break;
			}
		}
	}

	if (update_needed)
	{
		Trace_Dump(this,"isUpdateNeeded()","true, updates pending");
	}

	//Trace_Exit<bool>(this,"isUpdateNeeded()", update_needed);
	return update_needed;
}

void AttributeManager::prepareFullUpdateMsg4Supervisor(SCMessage_SPtr msg)
{
	Trace_Entry(this,"prepareFullUpdateMsg4Supervisor()");

	//for every map
	//write name, incarnation number, attribute table, to the message.
	//do NOT skip tables with version 0

	ByteBuffer& buffer = *(msg->getBuffer());
	buffer.writeInt(viewMap_.size());

	std::ostringstream oss;

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);

		//if (myAttributeTable_.getVersion() > (uint64_t)0)
		{
			buffer.writeNodeID(myID_);
			buffer.writeLong(myVersion_.getIncarnationNumber());
			myAttributeTable_.writeMapEntriesToMessage(msg);

			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				oss << myID_->getNodeName() << ' ' << myVersion_.getIncarnationNumber() << ' '
						<< myAttributeTable_.size() << "; ";
			}
		}
	}

	for (MembershipViewMap::const_iterator it = viewMap_.begin();
			it != viewMap_.end(); ++it)
	{
		//checking that we don't write the bogus (my) map version:
		if (it->first->getNodeName() != myID_->getNodeName())
		{
			buffer.writeNodeID(it->first);
			buffer.writeLong(it->second.nodeVersion.getIncarnationNumber());
			it->second.attributeTable->writeMapEntriesToMessage(msg);

			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				oss << it->first->getNodeName() << ' ' << it->second.nodeVersion.getIncarnationNumber()
					<< ' ' << it->second.attributeTable->size() << "; ";
			}
		}
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this,"prepareFullUpdateMsg4Supervisor()");
		tb->addProperty<int> ("numItems", viewMap_.size());
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			tb->addProperty("update", oss.str());
		}
		tb->invoke();
	}

	Trace_Exit(this,"prepareFullUpdateMsg4Supervisor()");
}

/*
 * Prepare a full update message (for a new neighbor)
 */
bool AttributeManager::prepareFullUpdateMsg(SCMessage_SPtr msg)
{
	Trace_Entry(this,"prepareFullUpdateMsg()");

	//for every map
	//write name, node-version, table-version, to the message.
	//skip tables with version 0

	ByteBuffer& buffer = *(msg->getBuffer());
	msg->writeH1Header(SCMessage::Type_Mem_Metadata_Update);
	size_t pos_num_items = buffer.getPosition();
	int32_t num_items = 0;
	buffer.writeInt(num_items);

	std::ostringstream oss;

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);

		if (myAttributeTable_.getVersion() > (uint64_t)0)
		{
			buffer.writeString(myID_->getNodeName());
			msg->writeNodeVersion(myVersion_);
			buffer.writeSize_t(myAttributeTable_.getVersion());
			++num_items;

			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				oss << myID_->getNodeName() << ' ' << myVersion_.toString() << ' '
						<< myAttributeTable_.getVersion() << "; ";
			}
		}
	}

	for (MembershipViewMap::const_iterator it = viewMap_.begin();
			it != viewMap_.end(); ++it)
	{
		//checking that we don't write the bogus (my) map version:
		//No need to check the bogus (my) map because it always has version 0.
		if (it->second.attributeTable->getVersion() > (uint64_t)0)
		{
			buffer.writeString(it->first->getNodeName());
			msg->writeNodeVersion(it->second.nodeVersion);
			buffer.writeSize_t(it->second.attributeTable->getVersion());
			++num_items;

			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				oss << it->first->getNodeName() << ' ' << it->second.nodeVersion.toString()
						<< ' ' << it->second.attributeTable->getVersion() << "; ";
			}
		}
	}

	//RetainAtt
	if (config_.isRetainAttributesOnSuspectNodesEnabled())
	{
		for (NodeHistoryMap::const_iterator it = historyMap_.begin();
				it != historyMap_.end(); ++it)
		{
			if (it->second.attributeTable && it->second.attributeTable->getVersion() > (uint64_t) 0)
			{
				buffer.writeString(it->first->getNodeName());
				msg->writeNodeVersion(it->second.nodeVersion);
				buffer.writeSize_t(it->second.attributeTable->getVersion());
				++num_items;

				if (ScTraceBuffer::isDebugEnabled(tc_))
				{
					oss << it->first->getNodeName() << ' '
							<< it->second.nodeVersion.toString() << ' '
							<< it->second.attributeTable->getVersion() << "; ";
				}
			}
		}
	}

	size_t last_pos = buffer.getPosition();
	buffer.setPosition(pos_num_items);
	buffer.writeInt(num_items);
	buffer.setPosition(last_pos);

	msg->updateTotalLength();
	if (crcEnabled_)
	{
		msg->writeCRCchecksum();
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this,"prepareFullUpdateMsg()");
		tb->addProperty<int> ("numItems", num_items);
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			tb->addProperty("update", oss.str());
		}
		tb->invoke();
	}

	bool send = (num_items>0);

	Trace_Exit<bool>(this,"prepareFullUpdateMsg()", send);
	return send;
}

/*
 * Prepare a differential update message (for an old neighbor)
 * And return myAttributeTable version
 */
uint64_t AttributeManager::prepareDiffUpdateMsg(SCMessage_SPtr msg)
{
	Trace_Entry(this,"prepareDiffUpdateMsg()");

	//for every map with (current-version > sent-version)
	//write name, node-version, table-version, to the message.

	ByteBuffer& buffer = *(msg->getBuffer());
	msg->writeH1Header(SCMessage::Type_Mem_Metadata_Update);
	size_t pos_num_items = buffer.getPosition();
	int32_t num_items = 0;
	buffer.writeInt(num_items);

	uint64_t my_table_version = 0;

	std::ostringstream oss;

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		my_table_version = myAttributeTable_.getVersion();
		if (myAttributeTable_.isUpdateNeeded())
		{
			buffer.writeString(myID_->getNodeName());
			msg->writeNodeVersion(myVersion_);
			buffer.writeSize_t(myAttributeTable_.getVersion());
			//myAttributeTable_.markVersionSent(); //extracted to markVersionSent()
			++num_items;

			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				oss << myID_->getNodeName()	<< ' '
						<< myVersion_.toString() << ' '
						<< myAttributeTable_.getVersion() << "; ";
			}
		}
	}

	for (MembershipViewMap::const_iterator it = viewMap_.begin();
			it != viewMap_.end(); ++it)
	{
		//checking the bogus (my) map is faster then checking every ID
		//the bogus map will never need and update
		if (it->second.attributeTable->isUpdateNeeded())
		{
			buffer.writeString(it->first->getNodeName());
			msg->writeNodeVersion(it->second.nodeVersion);
			buffer.writeSize_t(it->second.attributeTable->getVersion());
			//it->second.attributeTable.markVersionSent(); //extracted to markVersionSent()
			++num_items;

			//Trace
			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				oss << it->first->getNodeName() << ' ' << it->second.nodeVersion.toString()
						<< ' ' << it->second.attributeTable->getVersion() << "; ";
			}
		}
	}

	//RetainAtt
	for (NodeHistoryMap::const_iterator it = historyMap_.begin();
			it != historyMap_.end(); ++it)
	{
		if (it->second.attributeTable && it->second.attributeTable->isUpdateNeeded())
		{
			buffer.writeString(it->first->getNodeName());
			msg->writeNodeVersion(it->second.nodeVersion);
			buffer.writeSize_t(it->second.attributeTable->getVersion());
			//it->second.attributeTable.markVersionSent(); //extracted to markVersionSent()
			++num_items;

			//Trace
			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				oss << it->first->getNodeName() << ' ' << it->second.nodeVersion.toString()
						<< ' ' << it->second.attributeTable->getVersion() << "; ";
			}
		}
	}

	size_t last_pos = buffer.getPosition();
	buffer.setPosition(pos_num_items);
	buffer.writeInt(num_items);
	buffer.setPosition(last_pos);
	msg->updateTotalLength();
	if (crcEnabled_)
	{
		msg->writeCRCchecksum();
	}

	//Trace
	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr tb = ScTraceBuffer::debug(this, "prepareDiffUpdateMsg()");
		tb->addProperty<int>("#Items",num_items);
		if (ScTraceBuffer::isDumpEnabled(tc_))
		{
			tb->addProperty("update", oss.str());
		}
		tb->invoke();
	}

	Trace_Exit<uint64_t>(this,"prepareDiffUpdateMsg()", my_table_version);
	return my_table_version;
}

void AttributeManager::resetMyVersionSent()
{
	Trace_Entry(this,"resetMyVersionSent()");
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);

		if (!myAttributeTable_.isUpdateNeeded())
		{
			myAttributeTable_.resetVersionSent();
			Trace_Event(this,"resetMyVersionSent()", "reset");
		}
		else
		{
			Trace_Event(this,"resetMyVersionSent()", "update needed, skipped");
		}
	}
	Trace_Exit(this,"resetMyVersionSent()");
}

void AttributeManager::writeMyRebuttalKey()
{
	Trace_Entry(this,"writeMyRebuttalKey()");
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);

		if (!myAttributeTable_.isUpdateNeeded())
		{
			myAttributeTable_.writeRebuttalKey();
			Trace_Event(this,"writeMyRebuttalKey()", "written");
		}
		else
		{
			Trace_Event(this,"writeMyRebuttalKey()", "update needed, skipped");
		}
	}
	Trace_Exit(this,"writeMyRebuttalKey()");
}

/* for every map (but mine) with (current-version > sent-version)
 * set sent-version = current-version
 *
 * for my map, set to my_table_version_sent
 */
void AttributeManager::markVersionSent(uint64_t my_table_version_sent)
{
	Trace_Entry(this,"markVersionSent()");

	int32_t num_items = 0;
	std::ostringstream oss;

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		if (myAttributeTable_.isUpdateNeeded())
		{
			myAttributeTable_.markVersionSent(my_table_version_sent);
			++num_items;

			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				oss << myID_->getNodeName()	<< ' ';
			}
		}
	}

	for (MembershipViewMap::const_iterator it = viewMap_.begin();
			it != viewMap_.end(); ++it)
	{
		//checking the bogus (my) map is faster then checking every ID
		//the bogus map will never need and update
		if (it->second.attributeTable->isUpdateNeeded())
		{
			it->second.attributeTable->markVersionSent();
			++num_items;

			//Trace
			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				oss << it->first->getNodeName() << ' ';
			}
		}
	}

	//RetainAtt

	for (NodeHistoryMap::const_iterator it = historyMap_.begin();
			it != historyMap_.end(); ++it)
	{
		if (it->second.attributeTable && it->second.attributeTable->isUpdateNeeded())
		{
			it->second.attributeTable->markVersionSent();
			++num_items;

			//Trace
			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				oss << it->first->getNodeName() << ' ';
			}
		}
	}

	//Trace
	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr tb = ScTraceBuffer::debug(this, "markVersionSent()");
		tb->addProperty<int>("#Items",num_items);
		if (ScTraceBuffer::isDumpEnabled(tc_))
		{
			tb->addProperty("marked-tables", oss.str());
		}
		tb->invoke();
	}

	Trace_Exit(this,"markVersionSent()");
}

bool AttributeManager::processIncomingUpdateMessage(SCMessage_SPtr inUpdate,
		SCMessage_SPtr outRequest)
{
	Trace_Entry(this, "processIncomingUpdateMessage()");

	ByteBuffer& inBuffer = *(inUpdate->getBuffer());
	int32_t num_digest_items_in = inBuffer.readInt();

	ByteBuffer& outBuffer = *(outRequest->getBuffer());
	outRequest->writeH1Header(SCMessage::Type_Mem_Metadata_Request);
	size_t pos_num_digest_items_out = outBuffer.getPosition();
	int32_t num_digest_items_out = 0;
	outBuffer.writeInt(num_digest_items_out);

	std::ostringstream oss_update;
	std::ostringstream oss_request;

	for (int32_t i=0; i<num_digest_items_in; ++i)
	{
		String name = inBuffer.readString();
		NodeVersion node_version = inUpdate->readNodeVersion();
		uint64_t table_version = inBuffer.readSize_t();

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			oss_update << (name) << ' ' << node_version.toString() << ' ' << table_version << "; ";
		}

		NodeIDImpl_SPtr id = nodeIDCache_.get(name);

		if (id && (*id != *myID_))
		{

			MembershipViewMap::const_iterator pos_view = viewMap_.find(id);
			if (pos_view != viewMap_.end())
			{
				//SplitBrain take into account sender's incarnation
				//check node-version - if not same incarnation number, skip and let membership do its thing
				if (pos_view->second.nodeVersion.getIncarnationNumber() == node_version.getIncarnationNumber())
				{
					if (pos_view->second.attributeTable->getVersion() < table_version &&
							pos_view->second.attributeTable->getVersionPending() < table_version)
					{
						//TRACE generate request
						outBuffer.writeString(pos_view->first->getNodeName());
						outRequest->writeNodeVersion(pos_view->second.nodeVersion);
						outBuffer.writeSize_t(pos_view->second.attributeTable->getVersion());

						//pos->second.attributeTable.markPending(table_version, id); //BUG 2010/08/17
						pos_view->second.attributeTable->markPending(table_version, inUpdate->getSender());
						++num_digest_items_out;

						if (ScTraceBuffer::isDebugEnabled(tc_))
						{
							oss_request << pos_view->first->getNodeName() << ' '
									<< pos_view->second.nodeVersion.toString() << ' '
									<< pos_view->second.attributeTable->getVersion() << "; ";
						}
					}
					else
					{
						//I am either updated, or expecting a better reply
						if (ScTraceBuffer::isDebugEnabled(tc_))
						{
							ScTraceBufferAPtr tb = ScTraceBuffer::debug(this,"processIncomingUpdateMessage()",
									"either updated, or expecting a better reply, ignoring");
							tb->addProperty("target", name);
							tb->addProperty<uint64_t>("table-ver", table_version);
							tb->addProperty<uint64_t>("local-table-ver", pos_view->second.attributeTable->getVersion());
							tb->addProperty<uint64_t>("pending-table-ver",pos_view->second.attributeTable->getVersionPending());
							tb->addProperty("pending-target",spdr::toString<NodeIDImpl>(pos_view->second.attributeTable->getPendingTarget()));
							tb->invoke();
						}
					}
				}
				else if (pos_view->second.nodeVersion.getIncarnationNumber() > node_version.getIncarnationNumber())
				{
					//I am more updated, ignore digest on target
					if (ScTraceBuffer::isDebugEnabled(tc_))
					{
						ScTraceBufferAPtr tb = ScTraceBuffer::debug(this,"processIncomingUpdateMessage()",
								"local incarnation number of target is more updated, ignoring");
						tb->addProperty("target", name);
						tb->addProperty("local-ver", pos_view->second.nodeVersion.toString());
						tb->addProperty("update-ver", node_version.toString());
						tb->invoke();
					}
				}
				else
				{
					//Error, Sender is more updated, which can't be because membership update is sent first
					std::ostringstream oss;
					oss		<< "SpiderCastRuntimeError: processing metadata update on target="
							<< id->getNodeName() << ", sender="
							<< inUpdate->getSender()->getNodeName()
							<< " target's incarnation number ="
							<< node_version.getIncarnationNumber()
							<< ", is bigger than local view version ="
							<< pos_view->second.nodeVersion.getIncarnationNumber();

					Trace_Error(this,"processIncomingUpdateMessage()", oss.str(),
							"update", oss_update.str(), "request", oss_request.str());

					throw SpiderCastRuntimeError(oss.str());
				}
			}

			//RetainAtt
			NodeHistoryMap::const_iterator pos_hist = historyMap_.end();
			if (config_.isRetainAttributesOnSuspectNodesEnabled())
			{ //history
				pos_hist = historyMap_.find(id);
				if (pos_hist != historyMap_.end())
				{
					//check node-version - if not same incarnation number, skip and let membership do its thing
					if (pos_hist->second.nodeVersion.getIncarnationNumber() == node_version.getIncarnationNumber())
					{
						if (pos_hist->second.attributeTable)
						{
							if (pos_hist->second.attributeTable->getVersion() < table_version &&
									pos_hist->second.attributeTable->getVersionPending() < table_version)
							{
								//TRACE generate request
								outBuffer.writeString(pos_hist->first->getNodeName());
								outRequest->writeNodeVersion(pos_hist->second.nodeVersion);
								outBuffer.writeSize_t(pos_hist->second.attributeTable->getVersion());

								//pos->second.attributeTable.markPending(table_version, id); //BUG 2010/08/17
								pos_hist->second.attributeTable->markPending(table_version, inUpdate->getSender());
								++num_digest_items_out;

								if (ScTraceBuffer::isDebugEnabled(tc_))
								{
									oss_request << pos_hist->first->getNodeName() << ' '
											<< pos_hist->second.nodeVersion.toString() << ' '
											<< pos_hist->second.attributeTable->getVersion() << "; ";
								}
							}
							else
							{
								//I am either updated, or expecting a better reply
								if (ScTraceBuffer::isDebugEnabled(tc_))
								{
									ScTraceBufferAPtr tb = ScTraceBuffer::debug(this,"processIncomingUpdateMessage()",
											"either updated, or expecting a better reply, ignoring");
									tb->addProperty("target", name);
									tb->addProperty<uint64_t>("table-ver", table_version);
									tb->addProperty<uint64_t>("local-table-ver", pos_hist->second.attributeTable->getVersion());
									tb->addProperty<uint64_t>("pending-table-ver",pos_hist->second.attributeTable->getVersionPending());
									tb->addProperty("pending-target",spdr::toString<NodeIDImpl>(pos_hist->second.attributeTable->getPendingTarget()));
									tb->invoke();
								}
							}
						}
						else
						{
							if (pos_hist->second.status == event::STATUS_REMOVE)
							{
								Trace_Debug(this,"processIncomingUpdateMessage()",
										"an update on a node with status=STATUS_REMOVE attribute, ignoring",
										"target", name,
										"ver-local", pos_hist->second.nodeVersion.toString(),
										"ver-in-msg", node_version.toString());
							}
							else
							{
								//Warning, NULL attributes, on a non-removed node, which can't be //RetainAtt Ignore?
								std::ostringstream oss;
								oss		<< "SpiderCastRuntimeError: history (retained) has NULL attribute map, with non-REMOVED status, processing metadata update on target="
										<< id->getNodeName() << ", sender="
										<< inUpdate->getSender()->getNodeName()
										<< " target's ver-in-msg="
										<< node_version.toString()
										<< ", local version ="
										<< pos_hist->second.nodeVersion.toString()
										<< ", status=" << pos_hist->second.status;

								Trace_Error(this, "processIncomingUpdateMessage()", oss.str(),
										"update", oss_update.str(),	"request", oss_request.str());
								throw SpiderCastRuntimeError(oss.str());
							}
						}
					}
					else if (pos_hist->second.nodeVersion.getIncarnationNumber() > node_version.getIncarnationNumber())
					{
						//I am more updated, ignore digest on target
						if (ScTraceBuffer::isDebugEnabled(tc_))
						{
							ScTraceBufferAPtr tb = ScTraceBuffer::debug(this,"processIncomingUpdateMessage()",
									"local incarnation number of target is more updated, ignoring");
							tb->addProperty("target", name);
							tb->addProperty("local-ver", pos_hist->second.nodeVersion.toString());
							tb->addProperty("update-ver", node_version.toString());
							tb->invoke();
						}
					}
					else
					{
						//Error, Sender is more updated, which can't be because membership update is sent first
						std::ostringstream oss;
						oss		<< "SpiderCastRuntimeError: processing metadata update on target="
								<< id->getNodeName() << ", sender="
								<< inUpdate->getSender()->getNodeName()
								<< " target's incarnation number="
								<< node_version.getIncarnationNumber()
								<< ", is bigger than local history version="
								<< pos_hist->second.nodeVersion.getIncarnationNumber();

						Trace_Error(this,"processIncomingUpdateMessage()", oss.str(),
								"update", oss_update.str(),
								"request", oss_request.str());

						throw SpiderCastRuntimeError(oss.str());
					}
				}
			}

			if (pos_view == viewMap_.end() && pos_hist == historyMap_.end())
			{
				//Error, This should not happen: NodeIDCache != (view U history).
				std::ostringstream oss;
				oss		<< "SpiderCastRuntimeError: metadata update target="
						<< id->getNodeName() << " is in NodeIDCache but not in ViewMap or HistoryMap, sender="
						<< inUpdate->getSender()->getNodeName();

				Trace_Error(this,"processIncomingUpdateMessage()", oss.str(),
						"update", oss_update.str(),
						"request", oss_request.str());

				throw SpiderCastRuntimeError(oss.str());
			}
		}
		else
		{
			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				ScTraceBufferAPtr tb = ScTraceBuffer::debug(this, "processIncomingUpdateMessage()",
						"Update target not in id-cache or is myself, ignoring");
				tb->addProperty("sender", inUpdate->getSender()->getNodeName());
				tb->addProperty("target", name);
				tb->addProperty("target-node-ver", node_version.toString());
				tb->addProperty<int64_t>("target-table-ver", table_version);
				tb->invoke();
			}
		}
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingUpdateMessage()", "Summary");
		tb->addProperty("sender", inUpdate->getSender()->getNodeName());

		tb->addProperty<int>("#digest-items-in", num_digest_items_in );
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			tb->addProperty("update", oss_update.str());
		}

		tb->addProperty<int>("#digest-items-out", num_digest_items_out );
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			tb->addProperty("request", oss_request.str());
		}

		tb->invoke();
	}

	bool res = (num_digest_items_out>0);

	if (res)
	{
		size_t last_pos = outBuffer.getPosition();
		outBuffer.setPosition(pos_num_digest_items_out);
		outBuffer.writeInt(num_digest_items_out);
		outBuffer.setPosition(last_pos);
		outRequest->updateTotalLength();
		if (crcEnabled_)
		{
			outRequest->writeCRCchecksum();
		}
	}

	Trace_Exit<bool>(this, "processIncomingUpdateMessage()", res);

	return res;
}

bool AttributeManager::processIncomingRequestMessage(SCMessage_SPtr inRequest,
		SCMessage_SPtr outReply, bool pushRequest)
{
	Trace_Entry(this, "processIncomingRequestMessage()");

	ByteBuffer& inBuffer = *(inRequest->getBuffer());
	int32_t num_digest_items_in = inBuffer.readInt();

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingRequestMessage()");
		buffer->addProperty("sender", toString<NodeIDImpl>(inRequest->getSender()));
		buffer->addProperty<int>("#items-in", num_digest_items_in);
		buffer->addProperty<bool>("push", pushRequest);
		buffer->invoke();
	}

	ByteBuffer& outBuffer = *(outReply->getBuffer());
	outReply->writeH1Header(SCMessage::Type_Mem_Metadata_Reply);
	size_t pos_num_items_out = outBuffer.getPosition();
	int32_t num_items_out = 0;
	outBuffer.writeInt(num_items_out);

	std::ostringstream oss_request;
	std::ostringstream oss_reply;

	for (int i=0; i<num_digest_items_in; ++i)
	{
		String name = inBuffer.readString();
		NodeVersion node_version = inRequest->readNodeVersion();
		uint64_t table_version = inBuffer.readSize_t();

		//Extended Trace
		if (ScTraceBuffer::isDumpEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this, "processIncomingRequestMessage()");
			buffer->addProperty<int>("req-item", i);
			buffer->addProperty("name",name);
			buffer->addProperty("N-ver", node_version.toString());
			buffer->addProperty<int>("T-ver", table_version);
			buffer->invoke();
		}

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			oss_request << (name) << ' ' << node_version.toString() << ' ' << table_version << "; ";
		}

		NodeIDImpl_SPtr id = nodeIDCache_.get(name);

		if (id)
		{
			if (*id != *myID_) //not me
			{
				MembershipViewMap::const_iterator pos_view = viewMap_.find(id);
				if (pos_view != viewMap_.end())
				{
					int32_t n_table_items = 0;
					bool invalidate = false;
					//SplitBrain take into account sender's incarnation
					//check node-version in request
					if (pos_view->second.nodeVersion.getIncarnationNumber() == node_version.getIncarnationNumber())
					{
						if (pos_view->second.attributeTable->getVersion() > table_version)
						{
							//append data to Reply
							outBuffer.writeString(pos_view->first->getNodeName());
							outReply->writeNodeVersion(pos_view->second.nodeVersion);
							n_table_items = pos_view->second.attributeTable->writeEntriesToMessage(table_version, outReply);
							++num_items_out;

							//Extended Trace
							if (ScTraceBuffer::isEntryEnabled(tc_))
							{
								ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingRequestMessage()",
										"on-other, normal");
								buffer->addProperty<int>("rep-item", num_items_out);
								buffer->addProperty("name", pos_view->first->getNodeName());
								buffer->addProperty("N-ver", pos_view->second.nodeVersion.toString());
								buffer->addProperty<uint64_t>("T-ver", pos_view->second.attributeTable->getVersion());
								buffer->addProperty("#table-items", n_table_items);
								buffer->invoke();
							}
						}
						else
						{
							invalidate = true;
							Trace_Event(this, "processIncomingRequestMessage()","Table version in request is higher than view, going to invalidate request",
									"target", id->getNodeName(),
									"T-ver-in-msg", boost::lexical_cast<string>(table_version),
									"T-ver-in-view", boost::lexical_cast<string>(pos_view->second.attributeTable->getVersion()));
						}
					}
					else
					{
						invalidate = true;
						Trace_Event(this, "processIncomingRequestMessage()","Incarnation in request is different than in view, going to invalidate request",
								"ver-in-msg", node_version.toString(),
								"ver-in-view", pos_view->second.nodeVersion.toString());
					}

					//===
					if (invalidate)
					{
						if (!pushRequest)
						{
							//He requested, so I had it, which means I lost it, or incarnation got updated between the update and the request
							//append request invalidate to Reply
							outBuffer.writeString(pos_view->first->getNodeName());
							outReply->writeNodeVersion(pos_view->second.nodeVersion);
							n_table_items = -1; //Invalidate request
							outReply->getBuffer()->writeInt(n_table_items);
							++num_items_out;

							//Extended Trace
							if (ScTraceBuffer::isDumpEnabled(tc_))
							{
								ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this, "processIncomingRequestMessage()",
										"on-other, request-invalidation");
								buffer->addProperty<int>("rep-item", num_items_out);
								buffer->addProperty("name", pos_view->first->getNodeName());
								buffer->addProperty("N-ver", pos_view->second.nodeVersion.toString());
								buffer->addProperty<uint64_t>("T-ver", pos_view->second.attributeTable->getVersion());
								buffer->addProperty("#table-items", n_table_items);
								buffer->invoke();
							}
						}
						else
						{
							//Extended Trace
							if (ScTraceBuffer::isEntryEnabled(tc_))
							{
								ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingRequestMessage()",
										"on-other, push, skipping request-invalidation");
								buffer->addProperty("name", pos_view->first->getNodeName());
								buffer->addProperty("N-ver", pos_view->second.nodeVersion.toString());
								buffer->addProperty<uint64_t>("T-ver", pos_view->second.attributeTable->getVersion());
								buffer->invoke();
							}
						}
					}

					//Trace
					if (ScTraceBuffer::isDebugEnabled(tc_))
					{
						if (n_table_items != 0) //0 means skipped
						{
							oss_reply << pos_view->first->getNodeName() << ' '
									<< pos_view->second.nodeVersion.toString() << ' '
									<< pos_view->second.attributeTable->getVersion() << ' '
									<< n_table_items << "; "; //the number of data items in reply
						}
					}
				}

				//RetainAtt
				NodeHistoryMap::const_iterator pos_hist = historyMap_.end();
				if (config_.isRetainAttributesOnSuspectNodesEnabled())
				{
					pos_hist = historyMap_.find(id);
					if (pos_hist != historyMap_.end())
					{
						int32_t n_table_items = 0;
						bool invalidate = false;
						 //check node-version in request
						if (pos_hist->second.nodeVersion.getIncarnationNumber() == node_version.getIncarnationNumber())
						{
							if (pos_hist->second.attributeTable && pos_hist->second.attributeTable->getVersion() > table_version)
							{
								//append data to Reply
								outBuffer.writeString(pos_hist->first->getNodeName());
								outReply->writeNodeVersion(pos_hist->second.nodeVersion);
								n_table_items = pos_hist->second.attributeTable->writeEntriesToMessage(table_version, outReply);
								++num_items_out;

								//Extended Trace
								if (ScTraceBuffer::isEntryEnabled(tc_))
								{
									ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingRequestMessage()",
											"on-other, normal");
									buffer->addProperty<int>("rep-item", num_items_out);
									buffer->addProperty("name", pos_hist->first->getNodeName());
									buffer->addProperty("N-ver", pos_hist->second.nodeVersion.toString());
									buffer->addProperty<uint64_t>("T-ver", pos_hist->second.attributeTable->getVersion());
									buffer->addProperty("#table-items", n_table_items);
									buffer->invoke();
								}
							}
							else
							{
								invalidate = true;
								Trace_Event(this, "processIncomingRequestMessage()",
										"Table version in request is higher than in history, going to invalidate request",
										"target", id->getNodeName(),
										"T-ver-in-msg", boost::lexical_cast<string>(table_version),
										"T-ver-in-hist",
										(pos_hist->second.attributeTable ? boost::lexical_cast<string>(pos_hist->second.attributeTable->getVersion()) : "NULL table"));
							}
						}
						else
						{
							invalidate = true;
							Trace_Event(this, "processIncomingRequestMessage()","Incarnation in request is different than in history, going to invalidate request",
									"ver-in-msg", node_version.toString(),
									"ver-in-hist", pos_hist->second.nodeVersion.toString(),
									"status", boost::lexical_cast<string>(pos_hist->second.status));
						}

						if (invalidate)
						{
							if (!pushRequest)
							{
								//He requested, so I had it, which means I lost it between the update and the request
								//append request invalidate to Reply
								outBuffer.writeString(pos_hist->first->getNodeName());
								outReply->writeNodeVersion(pos_hist->second.nodeVersion);
								n_table_items = -1; //Invalidate request
								outReply->getBuffer()->writeInt(n_table_items);
								++num_items_out;

								//Extended Trace
								if (ScTraceBuffer::isDumpEnabled(tc_))
								{
									ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this, "processIncomingRequestMessage()",
											"on-other, request-invalidation");
									buffer->addProperty<int>("rep-item", num_items_out);
									buffer->addProperty("name", pos_hist->first->getNodeName());
									buffer->addProperty("N-ver", pos_hist->second.nodeVersion.toString());

									if (pos_hist->second.attributeTable)
										buffer->addProperty<uint64_t>("T-ver", pos_hist->second.attributeTable->getVersion());
									else
										buffer->addProperty("T-ver", "NULL table");

									buffer->addProperty("#table-items", n_table_items);
									buffer->invoke();
								}
							}
							else
							{
								//Extended Trace
								if (ScTraceBuffer::isEntryEnabled(tc_))
								{
									ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingRequestMessage()",
											"on-other, push, skipping request-invalidation");
									buffer->addProperty("name", pos_hist->first->getNodeName());
									buffer->addProperty("N-ver", pos_hist->second.nodeVersion.toString());

									if (pos_hist->second.attributeTable)
										buffer->addProperty<uint64_t>("T-ver", pos_hist->second.attributeTable->getVersion());
									else
										buffer->addProperty("T-ver", "NULL table");

									buffer->invoke();
								}
							}
						}

						//Trace
						if (ScTraceBuffer::isDebugEnabled(tc_))
						{
							if (n_table_items != 0) //0 means skipped
							{
								oss_reply << pos_hist->first->getNodeName() << ' '
										<< pos_hist->second.nodeVersion.toString() << ' '
										<< (pos_hist->second.attributeTable ? boost::lexical_cast<string>(pos_hist->second.attributeTable->getVersion()) : "NULL-table")
										<< ' '
										<< n_table_items << "; "; //the number of data items in reply
							}
						}
					}
				}

				if (pos_view == viewMap_.end() && pos_hist == historyMap_.end())
				{
					//Error: this should not happen: NodeIDCache != view U history.
					std::ostringstream oss;
					oss		<< "SpiderCastRuntimeError: metadata request target="
							<< id->getNodeName() << " is in NodeIDCache but not in ViewMap or HistoryMap, sender="
							<< inRequest->getSender()->getNodeName();

					Trace_Error(this,"processIncomingRequestMessage()", oss.str(),
							"request", oss_request.str(),
							"reply", oss_reply.str());

					throw SpiderCastRuntimeError(oss.str());
				}
			}
			else
			{
				//My table

				boost::recursive_mutex::scoped_lock lock(mutex_);
				bool invalidate = false;
				//check node-version in request
				if (myVersion_.getIncarnationNumber() == node_version.getIncarnationNumber())
				{
					if (myAttributeTable_.getVersion() > table_version)
					{
						//append to Replay
						outBuffer.writeString(myID_->getNodeName());
						outReply->writeNodeVersion(myVersion_);
						int32_t n_table_items = myAttributeTable_.writeEntriesToMessage(table_version, outReply);
						++num_items_out;

						//Extended Trace
						if (ScTraceBuffer::isEntryEnabled(tc_))
						{
							ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingRequestMessage()","on-me");
							buffer->addProperty<int>("rep-item", num_items_out);
							buffer->addProperty("name",myID_->getNodeName());
							buffer->addProperty("N-ver", myVersion_.toString());
							buffer->addProperty<uint64_t>("T-ver", myAttributeTable_.getVersion());
							buffer->addProperty("#table-items", n_table_items);
							buffer->invoke();
						}

						//Trace
						if (ScTraceBuffer::isDebugEnabled(tc_))
						{
							oss_reply << myID_->getNodeName() << ' '
									<< myVersion_.toString() << ' '
									<< n_table_items << "; "; //the number of data items in reply
						}
					}
					else
					{
						invalidate = true;
						Trace_Event(this,"processIncomingRequestMessage()","On-me, Table version in request is lower than in view, going to invalidate",
								"T-ver-in-msg", boost::lexical_cast<string>(table_version),
								"T-ver-in-view", boost::lexical_cast<string>(myAttributeTable_.getVersion()));
					}
				}
				else
				{
					invalidate = true;
					Trace_Event(this,"processIncomingRequestMessage()","On-me, Incarnation in request is different than in view, going to invalidate"
							"ver-in-msg", node_version.toString(),
							"ver-in-view", myVersion_.toString());
				}

				if (invalidate)
				{
					Trace_Warning(this,"processIncomingRequestMessage()","Warning: This is weird, request-invalidation on my own attribute table!");

					if (!pushRequest)
					{
						//He requested, so I had it, which means I lost it, or incarnation got updated between the update and the request
						//append request invalidate to Reply
						outBuffer.writeString(myID_->getNodeName());
						outReply->writeNodeVersion(myVersion_);
						int num_table_items = -1; //Invalidate request
						outReply->getBuffer()->writeInt(num_table_items);
						++num_items_out;

						if (ScTraceBuffer::isEventEnabled(tc_))
						{
							ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "processIncomingRequestMessage()",
									"on-me, request-invalidation");
							buffer->addProperty<int>("rep-item", num_items_out);
							buffer->addProperty("name", myID_->getNodeName());
							buffer->addProperty("N-ver", myVersion_.toString());
							buffer->addProperty<uint64_t>("T-ver", myAttributeTable_.getVersion());
							buffer->addProperty("#table-items", num_table_items);
							buffer->invoke();
						}
					}
					else
					{
						if (ScTraceBuffer::isEventEnabled(tc_))
						{
							ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "processIncomingRequestMessage()",
									"on-me, push, skipping request-invalidation");
							buffer->addProperty("name", myID_->getNodeName());
							buffer->addProperty("N-ver", myVersion_.toString());
							buffer->addProperty<uint64_t>("T-ver", myAttributeTable_.getVersion());
							buffer->invoke();
						}
					}

				}
			}
		}
		else
		{
			//Request target not in view & not in retained history

			if (!pushRequest)
			{
				//He requested, so I had it, which means I lost it between the update and the request
				//append request invalidate to Reply
				outBuffer.writeString(name);
				outReply->writeNodeVersion(node_version);
				int32_t n_table_items = -1; //Invalidate request
				outReply->getBuffer()->writeInt(n_table_items);
				++num_items_out;

				if (ScTraceBuffer::isDebugEnabled(tc_))
				{
					ScTraceBufferAPtr tb = ScTraceBuffer::debug(this, "processIncomingRequestMessage()",
							"Request target not in id-cache, invalidate-request");
					tb->addProperty("sender", inRequest->getSender()->getNodeName());
					tb->addProperty("target", name);
					tb->addProperty("target-node-ver", node_version.toString());
					tb->addProperty<int64_t>("target-table-ver", table_version);
					tb->invoke();
				}
				//Trace
				if (ScTraceBuffer::isDebugEnabled(tc_))
				{
					oss_reply << name << ' '
							<< node_version.toString() << ' '
							<< table_version << ' '
							<< n_table_items << "; "; //the number of data items in reply
				}
			}
			else
			{
				if (ScTraceBuffer::isDebugEnabled(tc_))
				{
					ScTraceBufferAPtr tb = ScTraceBuffer::debug(this, "processIncomingRequestMessage()",
							"Request target not in id-cache, push, ignoring");
					tb->addProperty("sender", inRequest->getSender()->getNodeName());
					tb->addProperty("target", name);
					tb->addProperty("target-node-ver", node_version.toString());
					tb->addProperty<int64_t>("target-table-ver", table_version);
					tb->invoke();
				}
			}

		}
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingRequestMessage()");
		tb->addProperty("sender", inRequest->getSender()->getNodeName());
		tb->addProperty<int>("#digest-items-in", num_digest_items_in );
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			tb->addProperty("request", oss_request.str());
		}

		tb->addProperty<int>("#items-out", num_items_out );
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			tb->addProperty("reply", oss_reply.str());
		}

		tb->invoke();
	}

	bool res = (num_items_out > 0);
	if (res)
	{
		size_t last_pos = outBuffer.getPosition();
		outBuffer.setPosition(pos_num_items_out);
		outBuffer.writeInt(num_items_out);
		outBuffer.setPosition(last_pos);
		outReply->updateTotalLength();
		if (crcEnabled_)
		{
			outReply->writeCRCchecksum();
		}
	}

	Trace_Exit<bool>(this, "processIncomingRequestMessage()", res);
	return res;
}

std::pair<bool,bool> AttributeManager::processIncomingReplyMessage(
		SCMessage_SPtr inReply,
		SCMessage_SPtr outRequest)
{
	Trace_Entry(this, "processIncomingReplyMessage()");

	bool notify = false;
	bool request_push = false;

	ByteBuffer& inBuffer = *(inReply->getBuffer());
	int32_t num_tables = inBuffer.readInt();

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingReplyMessage()");
		tb->addProperty("sender", inReply->getSender()->getNodeName());
		tb->addProperty<int>("#tables", num_tables);
		tb->invoke();
	}

	//append new requests if there are request invalidations
	ByteBuffer& outBuffer = *(outRequest->getBuffer());
	outRequest->writeH1Header(SCMessage::Type_Mem_Metadata_RequestPush);
	size_t pos_num_digest_items_out = outBuffer.getPosition();
	int32_t num_digest_items_out = 0;
	outBuffer.writeInt(num_digest_items_out); //place-holder

	std::ostringstream oss_reply;
	std::ostringstream oss_request;

	for (int32_t i=0; i<num_tables; ++i)
	{
		const String name = inBuffer.readString();
		const NodeVersion node_version = inReply->readNodeVersion();

		//Extended Trace
		if (ScTraceBuffer::isDumpEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this, "processIncomingReplyMessage()");
			buffer->addProperty<int>("table", i);
			buffer->addProperty("name", name);
			buffer->addProperty("N-ver", node_version.toString());
			buffer->invoke();
		}

		NodeIDImpl_SPtr id = nodeIDCache_.get(name);

		if (id)
		{
			if (*id != *myID_)
			{
				MembershipViewMap::const_iterator pos_view = viewMap_.find(id);
				if (pos_view != viewMap_.end())
				{
					//check node-version
					//SplitBrain take into account sender's incarnation
					if (pos_view->second.nodeVersion.getIncarnationNumber() == node_version.getIncarnationNumber())
					{
						int32_t n_table_entries = pos_view->second.attributeTable->mergeEntriesFromMessage(inReply);
						bool table_notify = pos_view->second.attributeTable->isNotifyNeeded();
						notify = notify || table_notify;

						//Extended Trace
						if (ScTraceBuffer::isEntryEnabled(tc_))
						{
							ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingReplyMessage()");
							buffer->addProperty<int>("#table-entries", n_table_entries);
							buffer->addProperty<bool>("notify", table_notify);
							buffer->invoke();
						}

						if (ScTraceBuffer::isDebugEnabled(tc_))
						{
							oss_reply << (name) << ' ' << node_version.toString() << ' '
									<< n_table_entries
									<< (table_notify ? "Ntfy; " : "; " );
						}

						//if n_table_entries = (-1) then invalidate the pending request, push new request
						if (n_table_entries < 0)
						{
							//generate new request
							outBuffer.writeString(pos_view->first->getNodeName());
							outRequest->writeNodeVersion(pos_view->second.nodeVersion);
							outBuffer.writeSize_t(pos_view->second.attributeTable->getVersion());
							++num_digest_items_out;

							//clear the pending request memory
							pos_view->second.attributeTable->clearPending();

							if (ScTraceBuffer::isDebugEnabled(tc_))
							{
								oss_request << pos_view->first->getNodeName() << ' '
										<< pos_view->second.nodeVersion.toString() << ' '
										<< pos_view->second.attributeTable->getVersion() << "; ";
							}
						}
					}
					else if (pos_view->second.nodeVersion.getIncarnationNumber() > node_version.getIncarnationNumber())
					{
						Trace_Event(this, "processIncomingReplyMessage()", "Reply message refers to an older incarnation, ignoring and pushing new request",
								"N-ver in msg", node_version.toString(), "N-ver in view", pos_view->second.nodeVersion.toString());

						//skip the table
						int32_t num_skipped = AttributeTable::skipEntriesFromMessage(inReply);
						if (ScTraceBuffer::isEventEnabled(tc_))
						{
							ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingReplyMessage()", "skipped");
							tb->addProperty("num-entries", num_skipped);
							tb->invoke();
						}

						//generate new request
						outBuffer.writeString(pos_view->first->getNodeName());
						outRequest->writeNodeVersion(pos_view->second.nodeVersion);
						outBuffer.writeSize_t(pos_view->second.attributeTable->getVersion());
						++num_digest_items_out;

						//clear the pending request memory
						pos_view->second.attributeTable->clearPending();

						if (ScTraceBuffer::isDebugEnabled(tc_))
						{
							oss_request << pos_view->first->getNodeName() << ' '
									<< pos_view->second.nodeVersion.toString() << ' '
									<< pos_view->second.attributeTable->getVersion() << "; ";
						}
					}
					else
					{
						Trace_Event(this, "processIncomingReplyMessage()", "Reply message refers to an newer incarnation, ignoring and waiting for membership to catch up",
								"N-ver in msg", node_version.toString(), "N-ver in table", pos_view->second.nodeVersion.toString());
						//skip the table
						int32_t num_skipped = AttributeTable::skipEntriesFromMessage(inReply);
						if (ScTraceBuffer::isEventEnabled(tc_))
						{
							ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingReplyMessage()", "skipped");
							tb->addProperty("num-entries", num_skipped);
							tb->invoke();
						}
					}
				}

				//RetainAtt
				NodeHistoryMap::const_iterator pos_hist = historyMap_.end();
				if (config_.isRetainAttributesOnSuspectNodesEnabled())
				{
					pos_hist = historyMap_.find(id);
					if (pos_hist != historyMap_.end())
					{
						if (pos_hist->second.nodeVersion.getIncarnationNumber() == node_version.getIncarnationNumber() )
						{
							if (pos_hist->second.attributeTable)
							{
								int32_t n_table_entries = pos_hist->second.attributeTable->mergeEntriesFromMessage(inReply);
								bool table_notify = pos_hist->second.attributeTable->isNotifyNeeded();
								notify = notify || table_notify;

								//Extended Trace
								if (ScTraceBuffer::isEntryEnabled(tc_))
								{
									ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingReplyMessage()");
									buffer->addProperty<int>("#table-entries", n_table_entries);
									buffer->addProperty<bool>("notify", table_notify);
									buffer->invoke();
								}

								if (ScTraceBuffer::isDebugEnabled(tc_))
								{
									oss_reply << (name) << ' ' << node_version.toString() << ' '
											<< n_table_entries
											<< (table_notify ? "Ntfy; " : "; " );
								}

								//if n_table_entries = (-1) then invalidate the pending request, push new request
								if (n_table_entries < 0)
								{
									//generate new request
									outBuffer.writeString(pos_hist->first->getNodeName());
									outRequest->writeNodeVersion(pos_hist->second.nodeVersion);
									outBuffer.writeSize_t(pos_hist->second.attributeTable->getVersion());
									++num_digest_items_out;

									//clear the pending request memory
									pos_hist->second.attributeTable->clearPending();

									if (ScTraceBuffer::isDebugEnabled(tc_))
									{
										oss_request << pos_hist->first->getNodeName() << ' '
												<< pos_hist->second.nodeVersion.toString() << ' '
												<< pos_hist->second.attributeTable->getVersion() << "; ";
									}
								}
							}
							else
							{
								if (ScTraceBuffer::isEventEnabled(tc_))
								{
									ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingReplyMessage()",
											"Reply target not retained anymore, ignoring");
									tb->addProperty("sender", inReply->getSender()->getNodeName());
									tb->addProperty("target", name);
									tb->addProperty("target-node-ver", node_version.toString());
									tb->invoke();
								}
							}
						}
						else if (pos_hist->second.nodeVersion.getIncarnationNumber() > node_version.getIncarnationNumber())
						{
							Trace_Event(this, "processIncomingReplyMessage()", "Reply message refers to an older incarnation, ignoring and pushing new request",
									"N-ver in msg", node_version.toString(), "N-ver in history", pos_hist->second.nodeVersion.toString());
							//generate new request
							outBuffer.writeString(pos_hist->first->getNodeName());
							outRequest->writeNodeVersion(pos_hist->second.nodeVersion);
							outBuffer.writeSize_t(pos_hist->second.attributeTable->getVersion());
							++num_digest_items_out;

							//skip the table
							int32_t num_skipped = AttributeTable::skipEntriesFromMessage(inReply);
							if (ScTraceBuffer::isEventEnabled(tc_))
							{
								ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingReplyMessage()", "skipped");
								tb->addProperty("num-entries", num_skipped);
								tb->invoke();
							}

							//clear the pending request memory
							pos_hist->second.attributeTable->clearPending();

							if (ScTraceBuffer::isDebugEnabled(tc_))
							{
								oss_request << pos_hist->first->getNodeName() << ' '
										<< pos_hist->second.nodeVersion.toString() << ' '
										<< pos_hist->second.attributeTable->getVersion() << "; ";
							}
						}
						else
						{
							Trace_Event(this, "processIncomingReplyMessage()", "Reply message refers to an newer incarnation, ignoring and waiting for membership to catch up",
									"N-ver in msg", node_version.toString(), "N-ver in history", pos_hist->second.nodeVersion.toString());
							//skip the table
							int32_t num_skipped = AttributeTable::skipEntriesFromMessage(inReply);
							if (ScTraceBuffer::isEventEnabled(tc_))
							{
								ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingReplyMessage()", "skipped");
								tb->addProperty("num-entries", num_skipped);
								tb->invoke();
							}
						}
					}
				}

				if (pos_view == viewMap_.end() && pos_hist == historyMap_.end())
				{
					//Error, this should not happen: NodeIDCache != view U history. //RetainAtt - Ignore?
					std::ostringstream oss;
					oss		<< "SpiderCastRuntimeError: metadata reply target="
							<< id->getNodeName() << " is in NodeIDCache but not in ViewMap or HistoryMap retained, sender="
							<< inReply->getSender()->getNodeName();

					Trace_Error(this,"processIncomingReplyMessage()", oss.str(), "reply", oss_reply.str());

					throw SpiderCastRuntimeError(oss.str());
				}
			}
			else
			{
				//Error, should never happen: a data reply on myself from someone else
				std::ostringstream oss;
				oss		<< "SpiderCastRuntimeError: metadata reply on myself from someone else: target="
						<< id->getNodeName() << ", sender="
						<< inReply->getSender()->getNodeName();

				Trace_Error(this,"processIncomingReplyMessage()", oss.str(),
						"reply", oss_reply.str());

				throw SpiderCastRuntimeError(oss.str());
			}
		}
		else
		{
			if (ScTraceBuffer::isEventEnabled(tc_))
			{
				ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingReplyMessage()",
						"Reply target not in id-cache, skipping");
				tb->addProperty("sender", inReply->getSender()->getNodeName());
				tb->addProperty("target", name);
				tb->addProperty("target-node-ver", node_version.toString());
				tb->invoke();
			}
			int32_t num_skipped = AttributeTable::skipEntriesFromMessage(inReply);

			if (ScTraceBuffer::isEventEnabled(tc_))
			{
				ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingReplyMessage()",
						"skipped");
				tb->addProperty("num-entries", num_skipped);
				tb->invoke();
			}
		}
	}

	request_push = (num_digest_items_out>0);

	if (request_push)
	{
		size_t last_pos = outBuffer.getPosition();
		outBuffer.setPosition(pos_num_digest_items_out);
		outBuffer.writeInt(num_digest_items_out);
		outBuffer.setPosition(last_pos);
		outRequest->updateTotalLength();
		if (crcEnabled_)
		{
			outRequest->writeCRCchecksum();
		}
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "processIncomingReplyMessage()");
		tb->addProperty("sender", inReply->getSender()->getNodeName());
		tb->addProperty<int>("#tables", num_tables);
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			tb->addProperty("reply", oss_reply.str());
		}
		tb->addProperty<std::size_t>("notify", notify);
		tb->addProperty<std::size_t>("#invalidations", num_digest_items_out);
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			tb->addProperty("request", oss_request.str());
		}
		tb->invoke();
	}

	Trace_Exit(this, "processIncomingReplyMessage()");

	return std::make_pair(notify, request_push);
}

/**
 * Evaluate whether a disconnected node had pending requests,
 * build a request message with all its pending requests for retransmission to all
 * update the tables pending fields
 *
 * @param id
 * @param outRequest
 * @return
 */
bool AttributeManager::undoPendingRequests(NodeIDImpl_SPtr id, SCMessage_SPtr outRequest)
{
	Trace_Entry(this, "undoPendingRequests()", "id", id->getNodeName());

	ByteBuffer& outBuffer = *(outRequest->getBuffer());
	outRequest->writeH1Header(SCMessage::Type_Mem_Metadata_RequestPush);
	size_t pos_num_digest_items_out = outBuffer.getPosition();
	int32_t num_digest_items_out = 0;
	outBuffer.writeInt(num_digest_items_out); //place-holder

	std::ostringstream oss_request;

	for (MembershipViewMap::const_iterator it = viewMap_.begin();
			it != viewMap_.end(); ++it)
	{
		//checking the bogus (my) map is faster then checking every ID
		//the bogus map will never have a pending request
		if (it->second.attributeTable->isPending(id))
		{
			//generate request
			outBuffer.writeString(it->first->getNodeName());
			outRequest->writeNodeVersion(it->second.nodeVersion);
			outBuffer.writeSize_t(it->second.attributeTable->getVersion());
			//clear the pending request memory
			it->second.attributeTable->clearPending();
			++num_digest_items_out;

			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				oss_request << it->first->getNodeName() << ' '
						<< it->second.nodeVersion.toString() << ' '
						<< it->second.attributeTable->getVersion() << "; ";
			}
		}
	}

	//RetainAtt
	if (config_.isRetainAttributesOnSuspectNodesEnabled())
	{
		for (NodeHistoryMap::const_iterator it = historyMap_.begin();
				it != historyMap_.end(); ++it)
		{
			if (it->second.attributeTable && it->second.attributeTable->isPending(id))
			{
				//generate request
				outBuffer.writeString(it->first->getNodeName());
				outRequest->writeNodeVersion(it->second.nodeVersion);
				outBuffer.writeSize_t(it->second.attributeTable->getVersion());
				//clear the pending request memory
				it->second.attributeTable->clearPending();
				++num_digest_items_out;

				if (ScTraceBuffer::isDebugEnabled(tc_))
				{
					oss_request << it->first->getNodeName() << ' '
							<< it->second.nodeVersion.toString() << ' '
							<< it->second.attributeTable->getVersion() << "; ";
				}
			}
		}
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "undoPendingRequests()");
		tb->addProperty("id",id->getNodeName());
		tb->addProperty<int>("#digest-items-out", num_digest_items_out );

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			tb->addProperty("out-request", oss_request.str());
		}

		tb->invoke();
	}

	bool res = (num_digest_items_out>0);

	if (res)
	{
		size_t last_pos = outBuffer.getPosition();
		outBuffer.setPosition(pos_num_digest_items_out);
		outBuffer.writeInt(num_digest_items_out);
		outBuffer.setPosition(last_pos);
		outRequest->updateTotalLength();
		if (crcEnabled_)
		{
			outRequest->writeCRCchecksum();
		}
	}


	Trace_Exit<bool>(this, "undoPendingRequests()", res);
	return res;
}

//=== Attribute manipulation ===

bool AttributeManager::setAttribute(const String& key, std::pair<const int32_t,
		const char*> value)
{
	bool res = false;
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		res = myAttributeTable_.set(key, value);
	}
	coreInterface_.getMembershipManager()->scheduleChangeOfMetadataDeliveryTask();
	return res;
}

std::pair<event::AttributeValue,bool> AttributeManager::getAttribute(const String& key)
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	return myAttributeTable_.get(key);
}

bool AttributeManager::removeAttribute(const String& key)
{
	bool res = false;
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		res = myAttributeTable_.remove(key);
	}
	coreInterface_.getMembershipManager()->scheduleChangeOfMetadataDeliveryTask();
	return res;
}

bool AttributeManager::containsAttributeKey(const String& key)
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	return myAttributeTable_.contains(key);
}

void AttributeManager::clearAttributeMap()
{
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		myAttributeTable_.clear();
	}
	coreInterface_.getMembershipManager()->scheduleChangeOfMetadataDeliveryTask();
}

void AttributeManager::clearInternalAttributeMap()
{
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		myAttributeTable_.clearPrefix(internalKeyPrefix);
	}
	coreInterface_.getMembershipManager()->scheduleChangeOfMetadataDeliveryTask();
}

void AttributeManager::clearExternalAttributeMap()
{
	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		myAttributeTable_.clearNoPrefix(internalKeyPrefix);
	}
	coreInterface_.getMembershipManager()->scheduleChangeOfMetadataDeliveryTask();
}

bool AttributeManager::isEmptyAttributeMap()
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	return myAttributeTable_.isEmpty();
}

size_t AttributeManager::sizeOfAttributeMap()
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	return myAttributeTable_.size();
}

StringSet AttributeManager::getAttributeKeySet()
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	return myAttributeTable_.getKeySet();
}

void AttributeManager::reportStats(boost::posix_time::ptime time, bool labels)
{
	if (ScTraceBuffer::isConfigEnabled(tc_))
	{
		std::string time_str(boost::posix_time::to_iso_extended_string(time));

		std::ostringstream oss;
		oss << std::endl;
		if (labels)
		{
			oss << _instanceID << ", " << time_str << ", SC_Stats_AttrSum_Self, "
					<< "NumKeys, KeySizeByte, ValSizeByte, ValMaxByte, DCM-NumKeys, DCM-KeySizeByte, DCM-ValSizeByte" << std::endl;
			oss << _instanceID << ", " << time_str << ", SC_Stats_AttrSum_Remote, ViewSize, "
					<< "NumKeys, KeySizeByte, ValSizeByte, ValMaxByte, DCM-NumKeys, DCM-KeySizeByte, DCM-ValSizeByte" << std::endl;
			oss << _instanceID << ", " << time_str << ", SC_Stats_AttrSum_Hist, HistSize, "
					<< "NumKeys, KeySizeByte, ValSizeByte, ValMaxByte, DCM-NumKeys, DCM-KeySizeByte, DCM-ValSizeByte" << std::endl;
		}
		else
		{
			std::size_t array[7];
			std::size_t array1[7];

			{
				boost::recursive_mutex::scoped_lock lock(mutex_);
				myAttributeTable_.getSizeSummary(array);
			}

			oss << _instanceID << ", " << time_str << ", SC_Stats_AttrSum_Self, ";
			for (int i=0; i<6; ++i)
			{
				oss << array[i] << ", ";
			}
			oss << array[6] << std::endl;

			memset(array,0,sizeof(std::size_t[7]));
			for (MembershipViewMap::const_iterator it = viewMap_.begin();
					it != viewMap_.end(); ++it)
			{
				//checking the bogus (my) map is faster then checking every ID
				//the bogus map has zero counters
				if (it->second.attributeTable)
				{
					it->second.attributeTable->getSizeSummary(array1);
					for (int i=0; i<7; ++i)
					{
						if (i==3)
						{
							array[i] = std::max(array[i],array1[i]);
						}
						else
						{
							array[i] = array[i] + array1[i];
						}
					}
				}
			}

			oss << _instanceID << ", " << time_str << ", SC_Stats_AttrSum_Remote, " << (viewMap_.size()-1) << ", ";
			for (int i=0; i<6; ++i)
			{
				oss << array[i] << ", ";
			}
			oss << array[6] << std::endl;

			memset(array,0,sizeof(std::size_t[7]));
			for (NodeHistoryMap::const_iterator it = historyMap_.begin();
							it != historyMap_.end(); ++it)
			{
				//checking the bogus (my) map is faster then checking every ID
				//the bogus map has zero counters
				if (it->second.attributeTable)
				{
					it->second.attributeTable->getSizeSummary(array1);
					for (int i=0; i<7; ++i)
					{
						if (i==3)
						{
							array[i] = std::max(array[i],array1[i]);
						}
						else
						{
							array[i] = array[i] + array1[i];
						}
					}
				}
			}

			oss << _instanceID << ", " << time_str << ", SC_Stats_AttrSum_Hist, " << historyMap_.size() << ", ";
			for (int i=0; i<6; ++i)
			{
				oss << array[i] << ", ";
			}
			oss << array[6] << std::endl;
		}


		ScTraceBufferAPtr buffer = ScTraceBuffer::config(this, "reportStats()", oss.str());
		buffer->invoke();
	}
}

}
