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
 * BasicConfig.cpp
 *
 *  Created on: 24/02/2010
 */


#include <iostream>
#include <sstream>
#include <boost/lexical_cast.hpp>

#include "BasicConfig.h"

namespace spdr
{

BasicConfig::BasicConfig(const PropertyMap& properties) :
	prop(properties)
{
	//std::cout << "BasicConfig(prop)" << std::endl;
}

BasicConfig::BasicConfig(const BasicConfig& config) :
		prop(config.prop)
{
	//std::cout << "BasicConfig(BasicConfig&)" << std::endl;
}

BasicConfig& BasicConfig::operator=(const BasicConfig& config)
{
	if (this == &config)
	{
		return *this;
	}
	else
	{
		prop = config.prop;
		return *this;
	}
}

BasicConfig::~BasicConfig()
{
	//std::cout << "~BasicConfig()" << std::endl;
}

pair<String, bool> BasicConfig::getProperty(const String & key)
{
	return prop.getProperty(key);
}

String BasicConfig::getOptionalProperty(const String & propName,
		const String& defaultVal)
{
	std::pair<String, bool> elem = prop.getProperty(propName);

	if (!elem.second)
	{
		return String(defaultVal);
	}

	return elem.first;
}

bool BasicConfig::getOptionalBooleanProperty(const String & propName,
		const bool defaultVal) throw (IllegalConfigException)
{
	//std::cout << propName << std::endl;

	bool val = defaultVal;

	pair<String, bool> elem = getProperty(propName);

	if (elem.second)
	{
		//std::cout << elem.first << std::endl;

		try
		{
			//std::cout << "before-read" << std::endl;

			std::istringstream iss(elem.first);
			iss.exceptions(std::ios_base::failbit | std::ios_base::badbit);
			iss >> std::boolalpha >> val;

		} catch (std::ios_base::failure& e)
		{
			//std::cout << "in-catch: " << e.what() << std::endl;
			reportIllegalNumberFormat(propName, elem.first, e);
		}
	}

	return val;
}

int32_t BasicConfig::getOptionalInt32Property(const String & propName,
		const int32_t defaultVal) throw (IllegalConfigException)
{
	int32_t val = defaultVal;

	pair<String, bool> elem = prop.getProperty(propName);
	if (elem.second)
	{
		try
		{
			val = boost::lexical_cast<int32_t>(elem.first);
		} catch (boost::bad_lexical_cast& e)
		{
			reportIllegalNumberFormat(propName, elem.first, e);
		}
	}

	return val;
}

int64_t BasicConfig::getOptionalInt64Property(const String & propName,
		const int64_t defaultVal) throw (IllegalConfigException)
{
	int64_t val = defaultVal;

	pair<String, bool> elem = prop.getProperty(propName);
	if (elem.second)
	{
		try
		{
			val = boost::lexical_cast<int64_t>(elem.first);
		} catch (boost::bad_lexical_cast& e)
		{
			reportIllegalNumberFormat(propName, elem.first, e);
		}
	}

	return val;
}

double BasicConfig::getOptionalDoubleProperty(const String & propName,
		const double defaultVal) throw (IllegalConfigException)
{
	double val = defaultVal;

	pair<String, bool> elem = prop.getProperty(propName);
	if (elem.second)
	{
		try
		{
			val = boost::lexical_cast<double>(elem.first);
		} catch (boost::bad_lexical_cast& e)
		{
			reportIllegalNumberFormat(propName, elem.first, e);
		}
	}

	return val;
}

//=== Mandatory ==

String BasicConfig::getMandatoryProperty(
		const String & propName) throw (IllegalConfigException)
{
	pair<String, bool> elem = prop.getProperty(propName);
	if (!elem.second)
	{
		throw IllegalConfigException(String("Missing property: " + propName
				+ "; props: " + prop.toString()));
	}

	return elem.first;
}

double BasicConfig::getMandatoryDoubleProperty(const String & propName)
		throw (IllegalConfigException)
{
	String str = getMandatoryProperty(propName);

	double val = 0;
	try
	{
		val = boost::lexical_cast<double>(str);
	} catch (boost::bad_lexical_cast& e)
	{
		reportIllegalNumberFormat(propName, str, e);
	}

	return val;
}

int64_t BasicConfig::getMandatoryInt64Property(const String & propName)
		throw (IllegalConfigException)
{
	String str = getMandatoryProperty(propName);

	int64_t val = 0;
	try
	{
		val = boost::lexical_cast<int64_t>(str);
	} catch (boost::bad_lexical_cast& e)
	{
		reportIllegalNumberFormat(propName, str, e);
	}

	return val;
}

int32_t BasicConfig::getMandatoryInt32Property(const String & propName)
		throw (IllegalConfigException)
{
	String str = getMandatoryProperty(propName);

	int32_t val = 0;
	try
	{
		val = boost::lexical_cast<int32_t>(str);
	} catch (boost::bad_lexical_cast& e)
	{
		reportIllegalNumberFormat(propName, str, e);
	}

	return val;
}

//=== error handling ===

void BasicConfig::reportIllegalNumberFormat(const String& propName,
		const String& value, const std::exception& cause)
		throw (IllegalConfigException)
{
	String what = "Illegal number format in property: " + propName + " = " + value
					+ "; " + cause.what();

	throw IllegalConfigException(what);
}

String BasicConfig::toString() const
{
	return prop.toString();
}

}

