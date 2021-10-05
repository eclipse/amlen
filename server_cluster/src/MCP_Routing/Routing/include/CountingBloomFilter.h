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

#ifndef COUNTINGBLOOMFILTER_H_
#define COUNTINGBLOOMFILTER_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "ASMFilter.h"
#include "BloomFilter.h"
#include "MCPReturnCode.h"

namespace mcp {

//enum BucketSize{
//	FourBits,
//	EightBits,
//};

/**
 * get element count
 *
 * cacl error rate
 */

class CountingBloomFilter : public ASMFilter
{
public:

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
	static std::pair<uint64_t, uint8_t> computeOptimalParameters(const int32_t projected_element_count = 64000, const double false_positive_probability = 0.01);

	/**
	 * Compute the next highest power of 2 of 32-bit x
	 *
	 * Reference:
	 * http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	 *
	 * @param a 64-bit integer x
	 * @return the next highest power of 2 of 32-bit x
	 */
	static uint64_t powerOfTwoRoundUp (uint64_t x) {
	    if (x < 0)
	        return 0;

	    --x;
	    x |= x >> 1;
	    x |= x >> 2;
	    x |= x >> 4;
	    x |= x >> 8;
	    x |= x >> 16;
	    x |= x >> 32;
	    return x+1;
	}

	// CountingBloomFilter(size_t numBuckets, unsigned int numHashes, HashType hashType);
	CountingBloomFilter(size_t numCounters, uint8_t numHashes, mcc_hash_HashType_t hashType, uint8_t bitsPerBucket = 4);
	// CountingBloomFilter(size_t numBuckets, uint8_t numHashes, HashType hashType, BucketSize bitsPerBucket = FourBits);

	virtual ~CountingBloomFilter();

	void copyBuffer(const char *);

	BloomFilter_SPtr produceBloomFilter();
	MCPReturnCode updateBloomFilter (BloomFilter_SPtr bf);

	/**
	 * Put a single element
	 */
	void put(const std::string& element);
	void put(const char * element, std::size_t length);
	//TODO virtual void put(const char * element) = 0; //NULL-terminated string


        std::vector<int32_t> add(const std::string& element);
        std::vector<int32_t> add(const char * element, std::size_t length);

	//TODO vector<int32_t> add(const char * element, std::size_t length); //NULL-terminated string

	/**
	 * Remove a single element
	 */
        std::vector<int32_t> remove(const std::string& element);
        std::vector<int32_t> remove(const char * element, std::size_t length);
	//TODO vector<int32_t> remove(const char * element, std::size_t length); //NULL-terminated string

	/**
	 *
	 * @param element
	 * @return
	 */
	bool contains(const std::string& element);
	bool contains_wHashes(const std::string& element);

	bool contains(const char * element, std::size_t length);
	//TODO bool contains(const char * element); //NULL-terminated string


	// double estimateCurrentErrorRate();

	// boost::shared_ptr<BloomFilter> convertToBloomFilter();

	/**
	 * @return number of bins and number of hash functions
	 */
	//static pair<size_t, std::size_t> calcParameters(std::size_t maxElements, double maxErrorRate);

	uint8_t increaseAt(std::size_t i);
	uint8_t decreaseAt(std::size_t i);

	uint8_t getCountAt(size_t i) const;
	uint8_t setCountAt(size_t i, uint8_t value);

	size_t getNumCounters() const {
		return m_numCounters;
	}

	int32_t getNumElements() const {
		return m_numElements;
	}

	/**
	 *
	 * @return
	 */
	double estimateFPP() const;

	double getDesiredFPP() const {
		return m_desiredFPP;
	}

	void setDesiredFPP(double falsePositiveProbability) {
		m_desiredFPP = falsePositiveProbability;
	}

	int32_t getProjectedNumElements() const {
		return m_projectedNumElements;
	}

	void setProjectedNumElements(int32_t projectedElementCount) {
		m_projectedNumElements = projectedElementCount;
	}

	uint8_t getCounterSize() const {
		return m_counterSize;
	}

protected:
	// vector<int> m_buckets;
	// boost::dynamic_bitset<> m_bits;

	// Need to use "unsigned char"
	// http://stackoverflow.com/questions/8385824/bytewise-reading-of-memory-signed-char-vs-unsigned-char
	// unsigned char * m_bucketBuffer;

	/**
	 * Number of counters
	 */
	size_t m_numCounters;

	/**
	 * Number of bits per bucket: usually 4 bits, could be 8 bits
	 */
	// BucketSize m_bucketSize;
	uint8_t m_counterSize; // number of bits per counter

	/**
	 * Bit buffer to store count at each bucket
	 */
	// vector<uint8_t> m_bucketBytes; // uint8_t *
	std::vector<char> m_counterBuffer;

	/**
	 * The number of elements currently inserted
	 */
	int32_t m_numElements;


	/**
	 * The projected number of elements
	 */
	int32_t m_projectedNumElements;

	/**
	 * Desired false positive probability
	 */
	double m_desiredFPP;

private:
	CountingBloomFilter();
	CountingBloomFilter( const CountingBloomFilter & other );
	CountingBloomFilter& operator=( const CountingBloomFilter& other );

};

typedef boost::shared_ptr<CountingBloomFilter> CountingBloomFilter_SPtr;

} /* namespace mcp */

#endif /* COUNTINGBLOOMFILTER_H_ */
