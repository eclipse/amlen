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
/*-----------------------------------------------------------------
 * File        : ScTr.cpp
 * Author      :
 * Version     : $Revision: 1.5 $
 * Last updated: $Date: 2015/11/18 08:37:04 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTr.cpp,v $
 * Revision 1.5  2015/11/18 08:37:04 
 * Update copyright headers
 *
 * Revision 1.4  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.3  2015/07/02 10:29:12 
 * clean trace, byte-buffer
 *
 * Revision 1.2  2015/06/30 13:26:45 
 * Trace 0-8 levels, trace levels by component
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.8  2012/10/25 10:11:18 
 * Added copyright and proprietary notice
 *
 * Revision 1.7  2011/02/27 12:20:21 
 * Refactoring, header file organization
 *
 * Revision 1.6  2011/01/09 07:00:57 
 * Formating, comments
 *
 * Revision 1.5  2010/07/13 13:22:12 
 * *** empty log message ***
 *
 * Revision 1.4  2010/06/27 12:38:47 
 * *** empty log message ***
 *
 * Revision 1.3  2010/06/17 14:07:57 
 * node attributes implementation
 *
 * Revision 1.2  2010/05/23 16:09:19 
 * Tracer - take two
 *
 * Revision 1.1  2010/05/16 07:13:33 
 * Tracer - first version
 *
 *
 * Function: Trace components implementation
 */
#include "ScTr.h"
#include <time.h>
#include <string.h>
#include <memory>
#include <iostream>
#include "ScTrWriter.h"
#include <sstream>
#include <cassert>
#include <boost/thread/mutex.hpp>
#include <exception>

namespace spdr
{

using namespace std;

const ScTraceComponent* ScTr::SCTC;
boost::mutex _trConfigMutex;
boost::mutex _trIOMutex;
boost::once_flag ScTr::flag = BOOST_ONCE_INIT;

//static Mutex *_trConfigMutex;
ScTrWriterRCSPtr ScTr::_scTrWriter;
ScTr::SCTCMap *ScTr::_tcMap;
static const std::string& getDefaultCfgKey()
{
	const static std::string rc("DEFAULT_CFG_KEY");
	return rc;
}

void ScTr::error(const ScTraceComponent* tc, const std::string &msg)
{
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeLineToTrace(tc, L_ERROR, msg);
}

void ScTr::warning(const ScTraceComponent* tc, const std::string &msg)
{
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeLineToTrace(tc, L_WARNING, msg);
}

void ScTr::info(const ScTraceComponent* tc, const std::string &msg)
{
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeLineToTrace(tc, L_INFO, msg);
}

void ScTr::config(const ScTraceComponent* tc, const std::string &msg)
{
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeLineToTrace(tc, L_CONFIG, msg);
}

void ScTr::event(const ScTraceComponent* tc, const string &msg)
{
	//if(_scTrWriter.isNullPtr()) return;
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeLineToTrace(tc, L_EVENT, msg);
}
void ScTr::debug(const ScTraceComponent* tc, const string &msg)
{
	//if(_scTrWriter.isNullPtr()) return;
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeLineToTrace(tc, L_DEBUG, msg);
}
void ScTr::dump(const ScTraceComponent* tc, const string &msg)
{
	//if(_scTrWriter.isNullPtr()) return;
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeLineToTrace(tc, L_DUMP, msg);
}
void ScTr::entry(const ScTraceComponent* tc, const string &msg)
{
	//if(_scTrWriter.isNullPtr()) return;
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeLineToTrace(tc, L_ENTRY, msg);
}
void ScTr::exit(const ScTraceComponent* tc, const string &msg)
{
	//   if(_scTrWriter.isNullPtr()) return;
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeLineToTrace(tc, L_EXIT, msg);
}

void ScTr::infoNLS(const ScTraceComponent* tc, const string& nlsMsgKey,
		const char* defaultNlsMessage, ...)
{
	//if(_scTrWriter.isNullPtr()) return;
	if (_scTrWriter == NULL)
		return;
	va_list args;
	va_start(args, defaultNlsMessage);
	_scTrWriter->writeLineToNLS(tc, L_INFO, nlsMsgKey, defaultNlsMessage, args);
}
void ScTr::errorNLS(const ScTraceComponent* tc, const string& nlsMsgKey,
		const char* defaultNlsMessage, ...)
{
	//if(_scTrWriter.isNullPtr()) return;
	if (_scTrWriter == NULL)
		return;
	va_list args;
	va_start(args, defaultNlsMessage);
	_scTrWriter->writeLineToNLS(tc, L_FATAL_ERROR, nlsMsgKey,
			defaultNlsMessage, args);
}
void ScTr::warningNLS(const ScTraceComponent* tc, const string& nlsMsgKey,
		const char* defaultNlsMessage, ...)
{
	//if(_scTrWriter.isNullPtr()) return;
	if (_scTrWriter == NULL)
		return;
	va_list args;
	va_start(args, defaultNlsMessage);
	_scTrWriter->writeLineToNLS(tc, L_WARNING, nlsMsgKey, defaultNlsMessage,
			args);
}

void ScTr::rumTrace(int logLevel, int category, const char *msgKey,
		const char *message)
{
	//if(_scTrWriter.isNullPtr()) return;
	if (_scTrWriter == NULL)
		return;
	_scTrWriter->writeRumTrace(logLevel, category, msgKey, message);
}

ScTraceComponent* ScTr::enroll(const char *compName, const char *subcompName,
		int subcompID, const char *trID, const char *resBun)
{
	const static string EMPTY_STRING;
	const string componentName(compName);
	const string subcomponentName(subcompName);
	const string traceID(trID);
	const string resourceBundle(resBun);
	ScTraceComponent* pTC = NULL;

	//cout << "ScTr::enroll " << compName << " " << subcompName << " " << subcompID << " " << trID << "\n";
	boost::call_once(setStaticVariables, flag);

	//Start synchronized block - will end at function return
	boost::mutex::scoped_lock lock(_trConfigMutex);

	SCTCMap::iterator it;
	bool isNew = false;
	do
	{
		string key = createTrKey(componentName, subcomponentName, traceID);
		it = _tcMap->find(key);
		if (it != _tcMap->end())
		{
			break;
		}
		isNew = true;
		key = createTrKey(componentName, subcomponentName, EMPTY_STRING);
		it = _tcMap->find(key);
		if (it != _tcMap->end())
		{
			break;
		}
		key = createTrKey(componentName, EMPTY_STRING, EMPTY_STRING);
		it = _tcMap->find(key);
		if (it != _tcMap->end())
		{
			break;
		}
		key = createTrKey(EMPTY_STRING, EMPTY_STRING, EMPTY_STRING);
		it = _tcMap->find(key);
		//sysAssert(it != _tcMap->end());
		if (it == _tcMap->end())
		{
			throw std::runtime_error("Error: ScTr::enroll default key not in map");
			//assert(it != _tcMap->end());
		}
	} while (false);

	if (isNew)
	{
		pTC = new ScTraceComponent(compName, subcompName, subcompID, trID,
				resBun, it->second->getTrConfig());
		string key = createTrKey(componentName, subcomponentName, traceID);
		_tcMap->insert(make_pair(key, pTC));
	}
	else
	{
		pTC = it->second;
		if (pTC->getResourceBundleName().empty() && resBun)
		{
			//Configuartion was set before enroll, replace with real TC
			int trCfg = pTC->getTrConfig();
			delete pTC;
			pTC = new ScTraceComponent(compName, subcompName, subcompID, trID,
					resBun, trCfg);
			it->second = pTC;
		}
	}
	return pTC;
}

bool ScTr::init(ScTrWriterRCSPtr writer)
{

	//Start syncronized block - will end at function return
	boost::mutex::scoped_lock lock(_trConfigMutex);

	//if(!_scTrWriter.isNullPtr()){
	if (!(_scTrWriter == NULL))
	{
		pair<scLogListener, void*> logListener = writer->getLogListener();
		_scTrWriter->updateLogListener(logListener.first, logListener.second);
		return true;
	}
	_scTrWriter = writer;
	return true;
}

void ScTr::updateConfig(
		int trCfg,
		const string& componentName,
		const string& subcomponentName,
		int subcompID,
		const string& traceID)
{
	string trKey = createTrKey(componentName, subcomponentName, traceID);
	//cout << "ScTr::updateConfig received: " << trKey << endl;

	boost::mutex::scoped_lock lock(_trConfigMutex);

	SCTCMap::iterator it = _tcMap->find(trKey);
	if (it == _tcMap->end())
	{
		//cout << "ScTr::updateConfig creating: " << componentName.c_str() << endl;
		ScTraceComponent* pTC =
				new ScTraceComponent(componentName.c_str(),
						subcomponentName.c_str(), subcompID, traceID.c_str(),
						"", trCfg);
		_tcMap->insert(make_pair(trKey, pTC));
	}
	else
	{
		it->second->updateTrConfig(trCfg);
	}

	if (trKey == getDefaultCfgKey())
	{
		for (it = _tcMap->begin(); it != _tcMap->end(); ++it)
		{
			it->second->updateTrConfig(trCfg);
		}
	}
	else
	{
		const char *prefix = trKey.c_str();
		size_t len = trKey.length();
		for (it = _tcMap->begin(); it != _tcMap->end(); ++it)
		{
			const char *curKey = it->first.c_str();
			if (!strncmp(prefix, curKey, len) && ((curKey[len] == '.')
					|| (curKey[len] == 0)))
			{
				it->second->updateTrConfig(trCfg);
			}
		}
	}
}

string ScTr::createTrKey(const string& componentName,
		const string& subcomponentName, const string& traceID)
{
	if (componentName.empty())
		return string(getDefaultCfgKey());
	string result(componentName);
	if (subcomponentName.empty())
		return result;
	result += "." + subcomponentName;
	if (traceID.empty())
		return result;
	result += "." + traceID;
	return result;
}

/**
 * @name    ScTr::setStaticVariables
 */
void ScTr::setStaticVariables()
{
	//    if (init) {
	//_trConfigMutex = Mutex::createMutex("trConfigMutex");
	_tcMap = new SCTCMap();
#ifdef _DEBUG
	int defaultTrCfg = 0xFFFFFFFF;
#else
	int defaultTrCfg = 0;
#endif

	ScTraceComponent* pTC = new ScTraceComponent("", "", 0, "", "",
			defaultTrCfg);

	_tcMap->insert(make_pair(getDefaultCfgKey(), pTC));

	SCTC = pTC;

	//    } else {
	//    	delete _scTrWriter;
	//    	_scTrWriter = NULL;
	//        //_scTrWriter.reset();
	//        SCTCMap::iterator it;
	//        for (it=_tcMap->begin(); it != _tcMap->end(); ++it) {
	//            delete it->second;
	//        }
	//        delete _tcMap;
	//        _tcMap = NULL;
	//        //delete _trConfigMutex;
	//       // _trConfigMutex = NULL;
	//    }
}
//
//void memDbgInfoPrint(const char * className, const char * action,
//		void* thisPtr, int line)
//{
//	ScTr::memDbgInfoPrint(className, action, thisPtr, line);
//}
//
//void ScTr::memDbgInfoPrint(const char * className, const char * action,
//		void* thisPtr, int line)
//{
//	//if(_scTrWriter.isNullPtr()) return;
//	if (_scTrWriter == NULL)
//		return;
//	ostringstream strBuff;
//	strBuff << className << " : " << action << " : " << thisPtr << " : "
//			<< line << endl;
//	string msg = strBuff.str();
//	_scTrWriter->writeLineToTrace(SCTC, L_DEBUG, msg.c_str());
//}

}
