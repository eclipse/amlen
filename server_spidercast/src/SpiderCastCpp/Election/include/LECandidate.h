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
 * LECandidate.h
 *
 *  Created on: Jun 17, 2012
 */

#ifndef LECANDIDATE_H_
#define LECANDIDATE_H_


#include "LeaderElectionService.h"
#include "SpiderCastConfigImpl.h"
#include "LeaderElectionListener.h"
#include "LEViewListener.h"
#include "MembershipManager.h"
#include "TaskSchedule.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace leader_election
{

class LECandidate : public LeaderElectionService, public LEViewListener, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	LECandidate(
			const String& instID,
			const SpiderCastConfigImpl& config,
			MembershipManager_SPtr memMngr,
			TaskSchedule_SPtr taskSched,
			LeaderElectionListener& electionListener,
			bool candidate,
			const PropertyMap& properties);

	virtual ~LECandidate();

	/*
	 * @see LeaderElectionService
	 */
	NodeID_SPtr getLeader() const;

	/*
	 * @see LeaderElectionService
	 */
	bool isLeader() const;

	/*
	 * @see LeaderElectionService
	 */
	bool isCandidate() const
	{
		return candidate_;
	}

	/*
	 * @see LeaderElectionService
	 */
	void close();

	/*
	 * @see LeaderElectionService
	 */
	bool isClosed() const;

	/*
	 * @see LEViewListener
	 * MemTopoThread only.
	 */
	void leaderViewChanged(
				const LeaderView& leaderView, const CandidateView& candidateView);

	/*
	 * MemTopoThread only.
	 */
	void warmupExpired();

	int32_t getWarmupTimeoutMillis() const;


private:
	const SpiderCastConfigImpl& config_;
	MembershipManager_SPtr memMngr_;
	TaskSchedule_SPtr taskSched_;
	LeaderElectionListener& electionListener_;
	int32_t warmupTimeoutMillis_;
	mutable boost::mutex mutex_;
	bool closed_;
	const bool candidate_;
	const PropertyMap properties_;
	NodeID_SPtr myID_;
	NodeID_SPtr leader_;
	bool warmup_;

	void observerLeaderViewChanged(const LeaderView& leaderView);
	void writeElectionAttribute(bool leader);

};

typedef boost::shared_ptr<LECandidate> LECandidate_SPtr;

}

}

#endif /* LECANDIDATE_H_ */
