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
 * SpiderCastFactoryImpl.h
 *
 *  Created on: 01/02/2010
 */

#ifndef SPIDERCASTFACTORYIMPL_H_
#define SPIDERCASTFACTORYIMPL_H_

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/thread/mutex.hpp>

#include "SpiderCastFactory.h"
#include "SpiderCastConfigImpl.h"
#include "SpiderCastImpl.h"
#include "SpiderCastEventListener.h"
#include "NodeIDImpl.h"
#include "NetworkEndpoints.h"
#include "SpiderCastLogicError.h"

#include "TraceUtils.h"

namespace spdr
{

class SpiderCastFactoryImpl: public SpiderCastFactory //, public ScTraceContext
{
private:
//	static ScTraceComponent* tc_;

	boost::mutex mutex;
	int instanceID; //gives a unique ID to each SpiderCast created.
	log::LogListener* logListener;
	log::Level logLevel;

	NodeIDImpl* createNodeID_FromString(const String& idstr) throw (SpiderCastLogicError);

public:
	SpiderCastFactoryImpl();
	virtual ~SpiderCastFactoryImpl();

	SpiderCast_SPtr createSpiderCast(
			SpiderCastConfig& config,
			SpiderCastEventListener& eventListener)
			throw (SpiderCastRuntimeError);

	SpiderCastConfig_SPtr createSpiderCastConfig(
			const PropertyMap& properties,
			const std::vector<NodeID_SPtr >& bootstrapSet)
			throw (SpiderCastLogicError);

	SpiderCastConfig_SPtr createSpiderCastConfig(
				const PropertyMap& properties,
				const std::vector<NodeID_SPtr >& bootstrapSet,
				const std::vector<NodeID_SPtr >& supervisorBootstrapSet)
				throw (SpiderCastLogicError);

	NodeID_SPtr createNodeID_SPtr(const String& idstr)
			throw (SpiderCastLogicError);

	std::vector<NodeID_SPtr > loadBootstrapSet(std::istream& inputStream)
				throw(std::ios_base::failure, SpiderCastLogicError);

	std::vector<NodeID_SPtr > loadBootstrapSetSimpleLine(const char* line)
							throw(SpiderCastLogicError);

	std::vector<NodeID_SPtr > loadBootstrapSetURIs(const char* line)
							throw(SpiderCastLogicError);

	void registerLogListener(log::LogListener * const log_listener, log::Level log_level);

	void changeLogLevel(log::Level log_level);

	void changeLogLevel(log::Level log_level, const std::string& componentName, const std::string& subComponentName = "");

	void changeRUMLogLevel(int log_level);

	static void logListenerAdapter(void *user, int log_level, const char *id,
			const char *message)
	{
		log::LogListener* listener = (log::LogListener*) user;
		listener->onLogEvent(static_cast<log::Level>(log_level), id, message);
	}

};

}

#endif /* SPIDERCASTFACTORYIMPL_H_ */
