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
 * BusName.cpp
 *
 *  Created on: Sep 12, 2010
 *      Author:
 */

#include <iostream>
//#include <fstream>
//#include <map>
#include <sstream>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/lexical_cast.hpp>

#include "BusName.h"


namespace spdr
{

using namespace std;

BusName::BusName(const char * busName) throw(IllegalArgumentException)
		: _busName(busName)

{
	boost::trim(_busName);
	if (_busName[0] != '/')
	{
		String err = "Bad bus name, does not starts with '/'; "
				+ _busName;
		throw IllegalArgumentException(err);
	}

	typedef boost::tokenizer<boost::char_separator<char> >
	    tokenizer;
	boost::char_separator<char> sep("/"); //drop delimiters, drop empty tokens
	tokenizer tokens(_busName, sep);

	int i = 0;
	for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter, i++)
	{
		if (i == 0)
		{
			_busL1Name = *tok_iter;
		}

		if (i == 1)
		{
			_busL2Name = *tok_iter;
		}

		if (i > 1)
		{
			String err = "Bad bus name, too many levels; " + _busName;
			throw IllegalArgumentException(err);
		}
	}

	if (_busL1Name.empty())
	{
		String err = "Bad bus name, L1 segment is empty; "
				+ _busName;
		throw IllegalArgumentException(err);
	}


	_busName = "/" + _busL1Name;
	if (!_busL2Name.empty())
	{
		_busName.append("/");
		_busName += _busL2Name;
	}

	validateBusName(_busName); //make sure only valid characters in _busL1Name and _busL2Name
}

bool forbiddenInLevel(char c)
{
	if (c == ',' || c == '"' || c == '=' || c == '\\')
			return true;
		else
			return false;
}

void BusName::validateBusName(const std::string& busName)
{
	if (boost::algorithm::any_of(busName, boost::algorithm::is_cntrl())
			|| boost::algorithm::any_of(busName, forbiddenInLevel))
	{
		String what("Bad bus name - ");
		what.append("cannot contain control characters, commas, ");
		what.append("double quotation marks, backslashes or equal signs: '");
		what.append(busName).append("'");
		throw IllegalArgumentException(what);
	}
}



BusName::~BusName()
{
}

bool spdr::BusName::isEqual(const BusName & other) const
{
	return !_busL1Name.compare(other._busL1Name) && !_busL2Name.compare(other._busL2Name);
}



bool spdr::BusName::isManager(const BusName & other) const
{
	if (!isEqual(other))
	{
		if (!_busL1Name.compare(other._busL1Name))
		{
			return _busL2Name.empty() && !other._busL2Name.empty();
		}
	}
	return false;
}



bool spdr::BusName::isManaged(const BusName & other) const
{
	if (!isEqual(other))
	{
		if (!_busL1Name.compare(other._busL1Name))
		{
			return other._busL2Name.empty() && !_busL2Name.empty();
		}
	}
	return false;
}

int spdr::BusName::comparePrefix(const BusName& other) const
{
	int res = 0;
	if (!_busL1Name.compare(other._busL1Name))
	{
		res++;
		if (!_busL2Name.compare(other._busL2Name))
		{
			res++;
		}
	}
	return res;
}


int spdr::BusName::getLevel() const
{
	if (!_busL2Name.empty())
		return 2;
	if (!_busL1Name.empty())
		return 1;
	return 0;
}

String spdr::BusName::toString() const
{
	return toOrgString();
}

String spdr::BusName::toShortString() const
{
	stringstream tmpOut;
	tmpOut << _busL1Name;
	if (!_busL2Name.empty())
		tmpOut << "-" << _busL2Name;
	return tmpOut.str();
}

String spdr::BusName::toOrgString() const
{
	return _busName;
}

}
