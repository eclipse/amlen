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
 * P2PStreamSyncCreationAdapter.h
 *
 *  Created on: Aug 6, 2012
 *      Author:
 *
 * Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamSyncCreationAdapter.h,v $
 * Revision 1.4  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.3.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.3  2015/11/18 08:37:00 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:18 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:21 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:18 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2012/10/10 15:33:37 
 * *** empty log message ***
 *
 * Revision 1.1  2012/09/30 12:39:44 
 * P2P initial version
 *
 */

#ifndef P2PSTREAMSYNCCREATIONADAPTER_H_
#define P2PSTREAMSYNCCREATIONADAPTER_H_

#include <boost/thread/condition_variable.hpp>

#include "ScTraceContext.h"
#include "ConnectionsAsyncCompletionListener.h"

namespace spdr
{

namespace messaging
{

class P2PStreamSyncCreationAdapter: public ScTraceContext,
		public spdr::ConnectionsAsyncCompletionListener
{
public:
	P2PStreamSyncCreationAdapter(const String& instID, const String& nodeName);
	virtual ~P2PStreamSyncCreationAdapter();

	virtual void onSuccess(Neighbor_SPtr result, ConnectionContext ctx);
	virtual void onFailure(String FailedTargetName, int rc, String message, ConnectionContext ctx);

	virtual Neighbor_SPtr waitForCompletion();
	virtual String getErrorMessage();

private:
	static ScTraceComponent* _tc;
	const String _instID;
	String _name;
	Neighbor_SPtr _neighbor;
	int _rc;
	String _msg;

	boost::condition_variable_any _condVar;
	boost::recursive_mutex _mutex;

	bool notDone();
};

}

}

#endif /* P2PSTREAMSYNCCREATIONADAPTER_H_ */
