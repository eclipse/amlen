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
 * CommRumAdapter.h
 *
 *  Created on: Feb 9, 2010
 *      Author:
 *
 * Version     : $Revision: 1.7 $
 *-----------------------------------------------------------------
 * $Log: CommRumAdapter.h,v $
 * Revision 1.7  2015/12/24 10:22:49 
 * make RUM trace no higher than CommRumAdapter
 *
 * Revision 1.6  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.5  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.4  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.3  2015/07/29 09:26:11 
 * split brain
 *
 * Revision 1.2  2015/05/05 12:48:25 
 * Safe connect
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.23  2015/01/26 12:38:14 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.22  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.21  2011/03/31 13:56:25 
 * Convert a couple of pointers to shared pointers
 *
 * Revision 1.20  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.19  2011/01/10 12:25:07 
 * Remove context from connectOnExisting() method
 *
 * Revision 1.18  2010/12/14 10:47:40 
 * Comm changes - receiver stream id in each message and neighbor, disconnect reject stream, if known and counts number tx and rcv before closing connection.
 *
 * Revision 1.17  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.16.2.1  2010/10/21 16:10:43 
 * Massive comm changes
 *
 * Revision 1.16  2010/08/23 13:33:05 
 * Embed RUM log within ours
 *
 * Revision 1.15  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.14  2010/07/01 11:54:49 
 * Remove setRumConfig as a sepeate method
 *
 * Revision 1.13  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.12  2010/06/27 12:01:19 
 * No functional changes
 *
 * Revision 1.11  2010/06/23 14:16:06 
 * No functional chnage
 *
 * Revision 1.10  2010/06/22 12:12:47 
 * No functional change
 *
 * Revision 1.9  2010/06/20 14:40:21 
 * Handle termination better. Add grace parameter to killRum()
 *
 * Revision 1.8  2010/06/14 15:50:45 
 * Remove unused print() method
 *
 * Revision 1.7  2010/06/08 15:46:25 
 * changed instanceID to String&
 *
 * Revision 1.6  2010/05/27 09:28:01 
 * Add disconnect() method
 *
 * Revision 1.5  2010/05/20 09:06:54 
 * Add incomingMsgQ, and the use of nodeId on the connect rather than a string
 *
 * Revision 1.4  2010/05/10 08:28:07 
 * Use a new interface: ConnectionsAsyncCompletionListener rather than an asyncCompletionListener template; remove the connect() method without a completion listener
 *
 * Revision 1.3  2010/04/12 09:17:33 
 * Add a mutex for roper synchronization + misc. non-funcational changes
 *
 * Revision 1.2  2010/04/07 13:53:11 
 * Add CVS log and a print() method
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 */

#ifndef COMMRUMADAPTER_H_
#define COMMRUMADAPTER_H_

#ifdef __MINGW__
#include <stdint.h>
#include <_mingw.h>
#endif

#include "rumCapi.h"
#include "CommAdapter.h"
#include "RumConnectionsMgr.h"
#include "CommRumReceiver.h"
#include "Definitions.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

#include <boost/thread/recursive_mutex.hpp>

namespace spdr
{
class CommRumAdapter: public CommAdapter, ScTraceContext
{
protected:

	rumInstance _rum;
	boost::shared_ptr<RumConnectionsMgr> _connMgr;
	boost::shared_ptr<CommRumReceiver> _rumReceiver;
	
	void killRum(bool grace);

private:
	static ScTraceComponent* _tc;

	boost::recursive_mutex _commMutex;

public:

	CommRumAdapter(const String& instID, const SpiderCastConfigImpl& config,
			NodeIDCache& cache, int64_t incarnationNumber);

	virtual ~CommRumAdapter();

	virtual void start();
	virtual void terminate(bool grace);

	//TODO: figure out
	static void on_alert_event(const rumEvent* alert_event, void *user)
	{

	}

	static void on_rum_log_event(const llmLogEvent_t *log_event, void *user);

	/*
	 * @see CommAdapter
	 */
	virtual bool connect(NodeIDImpl_SPtr target,
			ConnectionsAsyncCompletionListener* asyncCompletionListener, ConnectionContext ctx);

	/*
	 * @see CommAdapter
	 */
	virtual bool connect(NodeIDImpl_SPtr target, ConnectionContext ctx);

	/*
	 * @see CommAdapter
	 */
	virtual Neighbor_SPtr connectOnExisting(NodeIDImpl_SPtr target);

	/*
	 * @see CommAdapter
	 */
	virtual bool disconnect(Neighbor_SPtr target);

	/*
	 * @see CommAdapter
	 */
	virtual IncomingMsgQ_SPtr getIncomingMessageQ();

};

}

#endif /* COMMRUMADAPTER_H_ */
