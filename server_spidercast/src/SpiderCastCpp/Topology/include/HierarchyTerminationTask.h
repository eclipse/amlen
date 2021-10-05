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
 * HierarchyTerminationTask.h
 *
 *  Created on: Oct 21, 2010
 */

#ifndef HIERARCHYTERMINATIONTASK_H_
#define HIERARCHYTERMINATIONTASK_H_

#include "AbstractTask.h"
#include "CoreInterface.h"
#include "HierarchyManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class HierarchyTerminationTask: public AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;
	HierarchyManager_SPtr hierMngr_SPtr;
public:
	HierarchyTerminationTask(CoreInterface& core);
	virtual ~HierarchyTerminationTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;
};

}

#endif /* HIERARCHYTERMINATIONTASK_H_ */
