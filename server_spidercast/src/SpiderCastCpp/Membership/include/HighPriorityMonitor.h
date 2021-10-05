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
 * HighPriorityMonitor.h
 *
 *  Created on: May 7, 2012
 */

#ifndef HIGHPRIORITYMONITOR_H_
#define HIGHPRIORITYMONITOR_H_

#include <boost/noncopyable.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "SCMembershipListener.h"
#include "SpiderCastConfigImpl.h"
#include "SCMessage.h"
#include "CommAdapter.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class HighPriorityMonitor: boost::noncopyable,
		public SCMembershipListener,
		public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	HighPriorityMonitor(const String& instID,
			const SpiderCastConfigImpl& config,
			CommAdapter_SPtr commAdapter);

	virtual ~HighPriorityMonitor();

	/*
	 * @see spdr::SCMembershipListener
	 */
	void onMembershipEvent(const SCMembershipEvent& event);

//	typedef boost::unordered_set<NodeIDImpl_SPtr,
//			NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals> NodeIDImplSet;

	void send2Monitors(SCMessage_SPtr message);

private:

	const String hpmKey_;

	CommAdapter_SPtr commAdapter_;

	typedef std::set<NodeID_SPtr, NodeID::SPtr_Less >					NodeIDSet;
	/* High priority monitors.
	 * Threading: MemTopo-thread only.
	 */
	NodeIDSet highPriorityMonitors_;

};

typedef boost::shared_ptr<HighPriorityMonitor> HighPriorityMonitor_SPtr;

}

#endif /* HIGHPRIORITYMONITOR_H_ */
