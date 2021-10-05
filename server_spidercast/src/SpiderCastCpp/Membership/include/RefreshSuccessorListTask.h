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
 * RefreshSuccessorListTask.h
 *
 *  Created on: Aug 4, 2010
 */

#ifndef REFRESHSUCCESSORLISTTASK_H_
#define REFRESHSUCCESSORLISTTASK_H_

#include "AbstractTask.h"
//#include "ConcurrentSharedPtr.h"
#include "CoreInterface.h"
#include "MembershipManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class RefreshSuccessorListTask: public AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;
	MembershipManager_SPtr memMngr_SPtr;

public:
	RefreshSuccessorListTask(CoreInterface& core);
	virtual ~RefreshSuccessorListTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;
};

}

#endif /* REFRESHSUCCESSORLISTTASK_H_ */
