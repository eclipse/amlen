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
 * HierarchyDelegateTaskInterface.h
 *
 *  Created on: Nov 4, 2010
 */

#ifndef HIERARCHYDELEGATETASKINTERFACE_H_
#define HIERARCHYDELEGATETASKINTERFACE_H_

#include "NodeIDImpl.h"

namespace spdr
{
class HierarchyDelegateTaskInterface
{
public:
	HierarchyDelegateTaskInterface();
	virtual ~HierarchyDelegateTaskInterface();

	/**
	 * Connects to supervisors in case there are too few
	 * Disconnect from supervisors in case there are too many
	 */
	virtual void connectTask() = 0;

	/**
	 * Remove a supervisor candidate from quarantine
	 * @param peer
	 */
	virtual void unquarantineSupervisorCandidate(NodeIDImpl_SPtr peer) = 0;

	/**
	 * Send a view to the supervisor
	 */
	virtual void supervisorViewUpdate() = 0;

	/**
	 * Manage the state of the PubSubBridge
	 */
	virtual void pubsubBridgeTask() = 0;
};
}
#endif /* HIERARCHYDELEGATETASKINTERFACE_H_ */
