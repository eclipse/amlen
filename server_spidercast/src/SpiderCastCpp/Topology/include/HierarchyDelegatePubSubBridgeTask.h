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
 * HierarchyDelegatePubSubBridgeTask.h
 *
 *  Created on: Jul 9, 2012
 */

#ifndef HIERARCHYDELEGATEPUBSUBBRIDGETASK_H_
#define HIERARCHYDELEGATEPUBSUBBRIDGETASK_H_

#include "AbstractTask.h"
#include "HierarchyDelegateTaskInterface.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class HierarchyDelegatePubSubBridgeTask: public AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;
	HierarchyDelegateTaskInterface& delegateTaskInterface_;

public:
	HierarchyDelegatePubSubBridgeTask(const String& instID,
			HierarchyDelegateTaskInterface& delegateTaskInterface);

	virtual ~HierarchyDelegatePubSubBridgeTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;
};

}

#endif /* HIERARCHYDELEGATEPUBSUBBRIDGETASK_H_ */
