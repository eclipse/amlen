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
 * NeighborChangeTask.h
 *
 *  Created on: 18/05/2010
 */

#ifndef NEIGHBORCHANGETASK_H_
#define NEIGHBORCHANGETASK_H_

#include "AbstractTask.h"
//#include "ConcurrentSharedPtr.h"
#include "CoreInterface.h"
#include "MembershipManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class NeighborChangeTask: public spdr::AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;
	MembershipManager_SPtr memMngr_SPtr;

public:
	NeighborChangeTask(CoreInterface& core);
	virtual ~NeighborChangeTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString();
};

}

#endif /* NEIGHBORCHANGETASK_H_ */
