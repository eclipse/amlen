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

#include <sstream>

#include "SubscriptionPattern.h"

namespace mcp
{

SubscriptionPattern::SubscriptionPattern() :
		plus_locations(), hash_location(0), last_level(0)
{
}

SubscriptionPattern::SubscriptionPattern(const std::vector<uint16_t> & plus_loc,
		const uint16_t hash_loc, const uint16_t last_lev)
				throw (MCPIllegalArgumentError) :
		plus_locations(plus_loc), hash_location(hash_loc), last_level(last_lev)
{
	using namespace std;

	uint16_t last = 0;
	for (size_t i = 0; i < plus_locations.size(); ++i)
	{
		if (plus_locations[i] <= last)
		{
			throw MCPIllegalArgumentError("Plus location array is unordered");
		}

		if (hash_location > 0 && plus_locations[i] >= hash_location)
		{
			throw MCPIllegalArgumentError(
					"Plus location higher or equal than hash location");
		}

		if (plus_locations[i] > last_level)
		{
			throw MCPIllegalArgumentError(
					"Plus location higher than last level");
		}

		last = plus_locations[i];
	}

	if ((hash_location > 0) && (hash_location != last_level))
	{
		throw MCPIllegalArgumentError(
				"Hash location different than last level");
	}
}

SubscriptionPattern::~SubscriptionPattern()
{
}

MCPReturnCode SubscriptionPattern::parseSubscription(const std::string& subs)
{
	return parseSubscription(subs.data(),subs.size());
}

MCPReturnCode SubscriptionPattern::parseSubscription(const char* subs,
		const int length)
{
	clear();

	if (length < 1)
	{
		return ISMRC_ArgNotValid;
	}

	const int S_start = 0; //start of level
	const int S_mid_reg = 1; //middle of a regular level
	const int S_start_hash = 2; //after start-of-level and '#'
	const int S_start_plus = 3; //after start-of-level and '+'

	const int S_error = 4; //Error state

	int state = S_start;

	//int num_level = 0;

	for (int i = 0; i < length; ++i)
	{
		switch (state)
		{
		case S_start:	//start-of-level
		{
			++last_level;
			if (subs[i] == '/')
			{
				state = S_start;
			}
			else if (subs[i] == '#')
			{
				state = S_start_hash;
				hash_location = last_level;
			}
			else if (subs[i] == '+')
			{
				state = S_start_plus;
				plus_locations.push_back(last_level);
			}
			else //any other char
			{
				state = S_mid_reg;
			}
		}
			break;

		case S_mid_reg:	//middle of a regular level
		{
			if (subs[i] == '/')
			{
				state = S_start;
			}
			else if (subs[i] == '#')
			{
				state = S_error; //Error
			}
			else if (subs[i] == '+')
			{
				state = S_error; //Error
			}
			else //any other char
			{
				state = S_mid_reg;
			}
		}
			break;

		case S_start_hash:
		{
			state = S_error; //any char after a hash is an error
		}
			break;

		case S_start_plus:
		{
			if (subs[i] == '/')
			{
				state = S_start;
			}
			else //any other char
			{
				state = S_error;
			}
		}
			break;

		} //switch

		if (state == S_error)
		{
			clear();
			return ISMRC_ArgNotValid;
		}
	}

	//end-of-string
	if (state == S_start) 	//start-of-level
	{
		++last_level;
	}

	return ISMRC_OK;
}

bool SubscriptionPattern::operator==(const SubscriptionPattern& other) const
{
	if (plus_locations.size() == other.plus_locations.size())
	{
		for (std::size_t i = 0; i < plus_locations.size(); ++i)
		{
			if (plus_locations[i] != other.plus_locations[i])
			{
				return false;
			}
		}

		if (hash_location == other.hash_location
				&& last_level == other.last_level)
		{
			return true;
		}
	}

	return false;
}

bool SubscriptionPattern::operator<(const SubscriptionPattern& other) const
{
	std::size_t i = 0;

	uint16_t x = 0;
	uint16_t y = 0;

	while (i < plus_locations.size() + 2 && i < other.plus_locations.size() + 2)
	{
		if (i < plus_locations.size())
			x = plus_locations[i];
		else if (i == plus_locations.size())
			x = hash_location;
		else
			x = last_level;

		if (i < other.plus_locations.size())
			y = other.plus_locations[i];
		else if (i == other.plus_locations.size())
			y = other.hash_location;
		else
			y = other.last_level;

		if (x < y)
		{
			return true;
		}
		else if (x > y)
		{
			return false;
		}

		++i;
	}

	if (plus_locations.size() < other.plus_locations.size())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SubscriptionPattern::empty() const
{
	if (plus_locations.empty() && hash_location == 0 && last_level == 0)
		return true;
	else
		return false;

}

bool SubscriptionPattern::isWildcard() const
{
	if (hash_location != 0 || !plus_locations.empty())
		return true;
	else
		return false;
}

void SubscriptionPattern::clear()
{
	plus_locations.clear();
	hash_location = 0;
	last_level = 0;
}

std::string SubscriptionPattern::toString() const
{
	std::ostringstream oss;
	oss << "+:[";
	for (unsigned int i = 0; i < plus_locations.size(); ++i)
	{
		oss << plus_locations[i]
				<< ((i == plus_locations.size() - 1) ? "" : ",");
	}

	oss << "] #:" << hash_location << " E:" << last_level;

	return oss.str();
}

void SubscriptionPattern::formatTopic(const char* topic, std::size_t topic_length, std::string& formattedTopic) const
{

	if (topic_length == 0)
	{
		throw MCPIllegalArgumentError("Topic name must not be empty");
	}

	if (isWildcard())
	{
		unsigned int level = 1;
		unsigned int topic_index = 0;
		unsigned int plus_index = 0;

		FormatState state = SF_init;
		if (isLevelPlus(level, plus_index) || isLevelHash(level))
		{
			state = SF_start_level_replace;
		}
		else
		{
			state = SF_start_level_copy;
		}

		formattedTopic.clear();

		while (topic_index < topic_length)
		{
			switch (state)
			{

			case SF_start_level_copy:
			{
				char c = topic[topic_index++];
				formattedTopic.push_back(c);
				if (c == '/')
				{
					++level;
					if (level > last_level)
					{
						formattedTopic.clear(); //pattern not applicable
						state = SF_end;
					}
					else if (isLevelPlus(level, plus_index)
							|| isLevelHash(level))
					{
						state = SF_start_level_replace;
					}
					else
					{
						state = SF_start_level_copy;
					}
				}
				else
				{
					state = SF_in_level_copy;
				}
				break;
			}

			case SF_in_level_copy:
			{
				char c = topic[topic_index++];
				formattedTopic.push_back(c);
				if (c == '/')
				{
					++level;
					if (level > last_level)
					{
						formattedTopic.clear(); //pattern not applicable
						state = SF_end;
					}
					else if (isLevelPlus(level, plus_index)
							|| isLevelHash(level))
					{
						state = SF_start_level_replace;
					}
					else
					{
						state = SF_start_level_copy;
					}
				}
				else
				{
					state = SF_in_level_copy;
				}
				break;
			}

			case SF_start_level_replace:
			{
				char c = topic[topic_index++];
				if (c == '/')
				{
					if (isLevelHash(level))
					{
						formattedTopic.push_back('#');
						state = SF_end;
					}
					else //its a plus
					{
						formattedTopic.push_back('+');
						formattedTopic.push_back('/');
						++level;
						if (level > last_level)
						{
							formattedTopic.clear(); //pattern not applicable
							state = SF_end;
						}
						else if (isLevelPlus(level, plus_index)
								|| isLevelHash(level))
						{
							state = SF_start_level_replace;
						}
						else
						{
							state = SF_start_level_copy;
						}
					}
				}
				else
				{
					state = SF_in_level_replace;
				}

				break;
			}

			case SF_in_level_replace:
			{
				char c = topic[topic_index++];
				if (c == '/')
				{
					if (isLevelHash(level))
					{
						formattedTopic.push_back('#');
						state = SF_end;
					}
					else //its a plus
					{
						formattedTopic.push_back('+');
						formattedTopic.push_back('/');
						++level;
						if (level > last_level)
						{
							formattedTopic.clear(); //pattern not applicable
							state = SF_end;
						}
						else if (isLevelPlus(level, plus_index)
								|| isLevelHash(level))
						{
							state = SF_start_level_replace;
						}
						else
						{
							state = SF_start_level_copy;
						}
					}
				}
				else
				{
					state = SF_in_level_replace;
				}
				break;
			}

			default:
			{
				throw MCPRuntimeError("Error formatting topic, unexpected state");
			}

			} //switch

			if (state == SF_end)
			{
				//TODO trace
				break; // while loop
			}
		} //while

		if (state == SF_start_level_replace || state == SF_in_level_replace)
		{
			if (isLevelHash(level))
			{
				formattedTopic.push_back('#');
			}
			else //its a plus
			{
				if ( (plus_index < plus_locations.size()-1) //there are more pluses
					 ||	(level < last_level)) 				//the topic is shorter than the pattern
				{
					formattedTopic.clear(); //pattern not applicable
				}
				else
				{
					formattedTopic.push_back('+');
				}
			}
		}
		else if (state == SF_start_level_copy || state == SF_in_level_copy)
		{
			if (level < last_level) //the topic is shorter than the pattern
			{
				formattedTopic.clear(); //pattern not applicable
			}
		}
	}
	else
	{
		throw MCPIllegalArgumentError(
				"SubscriptionPattern must be wild-card expression");
	}

}

void SubscriptionPattern::formatTopic(const std::string& topic,
		std::string& formattedTopic) const
{
	formatTopic(topic.data(),topic.size(),formattedTopic);
}

/*
 * search forward in plus_locations array, assume array is sorted
 * plus_index is only changed here.
 */
bool SubscriptionPattern::isLevelPlus(unsigned int currentLevel,
		unsigned int& plus_index) const
{
	while (plus_index < plus_locations.size()
			&& plus_locations[plus_index] < currentLevel)
	{
		++plus_index;
	}

	if (plus_index < plus_locations.size()
			&& plus_locations[plus_index] == currentLevel)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * assume currentLevel is always >0
 */
bool SubscriptionPattern::isLevelHash(unsigned int currentLevel) const
{
	if (currentLevel == hash_location)
	{
		return true;
	}
	else
	{
		return false;
	}
}

} /* namespace mcp */
