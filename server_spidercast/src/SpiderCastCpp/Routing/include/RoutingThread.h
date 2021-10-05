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
 * RoutingThread.h
 *
 *  Created on: Feb 9, 2012
 */

#ifndef ROUTINGTHREAD_H_
#define ROUTINGTHREAD_H_

#include <boost/shared_ptr.hpp>

#include "Thread.h"
#include "SpiderCastConfigImpl.h"
#include "CoreInterface.h"
#include "RoutingManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace route
{

class RoutingThread : public Thread, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	RoutingThread(const String& instID, SpiderCastConfigImpl& config, CoreInterface& coreIfc);
	virtual ~RoutingThread();

	/*
	 * @see Thread
	 */
	const String getName() const;

	/*
	 * @see Thread
	 */
	void operator()();


private:
	const String instID_;
	SpiderCastConfigImpl& configImpl_;
	CoreInterface&  coreInterface_;
	const String name_;
	boost::shared_ptr<RoutingManager> routingTask_;
};

}

}

#endif /* ROUTINGTHREAD_H_ */
