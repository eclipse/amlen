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
 * Next2HopsBroadcast.cpp
 *
 *  Created on: Feb 19, 2012
 */

#include "Next2HopsBroadcast.h"

namespace spdr
{

namespace route
{

Next2HopsBroadcast::Next2HopsBroadcast()
{
}

Next2HopsBroadcast::~Next2HopsBroadcast()
{
}

std::string Next2HopsBroadcast::toString() const
{
	String s("1st: ");
	s.append(firstHop ? firstHop->getName() : "-");
	s.append(" / 1st-UB: ");
	s.append(firstHopUpperBound.toString());
	s.append(" / 2nd: ");
	s.append(secondHop ? secondHop->getName() : "-");
	return s;
}
}

}
