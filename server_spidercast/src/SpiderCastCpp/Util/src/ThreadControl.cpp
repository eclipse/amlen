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
 * ThreadControl.cpp
 *
 *  Created on: 09/03/2010
 */

#include "ThreadControl.h"

namespace spdr
{

const boost::posix_time::milliseconds ThreadControl::wait_millis(5000);
const boost::posix_time::milliseconds ThreadControl::zero_millis(0);

ThreadControl::ThreadControl()
{
}

ThreadControl::~ThreadControl()
{
}

}
