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
 * ChangeOfMetadataDeliveryTask.h
 *
 *  Created on: 15/06/2010
 */

#ifndef CHANGEOFMETADATADELIVERYTASK_H_
#define CHANGEOFMETADATADELIVERYTASK_H_

#include "AbstractTask.h"
#include "CoreInterface.h"
//#include "ConcurrentSharedPtr.h"
#include "MembershipManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class ChangeOfMetadataDeliveryTask: public AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;
	MembershipManager_SPtr memMngr_SPtr;

public:
	ChangeOfMetadataDeliveryTask(CoreInterface& core);
	virtual ~ChangeOfMetadataDeliveryTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;
};

}

#endif /* CHANGEOFMETADATADELIVERYTASK_H_ */
