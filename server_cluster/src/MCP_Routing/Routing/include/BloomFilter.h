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

#ifndef MCP_BLOOMFILTER_H_
#define MCP_BLOOMFILTER_H_

#include <string>
#include <bitset>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
//#include <boost/dynamic_bitset.hpp>

#include "ASMFilter.h"
//#include "CountingBloomFilter.h"

namespace mcp {

class BloomFilter : public ASMFilter {
public:

	friend class CountingBloomFilter;

	/**
	 *  This function attempts to find the number of hash functions
     *  and minimum amount of storage bits required to construct a bloom
     *  filter consistent with the user defined false positive probability
     *  and estimated element insertion count.
     *
	 *  This implementation is taken from open source
	 *  http://code.google.com/p/bloom/
	 *
	 * @param projected_element_count
	 * @param false_positive_probability
	 * @return
	 */
	// static pair<size_t, uint8_t> computeOptimalParameters(const uint32_t projected_element_count = 64000, const double false_positive_probability = 0.01);


	//TODO: size_t or int?
	// BloomFilter(pair<size_t, uint8_t>, HashType hashType);


	/**
	 * Create a new empty BF
	 */
	BloomFilter(size_t numBins, uint8_t numHashes, mcc_hash_HashType_t hashType);

	/**
	 * Create a new BF and copy the content of the buffer to it
	 */
	BloomFilter(size_t numBins, uint8_t numHashes, mcc_hash_HashType_t hashType, const char * buffer);

	virtual ~BloomFilter();


	/**
	 * Copy the new buffer into the internal buffer.
	 *
	 * If the number of bins is bigger than the internal,
	 * release the old buffer and allocate a new one with the correct size.
	 * In addition, update the number and type of hashes.
	 *
	 * @param numBins
	 * @param numHashes
	 * @param hashType
	 * @param buffer
	 */
	void setContent(size_t numBins, uint8_t numHashes, mcc_hash_HashType_t hashType, const char * buffer);

	/**
	 * Test membership of a given element
	 * @param element
	 * @return true if the element might have been put in this Bloom filter,
	 * 			false if this is definitely not the case.
	 */
	bool contains(const std::string& element);
	bool contains_wHashes(const std::string& element);
	bool contains(const char * element, std::size_t length);
	bool contains_wHashes(const char * element, std::size_t length);
	//TODO virtual bool contains(const char * element) = 0; //NULL-terminated string


	/**
	 * Puts an element into this BloomFilter.
	 * Ensures that subsequent invocations of mightContain() with the same element will always return true.
	 * Return 	true if the bloom filter's bits changed as a result of this operation.
	 * 			If the bits changed, this is definitely the first time object has been added to the filter.
	 * 			If the bits haven't changed, this might be the first time object has been added to the filter.
	 */
	void put(const std::string& element);
	void put(const char * element, std::size_t length);
	//TODO virtual void put(const char * element) = 0; //NULL-terminated string

	void setBinAt(size_t index);
	void resetBinAt(size_t index);
	bool checkBinAt(size_t index);

	bool checkBins(std::vector<uint32_t> bins);
	std::vector<uint32_t> binsOf( const std::string& element );
	std::vector<uint32_t> binsOf( const char * element, std::size_t length );
	//TODO vector<uint32_t> binsOf( const char * element); //NULL-terminated string

	const char * const buffer() const;

//	virtual void setBuffer(const char* buffer);
//
//	virtual bool setBit(int32_t index);
//
//	virtual bool resetBit(int32_t index);
//
//	// virtual size_t getNumBins();
//
//	// virtual size_t getNumHashFunctions();
//
//	virtual size_t estimateNumElements();
//
//	virtual size_t estimateErrorRate();

protected:
	// boost::dynamic_bitset<> m_bits;
	// bitset, or unsigned char *; unit_8 *;

	// uint8_t * m_binBytes;
	char * m_binBuffer;

	//HashFunctions * m_hashFuncs;

private:
	BloomFilter();
	BloomFilter( const BloomFilter & other );
	BloomFilter& operator=( const BloomFilter& other );
};

typedef boost::shared_ptr<BloomFilter> BloomFilter_SPtr;

} /* namespace mcp */

#endif /* BLOOMFILTER_H_ */
