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

#ifndef SUBSCRIPTIONPATTERN_H_
#define SUBSCRIPTIONPATTERN_H_

#include <vector>
#include <string>
#include <boost/cstdint.hpp>
#include <boost/operators.hpp>

#include "MCPExceptions.h"
#include "MCPReturnCode.h"

namespace mcp
{

/**
 * Assume MQTT subscription.
 *
 * Synthesized default copy constructor and assignment operator.
 */
class SubscriptionPattern : public boost::equality_comparable<SubscriptionPattern>, public boost::less_than_comparable<SubscriptionPattern>
{
public:
	SubscriptionPattern();

	SubscriptionPattern(const std::vector<uint16_t> & plus_loc, const uint16_t hash_loc, const uint16_t last_lev) throw (MCPIllegalArgumentError);

	virtual ~SubscriptionPattern();

	//SubscriptionPattern(const SubscriptionPattern& other); //synthetic

	//SubscriptionPattern& operator=(const SubscriptionPattern& other); //synthetic

	/**
	 * Detect the subscription filter to a pattern.
	 *
	 * Assume a legal MQTT subscription.
	 *
	 * @param subs A legal MQTT subscription.
	 * @param length The length of the subscription string
	 * @return
	 */
	MCPReturnCode parseSubscription(const char* subs, const int length);

	MCPReturnCode parseSubscription(const std::string& subs);

	bool operator==(const SubscriptionPattern& other) const;
	bool operator<(const SubscriptionPattern& other) const;

	bool empty() const;

	bool isWildcard() const;

	void clear();

	const std::vector<uint16_t>& getPlusLocations() const
	{
		return plus_locations;
	}

	uint16_t getHashLocation() const
	{
		return hash_location;
	}

	uint16_t getLastLevel() const
	{
		return last_level;
	}

	std::string toString() const;

	void formatTopic(const std::string& topic, std::string& formattedTopic) const;

	void formatTopic(const char* topic, std::size_t topic_length, std::string& formattedTopic) const;

private:
	std::vector<uint16_t> plus_locations;
	uint16_t hash_location;
	uint16_t last_level;

	bool isLevelPlus(unsigned int currentLevel, unsigned int& plus_index) const;

	bool isLevelHash(unsigned int currentLevel) const;

	enum FormatState
	{
		SF_init,
		SF_start_level_copy,
		SF_in_level_copy,
		SF_start_level_replace,
		SF_in_level_replace,
		SF_end
	};

};

typedef boost::shared_ptr<SubscriptionPattern> SubscriptionPattern_SPtr;

} /* namespace mcp */

#endif /* SUBSCRIPTIONPATTERN_H_ */
