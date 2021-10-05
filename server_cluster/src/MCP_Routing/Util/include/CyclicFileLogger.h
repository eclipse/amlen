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

#ifndef CYCLICFILELOGLISTENER_H_
#define CYCLICFILELOGLISTENER_H_

#include <iosfwd>
#include <string>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "LogListener.h"

namespace mcp
{

/**
 * Log listener that writes to a file, with support for cyclic log files of limited size.
 *
 */
class CyclicFileLogger : public spdr::log::LogListener
{
public:
	/**
	 * Log listener that writes to a file, with support for cyclic log files of limited size.
	 *
	 * If numFiles==1, the class ignores the maxFileSizeKB parameter and works with one unlimited file.
	 * In this case the log file name is logFileNameBase + '.' + logFileNameSuffix.
	 *
	 * If numFiles>=2, no more than numFiles will exist at any given time,
	 * and each of these files will not significantly exceed maxFileSizeKB.
	 * In this case the log file names are logFileNameBase + '_P#.' + logFileNameSuffix,
	 * where # is the index of the file, starting from 1, in increasing order.
	 *
	 * @param logFileNameBase e.g. "trace_node1"
	 * @param logFileNameSuffix e.g. "log"
	 * @param numFiles
	 * @param maxFileSizeKB
	 * @return
	 */
	CyclicFileLogger(const char* logFileNameBase, const char* logFileNameSuffix,
			int numFiles, int maxFileSizeKB = 10000);

	virtual ~CyclicFileLogger();

	bool isOpen();

	std::string getFileName() const;

	/*
	 * @see spdr::LogListener
	 */
	void onLogEvent(spdr::log::Level log_level, const char *id, const char *message)
	{
		print(log_level,id,message);
	}

	static void logListener(void *user, int log_level, const char *id,
			const char *message)
	{
		CyclicFileLogger* listener = (CyclicFileLogger*) user;
		listener->print(static_cast<spdr::log::Level>(log_level), id, message);
	}

private:
	const int numFiles_;
	const size_t maxFileSizeBytes_;

	const std::string trFileNameBase_;
	const std::string trFileNameSuffix_;

	std::string trFileName_;
	boost::shared_ptr<std::ofstream> out_;

	mutable boost::recursive_mutex mutex_;

	int currentFileIndex_;

	void print(spdr::log::Level log_level, const char *id, const char *message);

	std::string generateFileName(int index);

	void switchFiles();
};
}
#endif /* CYCLICFILELOGLISTENER_H_ */
