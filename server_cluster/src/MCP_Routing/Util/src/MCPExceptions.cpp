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

#include "MCPExceptions.h"

namespace mcp
{


MCPLogicError::MCPLogicError(const std::string& whatString) :
        spdr::SpiderCastLogicError(whatString),
        HasReturnCode(ISMRC_Error)
{
}

MCPLogicError::MCPLogicError(const std::string& whatString, const MCPReturnCode rc) :
        spdr::SpiderCastLogicError(whatString),
        HasReturnCode(rc)
{
}

MCPLogicError::~MCPLogicError() throw()
{
}

//=====================================================================================================
//
//
//MCPBadConfigurationError::MCPBadConfigurationError(const std::string& whatString) :
//		MCPLogicError(whatString)
//{
//}
//
//MCPBadConfigurationError::MCPBadConfigurationError(const std::string& whatString, const MCPReturnCode rc) :
//		MCPLogicError(whatString, rc)
//{
//}
//
//MCPBadConfigurationError::~MCPBadConfigurationError() throw()
//{
//}

//=====================================================================================================

MCPIllegalStateError::MCPIllegalStateError(const std::string& whatString) :
        MCPLogicError(whatString)
{
}

MCPIllegalStateError::MCPIllegalStateError(const std::string& whatString, const MCPReturnCode rc) :
        MCPLogicError(whatString, rc)
{
}

MCPIllegalStateError::~MCPIllegalStateError() throw()
{
}

//=====================================================================================================

MCPIndexOutOfBoundsError::MCPIndexOutOfBoundsError(const std::string& whatString) :
        MCPLogicError(whatString)
{
}

MCPIndexOutOfBoundsError::MCPIndexOutOfBoundsError(const std::string& whatString, const MCPReturnCode rc) :
        MCPLogicError(whatString, rc)
{
}

MCPIndexOutOfBoundsError::~MCPIndexOutOfBoundsError() throw()
{
}

//=====================================================================================================

MCPIllegalArgumentError::MCPIllegalArgumentError(const std::string& whatString) :
        MCPLogicError(whatString)
{
}

MCPIllegalArgumentError::MCPIllegalArgumentError(const std::string& whatString, const MCPReturnCode rc) :
        MCPLogicError(whatString, rc)
{
}

MCPIllegalArgumentError::~MCPIllegalArgumentError() throw()
{
}

//=====================================================================================================

MCPRuntimeError::MCPRuntimeError(const std::string& whatString) :
		spdr::SpiderCastRuntimeError(whatString),
		HasReturnCode(ISMRC_Error)
{
}

MCPRuntimeError::MCPRuntimeError(const std::string& whatString, const MCPReturnCode rc) :
		spdr::SpiderCastRuntimeError(whatString),
		HasReturnCode(rc)
{
}

MCPRuntimeError::~MCPRuntimeError() throw()
{
}

} /* namespace mcp */
