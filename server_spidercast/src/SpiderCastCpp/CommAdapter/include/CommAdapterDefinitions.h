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
 * CommAdapterDefinitions.h
 *
 *  Created on: Feb 16, 2010
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: CommAdapterDefinitions.h,v $
 * Revision 1.2  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.6  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.5  2011/06/19 13:48:06 
 * *** empty log message ***
 *
 * Revision 1.4  2010/08/23 13:33:05 
 * Embed RUM log within ours
 *
 * Revision 1.3  2010/06/23 14:15:52 
 * No functional chnage
 *
 * Revision 1.2  2010/04/07 13:53:24 
 * Add CVS log
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 */

#ifndef COMMADAPTERDEFINITIONS_H_
#define COMMADAPTERDEFINITIONS_H_

#include <string>
#include <vector>

namespace spdr
{
namespace comm
{
	std::string endpointScopeMatch(
			const std::vector<std::pair<std::string, std::string> >& localAddr,
			const std::vector<std::pair<std::string, std::string> >& targetAddr);
}
}


class RumContext
{
public:
	RumContext(void)
	{
	}
	virtual ~RumContext()
	{
	}
};

static const int RUM_LOGGING_CATEGORY = 0x040000;

#endif /* COMMADAPTERDEFINITIONS_H_ */
