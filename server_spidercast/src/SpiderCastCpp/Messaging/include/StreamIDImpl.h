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
 * StreamIDImpl.h
 *
 *  Created on: Jan 29, 2012
 */

#ifndef STREAMIDIMPL_H_
#define STREAMIDIMPL_H_

//#include <boost/operators.hpp>

#include "StreamID.h"

namespace spdr
{

namespace messaging
{

class StreamIDImpl: public StreamID
{
public:
	static const std::size_t Size = 16;

	StreamIDImpl();

	~StreamIDImpl();

	StreamIDImpl(uint64_t prefix, uint64_t suffix);

	StreamIDImpl(const StreamIDImpl& other);

	const StreamIDImpl& operator=(const StreamIDImpl& other);

	std::size_t hash_value() const;

	bool equalsPrefix(const StreamID& other) const;

	bool operator==(const StreamID& other) const;
	bool operator!=(const StreamID& other) const;

	bool operator<(const StreamID& other) const;
	bool operator<=(const StreamID& other) const;
	bool operator>(const StreamID& other) const;
	bool operator>=(const StreamID& other) const;

	std::string toString() const;

	inline uint64_t getPrefix() const
	{
		return streamID_.first;
	}

	inline uint64_t getSuffix() const
	{
		return streamID_.second;
	}
};

}

}

#endif /* STREAMIDIMPL_H_ */
