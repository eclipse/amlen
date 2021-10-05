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
 * CommEventInfo.cpp
 *
 *  Created on: 28/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.7 $
 *-----------------------------------------------------------------
 * $Log: CommEventInfo.cpp,v $
 * Revision 1.7  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.6  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.5  2015/07/30 12:14:35 
 * split brain
 *
 * Revision 1.4  2015/07/29 09:26:11 
 * split brain
 *
 * Revision 1.3  2015/06/25 10:26:45 
 * readiness for version upgrade
 *
 * Revision 1.2  2015/05/05 12:48:26 
 * Safe connect
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.12  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.11  2011/05/02 10:38:16 
 * Initialize the context to -1, so we can properly identify general comm events
 *
 * Revision 1.10  2011/01/13 10:13:57 
 * Changed toStaring(); no functional change
 *
 * Revision 1.9  2011/01/10 12:26:00 
 * Add toString()
 *
 * Revision 1.8  2010/11/25 16:15:05 
 * After merge of comm changes back to head
 *
 * Revision 1.7.2.1  2010/10/21 16:10:42 
 * Massive comm changes
 *
 * Revision 1.7  2010/08/03 13:56:56 
 * Add support for on_success and on_failure connection events to be passed in through the incoming msg Q
 *
 * Revision 1.6  2010/06/23 14:17:32 
 * No functional chnage
 *
 * Revision 1.5  2010/06/20 14:40:57 
 * Change the connection info to consist of the connection id
 *
 * Revision 1.4  2010/06/17 11:43:53 
 * Add connection info. For debugging purposes
 *
 * Revision 1.3  2010/05/20 09:15:14 
 * Initial version
 *
 */

#include "CommEventInfo.h"

using namespace std;

namespace spdr
{

const String CommEventInfo::EventTypeName[] = {
		"New_Source",
		"On_Break",
		"On_Success",
		"On_Failure",
		"Warning_Connection_Refused",
		"Warning_UDP_Message_Refused",
		"Suspicion_Duplicate_Remote_Node",
		"Fatal_Error"};

CommEventInfo::CommEventInfo(EventType eventType, uint64_t connectionId,
		Neighbor_SPtr neighbor) :
	_eType(eventType),
	_context(-1),
	_connectionId(connectionId),
	_neighbor(neighbor),
	_errCode(0),
	_errMsg(),
	_incNum(-1)
{
}

CommEventInfo::~CommEventInfo()
{
}

CommEventInfo::EventType CommEventInfo::getType() const
{
	return _eType;
}

uint64_t CommEventInfo::getConnectionId() const
{
	return _connectionId;
}

Neighbor_SPtr CommEventInfo::getNeighbor() const
{
	return _neighbor;
}

int CommEventInfo::getContext() const
{
	return _context;
}

int CommEventInfo::getErrCode() const
{
	return _errCode;
}
String CommEventInfo::getErrMsg() const
{
	return _errMsg;
}
int64_t CommEventInfo::getIncNum() const
{
	return _incNum;
}
void CommEventInfo::setErrCode(int code)
{
	_errCode = code;
}
void CommEventInfo::setErrMsg(String msg)
{
	_errMsg = msg;
}

void CommEventInfo::setContext(int context)
{
	_context = context;
}
void CommEventInfo::setIncNum(int64_t incNum)
{
	_incNum = incNum;
}

String CommEventInfo::toString() const
{
	ostringstream oss;

	oss << "Event type: " << EventTypeName[_eType] << "; context=" << _context
			<< "; connection id=" << _connectionId << "; errCode=" << _errCode
			<< "; errMsg=" << _errMsg << "; incNum=" << _incNum;
	return oss.str();
}
}
