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
 * BasicConfig.h
 *
 *  Created on: 24/02/2010
 */

#ifndef BASICCONFIG_H_
#define BASICCONFIG_H_

#include "Definitions.h"
#include "PropertyMap.h"
#include "SpiderCastLogicError.h"

#include <boost/cstdint.hpp>

namespace spdr
{
using std::pair;

/**
 * The base class of all configuration objects.
 *
 * This class contains a PropertyMap and has utility methods for parsing property keys and values
 * into the correct data types.
 *
 * @see SpiderCastConfig
 */
class BasicConfig
{

public:

	/**
	 * Construct from a  PropertyMap.
	 * @param properties
	 * @return
	 */
	BasicConfig(const PropertyMap& properties);
	/**
	 * Copy constructor.
	 * @param config
	 * @return
	 */
	BasicConfig(const BasicConfig& config);
	/**
	 * Assignment operator.
	 * @param config
	 * @return
	 */
	BasicConfig& operator=(const BasicConfig& config);

	/**
	 * Destructor.
	 * @return
	 */
	virtual ~BasicConfig();

	/**
	 * Get the value that corresponds to the property key.
	 *
	 * The method returns ("",false) if the key is not found.
	 *
	 * @param key
	 * @return String
	 */
	pair<String,bool> getProperty(const String& key);

	// === MANDATORY PROPERTIES ACCESSORS ===

	/**
	 * Get the value of a mandatory property, parsed as a double.
	 *
	 * If the property is not found, throws an exception.
	 *
	 * @param propName
	 * @return the value mapped to the property name.
	 * @throw IllegalConfigException
	 */
	double getMandatoryDoubleProperty(const String& propName)
			throw (IllegalConfigException);

	/**
	 * Get the value of a mandatory property, parsed as a int64_t.
	 *
	 * If the property is not found, throws an exception.
	 *
	 * @param propName
	 * @return the value mapped to the property name.
	 * @throw IllegalConfigException
	 */
	int64_t getMandatoryInt64Property(const String& propName)
			throw (IllegalConfigException);

	/**
	 * Get the value of a mandatory property, parsed as a int32_t.
	 *
	 * If the property is not found, throws an exception.
	 *
	 * @param propName
	 * @return the value mapped to the property name.
	 * @throw IllegalConfigException
	 */
	int32_t getMandatoryInt32Property(const String& propName)
			throw (IllegalConfigException);

	/**
	 * Get the String value of a mandatory property.
	 *
	 * If the property is not found, throws an exception.
	 *
	 * @param propName
	 * @return the value mapped to the property name.
	 * @throw IllegalConfigException
	 */
	String getMandatoryProperty(const String& propName)
			throw (IllegalConfigException);

	// === OPTIONAL PROPERTIES ACCESSORS ===

	/**
	 * Get an optional 64 bit integer property.
	 *
	 * If the property is not found, returns the default value.
	 *
	 * @param propName
	 * @param defaultVal
	 * @return long
	 * @throw IllegalConfigException
	 */
	int64_t getOptionalInt64Property(const String& propName, const int64_t defaultVal)
			throw (IllegalConfigException);

	/**
	 * Get an optional 32 bit integer property.
	 *
	 * If the property is not found, returns the default value.
	 *
	 *
	 * @param propName
	 * @param defaultVal
	 * @return int
	 * @throw IllegalConfigException
	 */
	int32_t	getOptionalInt32Property(const String& propName,
					const int32_t defaultVal) throw (IllegalConfigException);

	/**
	 * Get an optional double property.
	 *
	 * If the property is not found, returns the default value.
	 *
	 *
	 * @param propName
	 * @param defaultVal
	 * @return int
	 * @throw IllegalConfigException
	 */
	double getOptionalDoubleProperty(const String& propName,
			const double defaultVal) throw (IllegalConfigException);

	/**
	 * Get an optional boolean property.
	 *
	 * If the property is not found, returns the default value.
	 *
	 * @param propName
	 * @param defaultVal
	 * @return boolean
	 * @throw IllegalConfigException
	 */
	bool getOptionalBooleanProperty(const String& propName,
			const bool defaultVal) throw (IllegalConfigException);

	/**
	 * Get an optional String property.
	 *
	 * If the property is not found, returns the default value.
	 *
	 * @param propName
	 * @param defaultVal
	 * @return String
	 * @throw IllegalConfigException
	 */
	String getOptionalProperty(const String& propName,
			const String& defaultVal);

	/**
	 * The size of the property map.
	 *
	 * @return
	 */
	int size()
	{
		return prop.size();
	}

	/**
	 * Is the property map empty?
	 *
	 * @return
	 */
	bool empty()
	{
		return prop.empty();
	}

	/**
	 * A string representation.
	 * @return
	 */
	virtual String toString() const;

	/**
	 * Re-throw the bad_lexical_cast exception as an IllegalConfigException,
	 * with the name of the property and value that caused the problem.
	 *
	 * @param propName
	 * @param value
	 * @param cause
	 * @throw IllegalConfigException
	 */
	static void reportIllegalNumberFormat(const String& propName, const String& value,
			const std::exception& cause)
	throw(IllegalConfigException);

protected:
	/**
	 * The internal property map
	 */
	PropertyMap prop;

private:
	BasicConfig();



};
}
#endif /* BASICCONFIG_H_ */
