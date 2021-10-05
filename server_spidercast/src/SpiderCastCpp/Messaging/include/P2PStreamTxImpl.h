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
 * P2PStreamTxImpl.h
 *
 *  Created on: Jul 2, 2012
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamTxImpl.h,v $
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

#ifndef P2PSTREAMTXIMPL_H_
#define P2PSTREAMTXIMPL_H_


#include "P2PStreamTx.h"
#include "P2PStreamEventListener.h"

#include "PropertyMap.h"
#include "SpiderCastConfigImpl.h"
#include "NodeIDCache.h"
#include "CoreInterface.h"
#include "MessagingManager.h"

#include "ScTraceContext.h"

namespace spdr
{

namespace messaging
{

class P2PStreamTxImpl: public spdr::messaging::P2PStreamTx,
		public ScTraceContext
{
private:
	static ScTraceComponent* _tc;

public:
	//P2PStreamTxImpl();


	P2PStreamTxImpl(
				const String& instID,
				const SpiderCastConfigImpl& config,
				NodeIDCache& nodeIDCache,
				CoreInterface& coreInterface,
				P2PStreamEventListener& eventListener,
				const PropertyMap& propMap,
				StreamID_SPtr streamID,
				NodeID_SPtr target,
				Neighbor_SPtr neighbor);

	virtual ~P2PStreamTxImpl();

	NodeID_SPtr getTarget() const;

	StreamID_SPtr getStreamID() const;

	int64_t submitMessage(const TxMessage& message);

	void close();

	bool isOpen() const;

	std::string toString() const;

		//=====================================================

	protected:

		const String& _instID;
		const SpiderCastConfigImpl& _config;
		NodeIDCache& _nodeIDCache;
		CoreInterface& _coreInterface;
		P2PStreamEventListener& _eventListener;
		const PropertyMap& _propMap;
		StreamID_SPtr _streamID;
		NodeID_SPtr _target;
		Neighbor_SPtr _neighbor;

		mutable boost::recursive_mutex _mutex;
		bool _closed;
		bool _init_done;

		MessagingManager_SPtr messagingManager_SPtr;

		int64_t _next_msg_num;
		SCMessage_SPtr _outgoingDataMsg;
		std::size_t _header_size;
};

typedef boost::shared_ptr<P2PStreamTxImpl> P2PStreamTxImpl_SPtr;

}

}

#endif /* P2PSTREAMTXIMPL_H_ */
