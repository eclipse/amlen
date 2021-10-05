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
 * P2PRouter.h
 *
 *  Created on: Jan 25, 2012
 */

#ifndef P2PROUTER_H_
#define P2PROUTER_H_

#include <boost/noncopyable.hpp>

#include "NodeIDImpl.h"
#include "SCMessage.h"
#include "SpiderCastRuntimeError.h"

namespace spdr
{

namespace route
{

class P2PRouter : boost::noncopyable
{
public:
	P2PRouter();
	virtual ~P2PRouter();

	/**
	 * Route a message sent by this node.
	 *
	 * @param target
	 * @param message
	 * @return
	 */
	bool send(NodeIDImpl_SPtr target, SCMessage_SPtr message)
			throw (SpiderCastRuntimeError);
	/**
	 * Route a message that arrived from the network.
	 *
	 * @param message
	 * @return
	 */
	bool route(SCMessage_SPtr message) throw (SpiderCastRuntimeError);
};

}
}
#endif /* P2PROUTER_H_ */
