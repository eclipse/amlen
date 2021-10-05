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
 * PropertyMap.h
 *
 *  Created on: 22/02/2010
 */

#ifndef SPDR_PROPERTYMAP_H_
#define SPDR_PROPERTYMAP_H_

#include <iostream>
#include <fstream>

#include "Definitions.h"
#include "SpiderCastRuntimeError.h"

namespace spdr
{

//using boost::shared_ptr;
//using std::string;
//using std::istream;
//using std::ostream;
//using std::pair;

/**
 * This object is a map that holds string key-value pairs.
 *
 * The PropertyMap is a map that holds string key-value pairs (StringStringMap), and adds
 * convenience methods for their manipulation.
 *
 * It is used as the input for various configuration objects.
 *
 * One of the methods allows to load a PropertyMap from a text representation of the
 * key-value pairs, given in the form of an input stream.
 *
 * @see StringStringMap
 *
 */
class PropertyMap : public StringStringMap
{
public:
	/**
	 * Construct with empty map
	 *
	 * @return
	 */
	PropertyMap();

	/**
	 * Construct from a map.
	 *
	 * @param map The parameter map is deep copied into the PropertyConfig.
	 * @return
	 */
	PropertyMap(const StringStringMap & map);

	/**
	 * Copy constructor.
	 *
	 * @param propMap
	 * @return
	 */
	PropertyMap(const PropertyMap & propMap);

	/**
	 * Assignment operator.
	 *
	 * @param map
	 * @return
	 */
	PropertyMap& operator=(const PropertyMap & map);

	virtual ~PropertyMap();

	/**
	 * Set (and replace) a key-value pair, returning the old value.
	 *
	 * @param key
	 * @param value
	 * @return a pair holding the old value, and a flag that signals whether the old value exists. If the
	 * 		old value does not exist - returns <"",false>.
	 */
	std::pair<String,bool> setProperty(const String& key, const String& value);

	/**
	 * Get a value by its key.
	 *
	 * @param key
	 * @return a pair holding the wanted value, and a flag that signals whether the value exists. If the
	 * 		key-value do not exist - returns <"",false>.
	 */
	std::pair<String,bool> getProperty(const String& key) const;

	/**
	 * Remove a pair according to key, returning the old value.
	 *
	 * @param key
	 * @return a pair holding the old value, and a flag that signals whether the old value exists. If the
	 * 		old value does not exist - returns <"",false>.
	 */
	std::pair<String,bool> removeProperty(const String& key);

	/**
	 * Whether a key exists.
	 *
	 * @param key
	 * @return true if the key exists in the map.
	 */
	const bool containsKey(const String& key) const;

	/**
	 * Loads properties from an input stream.
	 *
	 * Expected format:
	 *
	 * Each property in a single line (TODO improve).
	 * A property is a key value pair, the separator is the first occurrence of '='.
	 * White space before the key, after the value, or surrounding the separator is trimmed.
	 *
	 * The key must not be an empty string, the value can be.
	 * A line without a separator is not allowed, an exception is thrown in that case.
	 *
	 * An empty line is ignored.
	 * If the line starts with '#' after trimming leading whitespace, the line is a comment.
	 *
	 *
	 * @param inputStream
	 * @throw std::ios_base::failure If a general IO error happens to the stream (badbit set)
	 * @throw SpiderCastRuntimeError If the properties are ill-formed
	 */
	void load(std::istream& inputStream) throw(std::ios_base::failure, SpiderCastRuntimeError);

	/**
	 * A string representation of the map.
	 *
	 * @return
	 */
	String toString() const;

	/**
	 * Send to an output stream operator.
	 *
	 * @param os
	 * @param map
	 * @return
	 */
	friend std::ostream& operator<<(std::ostream& os, const PropertyMap& map);

};

}

#endif /* SPDR_PROPERTYMAP_H_ */
