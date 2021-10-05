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
 * LEViewListener.h
 *
 *  Created on: Jun 19, 2012
 */

#ifndef LEVIEWLISTENER_H_
#define LEVIEWLISTENER_H_

#include <map>
#include <set>
#include "NodeID.h"

namespace spdr
{

namespace leader_election
{

typedef struct {
	bool leader;
} CandidateInfo;

typedef std::map<NodeID_SPtr,CandidateInfo,spdr::SPtr_Less<NodeID> > CandidateView;
typedef std::set<NodeID_SPtr,spdr::SPtr_Less<NodeID> > LeaderView;

class LEViewListener
{
public:
	virtual ~LEViewListener();

	/**
	 * MemTopoThread only.
	 * @param leaderView
	 * @param candidateView
	 */
	virtual void leaderViewChanged(
			const LeaderView& leaderView, const CandidateView& candidateView) = 0;

protected:
	LEViewListener();
};

typedef boost::shared_ptr<LEViewListener> LEViewListener_SPtr;

}
}

#endif /* LEVIEWLISTENER_H_ */
