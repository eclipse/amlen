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

#include <iostream>

#include "ASMFilter.h"

namespace mcp {

ASMFilter::ASMFilter(size_t numBits, uint8_t numHashes, mcc_hash_HashType_t hashType) :
			m_numBits(numBits),
			m_numHashes(numHashes),
			m_hashType(ISM_HASH_TYPE_MURMUR_x64_128_CH)
{
	assignHashFunction(hashType);
}

ASMFilter::~ASMFilter()
{
	//std::cout << "in ~ASMFilter() " << std::endl;
}

void ASMFilter::assignHashFunction(mcc_hash_HashType_t type) throw (MCPIllegalArgumentError)
{
	switch (type)
	{
	case ISM_HASH_TYPE_MURMUR_x64_128_CH:
		m_hashFunctionsPtr = &mcc_hash_getAllValues_murmur3_x64_128;
		break;

	case ISM_HASH_TYPE_CITY64_CH:
		m_hashFunctionsPtr = &mcc_hash_getAllValues_city64_simple;
		break;

	case ISM_HASH_TYPE_MURMUR_x64_128_LC:
		m_hashFunctionsPtr = &mcc_hash_getAllValues_murmur3_x64_128_LC;
		break;

	case ISM_HASH_TYPE_CITY64_LC:
		m_hashFunctionsPtr = &mcc_hash_getAllValues_city64_LC;
		break;


	default:
		throw MCPIllegalArgumentError("ASMFilter Illegal HashType");

	}

	m_hashType = type;
}

size_t ASMFilter::getNumBits() {
	return m_numBits;
}

uint8_t ASMFilter::getNumHashes(){
	return m_numHashes;
}

mcc_hash_HashType_t ASMFilter::getHashType() {
	return m_hashType;
}

}
