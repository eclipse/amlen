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
 * Next2HopsBroadcast.h
 *
 *  Created on: Feb 19, 2012
 */

#ifndef NEXT2HOPSBROADCAST_H_
#define NEXT2HOPSBROADCAST_H_

#include <string>

#include "Neighbor.h"
#include "VirtualID.h"

namespace spdr
{

namespace route
{

/**
 * The next two hops of a range-based broadcast algorithm.
 */
class Next2HopsBroadcast
{
public:
	Next2HopsBroadcast();
	virtual ~Next2HopsBroadcast();

	Neighbor_SPtr firstHop;
	util::VirtualID firstHopUpperBound;
	Neighbor_SPtr secondHop;

	std::string toString() const;

};

}

}

#endif /* NEXT2HOPSBROADCAST_H_ */
