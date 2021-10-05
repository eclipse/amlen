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
 * P2PStreamEvent.cpp
 *
 *  Created on: Jun 27, 2012
 *      Author:
 *
 *  Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamEvent.cpp,v $
 * Revision 1.2  2015/11/18 08:36:57 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:12 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:01 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2012/06/28 09:33:15 
 * P2P streams interface first version
 *
 */

#include "P2PStreamEvent.h"

namespace spdr
{

namespace event
{

P2PStreamEvent::P2PStreamEvent(EventType eventType) :
			SpiderCastEvent(eventType)
{

}

P2PStreamEvent::~P2PStreamEvent()
{
	// TODO Auto-generated destructor stub
}

//=========================================================

P2PStreamBreakEvent::P2PStreamBreakEvent(messaging::StreamID_SPtr sid) :
		P2PStreamEvent(STERAM_BROKE), _sid(sid)
{
}

P2PStreamBreakEvent::~P2PStreamBreakEvent()
{
}

messaging::StreamID_SPtr P2PStreamBreakEvent::getSid()
{
	return _sid;
}

}

}

