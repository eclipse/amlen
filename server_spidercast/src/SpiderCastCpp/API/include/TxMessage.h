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
 * TxMessage.h
 *
 *  Created on: Jan 26, 2012
 */

#ifndef TXMESSAGE_H_
#define TXMESSAGE_H_

#include <boost/shared_ptr.hpp>

#include "Definitions.h"
#include "SpiderCastRuntimeError.h"

namespace spdr
{

namespace messaging
{

/**
 * An outgoing message.
 *
 */
class TxMessage
{
public:
	TxMessage();

	virtual ~TxMessage();

	inline void setBuffer(const Const_Buffer& buffer)
	{
		buffer_ = buffer;
	}

	inline const Const_Buffer& getBuffer() const
	{
		return buffer_;
	}

private:
	Const_Buffer buffer_;
};

typedef boost::shared_ptr<TxMessage> TxMessage_SPtr;

}

}

#endif /* TXMESSAGE_H_ */
