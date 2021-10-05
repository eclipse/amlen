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
 * CommUDPThread.h
 *
 *  Created on: Jun 19, 2011
 */

#ifndef COMMUDPTHREAD_H_
#define COMMUDPTHREAD_H_

#include <boost/asio.hpp>

#include "Thread.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class CommUDPThread: public Thread, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

	String name_;
	boost::asio::io_service& io_service_;

public:
	CommUDPThread(const String& instID, const String name, boost::asio::io_service& io_service);
	virtual ~CommUDPThread();

	/*
	 * @see Thread
	 */
	const String getName() const;

	/*
	 * @see Thread
	 */
	void operator()();
};

}

#endif /* CommUDPThread_H_ */
