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
 * P2PRouter.cpp
 *
 *  Created on: Jan 25, 2012
 */

#include "P2PRouter.h"

namespace spdr
{

namespace route
{

P2PRouter::P2PRouter()
{
	// TODO Auto-generated constructor stub

}

P2PRouter::~P2PRouter()
{
	// TODO Auto-generated destructor stub
}

bool P2PRouter::send(NodeIDImpl_SPtr target, SCMessage_SPtr message)
		throw (SpiderCastRuntimeError)
{
	//TODO
	return false;
}

bool P2PRouter::route(SCMessage_SPtr message) throw (SpiderCastRuntimeError)
{
	//TODO
	return false;
}

}
}
