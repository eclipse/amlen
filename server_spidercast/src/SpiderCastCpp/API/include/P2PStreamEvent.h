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
 * P2PStreamEvent.h
 *
 *  Created on: Jun 27, 2012
 *      Author:
 *
 *  Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamEvent.h,v $
 * Revision 1.3  2015/11/18 08:36:57 
 * Update copyright headers
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:12 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:04 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2012/06/28 09:33:15 
 * P2P streams interface first version
 *
 */

#ifndef P2PSTREAMEVENT_H_
#define P2PSTREAMEVENT_H_

#include "SpiderCastEvent.h"
#include "StreamID.h"

namespace spdr
{

namespace event
{

class P2PStreamEvent: public spdr::event::SpiderCastEvent
{
public:
	virtual ~P2PStreamEvent();

protected:
	P2PStreamEvent(EventType eventType);
};


class P2PStreamBreakEvent : public P2PStreamEvent
{
public:
	P2PStreamBreakEvent(messaging::StreamID_SPtr sid);
	virtual ~P2PStreamBreakEvent();

	messaging::StreamID_SPtr getSid();

private:
	messaging::StreamID_SPtr _sid;
};

typedef boost::shared_ptr<P2PStreamEvent>				P2PStreamEvent_SPtr;
typedef boost::shared_ptr<P2PStreamBreakEvent>			P2PStreamBreakEvent_SPtr;
}

}

#endif /* P2PSTREAMEVENT_H_ */

