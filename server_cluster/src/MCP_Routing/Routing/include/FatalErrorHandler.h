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

#ifndef FATALERRORHANDLER_H_
#define FATALERRORHANDLER_H_

#include <string>

namespace mcp
{

/**
 * Called by internal threads to signal internal fatal errors.
 * Moves the state of the entire component to "Error", and may close internal resources.
 *
 */
class FatalErrorHandler
{
public:
	FatalErrorHandler();
	virtual ~FatalErrorHandler();

	/**
	 * Called by internal threads to signal internal fatal errors.
	 * Moves the state of the entire component to "Error", and may close internal resources.
	 *
	 * @param component
	 * @param errorMessage
	 * @param rc
	 * @return
	 */
	virtual int onFatalError(const std::string& component, const std::string& errorMessage, int rc) = 0;

	virtual int onFatalError_MaintenanceMode(const std::string& component, const std::string& errorMessage, int rc, int restartFlag) = 0;
};

} /* namespace spdr */

#endif /* FATALERRORHANDLER_H_ */
