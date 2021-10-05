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
 * TopologyManager.cpp
 *
 *  Created on: 15/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: TopologyManager.cpp,v $
 * Revision 1.3  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 11:30:18 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:14 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.9  2012/10/25 10:11:12 
 * Added copyright and proprietary notice
 *
 * Revision 1.8  2011/05/01 08:39:00 
 * Move connection context from TopologyManager to Definitions.h
 *
 * Revision 1.7  2010/12/29 13:20:50 
 * Add connect() context types
 *
 * Revision 1.6  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.5  2010/05/27 09:34:59 
 * Topology initial implementation
 *
 * */

#include "TopologyManager.h"

namespace spdr
{
const String TopologyManager::nodeStateName[] =
{ "INIT", "DISCOVERY", "NORMAL", "CLOSED" };

TopologyManager::TopologyManager()
{
	// Make auto-build happy
}

TopologyManager::~TopologyManager()
{
	// Make auto-build happy
}
}
