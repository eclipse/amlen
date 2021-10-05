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
 * P2PStreamRcvImpl.h
 *
 *  Created on: Jun 28, 2012
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamRcvImpl.h,v $
 * Revision 1.2  2015/11/18 08:37:00 
 * Update copyright headers
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

#ifndef P2PSTREAMRCVIMPL_H_
#define P2PSTREAMRCVIMPL_H_

#include <boost/noncopyable.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "P2PStreamRcv.h"
#include "P2PStreamEventListener.h"
#include "MessageListener.h"
#include "PropertyMap.h"
#include "SpiderCastConfigImpl.h"
#include "NodeIDCache.h"
#include "CoreInterface.h"
#include "MessagingManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"


#include "ScTraceContext.h"

namespace spdr
{

namespace messaging
{

class P2PStreamRcvImpl: boost::noncopyable,
		public P2PStreamRcv,
		public ScTraceContext
{

	static ScTraceComponent* _tc;

public:

	P2PStreamRcvImpl(const String& instID, const SpiderCastConfigImpl& config,
			NodeIDCache& nodeIDCache, CoreInterface& coreInterface,
			MessageListener& messageListener,
			P2PStreamEventListener& eventListener, const PropertyMap& propMap);

	virtual ~P2PStreamRcvImpl();

	void close();

	bool isOpen();

	void rejectStream(StreamID& sid);

	std::string toString() const;

	virtual void processIncomingDataMessage(SCMessage_SPtr message);

private:
	const String& _instID;
	const SpiderCastConfigImpl& _config;
	NodeIDCache& _nodeIDCache;
	CoreInterface& _coreInterface;
	MessageListener& _messageListener;
	P2PStreamEventListener& _eventListener;
	const PropertyMap& _propMap;

	mutable boost::recursive_mutex _mutex;
	bool _closed;

	MessagingManager_SPtr messagingManager_SPtr;

};

typedef boost::shared_ptr<P2PStreamRcvImpl> P2PStreamRcvImpl_SPtr;

}

}

#endif /* P2PSTREAMRCVIMPL_H_ */
