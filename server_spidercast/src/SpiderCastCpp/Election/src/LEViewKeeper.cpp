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
 * LEViewKeeper.cpp
 *
 *  Created on: Jun 17, 2012
 */

#include "LEViewKeeper.h"

namespace spdr
{

namespace leader_election
{

ScTraceComponent* LEViewKeeper::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership, "LEViewKeeper",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

LEViewKeeper::LEViewKeeper(
		const String& instID,
		const SpiderCastConfigImpl& config) :
		ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
		mutex_(),
		candidateView_(),
		leaderView_(),
		service_()
{
	Trace_Entry(this, "LEViewKeeper()");
}

LEViewKeeper::~LEViewKeeper()
{
	Trace_Entry(this, "~LEViewKeeper()");
}

void LEViewKeeper::onMembershipEvent(const SCMembershipEvent& event)
{
	using namespace spdr::event;

	TRACE_ENTRY3(this, "onMembershipEvent()",
			"SCMembershipEvent", event.toString());

	bool leaderViewChanged = false;
	bool candidateViewChanged = false;

	switch (event.getType())
	{
	case SCMembershipEvent::View_Change:
	{
		//Assumes this is the first event, that comes only once
		candidateView_.clear();
		leaderView_.clear();

		SCViewMap_SPtr view = event.getView();
		for (SCViewMap::iterator it = view->begin(); it != view->end(); ++it)
		{
			//put every node with .election attribute in the candidateView_
			//put every node with .election=true attribute in the leaderView_
			if (it->second) //MetaData valid
			{
				AttributeMap_SPtr attr_map_SPtr = it->second->getAttributeMap();
				if (attr_map_SPtr)
				{
					AttributeMap::const_iterator pos = attr_map_SPtr->find(Election_Attribute_KEY);

					if (pos != attr_map_SPtr->end())
					{
						CandidateInfo info = parseElectionAttribute(pos->second);
						candidateView_.insert(std::make_pair(it->first,info));
						candidateViewChanged = true;
						if (info.leader)
						{
							leaderView_.insert(it->first);
							leaderViewChanged = true;
						}
					}
				}
			}
		}
	}
		break;

	case SCMembershipEvent::Node_Join:
	{
		if (event.getMetaData()) //MetaData valid
		{
			AttributeMap_SPtr attr_map_SPtr =
					event.getMetaData()->getAttributeMap();
			if (attr_map_SPtr)
			{
				AttributeMap::const_iterator pos = attr_map_SPtr->find(Election_Attribute_KEY);

				if (pos != attr_map_SPtr->end())
				{
					CandidateInfo info = parseElectionAttribute(pos->second);
					candidateView_.insert(std::make_pair(event.getNodeID(),info));
					candidateViewChanged = true;
					if (info.leader)
					{
						leaderView_.insert(event.getNodeID());
						leaderViewChanged = true;
					}
				}
			}
		}
	}
		break;

	case SCMembershipEvent::Node_Leave:
	{
		size_t num1 = candidateView_.erase(event.getNodeID());
		if (num1 > 0)
		{
			candidateViewChanged = true;
		}

		size_t num = leaderView_.erase(event.getNodeID());
		if (num > 0)
		{
			leaderViewChanged = true;
		}
	}
		break;

	case SCMembershipEvent::Change_of_Metadata:
	{
		SCViewMap_SPtr view = event.getView();
		for (SCViewMap::iterator it = view->begin(); it != view->end(); ++it)
		{
			if (it->second) //MetaData valid
			{
				AttributeMap_SPtr attr_map_SPtr = it->second->getAttributeMap();
				if (attr_map_SPtr)
				{
					AttributeMap::const_iterator pos = attr_map_SPtr->find(Election_Attribute_KEY);

					if (pos != attr_map_SPtr->end())
					{
						CandidateInfo info = parseElectionAttribute(pos->second);

						std::pair<CandidateView::iterator,bool> res =
								candidateView_.insert( std::make_pair(it->first, info) );

						if (res.second) //new candidate
						{
							candidateViewChanged = true;
							if (info.leader)
							{
								leaderView_.insert(it->first);
								leaderViewChanged = true;
							}
						}
						else //old candidate
						{
							size_t num = leaderView_.count(it->first);
							if (num > 0)
							{
								if (!info.leader)
								{
									leaderView_.erase(it->first);
									leaderViewChanged = true;
								}
							}
							else
							{
								if (info.leader)
								{
									leaderView_.insert(it->first);
									leaderViewChanged = true;
								}
							}
						}
					}
					else
					{
						if (candidateView_.erase(it->first) > 0)
						{
							candidateViewChanged = true;
						}
						if (leaderView_.erase(it->first) > 0)
						{
							leaderViewChanged = true;
						}
					}
				}
				else
				{
					if (candidateView_.erase(it->first) > 0)
					{
						candidateViewChanged = true;
					}
					if (leaderView_.erase(it->first) > 0)
					{
						leaderViewChanged = true;
					}
				}
			}
			else
			{
				if (candidateView_.erase(it->first) > 0)
				{
					candidateViewChanged = true;
				}
				if (leaderView_.erase(it->first) > 0)
				{
					leaderViewChanged = true;
				}
			}
		}
	}
		break;

	default:
	{
		String what("unexpected event type: ");
		what.append(event.toString());
		throw SpiderCastRuntimeError(what);
	}
	}

	if (candidateViewChanged)
	{
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "onMembershipEvent()", "Candidate view changed.");

			std::ostringstream cv;
			cv << "size=" << candidateView_.size() << ";";
			for (CandidateView::const_iterator cv_it = candidateView_.begin();
					cv_it != candidateView_.end(); ++cv_it)
			{
				cv << " " << cv_it->first->getNodeName();
			}

			buffer->addProperty("view", cv.str());
			buffer->invoke();
		}
	}

	if (leaderViewChanged)
	{
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "onMembershipEvent()", "Leader view changed.");

			std::ostringstream lv;
			lv << "Leader view: size=" << leaderView_.size() << ";";
			for (LeaderView::const_iterator lv_it = leaderView_.begin(); lv_it != leaderView_.end(); ++lv_it)
			{
				lv << " " << (*lv_it)->getNodeName() ;
			}
			buffer->addProperty("view", lv.str());
			buffer->invoke();
		}

		{
			boost::recursive_mutex::scoped_lock lock(mutex_);
			if (service_)
			{
				service_->leaderViewChanged(leaderView_, candidateView_);
			}
		}
	}

	Trace_Exit(this, "onMembershipEvent()");
}

CandidateInfo LEViewKeeper::parseElectionAttribute(
		const event::AttributeValue& attrValue)
{
	//expected format: {char} 0=false, 1=true.

	CandidateInfo info;

	if (attrValue.getLength() > 0)
	{
		info.leader = !(attrValue.getBuffer()[0] == 0);
	}
	else
	{
		throw SpiderCastRuntimeError("Missing value on Election_Attribute_KEY");
	}

	return info;
}

void LEViewKeeper::setService(LEViewListener_SPtr service)
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	service_ = service;
}

void LEViewKeeper::resetService()
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	service_.reset();
}

void LEViewKeeper::firstViewDelivery()
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	if (service_)
	{
		service_->leaderViewChanged(leaderView_, candidateView_);
	}
}

}

}
