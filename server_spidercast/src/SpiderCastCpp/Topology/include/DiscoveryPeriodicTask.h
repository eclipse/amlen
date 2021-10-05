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
 * DiscoveryPeriodicTask.h
 *
 *  Created on: Aug 11, 2010
 *      Author:
 */

#ifndef DISCOVERYPERIODICTASK_H_
#define DISCOVERYPERIODICTASK_H_

#include "AbstractTask.h"
#include "CoreInterface.h"
#include "TopologyManager.h"

namespace spdr
{

class DiscoveryPeriodicTask: public spdr::AbstractTask
{
private:
	TopologyManager_SPtr topoMgr_SPtr;
public:
	DiscoveryPeriodicTask(CoreInterface& core);
	virtual ~DiscoveryPeriodicTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString();

};

}

#endif /* DISCOVERYPERIODICTASK_H_ */
