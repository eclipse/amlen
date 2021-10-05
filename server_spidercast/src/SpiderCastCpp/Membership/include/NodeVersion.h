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
 * NodeVersion.h
 *
 *  Created on: 28/03/2010
 */

#ifndef NODEVERSION_H_
#define NODEVERSION_H_

#include <sstream>

#include <boost/cstdint.hpp>
#include <boost/operators.hpp>

#include "Definitions.h"

namespace spdr
{

class NodeVersion: public boost::less_than_comparable<NodeVersion>,
		public boost::equality_comparable<NodeVersion>
{

private:
	int64_t _incarnationNumber;
	int64_t _minorVersion;

public:

	NodeVersion() :
		_incarnationNumber(0), _minorVersion(0)
	{
	}

	NodeVersion(int64_t incarnationNumber, int64_t minorVersion) :
		_incarnationNumber(incarnationNumber), _minorVersion(minorVersion)
	{
	}

	NodeVersion(const NodeVersion& nodeVersion) :
			_incarnationNumber(nodeVersion._incarnationNumber),
			_minorVersion(nodeVersion._minorVersion)
	{
	}

	virtual ~NodeVersion();

	NodeVersion& operator=(const NodeVersion& nodeVersion);

	void addToMinorVersion(int64_t delta)
	{
		_minorVersion += delta;
	}

	int64_t getMinorVersion() const
	{
		return _minorVersion;
	}

	int64_t getIncarnationNumber() const
	{
		return _incarnationNumber;
	}

	String toString() const;

	friend
	bool operator<(const NodeVersion& lhs, const NodeVersion& rhs);

	friend
	bool operator==(const NodeVersion& lhs, const NodeVersion& rhs);

};

}

#endif /* NODEVERSION_H_ */
