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
 * StreamID.h
 *
 *  Created on: Jan 29, 2012
 */

#ifndef STREAMID_H_
#define STREAMID_H_

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

namespace spdr
{

namespace messaging
{

/**
 * A unique identifier of a transport entity.
 * <p>
 * Identifies an instance of:
 * TopicPublisher
 * <p>
 * The implementation of StreamID is immutable, and provides equality and hash_value() operators.
 * <p>
 * The implementation of StreamID is composed of two distinct parts, the prefix
 * and the suffix. The prefix is a hash of the node identifiers (such as ID,
 * IP-address, port, etc.). It identifies an instance of the node.
 *
 * The suffix is composed of the instance creation time, and a counter that
 * starts from zero, and is incremented whenever a new StreamID is generated at
 * that instance.
 *
 * Thus, there is no ordering among the prefixes, but they can be
 * checked for equality. The suffixes of StreamID's with an equal prefix can be
 * ordered to determine which stream is "older".
 *
 * In order to use boost::shared_ptr<StreamID> in STL containers, see the class templates
 * spdr::SPtr_Hash, spdr::SPtr_Equals, spdr::SPtr_Less, spdr::SPtr_Greater.
 *
 */
class StreamID
{
public:
	virtual ~StreamID();

	/**
	 * Compare the prefixes of two StreamID's for equality.
	 *
	 * If the prefixes match, the relational operators establish the relation among the suffixes.
	 *
	 * @param other
	 *
	 * @return
	 */
	virtual bool equalsPrefix(const StreamID& other) const = 0;

	virtual bool operator==(const StreamID& other) const = 0;
	virtual bool operator!=(const StreamID& other) const = 0;

	virtual bool operator<(const StreamID& other) const = 0;
	virtual bool operator<=(const StreamID& other) const = 0;
	virtual bool operator>(const StreamID& other) const = 0;
	virtual bool operator>=(const StreamID& other) const = 0;

	/**
	 * Returns the hash value.
	 *
	 * @return
	 */
	virtual std::size_t hash_value() const = 0;

	virtual std::string toString() const = 0;

protected:
	StreamID();
	StreamID(uint64_t prefix, uint64_t suffix);

	std::pair<uint64_t, uint64_t> streamID_;
};

/**
 * A shared pointer to a StreamID.
 */
typedef boost::shared_ptr<StreamID> StreamID_SPtr;

}

}

#endif /* STREAMID_H_ */
