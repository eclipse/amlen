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
 * SupervisorNeighborTable.h
 *
 *  Created on: Feb 2, 2011
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: SupervisorNeighborTable.h,v $
 * Revision 1.3  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:16 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.9  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.8  2012/07/17 09:19:04 
 * add pub/sub bridges
 *
 * Revision 1.7  2011/03/31 13:58:26 
 * Make a single active delegate. Turn SupervisorViewKeeper into a shared pointer
 *
 * Revision 1.6  2011/02/28 18:13:05 
 * Implement foreign zone remote requests
 *
 * Revision 1.5  2011/02/21 07:55:49 
 * Add includeAttributes to the supervisor attributes
 *
 * Revision 1.4  2011/02/20 12:01:01 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.3  2011/02/16 20:08:34 
 * foreign zone membership task - take 1
 *
 * Revision 1.2  2011/02/10 13:28:19 
 * Added supervisor view keeper issues
 *
 * Revision 1.1  2011/02/07 14:57:24 
 * Initial version
 *
 */

#ifndef SUPERVISORNEIGHBORTABLE_H_
#define SUPERVISORNEIGHBORTABLE_H_

#include <vector>

#include "NeighborTable.h"
#include "SupervisorViewKeeper.h"

namespace spdr
{

class SupervisorNeighborTable;
typedef boost::shared_ptr<SupervisorNeighborTable> SupervisorNeighborTable_SPtr;

class SupervisorNeighborTable: public spdr::NeighborTable
{
public:
	SupervisorNeighborTable(String myName, String tableName, const String& instID);
	virtual ~SupervisorNeighborTable();

	int getNumActiveDelegates();
	NodeIDImpl_SPtr getAnActiveDelegateCandidate();

	void setActiveDelegate(NodeIDImpl_SPtr targetName);
	/**
	 *
	 * @param targetName
	 * @return true if target was indeed the active delegate
	 */
	bool setInactiveDelegate(NodeIDImpl_SPtr targetName);

	void processViewEvent(const SCMembershipEvent& event);
	int getViewSize();
	SCViewMap_SPtr getSCView() const;
	boost::shared_ptr<event::ViewMap > getView(bool includeAttributes);
	bool isActiveDelegate(NodeIDImpl_SPtr targetName);
	bool hasActiveDelegate();

private:
	String 			_myName;
	NodeIDImpl_SPtr _activeDelegate;

	boost::shared_ptr<SupervisorViewKeeper> _viewKeeper;
};

}

#endif /* SUPERVISORNEIGHBORTABLE_H_ */
