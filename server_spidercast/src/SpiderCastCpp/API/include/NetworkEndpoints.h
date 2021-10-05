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
 * NetworkEndpoints.h
 *
 *  Created on: 02/03/2010
 */

#ifndef NETWORKENDPOINTS_H_
#define NETWORKENDPOINTS_H_

#include <string>
#include <vector>
#include <sstream>
#include <boost/cstdint.hpp>

//#include <boost/operators.hpp>

#include "Definitions.h"
#include "SpiderCastLogicError.h"

namespace spdr
{

using std::string;
using std::vector;
using std::pair;

/**
 * Represents node network end-points, assuming multiple interfaces and a single port.
 *
 * Different interfaces may reside on different networks without connectivity.
 * Network scope is identified by the Network-ID;
 * Two Nodes that have host-addresses with the same network-ID can communicate with each other.
 *
 * This class holds a vector of scoped-addresses, and a port number.
 * Each scoped-address is a pair holding two strings;
 * first is the IP address or host-name, second is the Network-ID.
 *
 * Order within the vector does matter - the scoped addresses are usually scanned from
 * beginning to end when establishing a connection.
 *
 * This class supports equality operators == and !=, as well as less than comparisons and
 * all its variations <, <=, >, >=.
 *
 */
class NetworkEndpoints
{

public:
	/**
	 * @return  Empty addresses, port=0.
	 */
	NetworkEndpoints();

	/**
	 * Construct from a vector of scoped-addresses and a port number.
	 *
	 * @param addresses A vector containing string pairs, where each pair contains:
	 * 	(1) HostAddress String, (2) NetworkID String.
	 *
	 * @param port port number
	 *
	 * 	Where
	 * 	(1) HostAddress String: Either an IP address in "a.b.c.d" format, or a host-name (for DNS lookup);
	 * 	(2) NetworkID String: Represents the network ID.
	 * 	Nodes that have interfaces with the same network ID can communicate with each other.
	 * 	The empty string "" means global scope.
	 */
	NetworkEndpoints(const vector<pair<string, string> >& addresses,
			uint16_t port);

	/**
	 * Copy constructor.
	 *
	 * @param ne
	 * @return
	 */
	NetworkEndpoints(const NetworkEndpoints& ne);

	/**
	 * Assignment operator.
	 *
	 * @param ne
	 * @return
	 */
	NetworkEndpoints& operator=(const NetworkEndpoints& ne);

	/**
	 * Destructor.
	 *
	 * @return
	 */
	virtual ~NetworkEndpoints();

	/**
	 * Get the scoped-address vector.
	 *
	 * @return a copy of the internal scoped-address vector.
	 */
	const vector<pair<string, string> >& getAddresses() const
	{
		return _addresses;
	}

	/**
	 * Get the number of addresses.
	 *
	 * @return the number of addresses.
	 */
	size_t getNumAddresses() const
	{
			return _addresses.size();
	}

	/**
	 * Get the port number.
	 *
	 * @return
	 */
	uint16_t getPort() const
	{
		return _port;
	}

	/**
	 * A string representation of the network end-points.
	 * @return
	 */
	String toString() const;

	/**
	 * A string representation of the network end-points that is safe to use as a node name.
	 *
	 * No white space, control characters, backslash, comma, or equal sign.
	 * @return
	 */
	String toNameString() const;

	/**
	 * Compare two NetworkEndpoints objects for equality.
	 *
	 * @param lhs
	 * @param rhs
	 * @return true if equal
	 */
	friend
	bool operator==(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs);

	/**
	 * Compare two NetworkEndpoints objects for inequality.
	 *
	 * @param lhs
	 * @param rhs
	 * @return false if equal
	 */
	friend
	bool operator!=(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs);

	/**
	 * Compare two NetworkEndpoints objects for less-than relative order.
	 *
	 * Compares the vectors, using the natural < ordering of vector, pair, and string;
	 * If the vectors are equal compares the port.
	 *
	 * @param lhs
	 * @param rhs
	 * @return true if lhs is less than rhs.
	 */
	friend
	bool operator<(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs);

	friend
	bool operator<=(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs);

	friend
	bool operator>(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs);

	friend
	bool operator>=(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs);

	std::size_t hash_value() const
	{
		return _hashValue;
	}

	friend
	std::size_t hash_value( const NetworkEndpoints& ne)
	{
		return ne._hashValue;
	}

private:
	vector<pair<string, string> > _addresses;
	uint16_t _port;
	std::size_t _hashValue;
};

}//namespace spdr

#endif /* NETWORKENDPOINTS_H_ */
