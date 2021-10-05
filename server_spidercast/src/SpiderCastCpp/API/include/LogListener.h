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
 * LogListener.h
 *
 *  Created on: Jun 28, 2012
 */

#ifndef SPDR_LOG_LOGLISTENER_H_
#define SPDR_LOG_LOGLISTENER_H_

namespace spdr
{
namespace log
{

/**
 * Log level.
 *
 * Defines the logging level from highest priority, least detailed (ERROR) to
 * lowest priority most detailed (DUMP).
 *
 * An enum with higher priority (ERROR) receiving lower numbers.
 *
 * The lowest number (NONE) disables logging completely.
 */
enum Level
{
	Level_NONE    = 0, 			//!< Level_NONE
	Level_ERROR   = 1, 			//!< Level_ERROR
	Level_WARNING = 2, 			//!< Level_WARNING
	Level_INFO    = 3, 			//!< Level_INFO
	Level_CONFIG  = 4, 			//!< Level_CONFIG
	Level_EVENT   = 5, 			//!< Level_EVENT
	Level_DEBUG   = 6, 			//!< Level_DEBUG
	Level_ENTRY   = 7, 			//!< Level_ENTRY
	Level_DUMP    = 8, 			//!< Level_DUMP
	Level_ALL     = Level_DUMP  //!< Level_ALL
};

/**
 * An interface to receive internal log events.
 */
class LogListener
{

public:

	virtual ~LogListener();

	/**
	 * Called when a log event is delivered to the user.
	 *
	 * @param log_level
	 * @param id
	 * @param message
	 */
	virtual void
			onLogEvent(Level log_level, const char *id, const char *message) = 0;

protected:
	LogListener();
};

}
}

#endif /* SPDR_LOG_LOGLISTENER_H_ */
