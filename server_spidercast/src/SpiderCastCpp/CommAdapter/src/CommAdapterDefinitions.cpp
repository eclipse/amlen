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
 * CommAdapterDefinitions.cpp
 *
 *  Created on: Jun 19, 2011
 */

#include "CommAdapterDefinitions.h"

namespace spdr
{

namespace comm
{

//TODO if there are multiple matched, choose randomly
std::string endpointScopeMatch(
			const std::vector<std::pair<std::string, std::string> >& localAddr,
			const std::vector<std::pair<std::string, std::string> >& targetAddr)
{
	using namespace std;

	string targetIP;

	vector<pair<string, string> >::const_iterator myIter;
	vector<pair<string, string> >::const_iterator targetIter;

	bool found = false;
	for (myIter = localAddr.begin(); myIter != localAddr.end(); ++myIter)
	{
		if (found)
		{
			break;
		}

		std::string myScope = myIter->second;
		for (targetIter = targetAddr.begin(); targetIter != targetAddr.end(); ++targetIter)
		{
			std::string targetScope = targetIter->second;
			if (myIter->second == targetIter->second) //equal scope
			{
				targetIP = targetIter->first;
				found = true;
				break;
			}
		}
	}

	return targetIP;
}


} //namespace comm
} //namespace spdr
