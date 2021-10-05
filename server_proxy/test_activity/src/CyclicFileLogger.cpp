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

#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "CyclicFileLogger.h"

namespace pxcluster
{

CyclicFileLogger::CyclicFileLogger(
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
		throw std::logic_error("number of files must be: 1 <= n <=100");
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
			throw std::logic_error("maximal file size must be >=4kB");
		}

		currentFileIndex_ = 1;
		trFileName_ = generateFileName(currentFileIndex_);
	}

	out_.reset(new std::ofstream(trFileName_.c_str()));
	if (!isOpen())
	{
	    std::cout << "CyclicFileLogger logger could not open file: " << trFileName_ << "; trace will be directed to STDOUT" << std::endl;
	}
}

CyclicFileLogger::~CyclicFileLogger()
{
	out_->flush();
	out_->close();
}

bool CyclicFileLogger::isOpen()
{
    boost::recursive_mutex::scoped_lock lock(mutex_);

    if (out_)
        return out_->is_open();
    else
        return false;
}

std::string CyclicFileLogger::generateFileName(int index)
{
	std::string fileName = trFileNameBase_;
	fileName.append("_P");
	fileName.append(boost::lexical_cast<std::string>(index));
	fileName.append(".");
	fileName.append(trFileNameSuffix_);
	return fileName;
}

void CyclicFileLogger::print(int log_level, const char *id, const char *message)
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
        case 0:
		case 1:
			c = 'E';
			break;
		case 2:
			c = 'W';
			break;
		case 3:
			c = 'I';
			break;
		case 4:
			c = 'C';
			break;
		case 5:
			c = 'e';
			break;
		case 6:
			c = 'd';
			break;
		case 7:
			c = 'x';
			break;
        case 8:
        case 9:
            c = 'p';
            break;

		default:
			c = '?';
			break;
		}
		strBuff << id << ' ' << c << ' ' << message << std::endl;

		if (isOpen())
		{
		    (*out_) << strBuff.str();

		    if (((int)numFiles_ > (int)1) && ((std::size_t)out_->tellp() > (std::size_t)maxFileSizeBytes_))
		    {
		        switchFiles();
		    }
		}
		else
		{
		    std::cout << strBuff.str();
		}
	} 	//end_synchronized_block

}

void CyclicFileLogger::switchFiles()
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

std::string CyclicFileLogger::getFileName() const
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	return trFileName_;
}

}
