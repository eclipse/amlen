/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#ifndef PXCLUSTER_UNITTESTTRACEINIT_H_
#define PXCLUSTER_UNITTESTTRACEINIT_H_

#include "ismutil.h"

#include "CyclicFileLogger.h"

//#include "TestTraceHelper.h"

/**
 * Will:
 *
 * 1) initialize the tracer log file to ./log/trace_{iso-time}.log
 */
class PXClusterUnitTestTraceInit
{
public:
	static pxcluster::CyclicFileLogger* cfll_ptr;
private:
	boost::posix_time::ptime start;


public:
	static std::string mcpTraceDir; //"./pxcluster_unittest_trace";
	static int mcpTraceLevel; // default level: 1, range: [0,9]

	PXClusterUnitTestTraceInit();

	virtual ~PXClusterUnitTestTraceInit();

	/**
	 * Change trace level
	 *
	 * @param level 0 to 9
	 */
	static void changeTraceLevel(int level);

};

#endif /* PXCLUSTER_UNITTESTTRACEINIT_H_ */
