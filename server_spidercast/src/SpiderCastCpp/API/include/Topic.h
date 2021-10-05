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
 * Topic.h
 *
 *  Created on: Jan 30, 2012
 */

#ifndef TOPIC_H_
#define TOPIC_H_

#include <string>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

namespace spdr
{

/**
 * This name space carries all the classes that provide publish-subscribe messaging.
 */
namespace messaging
{

/**
 * This interface represents a topic.
 *
 * A Topic implementation must be generated using the factory method
 * SpiderCast#createTopic().
 *
 * Each topic has a scope - either Local or Global. Messages published on a Local topic
 * are routed only within their zone, whereas messages published on a Global topic are
 * routed to all zones. The scope is specified using the property TopicScope_Prop_Key.
 *
 * @see
 */
class Topic
{
public:
	virtual ~Topic();

	inline const std::string& getName() const
	{
		return name;
	}

	virtual bool operator==(const Topic& other) const = 0;
	virtual bool operator!=(const Topic& other) const = 0;

	virtual bool operator<(const Topic& other) const = 0;
	virtual bool operator<=(const Topic& other) const = 0;
	virtual bool operator>(const Topic& other) const = 0;
	virtual bool operator>=(const Topic& other) const = 0;

	/**
	 * Whether the topic has Global or Local scope.
	 *
	 * @return true for Global, false for Local.
	 */
	virtual bool isGlobalScope() const = 0;

	/**
	 * Returns the hash value.
	 *
	 * @return
	 */
	virtual std::size_t hash_value() const = 0;

	/**
	 * A  string representation.
	 *
	 * @return
	 */
	virtual std::string toString() const = 0;

protected:
	Topic();
	Topic(const std::string& topicName);

	const std::string name;

private:
	Topic(const Topic& topic);
	const Topic& operator=(const Topic& topic);
};

typedef boost::shared_ptr<Topic> Topic_SPtr;

}

}

#endif /* TOPIC_H_ */
