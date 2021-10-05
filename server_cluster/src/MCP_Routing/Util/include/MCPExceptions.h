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

#ifndef MCP_MCPEXCEPTION_H_
#define MCP_MCPEXCEPTION_H_

#include "SpiderCastLogicError.h"
#include "SpiderCastRuntimeError.h"
#include "HasErrorCode.h"

namespace mcp
{

class MCPLogicError : public spdr::SpiderCastLogicError, public HasReturnCode
{

public:
    /**
     * Construct with a "what" string.
     *
     * Return code is ISMRC_Error.
     *
     * @param whatString
     * @return
     */
    MCPLogicError(const std::string& whatString);

    /**
     * Construct with a "what" string and return code.
     *
     * @param whatString
     * @param rc
     */
    MCPLogicError(const std::string& whatString, const MCPReturnCode rc);


    /**
     * Destructor.
     * @return
     */
    virtual ~MCPLogicError() throw (); //throw nothing
};

//===================================================================================
//
//class MCPBadConfigurationError : public MCPLogicError
//{
//
//public:
//	/**
//	 * Construct with a "what" string.
//	 *
//	 * Return code is ISMRC_Error.
//	 *
//	 * @param whatString
//	 * @return
//	 */
//	MCPBadConfigurationError(const std::string& whatString);
//
//	/**
//	 * Construct with a "what" string and return code.
//	 *
//	 * @param whatString
//	 * @param rc
//	 */
//	MCPBadConfigurationError(const std::string& whatString, const MCPReturnCode rc);
//
//
//	/**
//	 * Destructor.
//	 * @return
//	 */
//	virtual ~MCPBadConfigurationError() throw (); //throw nothing
//};

//===================================================================================

class MCPIllegalStateError : public MCPLogicError
{

public:
	/**
	 * Construct with a "what" string.
	 *
	 * Return code is ISMRC_Error.
	 *
	 * @param whatString
	 * @return
	 */
	MCPIllegalStateError(const std::string& whatString);

	/**
	 * Construct with a "what" string and return code.
	 *
	 * @param whatString
	 * @param rc
	 */
	MCPIllegalStateError(const std::string& whatString, const MCPReturnCode rc);


	/**
	 * Destructor.
	 * @return
	 */
	virtual ~MCPIllegalStateError() throw (); //throw nothing
};

//====================================================================================

class MCPIndexOutOfBoundsError : public MCPLogicError
{

public:
	/**
	 * Construct with a "what" string.
	 *
	 * Return code is ISMRC_Error.
	 *
	 * @param whatString
	 * @return
	 */
	MCPIndexOutOfBoundsError(const std::string& whatString);

	/**
	 * Construct with a "what" string and return code.
	 *
	 * @param whatString
	 * @param rc
	 */
	MCPIndexOutOfBoundsError(const std::string& whatString, const MCPReturnCode rc);


	/**
	 * Destructor.
	 * @return
	 */
	virtual ~MCPIndexOutOfBoundsError() throw (); //throw nothing
};

//====================================================================================

class MCPIllegalArgumentError : public MCPLogicError
{

public:
	/**
	 * Construct with a "what" string.
	 *
	 * Return code is ISMRC_Error.
	 *
	 * @param whatString
	 * @return
	 */
	MCPIllegalArgumentError(const std::string& whatString);

	/**
	 * Construct with a "what" string and return code.
	 *
	 * @param whatString
	 * @param rc
	 */
	MCPIllegalArgumentError(const std::string& whatString, const MCPReturnCode rc);


	/**
	 * Destructor.
	 * @return
	 */
	virtual ~MCPIllegalArgumentError() throw (); //throw nothing
};

//====================================================================================

class MCPRuntimeError : public spdr::SpiderCastRuntimeError, public HasReturnCode
{

public:
	/**
	 * Construct with a "what" string.
	 *
	 * Return code is ISMRC_Error.
	 *
	 * @param whatString
	 * @return
	 */
	MCPRuntimeError(const std::string& whatString);

	/**
	 * Construct with a "what" string and return code.
	 *
	 * @param whatString
	 * @param rc
	 */
	MCPRuntimeError(const std::string& whatString, const MCPReturnCode rc);


	/**
	 * Destructor.
	 * @return
	 */
	virtual ~MCPRuntimeError() throw (); //throw nothing
};


} /* namespace mcp */

#endif /* MCP_MCPEXCEPTION_H_ */
