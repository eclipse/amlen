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
 * ConnectivityEvent.cpp
 *
 *  Created on: Mar 7, 2011
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: ConnectivityEvent.cpp,v $
 * Revision 1.2  2015/11/18 08:36:57 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:13 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:00 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2012/02/14 12:45:19 
 * Add structured topology
 *
 * Revision 1.1  2011/03/16 13:37:37 
 * Add connectivity event; fix a couple of bugs in topology
 *
 */

#ifndef CONNECTIVITYEVENT_CPP_
#define CONNECTIVITYEVENT_CPP_

#include <iostream>
#include <sstream>

#include "ConnectivityEvent.h"

namespace spdr
{

namespace event
{
String ConnectivityEvent::toString() const
{
	std::ostringstream oss;
	oss << SpiderCastEvent::toString() << " numRingNeighbors="
			<< _numRingNeighbors << ", numRandomNeighbors="
			<< _numRandomNeighbors << ", numOutStructuredNeighbors="
			<< _numOutgoingStructuredNeighbors << ", numInStructuredNeighbors="
			<< _numIncomingStructuredNeighbors;

	return oss.str();
}
} //namespace event

} //namespace spdr


#endif /* CONNECTIVITYEVENT_CPP_ */
