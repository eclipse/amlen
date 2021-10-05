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
 * VirtualID.h
 *
 *  Created on: Apr 17, 2011
 */

#ifndef SPDR_UTIL_VIRTUALID_H_
#define SPDR_UTIL_VIRTUALID_H_

#include <boost/cstdint.hpp>
#include <boost/operators.hpp>

#include "SHA1.h"

namespace spdr
{
namespace util
{

class VirtualID;
typedef boost::shared_ptr<VirtualID> VirtualID_SPtr;

class VirtualID
{
public:

	static const std::size_t HashSize = 20;

	/**
	 * All zeros.
	 */
	static const VirtualID MinValue;
	/**
	 * All ones.
	 */
	static const VirtualID MaxValue;
	/**
	 * One.
	 */
	static const VirtualID OneValue;
	/**
	 * 2^159.
	 */
	static const VirtualID MiddleValue;



	/**
	 * All zeros.
	 * @return
	 */
	VirtualID();

	/**
	 * Construct from an array of uint32_t.
	 * Array expected to be of size>=5, big-endian.
	 * @return
	 */
	VirtualID(const uint32_t* digest_array5);

	/**
	 * Construct from an array of char.
	 * Array expected to be of size>=20, big-endian.
	 * @return
	 */
	VirtualID(const char* digest_array20);

	/**
	 * Construct from five 32 bit integers.
	 * Big-endian, w0 is MSB, w4 is LSB.
	 * @return
	 */
	VirtualID(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3, uint32_t w4);

	/**
	 * Construct from a SHA1 object.
	 * Assumes the SHA1 object is fully updated, calls digest().
	 *
	 * @param sha1
	 * @return
	 *
	 * @throw IllegalArgumentException if SHA1 is corrupted
	 */
	VirtualID(SHA1& sha1);

	VirtualID(const VirtualID& other);

	VirtualID& operator=(const VirtualID& other);

	/**
	 * Create VirtualID from a random point on a unit ring.
	 *
	 * @param random a double in [0,1]
	 * @return
	 */
	static VirtualID createFromRandom(double random);


	virtual ~VirtualID();

	/**
	 * Copy to an array of char.
	 * Array expected to be of size>=20, format is big-endian.
	 * @return
	 */
	void copyTo(char* array20) const;

	/**
	 * Gel the lower 64 bits.
	 * @return
	 */
	uint64_t getLower64() const;

	String toString() const;

	//=== arithmetic ===
	/**
	 * Shift right, inserting leading zeros.
	 *
	 * @param n number of bits to shift, must be >=0
	 */
	void shiftRight(size_t n);

	/**
	 * Add to this VID, cyclic.
	 *
	 * Addition means going clock-wise from this, a distance of 'x'.
	 *
	 * @param x
	 */
	void add(const VirtualID& x);

	/**
	 * Subtract from this node VID, cyclic.
	 *
	 * Subtraction means going counter-clock-wise from this node, a distance of 'x'.
	 *
	 * @param x
	 */
	void sub(const VirtualID& x);

	/**
	 * The absolute distance between this node and x.
	 *
	 * this = min(this-x,x-this)
	 *
	 * @param x
	 */
	void absDist(const VirtualID& x);


	/**
	 * Add x to y, cyclic.
	 *
	 * Addition means going clock-wise from x, a distance of 'y'.
	 *
	 * @param x
	 */
	friend
	VirtualID add(const VirtualID& x, const VirtualID& y);

	/**
	 * Subtract y from x, cyclic.
	 *
	 * Subtraction means going counter-clock-wise from x, a distance of 'y'.
	 *
	 * @param x
	 */
	friend
	VirtualID sub(const VirtualID& x, const VirtualID& y);

	/**
	 *
	 * @param
	 * @return
	 */
	friend
	VirtualID absDist(const VirtualID& x,const VirtualID& y);

	//=== relational operators ===

	friend
	bool operator<(const VirtualID& lhs, const VirtualID& rhs);

	friend
	bool operator>(const VirtualID& lhs, const VirtualID& rhs);

	friend
	bool operator<=(const VirtualID& lhs, const VirtualID& rhs);

	friend
	bool operator>=(const VirtualID& lhs, const VirtualID& rhs);

	friend
	bool operator==(const VirtualID& lhs, const VirtualID& rhs);

	friend
	bool operator!=(const VirtualID& lhs, const VirtualID& rhs);

	inline std::size_t hash_value() const
	{
		return ((size_t)w[4]);
	}

	friend
	std::size_t hash_value( VirtualID const& id)
	{
		return id.hash_value();
	}

	//=== Helper Classes ==

	/**
	 * A helper function object for STL associative containers.
	 *
	 * Works with a container that holds VirtualID_SPtr.
	 */
	class SPtr_Equals
	{
	public:
		/**
		 * Compares the two VirtualID_SPtr objects with ==.
		 * @param a1
		 * @param a2
		 * @return true if (*a1) == (*a2)
		 */
		bool operator()(const VirtualID_SPtr & a1, const VirtualID_SPtr & a2) const
		{
			return ((*a1) == (*a2));
		}
	};

	/**
	 * A helper function object for STL associative containers.
	 *
	 * Works with a container that holds VirtualID_SPtr.
	 */
	class SPtr_Hash
	{
	public:
		/**
		 * Computes the hash of a VirtualID_SPtr.
		 * @param a
		 * @return hash
		 */
		std::size_t operator()(const VirtualID_SPtr & a) const
		{
			return (a->hash_value());
		}
	};

	/**
	 * A helper function object for STL container sort.
	 *
	 * Works with a container that holds VirtualID_SPtr.
	 */
	class SPtr_Less
	{
	public:
		/**
		 * Compares the two VirtualID_SPtr objects with <.
		 * @param a1
		 * @param a2
		 * @return true if (*a1) < (*a2)
		 */
		bool operator()(const VirtualID_SPtr & a1, const VirtualID_SPtr & a2) const
		{
			return ((*a1) < (*a2));
		}
	};

	/**
	 * A helper function object for STL container sort.
	 *
	 * Works with a container that holds VirtualID_SPtr.
	 */
	class SPtr_Greater
	{
	public:
		/**
		 * Compares the two NodeID objects with >.
		 * @param a1
		 * @param a2
		 * @return true if (*a1) > (*a2)
		 */
		bool operator()(const VirtualID_SPtr & a1, const VirtualID_SPtr & a2) const
		{
			return ((*a1) > (*a2));
		}
	};

private:

	/*
	 * big-endian: w[0] is MSB.
	 * Initialized to all zero.
	 */
	uint32_t w[5];
};


}
}
#endif /* SPDR_UTIL_VIRTUALID_H_ */
