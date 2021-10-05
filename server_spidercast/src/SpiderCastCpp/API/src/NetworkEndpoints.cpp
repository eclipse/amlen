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
 * NetworkEndpoints.cpp
 *
 *  Created on: 02/03/2010
 */

#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>

#include "NetworkEndpoints.h"

namespace spdr
{

NetworkEndpoints::NetworkEndpoints() :
	_port(0), _hashValue(0)
{
}

NetworkEndpoints::NetworkEndpoints(const vector< pair<string,string> >& addresses,
		uint16_t port) :
	_addresses(addresses), _port(port), _hashValue(0)
{
	boost::hash<string> hasher;

	for (vector< pair<string,string> >::const_iterator it = _addresses.begin(); it != _addresses.end(); ++it)
	{
		_hashValue += 1951L * hasher(it->first);
		_hashValue += 1973L * hasher(it->second);
	}

	_hashValue += 33L*_port;
}

NetworkEndpoints::~NetworkEndpoints()
{
}

NetworkEndpoints::NetworkEndpoints(const NetworkEndpoints& ne)
{
	_addresses = ne._addresses;
	_port = ne._port;
	_hashValue = ne._hashValue;
}

NetworkEndpoints& NetworkEndpoints::operator=(const NetworkEndpoints& ne)
{
	if (&ne == this)
	{
		return *this;
	}
	else
	{
		_addresses = ne._addresses;
		_port = ne._port;
		_hashValue = ne._hashValue;
		return *this;
	}
}

String NetworkEndpoints::toString() const
{
	String s("Addresses=[");
	vector< pair<string,string> >::const_iterator pos;
	for (pos =_addresses.begin(); pos != _addresses.end();)
	{
		s = s +  pos->first;
		if (pos->second != "")
		{
			s = s + "|" + pos->second;
		}

		if (++pos != _addresses.end())
		{
			s += " ";
		}
	}

	s = s + "] Port=" + boost::lexical_cast<String>(_port);

	return s;
}


String NetworkEndpoints::toNameString() const
{
	String s;
	vector< pair<string,string> >::const_iterator pos;
	for (pos =_addresses.begin(); pos != _addresses.end();)
	{
		s = s +  pos->first;
		if (pos->second != "")
		{
			s = s + "-" + pos->second;
		}

		if (++pos != _addresses.end())
		{
			s += "_";
		}
	}

	s = s + "_" + boost::lexical_cast<String>(_port);

	return s;
}


//friend
bool operator==(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs)
{
	return ((lhs._addresses == rhs._addresses) && (lhs._port == rhs._port));
}

//friend
bool operator!=(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs)
{
	return !(lhs == rhs);
}

//friend
bool operator<(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs)
{
	if ((lhs._addresses < rhs._addresses))
		return true;
	else if ((lhs._addresses == rhs._addresses) && (lhs._port < rhs._port))
		return true;
	else
		return false;
}

//friend
bool operator>=(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs)
{
	return !(lhs < rhs);
}

//friend
bool operator>(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs)
{
	return !(lhs < rhs) && !(lhs == rhs);
}

//friend
bool operator<=(const NetworkEndpoints& lhs, const NetworkEndpoints& rhs)
{
	return (lhs < rhs) || (lhs == rhs);
}

}
