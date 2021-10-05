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
 * HierarchyDelegateUnquarantineTask.h
 *
 *  Created on: Nov 4, 2010
 */

#ifndef HIERARCHYDELEGATEUNQUARANTINETASK_H_
#define HIERARCHYDELEGATEUNQUARANTINETASK_H_

#include "AbstractTask.h"
#include "HierarchyDelegateTaskInterface.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class HierarchyDelegateUnquarantineTask: public AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;
	HierarchyDelegateTaskInterface& delegateTaskInterface_;
	NodeIDImpl_SPtr peer_;

public:
	HierarchyDelegateUnquarantineTask(
			const String& instID,
			HierarchyDelegateTaskInterface& delegateTaskInterface,
			NodeIDImpl_SPtr peer);
	virtual ~HierarchyDelegateUnquarantineTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;

};

}

#endif /* HIERARCHYDELEGATEUNQUARANTINETASK_H_ */
