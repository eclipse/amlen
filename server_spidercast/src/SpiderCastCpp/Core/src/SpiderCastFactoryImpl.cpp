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
 * SpiderCastFactoryImpl.cpp
 *
 *  Created on: 01/02/2010
 */



#include <iostream>

#include "SpiderCastFactoryImpl.h"
#include "rumCapi.h"

namespace spdr
{
using namespace std;

//ScTraceComponent* SpiderCastFactoryImpl::tc_ = ScTr::enroll(
//		trace::ScTrConstants::ScTr_Component_Name,
//		trace::ScTrConstants::ScTr_SubComponent_Core,
//		trace::ScTrConstants::Layer_ID_Core,
//		"SpiderCastFactoryImpl",
//		trace::ScTrConstants::ScTr_ResourceBundle_Name);


SpiderCastFactoryImpl::SpiderCastFactoryImpl() :
		SpiderCastFactory(),
		//ScTraceContext(tc_,"SC0","singleton"),
		mutex(),
		instanceID(0),
		logListener(NULL),
		logLevel(log::Level_INFO)
{
	//std::cout << "SpiderCastFactoryImpl()" << std::endl;
	//Trace_Entry((ScTraceContext*)this,"SpiderCastFactoryImpl()");
}

SpiderCastFactoryImpl::~SpiderCastFactoryImpl()
{
	//std::cout << "~SpiderCastFactoryImpl()" << std::endl;
	//Trace_Entry((ScTraceContext*)this,"~SpiderCastFactoryImpl()");
}

SpiderCast_SPtr SpiderCastFactoryImpl::createSpiderCast(
		SpiderCastConfig& config,
		SpiderCastEventListener& eventListener)
	throw (SpiderCastRuntimeError)
{
	boost::mutex::scoped_lock lock(mutex);

	std::ostringstream oss;
	oss << "SC" << std::dec << (++instanceID);

//	Trace_Entry((ScTraceContext*)this, "createSpiderCast()", "instanceID", oss.str());

	SpiderCast_SPtr sc((new SpiderCastImpl(oss.str(), config, eventListener)));

	return sc;
}

SpiderCastConfig_SPtr SpiderCastFactoryImpl::createSpiderCastConfig(
		const PropertyMap& properties,
		const std::vector<NodeID_SPtr >& bootstrapSet)
	throw (SpiderCastLogicError)
{
	//Trace_Entry((ScTraceContext*)this, "createSpiderCastConfig()");

	boost::mutex::scoped_lock lock(mutex);
	SpiderCastConfig* pConfig = new SpiderCastConfigImpl(properties, bootstrapSet);

	return SpiderCastConfig_SPtr (pConfig);
}

SpiderCastConfig_SPtr SpiderCastFactoryImpl::createSpiderCastConfig(
		const PropertyMap& properties,
		const std::vector<NodeID_SPtr >& bootstrapSet,
		const std::vector<NodeID_SPtr >& supervisorBootstrapSet)
	throw (SpiderCastLogicError)
{

	boost::mutex::scoped_lock lock(mutex);
	SpiderCastConfig* pConfig = new SpiderCastConfigImpl(
			properties, bootstrapSet, supervisorBootstrapSet);

	return SpiderCastConfig_SPtr (pConfig);
}


NodeIDImpl* SpiderCastFactoryImpl::createNodeID_FromString(const String& idstr)
	throw (SpiderCastLogicError)
{
	std::vector<String> tokens;
	boost::split(tokens, idstr, boost::is_any_of(std::string(",")));
	if (tokens.size() < 4 || tokens.size() % 2 !=0)
	{
		String what = "Bad NodeID string representation (number of tokens): " + idstr;
		throw IllegalArgumentException(what);
	}

	std::vector<std::pair<String, String> > addresses;
	std::vector<String>::iterator it = tokens.begin();

	String name = (*it);
	boost::trim(name);

	//make sure name conforms to the rules, allow '$ANY'
	SpiderCastConfigImpl::validateNodeName(name,true);
	++it;

	for (std::vector<String>::size_type i=0; i<tokens.size()-2; )
	{
		std::pair<String,String> p; //address & scope
		p.first = (*it);
		boost::trim(p.first);
		++it;
		++i;

		p.second = (*it);
		boost::trim(p.second);
		++it;
		++i;

		addresses.push_back(p);
	}
	String str_port = (*it);
	boost::trim(str_port);
	uint16_t port;

	try
	{
		port = boost::lexical_cast<uint16_t>(str_port);
	}
	catch (boost::bad_lexical_cast& e)
	{
		String what(e.what());
		what += "; Bad port representation: " + idstr;
		throw IllegalArgumentException(what);
	}

	NetworkEndpoints ne(addresses,port);

	return new NodeIDImpl(name,ne);
}

//boost::shared_ptr<NodeID> SpiderCastFactoryImpl::createNodeID_SPtr(
//		const String& idstr) throw (SpiderCastLogicError)
//{
//	NodeIDImpl* id_ptr = createNodeID_FromString(idstr);
//	return boost::shared_ptr<NodeID>(id_ptr);
//}

NodeID_SPtr SpiderCastFactoryImpl::createNodeID_SPtr(
		const String& idstr) throw (SpiderCastLogicError)
{
	NodeIDImpl* id_ptr = createNodeID_FromString(idstr);
	return NodeID_SPtr(id_ptr);
}

std::vector<NodeID_SPtr > SpiderCastFactoryImpl::loadBootstrapSet(
		istream& inputStream) throw (std::ios_base::failure,
		SpiderCastLogicError)
{
	if (inputStream.fail())
	{
		throw std::ios_base::failure("Failed input stream");
	}

	inputStream.exceptions(std::ios::badbit);

	std::vector<NodeID_SPtr > vec;

	int line_number = 0;
	String line;

	while (getline(inputStream, line)) //continue while input stream is good
	{
		line_number++;
		boost::trim(line); //removes white-space from both ends

		if (line == "") //empty line
			continue;

		if (line[0] == '#') //comment line
			continue;

		try
		{
			NodeID_SPtr id_ptr = createNodeID_SPtr(line);
			vec.push_back(id_ptr);
		} catch (SpiderCastLogicError& e)
		{
			String what(e.what());
			what += "; line " + boost::lexical_cast<string>(line_number);
			throw SpiderCastLogicError(what);
		}
	}

	return vec;
}

std::vector<NodeID_SPtr> SpiderCastFactoryImpl::loadBootstrapSetSimpleLine(
		const char* line) throw (SpiderCastLogicError)
{
	using namespace std;

	std::vector<NodeID_SPtr > vec;

	if (line != NULL)
	{
		string line_str(line);
		std::vector<string> tokens;

		unsigned int start = 0;
		unsigned int end = 0;
		while (end < line_str.size())
		{
			if (line_str.at(end) == ',')
			{
				tokens.push_back(line_str.substr(start,end-start));
				start = ++end;
			}
			else
			{
				++end;
			}
		}

		if (start < end)
		{
			tokens.push_back(line_str.substr(start,end-1));
		}

		if ((tokens.size()) % 3 != 0)
		{
			throw SpiderCastLogicError("Bad (simple-line) bootstrap set format, wrong number of tokens");
		}

		for (unsigned int i=0; i < tokens.size(); i += 3)
		{
			ostringstream oss;
			//name, address, , port
			oss << tokens[i] << "," << tokens[i+1] << ",," << tokens[i+2];
			NodeID_SPtr nodeID = createNodeID_SPtr(oss.str());
			vec.push_back(nodeID);
		}
	}

	return vec;
}

std::vector<NodeID_SPtr > SpiderCastFactoryImpl::loadBootstrapSetURIs(const char* line)
throw(SpiderCastLogicError)
{
	using namespace std;

	std::vector<NodeID_SPtr > vec;

	if (line != NULL)
	{
		string line_str(line);
		std::vector<string> tokens;
		boost::split(tokens, line_str, boost::is_any_of(std::string(",")));

		for (unsigned int i=0; i < tokens.size(); ++i)
		{
			string token = tokens[i];
			boost::trim(token);
			if (token.empty())
				continue;

			size_t pos = token.find_last_of('@'); //Name may contain '@', e.g. - myserver@example.org
			if (pos == string::npos)
			{
				throw SpiderCastLogicError(string("Bad URI, missing '@' separator: '") + token + "'");
			}
			string name = token.substr(0,pos);
			try
			{
				SpiderCastConfigImpl::validateNodeName(name, true);
			}
			catch (IllegalConfigException& e)
			{
				throw SpiderCastLogicError(string("Bad URI, illegal name before @ separator: '") + token + "'; what=" + e.what());
			}

			string s = token.substr(pos+1);
			if (s.find_first_of('@') != string::npos)
			{
				throw SpiderCastLogicError(string("Bad URI, multiple '@' separators: '") + token + "'");
			}

			pos = s.find_last_of(':');
			if (pos == string::npos)
			{
				throw SpiderCastLogicError(string("Bad URI, missing ':' separator: '") + token + "'");
			}

			string address = s.substr(0,pos);
			if (address.empty())
			{
				throw SpiderCastLogicError(string("Bad URI, missing address '") + token + "'");
			}

			if (address[0] == '[')
			{
				if (address.size() > 1 && address[address.size()-1] == ']')
				{
					address =  address.substr(1,address.size()-2);
					if (address.empty())
					{
						throw SpiderCastLogicError(string("Bad URI, missing address '") + token + "'");
					}
				}
				else
				{
					throw SpiderCastLogicError(string("Bad URI, illegal address '") + token + "'");
				}
			}

			string sPort = s.substr(pos+1);

			int iPort = -1;

			try
			{
				iPort = boost::lexical_cast<int>(sPort);
			}
			catch (boost::bad_lexical_cast& e)
			{
				throw SpiderCastLogicError(string("Bad URI, port cannot be converted to a number: '") + token + "'");
			}

			if (iPort < 0 || iPort >= 64*1024)
			{
				throw SpiderCastLogicError(string("Bad URI, port out of range: '") + token + "'");
			}

			//name, address, , port
			ostringstream oss;
			oss << name << "," << address << ",," << iPort;
			NodeID_SPtr nodeID = createNodeID_SPtr(oss.str());
			vec.push_back(nodeID);
		}
	}

	//Multiple endpoint bootstrap is now supported

//	std::set<NodeID_SPtr, SPtr_Less<NodeID> > bss;
//	for (std::vector<NodeID_SPtr >::const_iterator it = vec.begin(); it != vec.end(); ++it)
//	{
//		std::pair<std::set<NodeID_SPtr, SPtr_Less<NodeID> >::iterator, bool> res = bss.insert(*it);
//		if (!res.second)
//		{
//			throw SpiderCastLogicError(string("Bad URI set, two definitions with the same node name: ")
//					+ (*(res.first))->toString() + " and " + (*it)->toString());
//		}
//	}

	return vec;
}

void SpiderCastFactoryImpl::registerLogListener(log::LogListener* const log_listener, log::Level log_level)
{
	boost::mutex::scoped_lock lock(mutex);
	logListener = log_listener;
	scSetLogListener(logListener, SpiderCastFactoryImpl::logListenerAdapter);
	ScTraceBuffer::setStaticVariables(true);
	ScTr::updateConfig(static_cast<int>(log_level));
	//RUM level is taken from the properties, default is GLOBAL
}

void SpiderCastFactoryImpl::changeLogLevel(log::Level log_level)
{
	boost::mutex::scoped_lock lock(mutex);
	int level = static_cast<int>(log_level);
	ScTr::updateConfig(level);
	//SpiderCast is 0-8 whereas RUM is 0-9
	changeRUMLogLevel( level=0 ? 0 : level+1 );
}

void SpiderCastFactoryImpl::changeLogLevel(log::Level log_level, const std::string& component, const std::string& subComponent)
{
	boost::mutex::scoped_lock lock(mutex);
	int level = static_cast<int>(log_level);
	ScTr::updateConfig(level,component,subComponent);
}

void SpiderCastFactoryImpl::changeRUMLogLevel(int log_level)
{
	int rc;
	if (RUM_SUCCESS != rumChangeLogLevel( log_level, &rc ))
	{
		std::cerr << "SpiderCastFactoryImpl::changeRUMLogLevel cannot set RUM trace level=" << log_level << ", rc=" << rc << std::endl;
	}
}
} //namespace spdr
