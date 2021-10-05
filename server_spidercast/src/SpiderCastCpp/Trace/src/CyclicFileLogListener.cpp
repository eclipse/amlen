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
 * CyclicFileLogListener.cpp
 *
 *  Created on: Mar 7, 2011
 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/date_time/posix_time/posix_time.hpp>

//#include "ScTrWriter.h"

#include "SpiderCastLogicError.h"
#include "CyclicFileLogListener.h"

namespace spdr
{

CyclicFileLogListener::CyclicFileLogListener(
		const char* logFileNameBase, const char* logFileNameSuffix,
		int numFiles, int maxFileSizeKB) :
		numFiles_(numFiles),
		maxFileSizeBytes_(1024*maxFileSizeKB),
		trFileNameBase_(logFileNameBase),
		trFileNameSuffix_(logFileNameSuffix),

		trFileName_(),
		out_(),
		mutex_(),
		currentFileIndex_(0)
{
	if (numFiles_ < 1 || numFiles_ > 100)
	{
		throw IllegalArgumentException("number of files must be: 1 <= n <=100");
	}

	if (numFiles_ == 1)
	{
		trFileName_ = trFileNameBase_;
		trFileName_.append(".");
		trFileName_.append(trFileNameSuffix_);
	}
	else
	{
		if (maxFileSizeKB < 4)
		{
			throw IllegalArgumentException("maximal file size must be >=4kB");
		}

		currentFileIndex_ = 1;
		trFileName_ = generateFileName(currentFileIndex_);
	}

	out_.reset(new std::ofstream(trFileName_.c_str()));
}

CyclicFileLogListener::~CyclicFileLogListener()
{
	out_->flush();
	out_->close();
}

bool CyclicFileLogListener::isOpen()
{
	if (out_)
		return out_->is_open();
	else
		return false;
}

std::string CyclicFileLogListener::generateFileName(int index)
{
	std::string fileName = trFileNameBase_;
	fileName.append("_P");
	fileName.append(boost::lexical_cast<std::string>(index));
	fileName.append(".");
	fileName.append(trFileNameSuffix_);
	return fileName;
}

void CyclicFileLogListener::print(log::Level log_level, const char *id, const char *message)
{

	std::ostringstream strBuff;

	{	//start_synchronized_block
		boost::recursive_mutex::scoped_lock lock(mutex_);
		//boost::mutex::scoped_lock lock(mutex_);

		strBuff << "[";

		boost::posix_time::ptime time =
				boost::posix_time::microsec_clock::local_time();
		strBuff << boost::posix_time::to_iso_extended_string(time);

		strBuff << "] ";

		strBuff << boost::this_thread::get_id() << " ";
		strBuff.fill(' ');
		char c;

		switch (log_level)
		{
		case log::Level_ERROR:
			c = 'E';
			break;
		case log::Level_WARNING:
			c = 'W';
			break;
		case log::Level_INFO:
			c = 'I';
			break;
		case log::Level_CONFIG:
			c = 'C';
			break;
		case log::Level_EVENT:
			c = 'e';
			break;
		case log::Level_DEBUG:
			c = 'd';
			break;
		case log::Level_ENTRY:
			c = 'x';
			break;
		case log::Level_DUMP:
			c = 'p';
			break;
		default:
			c = '?';
			break;
		}
		strBuff << id << ' ' << c << ' ' << message << std::endl;
		(*out_) << strBuff.str();

		if (((int)numFiles_ > (int)1) && ((std::size_t)out_->tellp() > (std::size_t)maxFileSizeBytes_))
		{
			switchFiles();
		}
	} 	//end_synchronized_block

}

void CyclicFileLogListener::switchFiles()
{
	out_->flush();
	out_->close();
	trFileName_ = generateFileName( ++currentFileIndex_ );
	out_->open(trFileName_.c_str());
	if (currentFileIndex_-numFiles_ > 0)
	{
		std::string oldest_file = generateFileName((currentFileIndex_-numFiles_));

		std::ostringstream strBuff;
		strBuff << "[";
		boost::posix_time::ptime time =
				boost::posix_time::microsec_clock::local_time();
		strBuff << boost::posix_time::to_iso_extended_string(time) << "] ";
		strBuff << boost::this_thread::get_id() << " Removing trace file: " << oldest_file;

		if (remove(oldest_file.c_str()) !=0)
		{
			int n=errno;
			strBuff << " Failed, error code #" << n << ", " << strerror(n) << std::endl;
		}
		else
		{
			strBuff << std::endl;
		}

		(*out_) << strBuff.str();
	}
}

std::string CyclicFileLogListener::getFileName() const
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	//boost::mutex::scoped_lock lock(mutex_);
	return trFileName_;
}

}
