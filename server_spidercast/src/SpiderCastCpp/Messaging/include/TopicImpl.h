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
 * TopicImpl.h
 *
 *  Created on: Jan 31, 2012
 */

#ifndef TOPICIMPL_H_
#define TOPICIMPL_H_

#include <boost/functional/hash.hpp>
#include "Topic.h"

namespace spdr
{

namespace messaging
{

class TopicImpl: public Topic
{
public:
	TopicImpl();
	TopicImpl(const std::string& topicName, bool global = false);
	TopicImpl(const TopicImpl& other);

	virtual ~TopicImpl();

	bool operator==(const Topic& other) const
	{
		return name == static_cast<const TopicImpl&>(other).name;
	}

	bool operator!=(const Topic& other) const
	{
		return name != static_cast<const TopicImpl&>(other).name;
	}

	bool operator<(const Topic& other) const
	{
		return name < static_cast<const TopicImpl&>(other).name;
	}

	bool operator<=(const Topic& other) const
	{
		return name <= static_cast<const TopicImpl&>(other).name;
	}

	bool operator>(const Topic& other) const
	{
		return name > static_cast<const TopicImpl&>(other).name;
	}

	bool operator>=(const Topic& other) const
	{
		return name >= static_cast<const TopicImpl&>(other).name;
	}

	/**
	 * Returns the hash value.
	 *
	 * @return
	 */
	std::size_t hash_value() const
	{
		return hash;
	}

	bool isGlobalScope() const
	{
		return global_;
	}

	std::string toString() const
	{
		std::string s(name);
		s.append(global_ ? "; G" : "; L");
		return s;
	}

	/**
	 * Topic name hash, TopicID
	 *
	 * @param name
	 * @return
	 */
	static int32_t hashName(const std::string& topic_name)
	{
		boost::hash<std::string> string_hash;
		return static_cast<int32_t>(string_hash(topic_name));
	}

private:
	const TopicImpl& operator=(const TopicImpl& other);
	mutable int32_t hash;
	const bool global_;

};

typedef boost::shared_ptr<TopicImpl> TopicImpl_SPtr;

}

}

#endif /* TOPICIMPL_H_ */
