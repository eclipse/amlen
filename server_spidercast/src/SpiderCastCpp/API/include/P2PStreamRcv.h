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
 * P2PStreamRcv.h
 *
 *  Created on: Jun 27, 2012
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamRcv.h,v $
 * Revision 1.3  2015/11/18 08:36:57 
 * Update copyright headers
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:11 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:05 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2012/10/10 15:33:36 
 * *** empty log message ***
 *
 * Revision 1.1  2012/06/28 09:33:15 
 * P2P streams interface first version
 *
 */

#ifndef P2PSTREAMRCV_H_
#define P2PSTREAMRCV_H_

#include "StreamID.h"

namespace spdr
{

namespace messaging
{

class P2PStreamRcv
{
protected:
	P2PStreamRcv();

public:

	virtual ~P2PStreamRcv() = 0;

	virtual void close() = 0;

	virtual bool isOpen() = 0;

	virtual void rejectStream(StreamID& sid) = 0;

	virtual std::string toString() const = 0;


};

typedef boost::shared_ptr<P2PStreamRcv> P2PStreamRcv_SPtr;
}

}

#endif /* P2PSTREAMRCV_H_ */

