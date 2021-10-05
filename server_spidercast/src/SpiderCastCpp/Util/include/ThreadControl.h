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
 * ThreadControl.h
 *
 *  Created on: 09/03/2010
 */

#ifndef THREADCONTROL_H_
#define THREADCONTROL_H_

#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace spdr
{

class ThreadControl
{
public:
	static const boost::posix_time::milliseconds wait_millis; //< 5000 milliseconds
	static const boost::posix_time::milliseconds zero_millis;

	ThreadControl();
	virtual ~ThreadControl();

	/**
	 * Wake a thread to do some work, bit-mask may signal what type of work is pending.
	 *
	 * Bit 0 - unspecified type.
	 * Bit 1-31 - TBD, implementation specific.
	 *
	 * @param mask marks the type of work pending. must be >0.
	 * @throw IllegalArgumentException if mask==0.
	 */
	virtual void wakeUp(uint32_t mask = (uint32_t)1 ) = 0;
};

}

#endif /* THREADCONTROL_H_ */
