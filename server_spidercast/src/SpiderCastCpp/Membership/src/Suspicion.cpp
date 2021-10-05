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
 * Suspicion.cpp
 *
 *  Created on: 21/04/2010
 */

#include "Suspicion.h"

namespace spdr
{

Suspicion::Suspicion(StringSPtr reportingNode, StringSPtr suspectedNode,
		NodeVersion suspectedVertion) :
	reporting(reportingNode), suspected(suspectedNode), suspectedNodeVersion(
			suspectedVertion)
{
}

Suspicion::Suspicion(const Suspicion& other) :
	reporting(other.reporting), suspected(other.suspected),
			suspectedNodeVersion(other.suspectedNodeVersion)
{
}

Suspicion& Suspicion::operator=(const Suspicion& other)
{
	if (&other == this)
	{
		return *this;
	}
	else
	{
		reporting = other.reporting;
		suspected = other.suspected;
		suspectedNodeVersion = other.suspectedNodeVersion;
		return *this;
	}
}

Suspicion::~Suspicion()
{
}

bool Suspicion::equalsWithVer(const Suspicion& other) const
{
	if ((*this == other) && suspectedNodeVersion == other.suspectedNodeVersion)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool operator<(const Suspicion& lhs, const Suspicion& rhs)
{
	//first by suspected, then by reporting

	if (*(lhs.suspected) < *(rhs.suspected))
	{
		return true;
	}
	else if (*(lhs.suspected) > *(rhs.suspected))
	{
		return false;
	}
	else
	{ //suspected equivalent
		if (*(lhs.reporting) < *(rhs.reporting))
		{
			return true;
		}
		else if (*(lhs.reporting) > *(rhs.reporting))
		{
			return false;
		}
		else
		{
			return false;
		}
	}
}

bool operator==(const Suspicion& lhs, const Suspicion& rhs)
{
	if ((*(lhs.suspected) == *(rhs.suspected)) && (*(lhs.reporting)
			== *(rhs.reporting)))
	{
		return true;
	}
	else
	{
		return false;
	}
}

}
