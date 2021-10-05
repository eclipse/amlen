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


#include "PXActivityUnitTestTraceInit.h"

#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>

pxcluster::CyclicFileLogger* PXClusterUnitTestTraceInit::cfll_ptr = NULL;
std::string PXClusterUnitTestTraceInit::mcpTraceDir = "./pxcluster_unittest_trace";
int PXClusterUnitTestTraceInit::mcpTraceLevel = 0;

extern "C"
{

ism_trclevel_t mcp_unittest_defaultTrace;
ism_trclevel_t * ism_defaultTrace = &mcp_unittest_defaultTrace;
ism_common_TraceFunction_t traceFunction = NULL;

void pxcluster_unittest_traceFunction(int level, int opt, const char * file,
		int lineno, const char * fmt, ...)
{
	//std::cout << ">>>" << level << " " << opt << " " << file << " " << lineno << fmt << std::endl;

	std::ostringstream oss_id;
	oss_id << "f:" << file << " l:" << lineno;

	va_list ap;
	va_start(ap,fmt);

	char* buff = NULL;
	int buff_size = vasprintf(&buff, fmt, ap);

	if (buff_size > 0)
	{
	    PXClusterUnitTestTraceInit::cfll_ptr->onLogEvent(level, oss_id.str().c_str(), buff);
		free(buff);
	}
	else
	{
	    PXClusterUnitTestTraceInit::cfll_ptr->onLogEvent(level, oss_id.str().c_str(), "Error formating message");
	}

//
//
//	int msg_sz = 8*1024;
//	char msg[msg_sz];
//
//	//vsprintf(msg,fmt,ap);
//	int actual_len = snprintf(msg, msg_sz, fmt, ap);
//	if (actual_len < msg_sz && actual_len > 0)
//	{
//		MCPUnitTestTraceInit::cfll_ptr->onLogEvent(log_level, oss_id.str().c_str(), msg);
//	}
//	else if (actual_len >= msg_sz)
//	{
//		char* buff = new char[actual_len+1];
//
//		MCPUnitTestTraceInit::cfll_ptr->onLogEvent(log_level, oss_id.str().c_str(), "Message too long");
//	}
}

}

PXClusterUnitTestTraceInit::PXClusterUnitTestTraceInit()
{
	using namespace std;

	cout << "PXActivity UnitTest Trace Init:" << endl;

	// Init the ISM tracer
	traceFunction = pxcluster_unittest_traceFunction;

	//string trDirName;

	char * trace_dir = getenv("PXA_UNITTEST_TRACE_DIR");
	if (trace_dir)
	{
		cout << "value of PXA_UNITTEST_TRACE_DIR is: " << trace_dir << endl;
		mcpTraceDir = string(trace_dir);
//		trDirName.append(trace_dir);
//		mcpTraceDir = trDirName;
	}
	else
	{
		cout << "variable PXA_UNITTEST_TRACE_DIR not defined." << endl;
		cout << "default: pxcTraceDir=" << mcpTraceDir << endl;
//		trDirName.append(mcpTraceDir);
	}

	char * trace_level = getenv("PXA_UNITTEST_TRACE_LEVEL");
	if (trace_level)
	{
        cout << "value of PXA_UNITTEST_TRACE_LEVEL is: " << trace_level << endl;
        int trLevel = boost::lexical_cast<int>(trace_level);
        if (trLevel >=0 && trLevel <= 9)
        {
            mcpTraceLevel = trLevel;
        }
        else
        {
            cout << "value of PXA_UNITTEST_TRACE_LEVEL is out of range [0,9]: " << trace_level << endl;
        }
	}
	else
	{
	    cout << "variable PXA_UNITTEST_TRACE_LEVEL not defined." << endl;
	    cout << "default: mcpTraceLevel=" << mcpTraceLevel << endl;
	}

	memset(ism_defaultTrace->trcComponentLevels, mcpTraceLevel, TRACECOMP_MAX);

	string trFileName = mcpTraceDir + "/trace_";
	trFileName.append(boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time()));
	string trFileNameSuffix("log");

	//50 file, 20M each
	cfll_ptr = new pxcluster::CyclicFileLogger(trFileName.c_str(), trFileNameSuffix.c_str(), 50, 20000);

	cout << "Trace File: " << cfll_ptr->getFileName() << (cfll_ptr->isOpen() ? " is open" : " is NOT open") << endl << endl;

	start = boost::posix_time::microsec_clock::universal_time();
}

PXClusterUnitTestTraceInit::~PXClusterUnitTestTraceInit()
{
	boost::posix_time::ptime time = boost::posix_time::microsec_clock::universal_time();
	std::cout << "Test duration: " << boost::posix_time::to_simple_string(time-start) << std::endl;

	std::cout << "~PXClusterUnitTestTraceInit() DONE." << std::endl;

	delete cfll_ptr;
	//delete sll_ptr;
}

void PXClusterUnitTestTraceInit::changeTraceLevel(int level)
{
    if (level >=0 && level <=9)
    {
        if (level <= mcpTraceLevel) //never increase above the default
        {
            memset(ism_defaultTrace->trcComponentLevels, level, TRACECOMP_MAX);
        }
    }
}
