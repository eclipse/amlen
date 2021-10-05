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
 * MetaData.cpp
 *
 *  Created on: 21/03/2010
 */

#include "MetaData.h"

namespace spdr
{
namespace event
{

MetaData::MetaData(AttributeMap_SPtr attributeMap, int64_t incarnationNumber, NodeStatus status) :
	attributeMap_(attributeMap),
	incarnationNumber_(incarnationNumber),
	nodeStatus_(status)
{
}

MetaData::~MetaData()
{
}

AttributeMap_SPtr MetaData::getAttributeMap()
{
	return attributeMap_;
}

int64_t MetaData::getIncarnationNumber() const
{
	return incarnationNumber_;
}

NodeStatus MetaData::getNodeStatus() const
{
	return nodeStatus_;
}

String MetaData::toString(AttributeValue::ToStringMode mode) const
{
	std::ostringstream oss;
	oss << "I=" << incarnationNumber_ << " S=" << static_cast<int>(nodeStatus_) << " A=";

	if (attributeMap_)
	{
		oss << attributeMap_->toString(mode);
	}
	else
	{
		oss << "null";
	}
	return oss.str();
}

MetaData_SPtr MetaData::clone(MetaData_SPtr other)
{
	MetaData_SPtr metadata_sptr;
	if (other)
	{
		metadata_sptr.reset(new MetaData(*other));
		metadata_sptr->attributeMap_ = AttributeMap::clone(other->attributeMap_);
	}

	return metadata_sptr;
}

//friend of MetaData
bool operator==(const MetaData& lhs, const MetaData& rhs)
		{
	if (lhs.incarnationNumber_ == rhs.incarnationNumber_)
	{
		if (lhs.nodeStatus_ == rhs.nodeStatus_)
		{
			if (lhs.attributeMap_ && rhs.attributeMap_)
			{
				if (*(lhs.attributeMap_) == *(rhs.attributeMap_))
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else if (!lhs.attributeMap_ && !rhs.attributeMap_)
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
	else
	{
		return false;
	}
}


//friend of MetaData
bool operator!=(const MetaData& lhs, const MetaData& rhs)
{
	return !(lhs == rhs);
}

}
}
