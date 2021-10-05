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
 * VirtualID.cpp
 *
 *  Created on: Apr 17, 2011
 */

#include <memory.h>

#ifdef SPDR_LINUX
#include <arpa/inet.h> //for hton<l/s> ntoh<l/s>
#endif

#ifdef SPDR_WINDOWS
#include <winsock2.h> //for hton<l/s> ntoh<l/s>
#endif

#include <math.h>
#include <float.h>

#include "VirtualID.h"

namespace spdr
{
namespace util
{

const VirtualID VirtualID::MinValue; // = VirtualID(); //all zeros
const VirtualID VirtualID::MaxValue(0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff); //all ones
const VirtualID VirtualID::OneValue(0,0,0,0,1); //one
const VirtualID VirtualID::MiddleValue(0x80000000,0,0,0,0); //2^159

VirtualID::VirtualID() : w()
{
}

VirtualID::VirtualID(const uint32_t* digest_array5)
{
	for (int i=0; i<5; ++i)
	{
		w[i] = *digest_array5;
		digest_array5++;
	}
}

VirtualID::VirtualID(SHA1& sha1)
{
	if(!sha1.digest(w))
	{
		throw IllegalArgumentException("Failed to create VirtualID, corrupted SHA1 object");
	}
}

VirtualID::VirtualID(const char* array20)
{
	for (int i=0; i<5; ++i)
	{
		uint32_t w_be;
		memcpy(&w_be,array20,sizeof(uint32_t));
		w[i] = ntohl(w_be);
		array20 += sizeof(uint32_t);
	}

//	uint32_t* p = (uint32_t*)array20;
//	for (int i=0; i<5; ++i)
//	{
//		w[i] = ntohl(*p);
//		p++;
//	}
}

void VirtualID::copyTo(char* array20) const
{
	for (int i=0; i<5; ++i)
	{
		uint32_t w_be = htonl(w[i]);
		memcpy(array20,&w_be,sizeof(uint32_t));
		array20 += sizeof(uint32_t);
	}

//	uint32_t* p = (uint32_t*)array20;
//	for (int i=0; i<5; ++i)
//	{
//		*p = htonl(w[i]);
//		p++;
//	}
}

void VirtualID::shiftRight(size_t n)
{
	if (n>=160)
	{
		w[0] = w[1] = w[2] = w[3] = w[4] = 0;
	}
	else
	{
		int K = n/32;
		int J = n%32;

		if (K>0) //big shift
		{
			for (int i=4; i>=0; --i)
			{
				if (i-K >=0)
				{
					w[i] = w[i-K];
				}
				else
				{
					w[i] = 0;
				}
			}
		}

		if (J>0) //small shift
		{
			for (int i=4; i>0; --i)
			{
				uint64_t x = ((uint64_t)w[i-1]) & 0x00000000ffffffff;
				x = x << 32;
				x = x | (((uint64_t)w[i]) & 0x00000000ffffffff);
				x = x >> J;
				w[i] = (uint32_t)x;
			}
			w[0] = w[0] >>J;
		}
	}
}

void VirtualID::add(const VirtualID& x)
{
	uint64_t s = 0;
	for (int i=4; i>=0; --i)
	{
		s = s + (uint64_t)(x.w[i]) + (uint64_t)w[i];
		w[i] = (uint32_t)s;
		s = s>>32;
	}
}

void VirtualID::sub(const VirtualID& x)
{
	//the complements method
	uint32_t z = 0xFFFFFFFF;

	uint64_t s = 1;
	for (int i=4; i>=0; --i)
	{
		s = (uint64_t)w[i] + ((uint64_t)(z - x.w[i])) + s;
		w[i] = (uint32_t)s;
		s = s>>32;
	}
}

void VirtualID::absDist(const VirtualID& x)
{
	VirtualID tmp(x);
	tmp.sub(*this);
	sub(x);
	if (tmp < *this)
	{
		w[0] = tmp.w[0];
		w[1] = tmp.w[1];
		w[2] = tmp.w[2];
		w[3] = tmp.w[3];
		w[4] = tmp.w[4];
	}
}

uint64_t VirtualID::getLower64() const
{
	uint64_t s = w[3];
	s = s<<32;
	s = s | (w[4] & 0x00000000FFFFFFFFL);
	return s;
}

VirtualID::VirtualID(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3, uint32_t w4)
{
	w[0] = w0;
	w[1] = w1;
	w[2] = w2;
	w[3] = w3;
	w[4] = w4;
}

String VirtualID::toString() const
{
	String s;

	for (uint32_t i=0; i<5; ++i)
	{
		std::ostringstream oss;
		oss << std::hex << w[i];
		s.append(oss.str());

		if (i<4)
		{
			s.append(":");
		}

		while (s.size() < (i+1)*9)
		{
			s.insert(i*9,"0");
		}
	}
	return s;
}

VirtualID::VirtualID(const VirtualID& other)
{
	for (int i=0; i<5; ++i)
	{
		w[i] = other.w[i];
	}
}

VirtualID& VirtualID::operator=(const VirtualID& other)
{
	if (this == &other)
	{
		return *this;
	}
	else
	{
		for (int i=0; i<5; ++i)
		{
			w[i] = other.w[i];
		}

		return *this;
	}
}

VirtualID::~VirtualID()
{
}

VirtualID VirtualID::createFromRandom(double random)
{
	if (random<0 || random>1.0)
	{
		throw IllegalArgumentException("argument must be in [0,1]");
	}

	int32_t exponent;
	int32_t dbl_mant_dig = DBL_MANT_DIG;
	int64_t one = 1;

	double mantissa = frexp(random, &exponent);
	int64_t mantissa_bits = static_cast<int64_t>(mantissa * (one << dbl_mant_dig));

//	std::cout << "r: " << random;
//	std::cout << " m: " << mantissa << " e: " << std::dec << exponent;
//	std::cout << " mbits: " << std::hex << mantissa_bits << std::endl;

	uint32_t w0 = (uint32_t)(mantissa_bits >> (32-11));
	uint32_t w1 = (uint32_t)(mantissa_bits << 11);;
	uint32_t w2 = 0;
	uint32_t w3 = 0;
	uint32_t w4 = 0;

	if (random == 0.0 || random == 1.0)
	{
		return VirtualID(0,0,0,0,0);
	}
	else
	{
		VirtualID rv(w0,w1,w2,w3,w4);
		rv.shiftRight(-exponent);
		return rv;
	}
}

bool operator<(const VirtualID& lhs, const VirtualID& rhs)
{
	for (int i=0; i<5; ++i)
	{
		if (lhs.w[i] < rhs.w[i])
			return true;
		else if (lhs.w[i] > rhs.w[i])
			return false;
	}

	return false; //equal
}

bool operator>(const VirtualID& lhs, const VirtualID& rhs)
{
	for (int i=0; i<5; ++i)
	{
		if (lhs.w[i] > rhs.w[i])
			return true;
		else if (lhs.w[i] < rhs.w[i])
			return false;
	}

	return false; //equal
}

bool operator>=(const VirtualID& lhs, const VirtualID& rhs)
{
	return !(lhs<rhs);
}

bool operator<=(const VirtualID& lhs, const VirtualID& rhs)
{
	return !(lhs>rhs);
}

bool operator==(const VirtualID& lhs, const VirtualID& rhs)
{
	for (int i=0; i<5; ++i)
	{
		if (lhs.w[i] != rhs.w[i])
			return false;
	}
	return true;
}

bool operator!=(const VirtualID& lhs, const VirtualID& rhs)
{
	return !(lhs==rhs);
}

VirtualID add(const VirtualID& x, const VirtualID& y)
{
	VirtualID tmp(x);
	tmp.add(y);
	return tmp;
}

VirtualID sub(const VirtualID& x, const VirtualID& y)
{
	VirtualID tmp(x);
	tmp.sub(y);
	return tmp;
}

VirtualID absDist(const VirtualID& x,const VirtualID& y)
{
	VirtualID tmp(x);
	tmp.absDist(y);
	return tmp;
}

}
}
