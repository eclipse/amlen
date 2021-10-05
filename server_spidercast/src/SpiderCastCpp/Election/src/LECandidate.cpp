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
 * LECandidate.cpp
 *
 *  Created on: Jun 17, 2012
 */

#include "LECandidate.h"

namespace spdr
{

namespace leader_election
{

ScTraceComponent* LECandidate::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"LECandidate",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

LECandidate::LECandidate(
		const String& instID,
		const SpiderCastConfigImpl& config,
		MembershipManager_SPtr memMngr,
		TaskSchedule_SPtr taskSched,
		LeaderElectionListener& electionListener,
		bool candidate,
		const PropertyMap& properties) :
		ScTraceContext(tc_,instID,config.getMyNodeID()->getNodeName()),
		config_(config),
		memMngr_(memMngr),
		taskSched_(taskSched),
		electionListener_(electionListener),
		warmupTimeoutMillis_(0),
		mutex_(),
		closed_(false),
		candidate_(candidate),
		properties_(properties),
		myID_(),
		leader_(),
		warmup_(true)
{
	Trace_Entry(this,"LECandidate()");

	myID_ = boost::static_pointer_cast<NodeID>(config_.getMyNodeID());

	if (candidate_)
	{
		BasicConfig le_config(properties_);
		warmupTimeoutMillis_ = le_config.getOptionalInt32Property(
				config::LeaderElectionWarmupTimeoutMillis_PROP_KEY,
				config_.getLeaderElectionWarmupTimeoutMillis());
		writeElectionAttribute(false);
	}

	Trace_Exit(this,"LECandidate()");
}

int32_t LECandidate::getWarmupTimeoutMillis() const
{
	return warmupTimeoutMillis_;
}

LECandidate::~LECandidate()
{
	Trace_Entry(this,"~LECandidate()");
}

/*
 * @see LeaderElectionService
 */
NodeID_SPtr LECandidate::getLeader() const
{
	boost::mutex::scoped_lock lock(mutex_);
	return leader_;
}

/*
 * @see LeaderElectionService
 */
bool LECandidate::isLeader() const
{
	boost::mutex::scoped_lock lock(mutex_);
	if (leader_)
		return leader_->getNodeName() == config_.getNodeName();
	else
		return false;
}

/*
 * @see LeaderElectionService
 */
void LECandidate::close()
{
	Trace_Entry(this,"close()");

	bool destroy = false;
	{
		boost::mutex::scoped_lock lock(mutex_);
		if (!closed_)
		{
			destroy = true;
			closed_ = true;
		}
	}

	if (destroy)
	{
		if (candidate_)
		{
			memMngr_->getAttributeControl().removeAttribute(Election_Attribute_KEY);
		}
		memMngr_->destroyLeaderElectionService();
		memMngr_.reset();
		taskSched_.reset();
	}

	Trace_Exit(this,"close()");
}

bool LECandidate::isClosed() const
{
	boost::mutex::scoped_lock lock(mutex_);
	return closed_;
}

/*
 * @see LEViewListener
 */
void LECandidate::leaderViewChanged(
			const LeaderView& leaderView, const CandidateView& candidateView)
{
	Trace_Entry(this,"leaderViewChanged()");

	{
		boost::mutex::scoped_lock lock(mutex_);
		if (closed_)
		{
			Trace_Exit(this,"leaderViewChanged()", "closed");
			return;
		}
	}

	if (candidate_)
	{
		if (!warmup_)
		{
			Trace_Debug(this,"leaderViewChanged()","candidate");

			NodeID_SPtr bestCandidate;
			if (leaderView.empty() && !candidateView.empty())
			{
				Trace_Debug(this,"leaderViewChanged()","No leader");
				bestCandidate = candidateView.begin()->first;
				if (bestCandidate->getNodeName() == config_.getNodeName())
				{
					writeElectionAttribute(true);
					Trace_Debug(this,"leaderViewChanged()","Assume leader role");
				}
			}
			else if (leaderView.size() > 1 && leaderView.count(myID_) > 0)
			{
				Trace_Debug(this,"leaderViewChanged()","More than 1 leader, including me");

				bestCandidate = candidateView.begin()->first;
				if (bestCandidate->getNodeName() != config_.getNodeName())
				{
					//back-off
					writeElectionAttribute(false);
					Trace_Debug(this,"leaderViewChanged()","Back-off from leader role");
				}
			}

			observerLeaderViewChanged(leaderView);
		}
	}
	else
	{
		observerLeaderViewChanged(leaderView);
	}

	Trace_Exit(this,"leaderViewChanged()");
}


void LECandidate::observerLeaderViewChanged(const LeaderView& leaderView)
{
	Trace_Entry(this,"observerLeaderViewChanged()");

	bool notify = false;

	{
		boost::mutex::scoped_lock lock(mutex_);

		if (!leaderView.empty())
		{
			NodeID_SPtr new_leader = *(leaderView.begin()); //minimal ID
			if (!leader_ || leader_->getNodeName() != new_leader->getNodeName())
			{
				leader_ = new_leader;
				notify = true;
			}
		}
		else if (leader_)
		{
			leader_.reset();
			notify = true;
		}
	}

	//notify w/o the lock
	if (notify)
	{
		electionListener_.onLeaderChange(leader_);
	}

	Trace_Exit(this,"observerLeaderViewChanged()");
}

void LECandidate::writeElectionAttribute(bool leader)
{
	char buff[1] = {(char)(leader?1:0)};
	memMngr_->getAttributeControl().setAttribute(
			Election_Attribute_KEY,Const_Buffer(1,buff));
}

void LECandidate::warmupExpired()
{
	Trace_Entry(this,"warmupExpired()");
	warmup_ = false;
	Trace_Exit(this,"warmupExpired()");
}

}

}
