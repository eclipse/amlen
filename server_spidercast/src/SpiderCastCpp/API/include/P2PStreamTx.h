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
 * P2PSreamTx.h
 *
 *  Created on: Jun 27, 2012
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamTx.h,v $
 * Revision 1.3  2015/11/18 08:36:57 
 * Update copyright headers
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:11 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:06 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2012/10/10 15:33:36 
 * *** empty log message ***
 *
 * Revision 1.1  2012/06/28 09:33:15 
 * P2P streams interface first version
 *
 */

#ifndef P2PSREAMTX_H_
#define P2PSREAMTX_H_

#include <boost/cstdint.hpp>

#include "NodeID.h"
#include "StreamID.h"
#include "TxMessage.h"

namespace spdr
{

namespace messaging
{

class P2PStreamTx
{
protected:
	P2PStreamTx();

public:

	virtual ~P2PStreamTx() = 0;

	virtual NodeID_SPtr getTarget() const = 0;

	virtual StreamID_SPtr getStreamID() const = 0;

	virtual int64_t submitMessage(const TxMessage& message) = 0;

	virtual void close() = 0;

	virtual bool isOpen() const = 0;

	virtual std::string toString() const = 0;


};

typedef boost::shared_ptr<P2PStreamTx> P2PStreamTx_SPtr;

}

}

#endif /* P2PSREAMTX_H_ */
