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
 * ConnectionsAsyncCompletionListener.h
 *
 *  Created on: Mar 2, 2010
 *      Author:
 *
 * Version     : $Revision: 1.5 $
 *-----------------------------------------------------------------
 * $Log: ConnectionsAsyncCompletionListener.h,v $
 * Revision 1.5  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.4.2.1  2016/01/21 14:52:47 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.4  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.3  2015/11/11 15:20:50 
 * *** empty log message ***
 *
 * Revision 1.2  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.10  2015/01/26 12:38:13 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.9  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.8  2012/10/10 15:33:37 
 * *** empty log message ***
 *
 * Revision 1.7  2011/06/29 07:39:20 
 * *** empty log message ***
 *
 * Revision 1.6  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.5.2.1  2010/10/21 16:10:42 
 * Massive comm changes
 *
 * Revision 1.5  2010/08/22 13:22:07 
 * Switch onSuccess to receive as a paramtere CSP(Neighbor) rather than (Neighbor *)
 *
 * Revision 1.4  2010/06/06 12:44:44 
 * Refactor; no functional changes
 *
 * Revision 1.3  2010/06/03 12:27:24 
 * No functional change
 *
 * Revision 1.2  2010/05/20 09:10:13 
 * No functional change
 *
 * Revision 1.1  2010/05/10 08:29:05 
 * Initial implementation if an interface for connections async completion listeners
 *
 */

#ifndef CONNECTIONSASYNCCOMPLETIONLISTENER_H_
#define CONNECTIONSASYNCCOMPLETIONLISTENER_H_

#include "Neighbor.h"
#include "Definitions.h"

namespace spdr
{

class ConnectionsAsyncCompletionListener
{
protected:
	ConnectionsAsyncCompletionListener(){};

public:
	virtual ~ConnectionsAsyncCompletionListener(){};

	virtual void onSuccess(Neighbor_SPtr result, ConnectionContext ctx) = 0;
	virtual void onFailure(String failedTargetName, int rc, String message, ConnectionContext ctx) = 0;

};

} // namespace spdr


#endif /* CONNECTIONASYNCCOMPLETIONLISTENER_H_ */
