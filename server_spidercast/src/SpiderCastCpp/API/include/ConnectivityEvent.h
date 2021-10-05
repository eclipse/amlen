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
 * ConnectivityEvent.h
 *
 *  Created on: Mar 3, 2011
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: ConnectivityEvent.h,v $
 * Revision 1.3  2015/11/18 08:36:57 
 * Update copyright headers
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:12 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.6  2012/10/30 13:19:45 
 * *** empty log message ***
 *
 * Revision 1.5  2012/10/25 10:11:05 
 * Added copyright and proprietary notice
 *
 * Revision 1.4  2012/02/14 12:45:18 
 * Add structured topology
 *
 * Revision 1.3  2011/08/07 12:12:21 
 * Add neighbors names list
 *
 * Revision 1.2  2011/03/24 08:44:30 
 * Documentation
 *
 * Revision 1.1  2011/03/16 13:37:36 
 * Add connectivity event; fix a couple of bugs in topology
 *
 */

#ifndef CONNECTIVITYEVENT_H_
#define CONNECTIVITYEVENT_H_

#include <iostream>
#include <sstream>

#include <boost/shared_ptr.hpp>
#include "Definitions.h"
#include "SpiderCastEvent.h"

namespace spdr
{

namespace event
{

/**
 * An event describing the connectivity level of a node with its peers.
 */
class ConnectivityEvent: public SpiderCastEvent
{

public:

	/**
	 * Constructor.
	 *
	 * @param numRandomNeighbors number of random neighbors
	 * @param numRingNeighbors number of ring neighbors
	 * @param neighbors a string with the random and ring neighbor names
	 * @param numOutgoingStructuredNeighbors number of outgoing structured neighbors
	 * @param outgoingStructuredNeighbors a string with outgoing structured neighbor names
	 * @param numIncomingStructuredNeighbors number of incoming  structured neighbors
	 * @param incomingStructuredNeighbors a string with incoming structured neighbor names
	 */
	ConnectivityEvent(
			short numRandomNeighbors,
			short numRingNeighbors,
			String neighbors,
			short numOutgoingStructuredNeighbors,
			String outgoingStructuredNeighbors,
			short numIncomingStructuredNeighbors,
			String incomingStructuredNeighbors) :
				SpiderCastEvent(Connectivity),
				_numRandomNeighbors(numRandomNeighbors),
				_numRingNeighbors(numRingNeighbors),
				_numOutgoingStructuredNeighbors(numOutgoingStructuredNeighbors),
				_numIncomingStructuredNeighbors(numIncomingStructuredNeighbors),
				_neighbors(neighbors), _outgoingStructuredNeighbors(
						outgoingStructuredNeighbors),
				_incomingStructuredNeighbors(incomingStructuredNeighbors)
	{
	}

	/**
	 * Destructor.
	 *
	 * @return
	 */
	virtual ~ConnectivityEvent()
	{
	}

	/**
	 * Get the number of random neighbors.
	 */
	virtual short getNumRandomNeighbors() const
	{
		return _numRandomNeighbors;
	}

	/**
	 * Get the number of ring neighbors.
	 */
	virtual short getNumRingNeighbors() const
	{
		return _numRingNeighbors;
	}

	/**
	 * Get the number of outgoing structured neighbors
	 */
	virtual short getNumOutStructuredNeighbors() const
	{
		return _numOutgoingStructuredNeighbors;
	}

	/**
	 * Get the number of incoming structured neighbors
	 */
	virtual short getNumInStructuredNeighbors() const
	{
		return _numIncomingStructuredNeighbors;
	}

	/**
	 * Get the names of ring and random neighbors.
	 *
	 * Names are separated by a comma.
	 * @return
	 */
	String getNeighbors() const
	{
		return _neighbors;
	}

	/**
	 * Get the names of outgoing structured neighbors.
	 *
	 * Names are separated by a comma.
	 * @return
	 */
	String getOutStructuredNeighbors() const
	{
		return _outgoingStructuredNeighbors;
	}

	/**
	 * Get the names of incoming structured neighbors.
	 *
	 * Names are separated by a comma.
	 * @return
	 */
	String getinStructuredNeighbors() const
	{
		return _incomingStructuredNeighbors;
	}

	virtual String toString() const;

private:
	const short _numRandomNeighbors;
	const short _numRingNeighbors;
	const short _numOutgoingStructuredNeighbors;
	const short _numIncomingStructuredNeighbors;
	String _neighbors;
	String _outgoingStructuredNeighbors;
	String _incomingStructuredNeighbors;
};

/**
 * A boost::shared_ptr to a ConnectivityEvent.
 */
typedef boost::shared_ptr<event::ConnectivityEvent> ConnectivityEvent_SPtr;

} //namespace event

} //namespace spdr


#endif /* CONNECTIVITYEVENT_H_ */
