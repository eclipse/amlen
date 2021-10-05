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
 * CommRumTxMgr.h
 *
 *  Created on: Feb 17, 2010
 *      Author:
 *
 * Version     : $Revision: 1.6 $
 *-----------------------------------------------------------------
 * $Log: CommRumTxMgr.h,v $
 * Revision 1.6  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.5  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.4  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.3  2015/07/02 10:29:12 
 * clean trace, byte-buffer
 *
 * Revision 1.2  2015/06/11 12:16:33 
 * *** empty log message ***
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.13  2015/01/26 12:38:13 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.12  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.11  2011/02/27 12:20:21 
 * Refactoring, header file organization
 *
 * Revision 1.10  2010/09/18 17:04:55 
 * Hierarhical bus name support
 *
 * Revision 1.9  2010/08/17 13:27:55 
 * Remove the RUM config param, as it should be done seperately for each call
 *
 * Revision 1.8  2010/08/05 09:25:01 
 * Init the RUM TX config structure only once
 *
 * Revision 1.7  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.6  2010/06/30 09:11:38 
 * Re-factor - no funcational changes
 *
 * Revision 1.5  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.4  2010/06/27 12:02:52 
 * Move all implementations from header files
 *
 * Revision 1.3  2010/06/27 08:20:29 
 * Pass RUM error code to ConnectionsAsyncCompletionLIstener::onFailure()
 *
 * Revision 1.2  2010/04/12 09:18:53 
 * Add a mutex for roper synchronization + misc. non-funcational changes
 *
 * Revision 1.1  2010/03/07 10:44:44 
 * Intial Version of the Comm Adapter
 *
 *
 */

#ifndef COMMRUMTXMGR_H_
#define COMMRUMTXMGR_H_

#ifdef __MINGW__
#include <stdint.h>
#include <_mingw.h>
#endif
#include "rumCapi.h"

#include "Definitions.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"
#include "SpiderCastRuntimeError.h"

#include "BusName.h"
#include "CommRumTxMgrListener.h"

#include <boost/thread/recursive_mutex.hpp>

namespace spdr
{

class CommRumTxMgr: public ScTraceContext
{
private:
	static ScTraceComponent* _tc;
	static ScTraceContextImpl _tcntx;
	rumInstance& _rum;
	String _queueName;
	boost::recursive_mutex _mutex;
	bool _closed;
	const String _instID;
	//rumTxQueueParameters config;
	CommRumTxMgrListener& _connMgr;

	void onStreamNotPresent(rumStreamID_t sid);

protected:
	const char* _thisMemberName;
	BusName_SPtr _thisBusName;

public:
	static const String queueNamePrefix;
	static const String queueNameSeparator;

	CommRumTxMgr(BusName_SPtr thisBusName, const char* thisMemberName,
			rumInstance& rum, const String& instID, CommRumTxMgrListener& connMgr);
	static void on_event(const rumEvent *event, void *event_user);

	virtual ~CommRumTxMgr();

	void terminate();
	void start();

	bool createTx(const String& target, const rumConnection& connection,
			rumQueueT* qt, rumStreamID_t* sid, int *rc); //TODO QT_SPtr

};

}

#endif /* COMMRUMTXMGR_H_ */
