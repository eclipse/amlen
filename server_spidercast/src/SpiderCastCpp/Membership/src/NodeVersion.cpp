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
 * NodeVersion.cpp
 *
 *  Created on: 28/03/2010
 */

#include "NodeVersion.h"

namespace spdr
{

NodeVersion::~NodeVersion()
{
}

NodeVersion& NodeVersion::operator=(const NodeVersion& nodeVersion)
{
	if (this == &nodeVersion)
		return *this;
	else
	{
		_incarnationNumber = nodeVersion._incarnationNumber;
		_minorVersion = nodeVersion._minorVersion;
		return *this;
	}
}

String NodeVersion::toString() const
{
	std::ostringstream oss;
	oss << getIncarnationNumber() << "." << getMinorVersion();
	return oss.str();
}

bool operator<(const NodeVersion& lhs, const NodeVersion& rhs)
{
	return (lhs._incarnationNumber < rhs._incarnationNumber
			|| (lhs._incarnationNumber == rhs._incarnationNumber
					&& lhs._minorVersion < rhs._minorVersion));
}

bool operator==(const NodeVersion& lhs, const NodeVersion& rhs)
{
	return (lhs._incarnationNumber == rhs._incarnationNumber
			&& lhs._minorVersion == rhs._minorVersion);
}

}
