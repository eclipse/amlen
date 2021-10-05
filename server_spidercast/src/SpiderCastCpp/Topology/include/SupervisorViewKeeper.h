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
 * SupervisorCViewKeeper.h
 *
 *  Created on: 09/02/2011
 *      Author:
 *
 * 	Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: SupervisorViewKeeper.h,v $
 * Revision 1.2  2015/11/18 08:37:00 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.6  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.5  2011/03/31 13:59:03 
 * no functional change
 *
 * Revision 1.4  2011/02/28 18:13:17 
 * Implement foreign zone remote requests
 *
 * Revision 1.3  2011/02/21 07:55:49 
 * Add includeAttributes to the supervisor attributes
 *
 * Revision 1.2  2011/02/16 20:08:34 
 * foreign zone membership task - take 1
 *
 * Revision 1.1  2011/02/10 13:28:27 
 * Initial version
 *
  */

#ifndef SUPERVISORVIEWKEEPER_H_
#define SUPERVISORVIEWKEEPER_H_

#include <boost/thread/recursive_mutex.hpp>

#include "SCMembershipListener.h"
#include "MembershipEvent.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class SupervisorViewKeeper: public ScTraceContext
{
private:
	static ScTraceComponent* _tc;

	String _myNodeName;
	SCViewMap_SPtr _baseZoneView;

	mutable boost::recursive_mutex _mutex;

public:
	SupervisorViewKeeper(String myNodeName);
	virtual ~SupervisorViewKeeper();

	void onMembershipEvent(const SCMembershipEvent& event);

	/**
	 * A shallow copy.
	 * The map is a new instance so it can be iterated with no danger.
	 * The keys and values are shared pointers pointing to the same objects as in the internal map.
	 *
	 * @return
	 */
	SCViewMap_SPtr getSCView() const;

	boost::shared_ptr<event::ViewMap > getView(bool includeAttributes);
	static boost::shared_ptr<event::ViewMap > convertSCtoViewMap(SCViewMap_SPtr incomingView);

	int getViewSize() const;
};

}

#endif //SUPERVISORVIEWKEEPER_H_
