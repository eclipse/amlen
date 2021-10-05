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
 * PubSubViewKeeper.cpp
 *
 *  Created on: Feb 26, 2012
 */

#include "PubSubViewKeeper.h"

namespace spdr
{

namespace route
{

ScTraceComponent* PubSubViewKeeper::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Route,
		trace::ScTrConstants::Layer_ID_Route, "PubSubViewKeeper",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

PubSubViewKeeper::PubSubViewKeeper(
		const String& instID,
		const SpiderCastConfigImpl& config,
		VirtualIDCache& vidCache,
		PubSubViewListener& listener) :
		ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
		mutex_(),
		config_(config),
		vidCache_(vidCache),
		viewListener_(listener),
		myNodeID_(config.getMyNodeID()),
		myVID_(vidCache.get(myNodeID_->getNodeName()))
{
	Trace_Entry(this, "PubSubViewKeeper()");
}

PubSubViewKeeper::~PubSubViewKeeper()
{
	Trace_Entry(this, "~PubSubViewKeeper()");
}

void PubSubViewKeeper::onMembershipEvent(const SCMembershipEvent& event)
{
	using namespace event;
	using namespace std;

	TRACE_ENTRY3(this, "onMembershipEvent()", "SCMembershipEvent",
			event.toString());

	{
		boost::mutex::scoped_lock lock(mutex_);

		switch (event.getType())
		{
		case SCMembershipEvent::View_Change:
		{
			Trace_Debug(this, "onMembershipEvent()", "VC start");
			//Assumes this is the first event, that comes only once
			topicID2ClosestSubscriber_.clear();
			node2subscriptionList_.clear();
			topicID2NodeSet_.clear();

			globalPublicationMap_.clear();
			globalSubscriptionMap_.clear();
			globalSub_Node2Topic_.clear();
			globalPub_Node2Topic_.clear();

			SCViewMap_SPtr view = event.getView();
			for (SCViewMap::const_iterator it = view->begin(); it != view->end(); ++it)
			{
				//put every node with .sub.* attributes in the maps
				if (it->second) //MetaData valid
				{
					AttributeMap_SPtr attr_map_SPtr = it->second->getAttributeMap();
					if (attr_map_SPtr)
					{
						TopicID_Set sub_tidSet;
						StringSet gsub_tnSet;
						StringSet gpub_tnSet;
						StringSet bsub_tnSet;
						filter2TopicSet(attr_map_SPtr,sub_tidSet,gsub_tnSet,gpub_tnSet,bsub_tnSet);

						NodeIDImpl_SPtr node  = boost::static_pointer_cast<NodeIDImpl>(it->first);
						if (!sub_tidSet.empty())
						{
							node2subscriptionList_.insert(std::make_pair(node,sub_tidSet));
							for (TopicID_Set::const_iterator pos = sub_tidSet.begin(); pos != sub_tidSet.end(); ++pos)
							{
								addSubscription(node,*pos);
							}

							Trace_Debug(this, "onMembershipEvent()", "added topic set",
									"node",it->first->getNodeName());
						}

						//insert to global

						for (StringSet::const_iterator it_gsub = gsub_tnSet.begin();
								it_gsub != gsub_tnSet.end(); ++it_gsub)
						{
							addGlobalSub(*it_gsub, node);
						}

						for (StringSet::const_iterator it_gpub = gpub_tnSet.begin();
								it_gpub != gpub_tnSet.end(); ++it_gpub)
						{
							addGlobalPub(*it_gpub, node);
						}

						globalSub_Node2Topic_.insert(std::make_pair(node,gsub_tnSet));
						globalPub_Node2Topic_.insert(std::make_pair(node,gpub_tnSet));
					}
				}
			}
			Trace_Debug(this, "onMembershipEvent()", "VC end");
		}
		break;

		case SCMembershipEvent::Node_Join:
		{
			Trace_Debug(this, "onMembershipEvent()", "NJ start");
			if (event.getMetaData()) //MetaData valid
			{
				AttributeMap_SPtr attr_map_SPtr =
						event.getMetaData()->getAttributeMap();
				if (attr_map_SPtr)
				{
					TopicID_Set sub_tidSet;
					StringSet gsub_tnSet;
					StringSet gpub_tnSet;
					StringSet bsub_tnSet;
					filter2TopicSet(attr_map_SPtr,sub_tidSet,gsub_tnSet,gpub_tnSet,bsub_tnSet);
					NodeIDImpl_SPtr node  = boost::static_pointer_cast<NodeIDImpl>(event.getNodeID());

					if (!sub_tidSet.empty())
					{
						node2subscriptionList_.insert(std::make_pair(node,sub_tidSet));
						for (TopicID_Set::const_iterator pos = sub_tidSet.begin(); pos != sub_tidSet.end(); ++pos)
						{
							addSubscription(node,*pos);
						}

						Trace_Debug(this, "onMembershipEvent()", "added topic set", "node",event.getNodeID()->getNodeName());
					}

					//insert to global

					for (StringSet::const_iterator it_gsub = gsub_tnSet.begin();
							it_gsub != gsub_tnSet.end(); ++it_gsub)
					{
						addGlobalSub(*it_gsub, node);
					}

					for (StringSet::const_iterator it_gpub = gpub_tnSet.begin();
							it_gpub != gpub_tnSet.end(); ++it_gpub)
					{
						addGlobalPub(*it_gpub, node);
					}

					globalSub_Node2Topic_.insert(std::make_pair(node,gsub_tnSet));
					globalPub_Node2Topic_.insert(std::make_pair(node,gpub_tnSet));

				}
			}
			Trace_Debug(this, "onMembershipEvent()", "NJ end");
		}
		break;

		case SCMembershipEvent::Node_Leave:
		{
			Trace_Debug(this, "onMembershipEvent()", "NL start");
			NodeIDImpl_SPtr node  = boost::static_pointer_cast<NodeIDImpl>(event.getNodeID());

			NodeID2TopicIDSet_Map::iterator pos = node2subscriptionList_.find(node);
			if (pos != node2subscriptionList_.end())
			{
				TopicID_Set tidSet = pos->second;
				node2subscriptionList_.erase(pos);

				for (TopicID_Set::const_iterator pos1 = tidSet.begin(); pos1 != tidSet.end(); ++pos1)
				{
					removeSubscription(node,*pos1);
				}

				Trace_Debug(this, "onMembershipEvent()", "removed topic set");
			}

			NodeID2TopicNameSet_Map::iterator gpos = globalSub_Node2Topic_.find(node);
			if (gpos != globalSub_Node2Topic_.end())
			{
				StringSet tnSet = gpos->second;
				globalSub_Node2Topic_.erase(gpos);
				for (StringSet::const_iterator tn_it = tnSet.begin(); tn_it != tnSet.end(); ++tn_it)
				{
					removeGlobalSub(*tn_it,node);
				}
			}

			gpos = globalPub_Node2Topic_.find(node);
			if (gpos != globalPub_Node2Topic_.end())
			{
				StringSet tnSet = gpos->second;
				globalPub_Node2Topic_.erase(gpos);
				for (StringSet::const_iterator tn_it = tnSet.begin(); tn_it != tnSet.end(); ++tn_it)
				{
					removeGlobalPub(*tn_it,node);
				}
			}

			Trace_Debug(this, "onMembershipEvent()", "NL end");
		}
		break;

		case SCMembershipEvent::Change_of_Metadata:
		{
			Trace_Debug(this, "onMembershipEvent()", "CM start");

			TopicID_Set tidSet;
			vector<int32_t> tidSetAdded;
			vector<int32_t> tidSetRemoved;

			StringSet gsub_tnSet;
			vector<String> gsub_tnSet_add;
			vector<String> gsub_tnSet_rem;

			StringSet gpub_tnSet;
			vector<String> gpub_tnSet_add;
			vector<String> gpub_tnSet_rem;

			SCViewMap_SPtr view = event.getView();
			for (SCViewMap::iterator it = view->begin(); it != view->end(); ++it)
			{
				tidSet.clear();
				tidSetAdded.clear();
				tidSetRemoved.clear();

				gsub_tnSet.clear();
				gsub_tnSet_add.clear();
				gsub_tnSet_rem.clear();

				gpub_tnSet.clear();
				gpub_tnSet_add.clear();
				gpub_tnSet_rem.clear();

				bool noSubscriptions = false;
				bool no_gsub = false;
				bool no_gpub = false;

				NodeIDImpl_SPtr node  = boost::static_pointer_cast<NodeIDImpl>(it->first);

				if (it->second) //MetaData valid
				{
					AttributeMap_SPtr attr_map_SPtr = it->second->getAttributeMap();
					if (attr_map_SPtr)
					{
						StringSet bsub_tnSet;
						filter2TopicSet(attr_map_SPtr,tidSet,gsub_tnSet,gpub_tnSet,bsub_tnSet);

						if (!tidSet.empty())
						{
							Trace_Debug(this,"onMembershipEvent()", "new tid set", "node",node->getNodeName());

							NodeID2TopicIDSet_Map::const_iterator pos = node2subscriptionList_.find(node);

							if (pos != node2subscriptionList_.end())
							{
								tidSetAdded = setDiffTopicID(tidSet,pos->second);
								tidSetRemoved = setDiffTopicID(pos->second,tidSet);
								Trace_Debug(this,"onMembershipEvent()", "calculated diff tid-sets");
							}
							else
							{
								Trace_Debug(this,"onMembershipEvent()", "no old tid-set");
								tidSetAdded.assign(tidSet.begin(),tidSet.end());
							}

							Trace_Debug(this,"onMembershipEvent()", "insert tid-set");
							std::pair<NodeID2TopicIDSet_Map::iterator,bool> res = node2subscriptionList_.insert(
									std::make_pair(node,tidSet));
							if (!res.second)
							{
								res.first->second = tidSet;
							}

							if (!gsub_tnSet.empty())
							{
								Trace_Debug(this,"onMembershipEvent()", "new gsub-set", "node",node->getNodeName());
								NodeID2TopicNameSet_Map::iterator gpos = globalSub_Node2Topic_.find(node);

								if (gpos != globalSub_Node2Topic_.end())
								{
									gsub_tnSet_add = setDiffString(gsub_tnSet, gpos->second);
									gsub_tnSet_rem = setDiffString(gpos->second, gsub_tnSet);
									Trace_Debug(this,"onMembershipEvent()", "calculated diff gsub-sets");
									gpos->second = gsub_tnSet;
								}
								else
								{
									Trace_Debug(this,"onMembershipEvent()", "no old gsub-set");
									gsub_tnSet_add.assign(gsub_tnSet.begin(),gsub_tnSet.end());
									globalSub_Node2Topic_.insert(std::make_pair(node,gsub_tnSet));
								}


							}
							else
							{
								Trace_Debug(this,"onMembershipEvent()", "empty gsub-set","node",node->getNodeName());
								no_gsub = true;
							}
						}
						else
						{
							Trace_Debug(this,"onMembershipEvent()", "empty tid-set","node",node->getNodeName());
							noSubscriptions = true;
							no_gsub = true;
						}

						if (!gpub_tnSet.empty())
						{
							Trace_Debug(this,"onMembershipEvent()", "new gpub-set", "node",node->getNodeName());
							NodeID2TopicNameSet_Map::iterator gpos = globalPub_Node2Topic_.find(node);

							if (gpos != globalPub_Node2Topic_.end())
							{
								gpub_tnSet_add = setDiffString(gpub_tnSet, gpos->second);
								gpub_tnSet_rem = setDiffString(gpos->second, gpub_tnSet);
								Trace_Debug(this,"onMembershipEvent()", "calculated diff gpub-sets");
								gpos->second = gpub_tnSet;
							}
							else
							{
								Trace_Debug(this,"onMembershipEvent()", "no old gpub-set");
								gpub_tnSet_add.assign(gpub_tnSet.begin(),gpub_tnSet.end());
								globalPub_Node2Topic_.insert(std::make_pair(node,gpub_tnSet));
							}
						}
						else
						{
							Trace_Debug(this,"onMembershipEvent()", "empty gpub-set","node",node->getNodeName());
							no_gpub = true;
						}
					}
					else
					{
						Trace_Debug(this,"onMembershipEvent()", "no attributes","node",node->getNodeName());
						noSubscriptions = true;
						no_gsub = true;
						no_gpub = true;
					}
				}
				else
				{
					Trace_Debug(this,"onMembershipEvent()", "no metadata","node",node->getNodeName());
					noSubscriptions = true;
					no_gsub = true;
					no_gpub = true;
				}

				//all subscribers
				if (noSubscriptions)
				{
					NodeID2TopicIDSet_Map::const_iterator pos = node2subscriptionList_.find(node);
					if (pos != node2subscriptionList_.end())
					{
						tidSetRemoved.assign(pos->second.begin(),pos->second.end());
					}
					node2subscriptionList_.erase(node);
				}

				if (!tidSetRemoved.empty())
				{
					Trace_Debug(this, "onMembershipEvent()", "going to remove topics", "node",node->getNodeName());
					for (vector<int32_t>::const_iterator tid_it = tidSetRemoved.begin();
							tid_it != tidSetRemoved.end(); ++tid_it)
					{
						removeSubscription(node, *tid_it);
					}
				}

				if (!tidSetAdded.empty())
				{
					Trace_Debug(this, "onMembershipEvent()", "going to add topics", "node",node->getNodeName());
					for (vector<int32_t>::const_iterator tid_it = tidSetAdded.begin();
							tid_it != tidSetAdded.end(); ++tid_it)
					{
						addSubscription(node, *tid_it);
					}
				}

				//global subscribers
				if (no_gsub)
				{
					NodeID2TopicNameSet_Map::const_iterator pos = globalSub_Node2Topic_.find(node);
					if (pos != globalSub_Node2Topic_.end())
					{
						gsub_tnSet_rem.assign(pos->second.begin(),pos->second.end());
					}
					globalSub_Node2Topic_.erase(node);
				}

				if (!gsub_tnSet_rem.empty())
				{
					Trace_Debug(this, "onMembershipEvent()", "going to remove gsub topics",
							"node",node->getNodeName());
					for (vector<String>::const_iterator tn_it = gsub_tnSet_rem.begin();
							tn_it != gsub_tnSet_rem.end(); ++tn_it)
					{
						removeGlobalSub(*tn_it, node);
					}
				}

				if (!gsub_tnSet_add.empty())
				{
					Trace_Debug(this, "onMembershipEvent()", "going to add gsub topics", "node",node->getNodeName());
					for (vector<String>::const_iterator tn_it = gsub_tnSet_add.begin();
							tn_it != gsub_tnSet_add.end(); ++tn_it)
					{
						addGlobalSub(*tn_it, node);
					}
				}

				//global publishers
				if (no_gpub)
				{
					NodeID2TopicNameSet_Map::const_iterator pos = globalPub_Node2Topic_.find(node);
					if (pos != globalPub_Node2Topic_.end())
					{
						gpub_tnSet_rem.assign(pos->second.begin(),pos->second.end());
					}
					globalPub_Node2Topic_.erase(node);
				}

				if (!gpub_tnSet_rem.empty())
				{
					Trace_Debug(this, "onMembershipEvent()", "going to remove gpub topics",
							"node",node->getNodeName());
					for (vector<String>::const_iterator tn_it = gpub_tnSet_rem.begin();
							tn_it != gpub_tnSet_rem.end(); ++tn_it)
					{
						removeGlobalPub(*tn_it, node);
					}
				}

				if (!gpub_tnSet_add.empty())
				{
					Trace_Debug(this, "onMembershipEvent()", "going to add gpub topics", "node",node->getNodeName());
					for (vector<String>::const_iterator tn_it = gpub_tnSet_add.begin();
							tn_it != gpub_tnSet_add.end(); ++tn_it)
					{
						addGlobalPub(*tn_it, node);
					}
				}

			} //for node

			Trace_Debug(this, "onMembershipEvent()", "CM end");
		}
		break;

		default:
		{
			String what("unexpected event type: ");
			what.append(event.toString());
			throw SpiderCastRuntimeError(what);
		}
		} //switch
	} //lock

	Trace_Exit(this, "onMembershipEvent()");
}

std::pair<NodeIDImpl_SPtr, util::VirtualID_SPtr> PubSubViewKeeper::getClosestSubscriber(
		int32_t topicID) const
{
	Trace_Entry<int32_t>(this,"getClosestSubscriber()", "TopicID", topicID);

	std::pair<NodeIDImpl_SPtr, util::VirtualID_SPtr> res;

	{
		boost::mutex::scoped_lock lock(mutex_);

		TopicID2Subscriber_Map::const_iterator pos = topicID2ClosestSubscriber_.find(topicID);
		if (pos != topicID2ClosestSubscriber_.end())
		{
			res = pos->second;
		}
	}

	if (ScTraceBuffer::isExitEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::exit(this,"getClosestSubscriber()");
	    buffer->addProperty("node", spdr::toString<NodeIDImpl>(res.first));
	    buffer->addProperty("vid", spdr::toString<util::VirtualID>(res.second));
	    buffer->invoke();
	}

	return res;
}

PubSubViewKeeper::TopicID_Set PubSubViewKeeper::getNodeSubscriptions(NodeIDImpl_SPtr node) const
{
	TopicID_Set tset;

	{
		boost::mutex::scoped_lock lock(mutex_);

		NodeID2TopicIDSet_Map::const_iterator pos = node2subscriptionList_.find(node);
		if (pos != node2subscriptionList_.end())
		{
			tset = pos->second;
		}
	}

	return tset;
}

PubSubViewKeeper::NodeSet PubSubViewKeeper::getTopicSubscribers(int32_t topicID) const
{
	NodeSet nset;

	{
		boost::mutex::scoped_lock lock(mutex_);

		TopicID2NodeSet_Map::const_iterator pos = topicID2NodeSet_.find(topicID);
		if (pos != topicID2NodeSet_.end())
		{
			nset = pos->second;
		}
	}

	return nset;
}

StringSet PubSubViewKeeper::getGlobalTopicSubscriptions() const
{
	StringSet set;

	{
		boost::mutex::scoped_lock lock(mutex_);

		for (TopicName2NodeSet_Map::const_iterator it = globalSubscriptionMap_.begin();
				it != globalSubscriptionMap_.end(); ++it)
		{
			set.insert(it->first);
		}
	}

	return set;
}


StringSet PubSubViewKeeper::getGlobalTopicPublications() const
{
	StringSet set;
	{
		boost::mutex::scoped_lock lock(mutex_);

		for (TopicName2NodeSet_Map::const_iterator it = globalPublicationMap_.begin();
				it !=globalPublicationMap_.end(); ++it)
		{
			set.insert(it->first);
		}
	}

	return set;
}


void PubSubViewKeeper::filter2TopicSet(
			const event::AttributeMap_SPtr& eventMap,
			TopicID_Set& all_subscriptions,
			StringSet& global_subscriptions,
			StringSet& global_publishers,
			StringSet& bridge_subscriptions) const
{
	using namespace event;

	Trace_Entry(this,"filter2TopicSet()");

	std::ostringstream oss;

	all_subscriptions.clear();
	global_subscriptions.clear();
	global_publishers.clear();
	boost::hash<std::string> string_hasher;

	AttributeMap::const_iterator it;
	for (it = eventMap->begin(); it != eventMap->end(); ++it)
	{
		if (boost::starts_with(it->first,
				messaging::MessagingManager::topicKey_Prefix))
		{
			String topic_name = it->first.substr(
					messaging::MessagingManager::topicKey_Prefix.length(),
					it->first.length()-messaging::MessagingManager::topicKey_Prefix.length());
			int32_t topic_hash = (int32_t)string_hasher(topic_name);
			char flags = 0x0;
			if (it->second.getLength()>0)
			{
				flags = (it->second.getBuffer())[0];
			}
			else
			{
				//FAIL FAST
				String what("Error: filter2TopicSet() empty value for ");
				what.append(it->first);
				throw SpiderCastRuntimeError(what);
			}

			if (ScTraceBuffer::isEntryEnabled(tc_))
			{
				oss << topic_name << " " << std::hex << ((uint32_t)flags) << " "<< topic_hash << " " <<  "; ";
			}

			if ((flags & messaging::MessagingManager::topicFlags_SubscriberMask) > 0 )
			{
				all_subscriptions.insert(topic_hash);
				if ((flags & messaging::MessagingManager::topicFlags_GlobalMask) > 0)
				{
					global_subscriptions.insert(topic_name);
				}
			}

			if ((flags & messaging::MessagingManager::topicFlags_PublisherMask) > 0)
			{
				global_publishers.insert(topic_name);
			}

			char bridge_sub_flags = messaging::MessagingManager::topicFlags_SubscriberMask |
					messaging::MessagingManager::topicFlags_BridgeMask;
			if ((flags & bridge_sub_flags) == bridge_sub_flags)
			{
				bridge_subscriptions.insert(topic_name);
			}
		}
	}

	Trace_Exit(this,"filter2TopicSet()", "topic-set", oss.str());
}

void PubSubViewKeeper::addSubscription(NodeIDImpl_SPtr node, int32_t tid)
{
	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "addSubscription()");
		buffer->addProperty("node",node->getNodeName());
		buffer->addProperty<int32_t>("tid", tid);
		buffer->invoke();
	}
	using namespace util;

	TopicID2NodeSet_Map::iterator it = topicID2NodeSet_.find(tid);
	if (it != topicID2NodeSet_.end())
	{	//add to NodeSet
		Trace_Debug(this,"addSubscription","existing subscribers on topic");
		it->second.insert(node);
	}
	else
	{ 	//add a new NodeSet
		Trace_Debug(this,"addSubscription","first subscriber on topic");
		std::pair<TopicID2NodeSet_Map::iterator,bool> res = topicID2NodeSet_.insert(
				std::make_pair(tid,NodeSet()));
		res.first->second.insert(node);
	}

	Trace_Debug(this,"addSubscription","added to tid to node-set map");

	//find closest subscriber which is not me
	if (*node != *myNodeID_)
	{
		VirtualID_SPtr vid = vidCache_.get(node->getNodeName());
		TopicID2Subscriber_Map::iterator pos = topicID2ClosestSubscriber_.find(tid);
		if (pos != topicID2ClosestSubscriber_.end())
		{
			Trace_Debug(this,"addSubscription","comparing to closest subscriber on topic",
					"current", spdr::toString<NodeIDImpl>(pos->second.first),
					"current-vid", spdr::toString<util::VirtualID>(pos->second.second));
			util::VirtualID vid_diff1 = sub(*vid,*myVID_);
			util::VirtualID vid_diff2 = sub(*(pos->second.second),*myVID_);
			if (vid_diff1 < vid_diff2)
			{
				Trace_Debug(this,"addSubscription","found new");
				pos->second.first = node;
				pos->second.second = vid;
			}
		}
		else
		{
			Trace_Debug(this,"addSubscription","first closest subscriber on topic");
			topicID2ClosestSubscriber_[tid] = std::make_pair(node,vid);
		}
	}

	Trace_Exit(this, "addSubscription()");
}

void PubSubViewKeeper::removeSubscription(NodeIDImpl_SPtr node, int32_t tid)
{
	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "removeSubscription()");
		buffer->addProperty("node",node->getNodeName());
		buffer->addProperty<int32_t>("tid", tid);
		buffer->invoke();
	}

	using namespace util;

	TopicID2NodeSet_Map::iterator tid2set_it = topicID2NodeSet_.find(tid);
	if (tid2set_it != topicID2NodeSet_.end())
	{
		tid2set_it->second.erase(node); //The node set
		if (tid2set_it->second.empty())
		{
			topicID2NodeSet_.erase(tid);
			topicID2ClosestSubscriber_.erase(tid);
		}
		else
		{
			TopicID2Subscriber_Map::iterator tid2node_it = topicID2ClosestSubscriber_.find(tid);
			if (*(tid2node_it->second.first) == *node)
			{
				tid2node_it->second.first.reset();
				tid2node_it->second.second.reset();

				//update
				for (NodeSet::const_iterator node_it = tid2set_it->second.begin();
						node_it != tid2set_it->second.end(); ++node_it)
				{
					if ( *(*node_it) != *myNodeID_ )
					{
						VirtualID_SPtr vid = vidCache_.get( (*node_it)->getNodeName() );
						if (tid2node_it->second.second)
						{
							util::VirtualID vid_diff1 = sub(*vid,*myVID_);
							util::VirtualID vid_diff2 = sub(*(tid2node_it->second.second),*myVID_);
							if (vid_diff1 < vid_diff2)
							{
								tid2node_it->second.first = *node_it;
								tid2node_it->second.second = vid;
							}
						}
						else
						{
							tid2node_it->second.first = *node_it;
							tid2node_it->second.second = vid;
						}
					}
				}

				if (!tid2node_it->second.first)
				{
					topicID2ClosestSubscriber_.erase(tid);
				}
			}
		}
	}

	Trace_Exit(this, "removeSubscription()");
}

void PubSubViewKeeper::addGlobalSub(String topic_name, NodeIDImpl_SPtr node)
{
	Trace_Entry(this, "addGlobalSub()", "topic", topic_name, "node", node->getNodeName());

	TopicName2NodeSet_Map::iterator pos = globalSubscriptionMap_.find(topic_name);
	if (pos != globalSubscriptionMap_.end())
	{
		pos->second.insert(node);
	}
	else
	{
		NodeSet set;
		set.insert(node);
		std::pair<TopicName2NodeSet_Map::iterator,bool> res = globalSubscriptionMap_.insert(
				std::make_pair(topic_name,set));
		if (res.second)
		{
			viewListener_.globalSub_add(topic_name);
			Trace_Debug(this, "addGlobalSub()","changed");
		}
		else
		{
			throw SpiderCastRuntimeError("PubSubViewKeeper::addGlobalSub() set insert failed");
		}
	}

	Trace_Exit(this, "addGlobalSub()");
}

void PubSubViewKeeper::removeGlobalSub(String topic_name, NodeIDImpl_SPtr node)
{
	Trace_Entry(this, "removeGlobalSub()", "topic", topic_name, "node", node->getNodeName());

	TopicName2NodeSet_Map::iterator pos = globalSubscriptionMap_.find(topic_name);
	if (pos != globalSubscriptionMap_.end())
	{
		pos->second.erase(node);
		if (pos->second.empty())
		{
			globalSubscriptionMap_.erase(pos);
			viewListener_.globalSub_remove(topic_name);
			Trace_Debug(this, "removeGlobalSub()","changed");
		}
	}

	Trace_Exit(this, "addGlobalSub()");
}

void PubSubViewKeeper::addGlobalPub(String topic_name, NodeIDImpl_SPtr node)
{
	Trace_Entry(this, "addGlobalPub()", "topic", topic_name, "node", node->getNodeName());

	TopicName2NodeSet_Map::iterator pos = globalPublicationMap_.find(topic_name);
	if (pos != globalPublicationMap_.end())
	{
		pos->second.insert(node);
	}
	else
	{
		NodeSet set;
		set.insert(node);
		std::pair<TopicName2NodeSet_Map::iterator,bool> res = globalPublicationMap_.insert(
				std::make_pair(topic_name,set));
		if (res.second)
		{
			viewListener_.globalPub_add(topic_name);
			Trace_Debug(this, "addGlobalPub()","changed");
		}
		else
		{
			throw SpiderCastRuntimeError("PubSubViewKeeper::addGlobalPub() set insert failed");
		}
	}

	Trace_Exit(this, "addGlobalPub()");
}

void PubSubViewKeeper::removeGlobalPub(String topic_name, NodeIDImpl_SPtr node)
{
	Trace_Entry(this, "removeGlobalPub()", "topic", topic_name, "node", node->getNodeName());

	TopicName2NodeSet_Map::iterator pos = globalPublicationMap_.find(topic_name);
	if (pos != globalPublicationMap_.end())
	{
		pos->second.erase(node);
		if (pos->second.empty())
		{
			globalPublicationMap_.erase(pos);
			viewListener_.globalPub_remove(topic_name);
			Trace_Debug(this, "removeGlobalPub()","changed");
		}
	}

	Trace_Exit(this, "addGlobalPub()");
}

//free function
std::vector<int32_t> setDiffTopicID(
		const PubSubViewKeeper::TopicID_Set& a, const PubSubViewKeeper::TopicID_Set& b)
{
	using namespace std;

	vector<int32_t> set_diff;
	set_diff.assign(a.size(),0);

	vector<int32_t>::iterator lastAdd = std::set_difference(
			a.begin(), a.end(),
			b.begin(), b.end(), set_diff.begin());
	set_diff.erase(lastAdd, set_diff.end());

	return set_diff;
}

//free function
std::vector<String > setDiffString(
		const StringSet& a, const StringSet& b)
{
	using namespace std;

	vector<String > set_diff;
	set_diff.assign(a.size(),"");

	vector<String >::iterator lastAdd = std::set_difference(
			a.begin(), a.end(),
			b.begin(), b.end(), set_diff.begin());
	set_diff.erase(lastAdd, set_diff.end());

	return set_diff;
}

}

}
