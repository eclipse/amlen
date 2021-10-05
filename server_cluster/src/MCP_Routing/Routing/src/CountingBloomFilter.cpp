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

#include <cmath>
#include <stdio.h>
#include <cstring>
#include <stdexcept>
#include <sstream>

#include "CountingBloomFilter.h"
#include "MCPReturnCode.h"
#include "MCPExceptions.h"

//#define __STDC_LIMIT_MACROS
//#include <stdint.h>

namespace mcp
{

/*
 * This implementation is taken from open source
 * http://code.google.com/p/bloom/
 *
 */
//pair<size_t, uint8_t> CountingBloomFilter::computeOptimalParameters(
//		const int32_t projected_element_count, const double false_positive_probability) {
//
//	double min_m = std::numeric_limits<double>::infinity();
//	double min_k = 0.0;
//	double curr_m = 0.0;
//	double k = 1.0;
//
//	while (k < 1000.0) {
//		double numerator = (-k * projected_element_count);
//		double denominator = log(
//				1.0 - pow(false_positive_probability, 1.0 / k));
//		curr_m = numerator / denominator;
//		if (curr_m < min_m) {
//			min_m = curr_m;
//			min_k = k;
//		}
//		k += 1.0;
//	}
//
//	size_t numBins = static_cast<size_t>(min_m);
//	uint8_t numHashes = static_cast<uint8_t>(min_k);
//
//	return std::make_pair(numBins, numHashes);
//}
/*
 * input:
 * n: projected number of elements
 * p: desired error rate
 *
 * output:
 * m: number of bits in the filter
 * k: number of hash functions
 *
 */
std::pair<uint64_t, uint8_t> CountingBloomFilter::computeOptimalParameters(
        const int32_t n, const double p)
{
    uint64_t m;
    uint8_t k;

    m = static_cast<uint64_t>(-(n * log(p)) / (pow(log(2.0), 2)));
    k = static_cast<uint8_t>(round(log(2.0) * m / n));

    m = powerOfTwoRoundUp(m);
    return std::make_pair(m, k);
}

CountingBloomFilter::CountingBloomFilter(size_t numCounters, uint8_t numHashes,
        mcc_hash_HashType_t hashType, uint8_t bitsPerCounter) :
        ASMFilter(numCounters * bitsPerCounter, numHashes, hashType), m_numCounters(
                numCounters), m_counterSize(bitsPerCounter), m_counterBuffer(
                (bitsPerCounter == 8 ?
                        numCounters : (numCounters / 2 + numCounters % 2)), 0), m_numElements(
                0), m_projectedNumElements(1024), m_desiredFPP(0.01)
{

    // m_hashes = new HashFunctions(m_numBuckets, m_numHashes, m_hashType);
}

//CountingBloomFilter::CountingBloomFilter(size_t numBuckets, uint8_t numHashes, HashType hashType, BucketSize bitsPerBucket )
//		: ASMFilter(numBuckets* (bitsPerBucket==EightBits?8:4), numHashes, hashType),
//		  m_numBuckets(numBuckets),
//		  m_bucketSize(bitsPerBucket),
//		  m_counterBuffer( bitsPerBucket == EightBits ? numBuckets : (numBuckets/2 + numBuckets%2) ),
//		  m_elementCount(0)
//{
//	m_hashes = new HashFunctions(m_numBuckets, m_numHashes, m_hashType);
//}

CountingBloomFilter::~CountingBloomFilter()
{
    //std::cout << "in ~CountingBloomFilter() " << std::endl;
    //m_counterBuffer.clear();
}

void CountingBloomFilter::copyBuffer(const char * bytes)
{
    for (uint8_t i = 0; i < m_counterBuffer.size(); i++)
    {
        m_counterBuffer[i] = bytes[i];
    }
}

BloomFilter_SPtr CountingBloomFilter::produceBloomFilter()
{
    boost::shared_ptr<BloomFilter> pBloomFilter(
            new BloomFilter(m_numCounters, m_numHashes, m_hashType));

    for (size_t i = 0; i < m_numCounters; i++)
    {
        if (getCountAt(i) > 0)
            pBloomFilter->setBinAt(i);
//		else
//			pBloomFilter->resetBinAt(i);
    }

    return pBloomFilter;
}

MCPReturnCode CountingBloomFilter::updateBloomFilter(BloomFilter_SPtr bf)

{
    if (!bf)
    {
        return ISMRC_NullPointer;
    }

    if (getNumCounters() != bf->getNumBits())
    {
        bf->m_numBits = getNumCounters();

        delete[] bf->m_binBuffer;
        size_t numBytes = (bf->m_numBits / 8)
                + ((bf->m_numBits % 8) > 0 ? 1 : 0);
        try
        {
            bf->m_binBuffer = new char[numBytes];
        }
        catch (std::bad_alloc& e)
        {
            return ISMRC_AllocateError;
        }
        memset(bf->m_binBuffer, 0, numBytes);
    }

    bf->m_numHashes = m_numHashes;
    bf->assignHashFunction(m_hashType);

    for (size_t i = 0; i < m_numCounters; i++)
    {
        if (getCountAt(i) > 0)
            bf->setBinAt(i);
        else
            bf->resetBinAt(i);
    }

    return ISMRC_OK;
}

//boost::shared_ptr<BloomFilter> CountingBloomFilter::produceBloomFilter(){
//	boost::dynamic_bitset<> bits(m_numBuckets);
//	for ( size_t i = 0; i < m_numBuckets; i++ ) {
//		if ( getCountAt(i) > 0 ) {
//			bits[i] = 1;
//		}
//	}
//	boost::shared_ptr<BloomFilter> pBloomFilter (new BloomFilter(m_numBuckets, m_numHashes, m_hashType, bits));
//
//	return pBloomFilter;
//}

//bool CountingBloomFilter::testMembership(const std::string& element) {
//	m_hashes->prepareMembershipTest(element);
//
//	for(unsigned int i = 0; i < m_numHashes; i++) {
//	    	uint32_t h = m_hashes->nextHash();
//	    	if ( m_buckets[h] == 0 ) {
//	    		m_hashes->clear();
//	    		return false;
//	    	}
//	    }
//
//	m_hashes->clear();
//	return true;
//}
//
//bool CountingBloomFilter::testMembership_wHashes(const std::string& element) {
//
//	vector<uint32_t> h = m_hashes->hashes(element);
//
//    for(unsigned int i = 0; i < m_numHashes; i++) {
//    	if ( m_buckets[h[i]] == 0 ){
//    		m_hashes->clear();
//    		return false;
//    	}
//    }
//
//    m_hashes->clear();
//	return true;
//}
//
//void CountingBloomFilter::add(const std::string& element) {
//	vector<uint32_t> h = m_hashes->hashes(element);
//	for( unsigned int i = 0; i < m_numHashes; i++ ) {
//	    	m_buckets[h[i]]++;		// TODO: check whether this bucket overflows
//	}
//}
//
//bool CountingBloomFilter::remove(const std::string& element) {
//    if( testMembership(element)==false )
//      return false;
//
//    vector<uint32_t> h = m_hashes->hashes(element);
//
//    for ( unsigned int i = 0; i < m_numHashes; i++ ) {
//    	m_buckets[h[i]]--;
//    }
//
//	return true;
//}

bool CountingBloomFilter::contains(const std::string& element)
{
    //	HashFunctions hashes(m_numCounters, m_numHashes, m_hashType);
    //	hashes.prepareMembershipTest(element);

    uint32_t h[m_numHashes];
    m_hashFunctionsPtr(element.data(), element.size(), m_numHashes,
            m_numCounters, h);

    for (unsigned int i = 0; i < m_numHashes; i++)
    {
        //uint32_t h = hashes.nextHash();
        if (getCountAt(h[i]) <= 0)
        {
            return false;
        }
    }

    return true;
}

bool CountingBloomFilter::contains_wHashes(const std::string& element)
{
//	HashFunctions hashes(m_numCounters, m_numHashes, m_hashType);
//	vector<uint32_t> h = hashes.hashes(element);

    uint32_t h[m_numHashes];
    m_hashFunctionsPtr(element.data(), element.size(), m_numHashes,
            m_numCounters, h);

    for (unsigned int i = 0; i < m_numHashes; i++)
    {
        if (getCountAt(h[i]) <= 0)
        {
            return false;
        }
    }

    return true;
}

bool CountingBloomFilter::contains(const char * element, size_t length)
{
//	HashFunctions hashes(m_numCounters, m_numHashes, m_hashType);
//	hashes.prepareMembershipTest(element, length);

    uint32_t h[m_numHashes];
    m_hashFunctionsPtr(element, length, m_numHashes, m_numCounters, h);

    for (unsigned int i = 0; i < m_numHashes; i++)
    {
        //uint32_t h = hashes.nextHash();
        if (getCountAt(h[i]) <= 0)
        {
            return false;
        }
    }

    return true;
}

void CountingBloomFilter::put(const std::string& element)
{
    add(element);
}

void CountingBloomFilter::put(const char * element, size_t length)
{
    add(element, length);
}

std::vector<int> CountingBloomFilter::add(const std::string& element)
{
    using namespace std;

    vector<int> addedIndices;

//	HashFunctions hashes(m_numCounters, m_numHashes, m_hashType);
//	vector<uint32_t> h = hashes.hashes(element);
    uint32_t h[m_numHashes];
    m_hashFunctionsPtr(element.data(), element.size(), m_numHashes,
            m_numCounters, h);

    for (unsigned int i = 0; i < m_numHashes; i++)
    {
        if (increaseAt(h[i]) == 1)
        {
            addedIndices.push_back(h[i] + 1); // convert to 1-index
        }
    }

    m_numElements++;

    return addedIndices;
}

std::vector<int> CountingBloomFilter::add(const char * element,
        const size_t length)
{
    using namespace std;

    vector<int> addedIndices;
//	HashFunctions hashes(m_numCounters, m_numHashes, m_hashType);
//	vector<uint32_t> h = hashes.hashes(element, length);

    uint32_t h[m_numHashes];
    m_hashFunctionsPtr(element, length, m_numHashes, m_numCounters, h);

    for (unsigned int i = 0; i < m_numHashes; i++)
    {
        if (increaseAt(h[i]) == 1)
        {
            addedIndices.push_back(h[i] + 1); // convert to 1-index
        }
    }

    m_numElements++;

    return addedIndices;
}

std::vector<int> CountingBloomFilter::remove(const std::string& element)
{
    using namespace std;

    vector<int> removedIndices;

    if (contains(element))
    {
//		HashFunctions hashes(m_numCounters, m_numHashes, m_hashType);
//		vector<uint32_t> h = hashes.hashes(element);

        uint32_t h[m_numHashes];
        m_hashFunctionsPtr(element.data(), element.size(), m_numHashes,
                m_numCounters, h);

        for (unsigned int i = 0; i < m_numHashes; i++)
        {
            if (decreaseAt(h[i]) == 0)
            {
                removedIndices.push_back(-(h[i] + 1)); // convert to 1-index
            }
        }
    }

    m_numElements--;
    return removedIndices;
}

std::vector<int> CountingBloomFilter::remove(const char * element,
        const size_t length)
{
    using namespace std;

    vector<int> removedIndices;

    if (contains(element, length))
    {
//		HashFunctions hashes(m_numCounters, m_numHashes, m_hashType);
//		vector<uint32_t> h = hashes.hashes(element,length);

        uint32_t h[m_numHashes];
        m_hashFunctionsPtr(element, length, m_numHashes, m_numCounters, h);

        for (unsigned int i = 0; i < m_numHashes; i++)
        {
            if (decreaseAt(h[i]) == 0)
            {
                removedIndices.push_back(-(h[i] + 1)); // convert to 1-index
            }
        }
    }

    m_numElements--;
    return removedIndices;
}

uint8_t CountingBloomFilter::increaseAt(size_t i)
{
    using namespace std;

    if (i < 0 || i >= m_numCounters)
    {
        std::ostringstream what;
        what
                << "Invalid argument in CountingBloomFilter::increaseAt(size_t): i="
                << i << " #bits=" << m_numBits << " m_numCounters="
                << m_numCounters;
        throw invalid_argument(what.str());
        // return ISMRC_RtngIllegalArgument;
    }

    uint8_t oldCount = getCountAt(i);
    uint8_t newCount = oldCount + 1;

    // check whether the count is overflow
    if ((m_counterSize == 4 && newCount > 15)
            || (m_counterSize == 8 && oldCount == 255))
    {
        throw logic_error(
                "Counter Overflow in CountingBloomFilter::increaseAt(size_t)");
    }

    setCountAt(i, newCount);

    return newCount;
}

uint8_t CountingBloomFilter::decreaseAt(size_t i)
{
    using namespace std;

    if (i < 0 || i >= m_numCounters)
    {
        throw invalid_argument(
                "Invalid argument in CountingBloomFilter::decreaseAt(size_t)");
        // return ISMRC_RtngIllegalArgument;
    }

    uint8_t oldCount = getCountAt(i);
    if (oldCount <= 0)
        throw logic_error(
                "Counter Overflow in CountingBloomFilter::decreaseAt(size_t)");

    uint8_t newCount = oldCount - 1;
    setCountAt(i, newCount);

    return newCount;
}

uint8_t CountingBloomFilter::getCountAt(size_t i) const
{
    using namespace std;

    if (i < 0 || i >= m_numCounters)
    {
        throw invalid_argument(
                "Invalid argument in CountingBloomFilter::getCountAt(size_t)");
    }

    char retValue;
    if (m_counterSize == 8)
    {
        retValue = m_counterBuffer[i];
    }
    else
    { // m_bucketSize == FourBits
        retValue = m_counterBuffer[i / 2];

        if (i % 2 == 0)
        { // lower four bits
            retValue >>= 4;
        }
        else
        { // higher four bits
            retValue &= 0b00001111;
        }
    }

    return (uint8_t) retValue;
}

uint8_t CountingBloomFilter::setCountAt(size_t i, uint8_t value)
{
    using namespace std;

    if (i < 0 || i >= m_numCounters)
    {
        throw invalid_argument(
                "Invalid argument in CountingBloomFilter::setCountAt(size_t)");
        // return ISMRC_RtngIllegalArgument;
    }

    if (m_counterSize == 8)
    {
        m_counterBuffer[i] = (char) value;
    }
    else
    { // m_bucketSize == FourBits
        char theByte = m_counterBuffer[i / 2];

        if (i % 2 == 0)
        { // lower four bits
            theByte &= 0b00001111;
            char valueInByte = value << 4;
            theByte |= valueInByte;
        }
        else
        { // higher four bits
            theByte &= 0b11110000;
            theByte |= value;
        }

        m_counterBuffer[i / 2] = theByte;
    }

    return value;
}

double CountingBloomFilter::estimateFPP() const
{
    int32_t m = m_numCounters;
    int32_t n = m_numElements;
    uint8_t k = m_numHashes;

    return pow(1 - pow((double) (1 - 1.0 / m), k * n), k);
}

} /* namespace mcp */
