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
 * TopicImpl.cpp
 *
 *  Created on: Jan 31, 2012
 */

#include "TopicImpl.h"

namespace spdr
{

namespace messaging
{

TopicImpl::TopicImpl() :
		Topic(), hash(TopicImpl::hashName(name)), global_(false)
{
//	boost::hash<std::string> string_hash;
//	hash = (int32_t)string_hash(name); //Topic-name hash
}

TopicImpl::TopicImpl(const std::string& topicName, bool global) :
		Topic(topicName), hash(0), global_(global)
{
	boost::hash<std::string> string_hash;
	hash = (int32_t)string_hash(name); //Topic-name hash
}

TopicImpl::TopicImpl(const TopicImpl& other) :
		Topic(other.name), hash(other.hash), global_(other.global_)
{
}

TopicImpl::~TopicImpl()
{
}

}

}
