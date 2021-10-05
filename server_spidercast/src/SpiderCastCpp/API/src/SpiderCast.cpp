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
 * SpiderCast.cpp
 *
 *  Created on: 28/01/2010
 */

#include <iostream>

#include "SpiderCast.h"

using namespace std;

namespace spdr
{

const String SpiderCast::nodeStateName[] = {"INIT", "STARTED", "CLOSED", "ERROR"};

String SpiderCast::getNodeStateName(NodeState nodeState)
{
	if (nodeState < SpiderCast::Init || nodeState > SpiderCast::Error)
	{
		std::ostringstream oss;
		oss << "Illegal enum NodeState: " << ((int)nodeState);
		throw IllegalArgumentException(oss.str());
	}
	return nodeStateName[nodeState];
}

SpiderCast::SpiderCast()
{
	//std::cout << "SpiderCast()" << endl;
}

SpiderCast::~SpiderCast()
{
	//std::cout << "~SpiderCast()" << endl;
}

}
