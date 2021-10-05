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
 * HierarchySupervisorTaskInterface.h
 *
 *  Created on: Nov 11, 2010
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisorTaskInterface.h,v $
 * Revision 1.3  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:16 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.7  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.6  2011/03/03 06:56:32 
 * Add support for foreign zone membership requests timeout
 *
 * Revision 1.5  2011/02/16 20:08:34 
 * foreign zone membership task - take 1
 *
 * Revision 1.4  2011/02/14 13:53:08 
 * Add the zone census task
 *
 */

#ifndef HIERARCHYSUPERVISORTASKINTERFACE_H_
#define HIERARCHYSUPERVISORTASKINTERFACE_H_

#include "BusName.h"

namespace spdr
{

class HierarchySupervisorTaskInterface
{
public:
	HierarchySupervisorTaskInterface();
	virtual ~HierarchySupervisorTaskInterface();

	virtual void zoneCensusTask() = 0;
	virtual void setActiveDelegatesTask() = 0;
	virtual void foreignZoneMembershipTask(int64_t reqId, BusName_SPtr zoneBusName, bool includeAttributes) = 0;
	virtual void foreignZoneMembershipTOTask(int64_t reqId, String zoneBusName) = 0;

};

}

#endif /* HIERARCHYSUPERVISORTASKINTERFACE_H_ */
