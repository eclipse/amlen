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

#include <cmath>        // std::log(double)
#include <string.h>
#include <stdexcept>
#include<iostream>

#include "BloomFilter.h"

using namespace std;

namespace mcp
{

/*
 * This implementation is taken from open source
 * http://code.google.com/p/bloom/
 *
 */
//pair<size_t, uint8_t> BloomFilter::computeOptimalParameters(
//		const uint32_t projected_element_count, const double false_positive_probability) {
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
//	int numHashes = static_cast<unsigned int>(min_k);
//
//	return std::make_pair(numBins, numHashes);
//}
//BloomFilter::BloomFilter(pair<size_t, uint8_t> pair, HashType hashType) :
//		ASMFilter(pair.first, pair.second, hashType) {
//	m_hashes = new HashFunctions(m_numBits, m_numHashes, m_hashType);
//}
BloomFilter::BloomFilter(size_t numBins, uint8_t numHashes, mcc_hash_HashType_t hashType) :
		ASMFilter(numBins, numHashes, hashType)
{
	size_t numBytes = (m_numBits / 8) + ((m_numBits % 8) > 0 ? 1 : 0);
	m_binBuffer = new char[numBytes];
	memset(m_binBuffer, 0, numBytes);

	//m_hashFuncs = new HashFunctions(m_numBits, m_numHashes, m_hashType);
}

BloomFilter::BloomFilter(size_t numBins, uint8_t numHashes, mcc_hash_HashType_t hashType,
		const char* buffer) :
		ASMFilter(numBins, numHashes, hashType)
{
	size_t numBytes = (m_numBits / 8) + ((m_numBits % 8) > 0 ? 1 : 0);
	m_binBuffer = new char[numBytes];
	memcpy(m_binBuffer, buffer, numBytes);

	//m_hashFuncs = new HashFunctions(m_numBits, m_numHashes, m_hashType);
}

void BloomFilter::setContent(size_t numBins, uint8_t numHashes,
		mcc_hash_HashType_t hashType, const char* buffer)
{

	size_t numBytes =  (numBins / 8) + ((numBins % 8) > 0 ? 1 : 0);

	if (numBins != m_numBits)
	{
		delete[] m_binBuffer;
		m_binBuffer = new char[numBytes];
	}

	m_numBits = numBins;
	m_numHashes = numHashes;
	assignHashFunction(hashType);

	memcpy(m_binBuffer, buffer, numBytes);

	//delete m_hashFuncs;
	//m_hashFuncs = new HashFunctions(m_numBits, m_numHashes, m_hashType);
}

BloomFilter::~BloomFilter()
{
	// m_bits.clear();
	delete[] m_binBuffer;

	//delete m_hashFuncs;
}

bool BloomFilter::contains(const string& element)
{
//	m_hashFuncs->prepareMembershipTest(element);
//
//	for (unsigned int i = 0; i < m_numHashes; i++)
//	{
//		uint32_t h = m_hashFuncs->nextHash();
//
//		if (checkBinAt(h) == false)
//		{
//			return false;
//		}
//	}
//
//	return true;
	return contains(element.data(), element.size());
}

bool BloomFilter::contains_wHashes(const string& element)
{
//	HashFunctions hashes(m_numBits, m_numHashes, m_hashType);
//	vector<uint32_t> h = hashes.hashes(element);

	uint32_t h[m_numHashes];
	m_hashFunctionsPtr(element.data(), element.size(), m_numHashes, m_numBits, h);

	for (unsigned int i = 0; i < m_numHashes; i++)
	{
		if (checkBinAt(h[i]) == false)
			return false;
	}

	return true;
}

bool BloomFilter::contains(const char * element, size_t length)
{
//	HashFunctions hashes(m_numBits, m_numHashes, m_hashType);
//	hashes.prepareMembershipTest(element, length);

	uint32_t h[m_numHashes];
	m_hashFunctionsPtr(element, length, m_numHashes, m_numBits, h);

	for (unsigned int i = 0; i < m_numHashes; i++)
	{
//		uint32_t h = hashes.nextHash();

		if (checkBinAt(h[i]) == false)
		{
			return false;
		}
	}

	return true;
}

bool BloomFilter::contains_wHashes(const char * element, size_t length)
{
//	HashFunctions hashes(m_numBits, m_numHashes, m_hashType);
//	vector<uint32_t> h = hashes.hashes(element, length);

	uint32_t h[m_numHashes];
	m_hashFunctionsPtr(element, length, m_numHashes, m_numBits, h);

	for (unsigned int i = 0; i < m_numHashes; i++)
	{
		if (checkBinAt(h[i]) == false)
			return false;
	}

	return true;
}

void BloomFilter::put(const string& element)
{
//	vector<uint32_t> h = m_hashFuncs->hashes(element);
//
//	for (unsigned int i = 0; i < m_numHashes; i++)
//	{
//		setBinAt(h[i]);
//	}
	put(element.data(),element.size());
}

void BloomFilter::put(const char * element, size_t length)
{
//	HashFunctions hashes(m_numBits, m_numHashes, m_hashType);
//	vector<uint32_t> h = hashes.hashes(element, length);

	uint32_t h[m_numHashes];
	m_hashFunctionsPtr(element, length, m_numHashes, m_numBits, h);

	for (unsigned int i = 0; i < m_numHashes; i++)
	{
		setBinAt(h[i]);
	}
}

void BloomFilter::setBinAt(size_t i)
{
	if (i < 0 || i >= m_numBits)
	{
		throw invalid_argument(
				"Invalid argument in CountingBloomFilter::setCountAt(size_t)");
	}

	size_t index = i >> 3; // size_t index = i / 8;
	size_t offset = i & 0b00000111; // uint8_t offset = i % 8;

	m_binBuffer[index] |= 1 << offset;
}

void BloomFilter::resetBinAt(size_t i)
{
	if (i < 0 || i >= m_numBits)
	{
		throw invalid_argument(
				"Invalid argument in BloomFilter::resetBinAt(size_t)");
	}

	size_t index = i >> 3; // size_t index = i / 8;
	size_t offset = i & 0b00000111; // uint8_t offset = i % 8;

	m_binBuffer[index] &= ~(1 << offset);
}

bool BloomFilter::checkBinAt(size_t i)
{
//	if (i < 0 || i >= m_numBits)
//	{
//		throw invalid_argument(
//				"Invalid argument in BloomFilter::checkBinAt(size_t)");
//	}

	size_t index = i >> 3; // size_t index = i / 8;
	size_t offset = i & 0b00000111; // uint8_t offset = i % 8;

	char theByte = m_binBuffer[index];

	return (theByte & (1 << offset));
}

bool BloomFilter::checkBins(std::vector<uint32_t> bins)
{
	for (size_t i = 0; i < bins.size(); i++)
	{
		if (checkBinAt(bins[i]) == false)
			return false;
	}
	return true;
}

vector<uint32_t> BloomFilter::binsOf(const string& element)
{
	uint32_t h[m_numHashes];
	m_hashFunctionsPtr(element.data(), element.size(), m_numHashes, m_numBits, h);

	vector<uint32_t> r;
	for (int i=0; i<m_numHashes; ++i)
	{
		r.push_back(h[i]);
	}
	return r;

//	HashFunctions hashes(m_numBits, m_numHashes, m_hashType);
//	return hashes.hashes(element);
}

vector<uint32_t> BloomFilter::binsOf(const char * element, size_t length)
{
	uint32_t h[m_numHashes];
	m_hashFunctionsPtr(element, length, m_numHashes, m_numBits, h);

	vector<uint32_t> r;
	for (int i=0; i<m_numHashes; ++i)
	{
		r.push_back(h[i]);
	}
	return r;

	//	HashFunctions hashes(m_numBits, m_numHashes, m_hashType);
	//	return hashes.hashes(element, length);
}

const char * const BloomFilter::buffer() const
{
	return m_binBuffer;
}

} /* namespace mcp */

