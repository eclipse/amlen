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
 * HierarchyPeriodicTask.h
 *
 *  Created on: Oct 5, 2010
 */

#ifndef HIERARCHYPERIODICTASK_H_
#define HIERARCHYPERIODICTASK_H_

#include "CoreInterface.h"
#include "HierarchyManager.h"
#include "AbstractTask.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class HierarchyPeriodicTask: public AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;
	HierarchyManager_SPtr hierMngr_SPtr;

public:
	HierarchyPeriodicTask(CoreInterface& core);
	virtual ~HierarchyPeriodicTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;
};

}

#endif /* HIERARCHYPERIODICTASK_H_ */
