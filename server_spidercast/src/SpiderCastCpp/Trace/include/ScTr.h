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
 * File        : ScTr.h
 * Author      :
 * Version     : $Revision: 1.6 $
 * Last updated: $Date: 2015/11/18 08:37:01 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTr.h,v $
 * Revision 1.6  2015/11/18 08:37:01 
 * Update copyright headers
 *
 * Revision 1.5  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.4  2015/06/30 13:26:45 
 * Trace 0-8 levels, trace levels by component
 *
 * Revision 1.3  2015/04/27 06:11:11 
 * Interface change in config
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.6  2012/10/25 10:11:16 
 * Added copyright and proprietary notice
 *
 * Revision 1.5  2011/02/27 12:20:20 
 * Refactoring, header file organization
 *
 * Revision 1.4  2011/01/09 07:00:57 
 * Formating, comments
 *
 * Revision 1.3  2010/11/29 14:10:07 
 * change std::map to boost::unordered_map
 *
 * Revision 1.2  2010/07/13 13:22:12 
 * *** empty log message ***
 *
 * Revision 1.1  2010/05/16 07:13:34 
 * Tracer - first version
 *
 *
 * Function: Trace components declaration
 */
#ifndef SCTR_H
#define SCTR_H

#include "ScTraceComponent.h"

#include <boost/unordered_map.hpp>

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp>

namespace spdr
{

class ScTr
{
private:
	const static ScTraceComponent* SCTC;

	//static RCSmartPtr<class TrWriter>       _trWriter;
	static class ScTrWriter* _scTrWriter;

	//typedef std::map<std::string, ScTraceComponent*> SCTCMap;
	typedef boost::unordered_map<std::string, ScTraceComponent*> SCTCMap;

	static SCTCMap *_tcMap;

	static std::string createTrKey(const std::string& componentName,
			const std::string& subcomponentName, const std::string& traceID);

	static void memDbgInfoPrint(const char * className, const char * action,
			void* thisPtr, int line);

	static boost::once_flag flag;

	static void setStaticVariables(); // Internal service function.

public:

	static ScTraceComponent* enroll(const char *componentName,
			const char *subcomponentName, int subcompID, const char *traceID,
			const char *resourceBundle);

	static void error(const ScTraceComponent* tc, const std::string &msg);

	static void warning(const ScTraceComponent* tc, const std::string &msg);

	static void info(const ScTraceComponent* tc, const std::string &msg);

	static void config(const ScTraceComponent* tc, const std::string &msg);

	static void event(const ScTraceComponent* tc, const std::string &msg);

	static void debug(const ScTraceComponent* tc, const std::string &msg);

	static void dump(const ScTraceComponent* tc, const std::string &msg);

	static void entry(const ScTraceComponent* tc, const std::string &msg);

	static void exit(const ScTraceComponent* tc, const std::string &msg);

	static void infoNLS(const ScTraceComponent* tc, const std::string& nslMsgKey,
			const char* defaultNlsMessage, ...);

	static void warningNLS(const ScTraceComponent* tc,
			const std::string& nslMsgKey, const char* defaultNlsMessage, ...);

	static void errorNLS(const ScTraceComponent* tc, const std::string& nslMsgKey,
			const char* defaultNlsMessage, ...);

	static void rumTrace(int logLevel, int category, const char *msgLey,
			const char *message);

	static bool init(ScTrWriter*);

	/**
	 * scTrConfig format:
	 * - the higher byte is the log level (0 to 8, 0 - None, 8 - most detailed)
	 * - the lower 24 bits are a bitmap of layer ID's
	 *
	 * @param scTrCfg
	 * @param componentName
	 * @param subcomponentName
	 * @param subcompID
	 * @param traceID
	 */
	static void updateConfig(
			int scTrCfg,
			const std::string& componentName = std::string(),
			const std::string& subcomponentName = std::string(),
			int subcompID = 0,
			const std::string& traceID = std::string());

	static const class ScTrWriter* getScTrWriter(void)
	{
		return _scTrWriter;
	}
//
//	friend void memDbgInfoPrint(const char * className, const char * action,
//			void* thisPtr, int line);

};

extern boost::mutex _trConfigMutex;
extern boost::mutex _trIOMutex;

}

#endif
