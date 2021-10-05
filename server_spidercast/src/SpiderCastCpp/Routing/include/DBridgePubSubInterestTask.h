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
 * DBridgePubSubInterestTask.h
 *
 *  Created on: Jul 17, 2012
 */

#ifndef DBRIDGEPUBSUBINTERESTTASK_H_
#define DBRIDGEPUBSUBINTERESTTASK_H_


#include "AbstractTask.h"
#include "RoutingManager.h"

namespace spdr
{

namespace route
{

class DBridgePubSubInterestTask : public AbstractTask
{
public:
	DBridgePubSubInterestTask(RoutingManager_SPtr routingManager);

	virtual ~DBridgePubSubInterestTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;

private:
	RoutingManager_SPtr routingManager_;
};

}

}

#endif /* DBRIDGEPUBSUBINTERESTTASK_H_ */
