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

#ifndef MCP_HASRETURNCODE_H_
#define MCP_HASRETURNCODE_H_

#include "MCPReturnCode.h"

namespace mcp
{

class HasReturnCode
{
public:
	virtual ~HasReturnCode();

	virtual MCPReturnCode getReturnCode();

protected:
	HasReturnCode(const MCPReturnCode returnCode = ISMRC_Error);

protected:
	MCPReturnCode rc;

};

} /* namespace mcp */

#endif /* MCP_HASRETURNCODE_H_ */
