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
 * StatisticsTask.h
 *
 *  Created on: Dec 7, 2010
 */

#ifndef STATISTICSTASK_H_
#define STATISTICSTASK_H_

#include "AbstractTask.h"
#include "CoreInterface.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class StatisticsTask :public AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;
	CoreInterface& core_;
	bool first_;

public:
	StatisticsTask(CoreInterface& core);
	virtual ~StatisticsTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;
};

}

#endif /* STATISTICSTASK_H_ */
