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
 * Definitions.h
 *
 *  Created on: 31/01/2010
 */

#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#include <string>
#include <map>
#include <set>
#include <exception>

#include <boost/shared_ptr.hpp>

namespace spdr
{

/**
 * Another name for std::string.
 */
typedef std::string String;
/**
 * An immutable std::string.
 */
typedef const std::string ConstString;
/**
 * A set of Strings.
 */
typedef std::set<std::string> StringSet;
/**
 * A boost::shared_ptr to a String.
 */
typedef boost::shared_ptr<String> StringSPtr;
/**
 * A boost::shared_ptr to a ConstString.
 */
typedef boost::shared_ptr<ConstString> ConstStringSPtr;

/**
 * A boost::shared_ptr to a StringSet.
 */
typedef boost::shared_ptr<StringSet> StringSetSPtr;

/**
 * A map of string keys and string values.
 */
typedef std::map<std::string, std::string> StringStringMap;

/**
 * Another name for std::exception.
 */
typedef std::exception Exception;

/**
 * A mutable byte buffer.
 * The first member is the length, and the second is a pointer to a dynamically allocated
 * byte buffer.
 *
 * A Mutable_Buffer can be assigned into a Const_Buffer but not vice versa.
 *
 * @see Const_Buffer
 */
typedef std::pair<int32_t, char*> Mutable_Buffer;

/**
 * An immutable byte buffer.
 *
 * The first member is the length, and the second is a pointer to a dynamically allocated
 * byte buffer.
 *
 * A Mutable_Buffer can be assigned into a Const_Buffer but not vice versa.
 *
 * @see Mutable_Buffer
 */
typedef std::pair<int32_t, const char*> Const_Buffer;

/**
 * A safe to-string of smart pointer to class that supports toString()
 * @param a
 * @return
 */
template<class T>
String toString(const boost::shared_ptr<T> & a)
{
	return (a ? (*a).toString() : "null");
}

/**
 * A helper function object for STL associative containers.
 *
 * Works with a container that holds boost::shared_ptr<T>.
 */
template<class T>
class SPtr_Equals
{
public:
	/**
	 * Compares the two T objects with ==.
	 * @param a1
	 * @param a2
	 * @return true if (*a1) == (*a2)
	 */
	bool operator()(const boost::shared_ptr<T> & a1,
			const boost::shared_ptr<T> & a2) const
	{
		return ((*a1) == (*a2));
	}
};

/**
 * A helper function object for STL associative containers.
 *
 * Works with a container that holds boost::shared_ptr<T>.
 */
template<class T>
class SPtr_Hash
{
public:
	/**
	 * Compute the hash value of a T object.
	 * @param a
	 * @return hash value of
	 */
	std::size_t operator()(const boost::shared_ptr<T> & a) const
	{
		return (a->hash_value());
	}
};

/**
 * A helper function object for STL container sort.
 *
 * Works with a container that holds boost::shared_ptr<T>.
 */
template<class T>
class SPtr_Less
{
public:
	/**
	 * Compares the two T objects with <.
	 * @param a1
	 * @param a2
	 * @return true if (*a1) < (*a2)
	 */
	bool operator()(const boost::shared_ptr<T> & a1,
			const boost::shared_ptr<T> & a2) const
	{
		return ((*a1) < (*a2));
	}
};

/**
 * A helper function object for STL container sort.
 *
 * Works with a container that holds boost::shared_ptr<T>.
 */
template<class T>
class SPtr_Greater
{
public:
	/**
	 * Compares the two T objects with >.
	 * @param a1
	 * @param a2
	 * @return true if (*a1) > (*a2)
	 */
	bool operator()(const boost::shared_ptr<T> & a1, const boost::shared_ptr<T> & a2) const
	{
		return ((*a1) > (*a2));
	}
};
} //namespace spdr
#endif /* DEFINITIONS_H_ */
