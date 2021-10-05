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
 * SCMembershipEvent.cpp
 *
 *  Created on: 21/03/2010
 */

#include <iostream>
#include <sstream>

#include <boost/algorithm/string/predicate.hpp>

#include "SCMembershipEvent.h"

namespace spdr
{

SCMembershipEvent::SCMembershipEvent(Type eventType, NodeID_SPtr nodeID,
		event::MetaData_SPtr metaData) :
	type_(eventType), nodeID_(nodeID), metadata_(metaData), view_()
{
	if (type_ != Node_Join && type_ != Node_Leave)
	{
		throw IllegalArgumentException("Illegal event type");
	}
}

SCMembershipEvent::SCMembershipEvent(Type eventType, SCViewMap_SPtr view) :
	type_(eventType), nodeID_(), metadata_(), view_(view)
{
	if (type_ != View_Change && type_ != Change_of_Metadata)
	{
		throw IllegalArgumentException("Illegal event type");
	}
}

SCMembershipEvent::Type SCMembershipEvent::getType() const
{
	return type_;
}

NodeID_SPtr SCMembershipEvent::getNodeID() const
{
	return nodeID_;
}

event::MetaData_SPtr SCMembershipEvent::getMetaData() const
{
	return metadata_;
}

SCViewMap_SPtr SCMembershipEvent::getView() const
{
	return view_;
}

bool operator==(const SCViewMap& lhs, const SCViewMap& rhs)
{
	if (lhs.size() == rhs.size())
	{
		for (SCViewMap::const_iterator itL = lhs.begin();
				itL	!= lhs.end(); ++itL)
		{
			SCViewMap::const_iterator itR = rhs.find(itL->first);
			if (itR != rhs.end())
			{
				if (*(itR->second) != *(itL->second))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool operator!=(const SCViewMap& lhs, const SCViewMap& rhs)
{
	return !(rhs == lhs);
}

bool operator==(const SCMembershipEvent& lhs, const SCMembershipEvent& rhs)
{
	if (lhs.type_ != rhs.type_)
	{
		return false;
	}
	else
	{
		switch (lhs.type_)
		{
		case SCMembershipEvent::Node_Leave:
		case SCMembershipEvent::Node_Join:
		{
			if (*(rhs.nodeID_) == *(lhs.nodeID_))
			{
				if (rhs.metadata_ && lhs.metadata_ && (*(rhs.metadata_) == *(lhs.metadata_)))
				{
					return true;
				}
				else if (!rhs.metadata_ && !lhs.metadata_)
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
		break;


		case SCMembershipEvent::View_Change:
		case SCMembershipEvent::Change_of_Metadata:
		{
			if (lhs.view_ && rhs.view_ && (*(lhs.view_) == *(rhs.view_)))
			{
				return true;
			}
			else if (!lhs.view_ && !rhs.view_)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		break;

		default:
		{
			throw SpiderCastRuntimeError("Unknown SCMembershipEvent::Type");
		}

		}
	}
}

bool operator!=(const SCMembershipEvent& lhs, const SCMembershipEvent& rhs)
{
	return !(lhs==rhs);
}


String SCMembershipEvent::toString() const
{
	switch (type_)
	{
	case SCMembershipEvent::View_Change:
	{
		String s("View_Change ");
		if (view_)
		{
			s.append(SCMembershipEvent::viewMapToString(*view_,true));
		}
		else
		{
			s.append("view=null");
		}
		return s;
	}

	case SCMembershipEvent::Node_Join:
	{
		String s("Node_Join ");
		s.append(nodeID_->toString()).append(" ");
		s.append(metadata_->toString(event::AttributeValue::BIN));
		return s;
	}

	case SCMembershipEvent::Node_Leave:
	{
		String s("Node_Leave ");
		s.append(nodeID_->toString());
		return s;
	}

	case SCMembershipEvent::Change_of_Metadata:
	{
		String s("Change_of_Metadata ");
		if (view_)
		{
			s.append(SCMembershipEvent::viewMapToString(*view_,true));
		}
		else
		{
			s.append("view=null");
		}
		return s;
	}

	default:
		return "Unknown event type";

	}
}

String SCMembershipEvent::viewMapToString(const SCViewMap& view,
		bool withMetadata, event::AttributeValue::ToStringMode mode)
{
	std::ostringstream oss;
	oss << "size=" << std::dec << view.size() << ", view={";

	SCViewMap::const_iterator it = view.begin();
	for (int i = 0; it != view.end(); ++it, ++i)
	{
		oss << it->first->getNodeName() << ' ';
		if (withMetadata && it->second)
		{
			oss << it->second->toString(mode);
		}

		if (i < (int) view.size() - 1)
		{
			oss << ", ";
		}
	}

	oss << "}";

	return oss.str();

// Ordered toString
//	std::set<std::string> ordered_view;
//
//	for (SCViewMap::const_iterator it = view.begin();
//			it != view.end(); ++it)
//	{
//		std::string s(it->first->getNodeName());
//		if (withMetadata && it->second)
//		{
//			s.append(" ");
//			s.append(it->second->toString(mode));
//		}
//		ordered_view.insert(s);
//	}
//
//	std::ostringstream oss;
//	oss << "size=" << std::dec << view.size() << ", view={";
//
//	std::set<std::string>::const_iterator it = ordered_view.begin();
//	for (std::size_t i = 0; it != ordered_view.end(); ++it, ++i)
//	{
//		oss << *it;
//
//		if (i < view.size() - 1)
//		{
//			oss << ", ";
//		}
//	}
//
//	oss << "}";
//
//	return oss.str();
}


event::AttributeMap_SPtr filter2AttributeMap(const event::AttributeMap& map, const String& keyPrefix)
{
	event::AttributeMap_SPtr filteredMap;

	event::AttributeMap::const_iterator it;
	for (it = map.begin(); it != map.end(); ++it)
	{
		if (boost::starts_with(it->first, keyPrefix))
		{
			if (!filteredMap)
			{
				filteredMap.reset(new event::AttributeMap);
			}
			filteredMap->insert(*it);
		}
	}

	return filteredMap;
}

}
