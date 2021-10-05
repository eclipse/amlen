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
 * NodeInfo.cpp
 *
 *  Created on: 21/04/2010
 */

#include "NodeInfo.h"

namespace spdr
{

NodeInfo::NodeInfo() :
				nodeVersion(),
				suspicionList(),
				attributeTable(new AttributeTable),
				status(spdr::event::STATUS_ALIVE),
				timeStamp()
{
}

NodeInfo::NodeInfo(
		const NodeVersion& nodeVersion,
		spdr::event::NodeStatus status,
		const boost::posix_time::ptime& timeStamp) :
				nodeVersion(nodeVersion),
				suspicionList(),
				attributeTable(),
				status(status),
				timeStamp(timeStamp)
{
}

NodeInfo::~NodeInfo()
{
}


}
