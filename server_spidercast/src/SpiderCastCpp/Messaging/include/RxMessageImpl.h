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
 * RxMessageImpl.h
 *
 *  Created on: Feb 8, 2012
 */

#ifndef RXMESSAGEIMPL_H_
#define RXMESSAGEIMPL_H_

#include "RxMessage.h"

namespace spdr
{

namespace messaging
{

//TODO add setter for BusName
class RxMessageImpl : public RxMessage
{
public:
	RxMessageImpl();
	RxMessageImpl(const RxMessageImpl& other);
	RxMessageImpl& operator=(const RxMessageImpl& other);
	virtual ~RxMessageImpl();

	virtual void setBuffer(const int32_t length, const char* buff);

	virtual void setBuffer(const Const_Buffer& buffer);

	virtual void setMessageID(const int64_t messageID);

	virtual void setStreamID(const StreamID_SPtr& sid);

	virtual void setTopic(const Topic_SPtr& topic);

	virtual void setSource(const NodeID_SPtr& topic);

	virtual void reset();
};

}

}

#endif /* RXMESSAGEIMPL_H_ */
