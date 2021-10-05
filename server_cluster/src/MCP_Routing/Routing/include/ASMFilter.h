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

#ifndef FILTERIFC_H_
#define FILTERIFC_H_

#include <string>
//#include "HashFunctions.h"
#include "hashFunction.h"
// #include "HashFunctionsDH.h"
#include "MCPExceptions.h"

namespace mcp{

//typedef mcc_hash_HashType_t HashType;

/**
 * Approximate Set Membership(ASM) Filter
 * An abstract base class for Bloom Filter, Counting Bloom Filter, etc.
 *
 */
class ASMFilter {
public:
	ASMFilter(size_t numBins, uint8_t numHash, mcc_hash_HashType_t hashType);
	virtual ~ASMFilter();

	/**
	 * Returns 	true if the element might have been put in this Bloom filter,
	 * 			false if this is definitely not the case.
	 */
	virtual bool contains(const std::string& element) = 0;
	virtual bool contains(const char * element, size_t length) = 0;

	/**
	 * Puts an element into this ASMFilter.
	 * Ensures that subsequent invocations of mightContain() with the same element will always return true.
	 * Return 	true if the bloom filter's bits changed as a result of this operation.
	 * 			If the bits changed, this is definitely the first time object has been added to the filter.
	 * 			If the bits haven't changed, this might be the first time object has been added to the filter.
	 */
	virtual void put(const std::string& element) = 0;
	virtual void put(const char * element, size_t length) = 0;

	size_t getNumBits();

	uint8_t getNumHashes();

	mcc_hash_HashType_t getHashType();

protected:

	/** The number of bins (i.e., vector size) of this filter */
	size_t m_numBits;

	/** The number of hash functions */
	uint8_t m_numHashes;

	/** Type of hash function to use */
	mcc_hash_HashType_t m_hashType;

	mcc_hash_getAllValues_t m_hashFunctionsPtr;

	void assignHashFunction(mcc_hash_HashType_t type) throw(MCPIllegalArgumentError);

	// HashFunctions * m_hashes;

private:
	ASMFilter();
	ASMFilter( const ASMFilter & other );
	ASMFilter& operator=( const ASMFilter& other );

};

}
#endif /* FILTERIFC_H_ */
