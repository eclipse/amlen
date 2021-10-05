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
 * LEWarmupTask.h
 *
 *  Created on: Jun 19, 2012
 */

#ifndef LEWARMUPTASK_H_
#define LEWARMUPTASK_H_

#include "AbstractTask.h"
#include "LEViewKeeper.h"
#include "LECandidate.h"

#include "MembershipManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace leader_election
{

class LEWarmupTask : public AbstractTask, public ScTraceContext
{

private:
	static ScTraceComponent* tc_;

public:
	LEWarmupTask(
			const String& instID,
			LEViewKeeper_SPtr viewKeeper,
			LECandidate_SPtr candidate);

	virtual ~LEWarmupTask();


	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;

private:
	LEViewKeeper_SPtr viewKeeper_;
	LECandidate_SPtr candidate_;


};

}

}

#endif /* LEWARMUPTASK_H_ */
