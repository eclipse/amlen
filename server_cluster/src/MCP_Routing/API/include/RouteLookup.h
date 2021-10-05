/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#ifndef ROUTELOOKUP_H_
#define ROUTELOOKUP_H_

#include "MCPTypes.h"
#include "MCPReturnCode.h"
#include "cluster.h"

namespace mcp
{

class RouteLookup
{
public:
	RouteLookup();
	virtual ~RouteLookup();

	/**
	 * Perform lookup on a given publication topic, the result is an array of
	 *  remote servers to which the message need to be forwarded.
	 *
	 * @param pLookupInfo
	 * @return
	 */
	virtual MCPReturnCode lookup(
			ismCluster_LookupInfo_t* pLookupInfo) = 0;


};

} /* namespace mcp */

#endif /* ROUTELOOKUP_H_ */
