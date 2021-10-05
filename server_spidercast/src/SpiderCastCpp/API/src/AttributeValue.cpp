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
 * AttributeValue.cpp
 *
 *  Created on: Feb 22, 2011
 */

#include <cstring>
#include <sstream>

#include "AttributeValue.h"

namespace spdr
{

namespace event
{

//=== AttributeValue ===

bool AttributeValue::equalsString(String s, ToStringMode mode) const
{
	if (mode == CSTR)
	{
		if ((int32_t) s.size() == length - 1)
		{
			int res = memcmp(bufferSPtr.get(), s.c_str(), s.size()+1);
			return (res == 0);
		}
		else
		{
			return false;
		}
	}
	else
	{
		if ((int32_t) s.size() == length)
		{
			if (length == 0)
			{
				return true;
			}
			else
			{
				int res = memcmp(bufferSPtr.get(), s.c_str(), s.size());
				return (res == 0);
			}
		}
		else
		{
			return false;
		}
	}
}

String AttributeValue::toString(ToStringMode mode) const
{
	const int MAX_PRINT_SIZE = 1024;
	const int lim = std::min(MAX_PRINT_SIZE,length);

	std::ostringstream oss;
	//oss << "L=" << std::dec << length << ' ';
	if (length < 0)
	{
		oss << "null";
	}
	else if (length == 0)
	{
		oss << "{}";
	}
	else
	{
		if (mode == CSTR)
		{
			String s(bufferSPtr.get(), lim);
			oss << "{" << s.c_str();

			if (length > MAX_PRINT_SIZE)
			{
				oss << ",...} L=" << std::dec << length << " Too long, truncated";
			}
			else
			{
				oss << "}";
			}
		}
		else if (mode == STR)
		{
			String s(bufferSPtr.get(), lim);
			oss << "{" << s.c_str();

			if (length > MAX_PRINT_SIZE)
			{
				oss << ",...} L=" << std::dec << length << " Too long, truncated";
			}
			else
			{
				oss << "}";
			}
		}
		else //BIN
		{

			oss << "{" << std::hex;
			const char* p = bufferSPtr.get();
			for (int i=0; i<lim; ++i)
			{
				oss << ((unsigned int)(*(p+i)) & 0x000000FF);
				if (i<lim-1)
				{
					oss << ',';
				}
			}

			if (length > MAX_PRINT_SIZE)
				oss << ",...} L=" << std::dec << length << " Too long, truncated";
			else
				oss << "}";
		}
	}
	return oss.str();
}

AttributeValue AttributeValue::clone(const AttributeValue& other)
{
	AttributeValue v(other);

	if (v.length <= 0)
	{
		v.bufferSPtr.reset();
		return v;
	}
	else
	{
		char* ptr = new char[v.length];
		if (ptr == NULL)
		{
			std::ostringstream oss;
			oss << "OutOfMemoryException: AttributeValue trying to clone() " << v.length << " bytes";
			throw OutOfMemoryException(oss.str());
		}
		memcpy(ptr, other.bufferSPtr.get(), v.length);
		v.bufferSPtr.reset(ptr);
		return v;
	}
}

//friend of AttributeValue
bool operator==(const AttributeValue& lhs, const AttributeValue& rhs)
{
	if (lhs.length == rhs.length)
	{
		if (lhs.length > 0)
		{
			int res = memcmp(lhs.bufferSPtr.get(), rhs.bufferSPtr.get(),
					lhs.length);
			return (res == 0);
		}
		else if (!lhs.bufferSPtr && !rhs.bufferSPtr)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

//friend of AttributeValue
bool operator!=(const AttributeValue& lhs, const AttributeValue& rhs)
{
	return !(lhs == rhs);
}
} //event
} //spdr
