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
 * PropertyConfig.cpp
 *
 *  Created on: 22/02/2010
 */

#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

#include "PropertyMap.h"

namespace spdr
{
//using std::cout;
//using std::endl;

PropertyMap::PropertyMap() :
	StringStringMap()
{
	//cout << "PropertyMap()" << endl;
}

PropertyMap::PropertyMap(const StringStringMap & map)
{
	clear();
	insert(map.begin(),map.end());
	//cout << "PropertyMap(map)" << endl;
}

PropertyMap::PropertyMap(const PropertyMap & propMap)
{
	clear();
	insert(propMap.begin(),propMap.end());

	//cout << "PropertyMap(PropertyMap&)" << endl;
}

PropertyMap& PropertyMap::operator=(const PropertyMap & map)
{
	if (this == &map)
	{
		return *this;
	}
	else
	{
		clear();
		insert(map.begin(),map.end());
		return *this;
	}
}


PropertyMap::~PropertyMap()
{
	//cout << "~PropertyMap()" << endl;
}

std::pair<String,bool> PropertyMap::getProperty(const String & key) const
{
	StringStringMap::const_iterator pos = find(key);
	if (pos == end())
	{
		return std::pair<String,bool>("",false);
	}
	else
	{
		return std::pair<String,bool>(pos->second,true);
	}
}

std::pair<String,bool> PropertyMap::setProperty(const String & key,
		const String & value)
{
	StringStringMap::const_iterator pos = find(key);
	if (pos == end())
	{
		(*this)[key] = value;
		return std::pair<String,bool>("",false);
	}
	else
	{
		std::pair<String,bool> rv(pos->second, true);
		erase(pos->first);
		(*this)[key] = value;
		return rv;
	}
}

std::pair<String,bool> PropertyMap::removeProperty(const String & key)
{
	StringStringMap::const_iterator pos = find(key);
	if (pos == end())
	{
		return std::pair<String,bool>("",false);
	}
	else
	{
		std::pair<String,bool> rv(pos->second, true);
		erase(pos->first);
		return rv;
	}
}

const bool PropertyMap::containsKey(const String & key) const
{
	return (count(key));
}

void PropertyMap::load(std::istream& inputStream) throw (std::ios_base::failure,
		SpiderCastRuntimeError)
{
	if (inputStream.fail())
	{
		throw std::ios_base::failure("Failed input stream");
	}

	inputStream.exceptions(std::ios::badbit);

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

		size_type sepPos = line.find_first_of("=");

		if (sepPos == String::npos)
		{ // no separator, we don't allow that
			String err = "Bad properties, missing separator (=), line "
					+ boost::lexical_cast<String>(line_number);
			throw SpiderCastRuntimeError(err);
		}

		String key = line.substr(0, sepPos);
		String value = line.substr(sepPos + 1, line.length() - sepPos);

		boost::trim_right(key);
		boost::trim_left(value);

		if ((key == "")) // || (value == ""))
		{
			String err = "Bad properties, empty key, line "
					+ boost::lexical_cast<String>(line_number);
			throw SpiderCastRuntimeError(err);
		}

		insert(std::pair<String, String>(key, value));
	}
}


String PropertyMap::toString() const
{
	String s = "[";
	unsigned int i=0;
	std::map<std::string,std::string>::const_iterator pos;
	for (pos = this->begin(); pos != this->end(); ++pos)
	{
		s.append(pos->first).append("=").append(pos->second);
		if (++i < this->size())
		{
			s.append(", ");
		}
	}
	s.append("]");

	return s;
}

std::ostream& operator<<(std::ostream& os, const PropertyMap& map)
{
	os << "[";
	unsigned int i=0;
	std::map<std::string,std::string>::const_iterator pos;
	for (pos = map.begin(); pos != map.end(); ++pos)
	{
		os << pos->first << "=" << pos->second;
		if (++i < map.size())
		{
			os << ", ";
		}
	}
	os << "]";
	return os;
}

}

