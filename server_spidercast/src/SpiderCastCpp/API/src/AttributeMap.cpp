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
 * AttributeMap.cpp
 *
 *  Created on: 28/06/2010
 */

#include <sstream>

#include "AttributeMap.h"


namespace spdr
{

namespace event
{

//=== AttributeMap ===
AttributeMap::AttributeMap()
{
}

AttributeMap::~AttributeMap()
{
}

String AttributeMap::toString(AttributeValue::ToStringMode mode) const
{
	std::ostringstream oss;
	oss << '[';
	AttributeMap::const_iterator it = this->begin();
	for (int i=0; it != this->end(); ++it, ++i)
	{
		oss << it->first << "=" << it->second.toString(mode);
		if (i < (int)this->size()-1)
		{
			oss << ',';
		}
	}
	oss << ']';
	return oss.str();
}

AttributeMap_SPtr AttributeMap::clone(AttributeMap_SPtr other)
{
	AttributeMap_SPtr map;
	if (other)
	{
		map.reset(new AttributeMap);
		AttributeMap::const_iterator it;
		for (it = other->begin(); it != other->end(); ++it)
		{
			(*map)[it->first] = AttributeValue::clone(it->second);
		}
	}
	return map;
}

//friend of AttributeMap
bool operator==(const AttributeMap& map1, const AttributeMap& map2)
{
	if (map1.size() != map2.size())
	{
		return false;
	}

	AttributeMap::const_iterator it1 = map1.begin();
	AttributeMap::const_iterator it2 = map2.begin();

	while (it1 != map1.end())
	{
		if (it1->first == it2->first)
		{
			if (it1->second != it2->second)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		++it1;
		++it2;
	}

	return true;
}

//friend of AttributeMap
bool operator!=(const AttributeMap& map1, const AttributeMap& map2)
{
	return !(map1 == map2);
}

} //namespace event
} //namespace spdr

