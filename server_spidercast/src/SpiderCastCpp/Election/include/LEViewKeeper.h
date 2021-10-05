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
 * LEViewKeeper.h
 *
 *  Created on: Jun 17, 2012
 */

#ifndef LEVIEWKEEPER_H_
#define LEVIEWKEEPER_H_

#include <boost/thread/recursive_mutex.hpp>

#include "SCMembershipListener.h"
#include "SpiderCastConfigImpl.h"
#include "LEViewListener.h"
#include "LeaderElectionService.h"


#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace leader_election
{

/**
 * Keeps track of the candidate-view and the leader-view
 * and notifies the service when the leader-view changes.
 */
class LEViewKeeper : public SCMembershipListener, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	LEViewKeeper(
			const String& instID,
			const SpiderCastConfigImpl& config);

	virtual ~LEViewKeeper();

	void onMembershipEvent(const SCMembershipEvent& event);

	/**
	 * MemTopoThread only.
	 *
	 * @param service
	 */
	void setService(LEViewListener_SPtr service);

	/**
	 * MemTopoThread only.
	 */
	void resetService();

	/**
	 * MemTopoThread only.
	 */
	void firstViewDelivery();

private:
	boost::recursive_mutex mutex_;
	CandidateView candidateView_;
	LeaderView leaderView_;

	LEViewListener_SPtr service_;

	//bool firstViewDelivered_;

	CandidateInfo parseElectionAttribute(const event::AttributeValue& attrValue);

};

typedef boost::shared_ptr<LEViewKeeper>  LEViewKeeper_SPtr;

}

}

#endif /* LEVIEWKEEPER_H_ */
