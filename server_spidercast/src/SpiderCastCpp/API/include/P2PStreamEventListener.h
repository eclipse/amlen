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
 * P2PStreamEventListener.h
 *
 *  Created on: Jun 28, 2012
 *      Author:
 *
 *  Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamEventListener.h,v $
 * Revision 1.3  2015/11/18 08:36:56 
 * Update copyright headers
 *
 * Revision 1.2  2015/03/22 14:43:19 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:11 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:06 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2012/06/28 09:33:15 
 * P2P streams interface first version
 *
 */

#ifndef P2PSTREAMEVENTLISTENER_H_
#define P2PSTREAMEVENTLISTENER_H_

#include "P2PStreamEvent.h"

namespace spdr
{

namespace messaging
{

class P2PStreamEventListener
{
public:

	virtual ~P2PStreamEventListener() = 0;

	virtual void onEvent(event::P2PStreamEvent_SPtr event) = 0;

protected:

	P2PStreamEventListener();
};

}

}

#endif /* P2PSTREAMEVENTLISTENER_H_ */
