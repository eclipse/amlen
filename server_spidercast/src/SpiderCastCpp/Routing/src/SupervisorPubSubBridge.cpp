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
 * SupervisorPubSubBridge.cpp
 *
 *  Created on: Jul 8, 2012
 */

#include "SupervisorPubSubBridge.h"

namespace spdr
{

namespace route
{

ScTraceComponent* SupervisorPubSubBridge::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Route,
		trace::ScTrConstants::Layer_ID_Route,
		"SupervisorPubSubBridge",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

SupervisorPubSubBridge::SupervisorPubSubBridge(
		const String& instID,
		const SpiderCastConfigImpl& config,
		boost::shared_ptr<PubSubViewKeeper> pubsubViewKeeper,
		AttributeControl& attributeControl) :
		ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
		config_(config),
		pubsubViewKeeper_(pubsubViewKeeper),
		attributeControl_(attributeControl),
		dBridgeState_table_()
{
	Trace_Entry(this, "SupervisorPubSubBridge()");
}

SupervisorPubSubBridge::~SupervisorPubSubBridge()
{
	Trace_Entry(this, "~SupervisorPubSubBridge()");
}

void SupervisorPubSubBridge::close()
{
	dBridgeState_table_.clear();
	bridgeSubsRefCount_.clear();
}


void SupervisorPubSubBridge::add_active(
		const String& busName,
		NodeIDImpl_SPtr id,
		Neighbor_SPtr neighbor)
{
	Trace_Entry(this, "add_active()", "bus", busName, "id", spdr::toString<NodeIDImpl>(id));

	DBridgeState state;
	state.delegate = id;
	state.neighbor = neighbor;

	std::pair<DBridgeStateTable::iterator, bool> res = dBridgeState_table_.insert(
			std::make_pair(busName,state));

	if (res.second)
	{
		Trace_Event(this,"add_active()","added new DBridgeState",
				"bus", busName, "id", spdr::toString<NodeIDImpl>(id));
	}
	else
	{
		Trace_Event(this,"add_active()","Error: DBridgeState already exists",
				"bus", busName,
				"id-old", spdr::toString<NodeIDImpl>(res.first->second.delegate),
				"id-new",  spdr::toString<NodeIDImpl>(id));
		std::ostringstream what;
		what << "Inconsistent DBridgeStateTable (add): " << busName
				<< ", delegate=" << res.first->second.delegate->toString()
				<< "; add id=" << id->getNodeName();
		throw SpiderCastRuntimeError(what.str());
	}

	Trace_Exit(this, "add_active()");
}


void SupervisorPubSubBridge::remove_active(
		BusName_SPtr bus,
		NodeIDImpl_SPtr id)
{
	Trace_Entry(this, "remove_active()", "bus", bus->toString(), "id", spdr::toString<NodeIDImpl>(id));

	DBridgeStateTable::iterator pos = dBridgeState_table_.find(bus->toString());
	if (pos != dBridgeState_table_.end())
	{
		if (*pos->second.delegate == *id)
		{
			StringSet empty;
			updateBridgeSubsAttributes(empty, pos->second.subscriptions); //add/remove sets
			dBridgeState_table_.erase(pos);
			Trace_Debug(this, "remove_active()", "removed");
		}
		else
		{
			Trace_Event(this, "remove_active()", "Error: Inconsistent DBridgeStateTable:",
					"bus", bus->toString(),
					"delegate-old", spdr::toString<NodeIDImpl>(pos->second.delegate),
					"delegate-new", spdr::toString<NodeIDImpl>(id));
			std::ostringstream what;
			what << "Inconsistent DBridgeStateTable (remove): "
					<< bus->toString() << ", delegate="
					<< pos->second.delegate->getNodeName() << "; remove id="
					<< id->getNodeName();
			throw SpiderCastRuntimeError(what.str());
		}
	}

	Trace_Exit(this, "remove_active()");
}

int SupervisorPubSubBridge::sendToActiveDelegates(
		SCMessage_SPtr msg,
		const int32_t tid,
		const SCMessage::H2Header& h2,
		const SCMessage::H1Header& h1,
		BusName_SPtr exclude_bus)
{

	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "sendToActiveDelegates()");
		buffer->addProperty("ex-bus",spdr::toString(exclude_bus));
		buffer->addProperty<int32_t>("tid", tid);
		buffer->invoke();
	}

	int count = 0;

	for (DBridgeStateTable::iterator it = dBridgeState_table_.begin();
			it != dBridgeState_table_.end(); ++it)
	{
		if (!exclude_bus ||  (exclude_bus && (it->first != exclude_bus->toString()) ) )
		{
			if (it->second.subscription_TIDs.count(tid) > 0)
			{
				if (it->second.neighbor->sendMessage(msg) == 0)
				{
					count++;
				}
			}
		}
	}

	Trace_Exit<int>(this, "sendToActiveDelegates()",count);
	return count;
}

void SupervisorPubSubBridge::processIncomingControlMessage(SCMessage_SPtr ctrlMsg)
{
	Trace_Entry(this, "processIncomingControlMessage()");

	SCMessage::H1Header h1 = (*ctrlMsg).readH1Header();
	NodeIDImpl_SPtr sender = ctrlMsg->getSender();
	BusName_SPtr bus = ctrlMsg->getBusName();

	Trace_Debug(this, "processIncomingControlMessage()", " ",
			"sender", spdr::toString<NodeIDImpl>(ctrlMsg->getSender()),
			"bus", spdr::toString(bus),
			"type", SCMessage::getMessageTypeName(h1.get<1>()));

	switch (h1.get<1>()) //message type
	{
	case SCMessage::Type_Hier_PubSubBridge_BaseZoneInterest:
	{
		std::ostringstream oss;

		ByteBuffer_SPtr bb = ctrlMsg->getBuffer();
		bb->skipString(); //source
		bb->skipString(); //target
		int32_t num_topics = bb->readInt();

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			oss << "Bus=" << spdr::toString(bus)
					<< "; BaseZoneInterest: #topics=" << num_topics<< "; ";
		}

		StringSet set;
		TopicID_HashSet hset;
		for (int32_t k=0; k < num_topics; ++k)
		{
			String topic = bb->readString();
			set.insert(topic);
			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				oss << topic << " ";
			}

			hset.insert(messaging::TopicImpl::hashName(topic));
		}

		Trace_Debug(this, "processIncomingControlMessage()", oss.str());

		DBridgeStateTable::iterator pos = dBridgeState_table_.find(bus->toString());
		if (pos != dBridgeState_table_.end())
		{
			//update bridge subscriptions - add the new ones, remove the old ones
			updateBridgeSubsAttributes(set,pos->second.subscriptions);
			pos->second.subscriptions.swap(set);
			pos->second.subscription_TIDs.swap(hset);
		}
		else
		{
			Trace_Debug(this, "processIncomingControlMessage()", "Bus not found in DBridgeStateTable, ignoring",
					"sender", spdr::toString<NodeIDImpl>(ctrlMsg->getSender()),
					"bus", spdr::toString(bus));
		}
	}
	break;

	default:
	{
		String what("Unexpected message type ");
		what.append(SCMessage::getMessageTypeName(h1.get<1>()));
		Trace_Exit(this,"processIncomingDataMessage()","SpiderCastRuntimeError",what);

		throw SpiderCastRuntimeError(what);
	}

	}

	Trace_Exit(this, "processIncomingControlMessage()");
}

void SupervisorPubSubBridge::updateBridgeSubsAttributes(
		const StringSet& add_set, const StringSet& remove_set)
{

	using namespace std;
	using namespace spdr::messaging;



	for (StringSet::const_iterator topic_it = add_set.begin();
			topic_it != add_set.end(); ++topic_it)
	{
		std::pair<BridgeSubsRefCountTable::iterator,bool> res_add = bridgeSubsRefCount_.insert(
				std::make_pair(*topic_it,1));
		if (res_add.second)
		{
			//add attribute, its a new one

			String topicKey(MessagingManager::topicKey_Prefix);
			topicKey.append(*topic_it);
			pair<event::AttributeValue,bool> res = attributeControl_.getAttribute(topicKey);

			char flags = 0x0;

			if (res.second)
			{
				if (res.first.getLength() > 0)
				{
					flags = (res.first.getBuffer())[0];
				}
				else
				{
					//Error, FAIL FAST
					String what("Error: SupervisorPubSubBridge::updateBridgeSubsAttributes() empty value on key ");
					what.append(topicKey);
					throw SpiderCastRuntimeError(what);
				}
			}

			flags = MessagingManager::addBridgeSub_Flags(flags);

			attributeControl_.setAttribute(
					topicKey, std::pair<int32_t,char*>(1,&flags));
		}
		else
		{
			res_add.first->second = res_add.first->second + 1;
		}
	}

	for (StringSet::const_iterator topic_it = remove_set.begin();
			topic_it != remove_set.end(); ++topic_it)
	{
		BridgeSubsRefCountTable::iterator pos = bridgeSubsRefCount_.find(*topic_it);
		if (pos != bridgeSubsRefCount_.end())
		{
			if (pos->second == 1)
			{
				bridgeSubsRefCount_.erase(pos);

				//remove attribute, last reference
				String topicKey(MessagingManager::topicKey_Prefix);
				topicKey.append(*topic_it);
				pair<event::AttributeValue,bool> res = attributeControl_.getAttribute(topicKey);

				if (res.second)
				{
					if (res.first.getLength() > 0)
					{
						char flags = (res.first.getBuffer())[0];
						flags = MessagingManager::removeBridgeSub_Flags(flags);

						if (flags > 0)
						{
							attributeControl_.setAttribute(
									topicKey,std::pair<int32_t,char*>(1,&flags));
						}
						else
						{
							attributeControl_.removeAttribute(topicKey);
						}
					}
					else
					{
						//Error, FAIL FAST
						String what("Error: SupervisorPubSubBridge::updateBridgeSubsAttributes() empty value on key ");
						what.append(topicKey);
						throw SpiderCastRuntimeError(what);
					}
				}
				else
				{
					//Error, FAIL FAST
					String what("Error: SupervisorPubSubBridge::updateBridgeSubsAttributes() missing value on key ");
					what.append(topicKey);
					throw SpiderCastRuntimeError(what);
				}
			}
			else
			{
				pos->second = pos->second - 1;
			}
		}
		else
		{
			//Error, FAIL FAST
			String what("Error: SupervisorPubSubBridge::updateBridgeSubsAttributes() Inconsistent  BridgeSubsRefCountTable, missing topic: ");
			what.append(*topic_it);
			throw SpiderCastRuntimeError(what);
		}

	}
}

}

}
