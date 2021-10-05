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
 * NodeInfo.h
 *
 *  Created on: 21/04/2010
 */

#ifndef NODEINFO_H_
#define NODEINFO_H_

#include <boost/date_time/posix_time/posix_time.hpp>

#include "NodeVersion.h"
#include "SuspicionList.h"
#include "AttributeTable.h"

namespace spdr
{

/**
 * A value in the view map.
 *
 * Holds the version, suspicion-list, and the attribute table (etc.) of a node.
 */
class NodeInfo
{
public:
	/**
	 * Adding a NodeInfo to the view.
	 * Allocates an empty attribute table, set status to STATUS_ALIVE.
	 */
	NodeInfo();

	/**
	 * Adding a NodeInfo to the history.
	 * Does NOT allocate an attribute table.
	 *
	 * @param nodeVersion
	 * @param timeStamp
	 */
	NodeInfo(
			const NodeVersion& nodeVersion,
			spdr::event::NodeStatus status,
			const boost::posix_time::ptime& timeStamp);

	virtual ~NodeInfo();

	NodeVersion nodeVersion;
	SuspicionList suspicionList;
	AttributeTable_SPtr attributeTable;
	spdr::event::NodeStatus status;
	boost::posix_time::ptime timeStamp;
};

}

#endif /* NODEINFO_H_ */
