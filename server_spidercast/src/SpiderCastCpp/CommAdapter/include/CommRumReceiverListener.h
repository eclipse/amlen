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
 * CommRumReceiverListener.h
 *
 *  Created on: Feb 28, 2010
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: CommRumReceiverListener.h,v $
 * Revision 1.3  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.2.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.2  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.5  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.4  2011/06/29 07:39:20 
 * *** empty log message ***
 *
 * Revision 1.3  2010/04/12 09:18:35 
 * Add #include "Definitions.h"
 *
 * Revision 1.2  2010/04/07 13:53:24 
 * Add CVS log
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 */

#ifndef COMMRUMRECEIVERLISTENER_H_
#define COMMRUMRECEIVERLISTENER_H_

#ifdef __MINGW__
#include <stdint.h>
#include <_mingw.h>
#endif
#include "rumCapi.h"
#include "Definitions.h"

namespace spdr
{

class CommRumReceiverListener
{
public:
	/**
	 *
	 * @param sid stream ID
	 * @param nodeName the sender's name extracted from the queue name
	 * @param con connection ID
	 * @return the sender's local name, from the connections map
	 */
	virtual std::string onNewStreamReceived(rumStreamID_t sid, String nodeName,
			rumConnectionID_t con) = 0;
	virtual void onStreamBreak(rumStreamID_t sid) = 0;

protected:
	virtual ~CommRumReceiverListener(){}
};
} // namespace spdr
#endif /* COMMRUMRECEIVERLISTENER_H_ */
