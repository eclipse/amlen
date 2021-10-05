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
 * MembershipTermination2Task.h
 *
 *  Created on: 11/04/2010
 */

#ifndef MEMBERSHIPTERMINATION2TASK_H_
#define MEMBERSHIPTERMINATION2TASK_H_

#include "AbstractTask.h"
#include "CoreInterface.h"
#include "MembershipManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class MembershipTermination2Task: public AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:

	MembershipManager_SPtr memMngr_SPtr;

	MembershipTermination2Task(CoreInterface& core);
	virtual ~MembershipTermination2Task();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	/*
	 * @see spdr::AbstractTask
	 */
	String toString() const;
};

}

#endif /* MEMBERSHIPTERMINATION2TASK_H_ */
