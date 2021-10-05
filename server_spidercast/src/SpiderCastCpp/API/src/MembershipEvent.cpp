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
 * MembershipEvent.cpp
 *
 *  Created on: 21/03/2010
 */

#include <iostream>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include "MembershipEvent.h"

namespace spdr
{
namespace event
{
//=== NodeJoinEvent ===

NodeJoinEvent::NodeJoinEvent(NodeID_SPtr nodeID, MetaData_SPtr metaData) :
	MembershipEvent(Node_Join), _nodeID(nodeID), _metaData(metaData)
{
	//std::cout << "NodeJoinEvent()" << std::endl;
}

NodeJoinEvent::~NodeJoinEvent()
{
	//std::cout << "~NodeJoinEvent()" << std::endl;
}

const NodeID_SPtr NodeJoinEvent::getNodeID() const
{
	return _nodeID;
}

const MetaData_SPtr NodeJoinEvent::getMetaData() const
{
	return _metaData;
}

String NodeJoinEvent::toString() const
{
	std::ostringstream oss;
	oss << MembershipEvent::toString();
	oss << " NodeName=" + _nodeID->getNodeName();
	if (_metaData)
	{
		oss << " I=" << _metaData->getIncarnationNumber()
				<< " S=" << static_cast<int>(_metaData->getNodeStatus());
		AttributeMap_SPtr attr = _metaData->getAttributeMap();
		if (attr)
		{
			oss << " #A=" << attr->size();
		}
		else
		{
			oss << " A=Null";
		}
	}
	else
	{
		oss << " MetaData=Null";
	}

	return oss.str();
}

//=== NodeLeaveEvent ===

NodeLeaveEvent::NodeLeaveEvent(NodeID_SPtr nodeID, MetaData_SPtr metaData) :
	MembershipEvent(Node_Leave), _nodeID(nodeID), _metaData(metaData)
{
	//std::cout << "NodeLeaveEvent()" << std::endl;
}

NodeLeaveEvent::~NodeLeaveEvent()
{
	//std::cout << "~NodeLeaveEvent()" << std::endl;
}

const NodeID_SPtr NodeLeaveEvent::getNodeID() const
{
	return _nodeID;
}

const MetaData_SPtr NodeLeaveEvent::getMetaData() const
{
	return _metaData;
}

String NodeLeaveEvent::toString() const
{
	std::ostringstream oss;
	oss << MembershipEvent::toString();
	oss << " NodeName=" + _nodeID->getNodeName();
	if (_metaData)
	{
		oss << " I=" << _metaData->getIncarnationNumber() << " S=" << static_cast<int>(_metaData->getNodeStatus());
		AttributeMap_SPtr attr = _metaData->getAttributeMap();
		if (attr)
		{
			oss << " #A=" << attr->size();
		}
		else
		{
			oss << " A=Null";
		}
	}
	else
	{
		oss << " MetaData=Null";
	}

	return oss.str();
}

//=== ViewChangeEvent ===

ViewChangeEvent::ViewChangeEvent(ViewMap_SPtr view) :
	MembershipEvent(View_Change), _view(view)
{
	//std::cout << "ViewChangeEvent()" << std::endl;
}

ViewChangeEvent::~ViewChangeEvent()
{
	//std::cout << "~ViewChangeEvent()" << std::endl;
}

ViewMap_SPtr ViewChangeEvent::getView()
{
	return _view;
}

String ViewChangeEvent::toString() const
{
	String s = MembershipEvent::toString();
	s.append(" ");
	s.append(viewMapToString(_view));

	return s;
}

String ViewChangeEvent::viewMapToString(ViewMap_SPtr view)
{
	std::ostringstream oss;

	if (view)
	{
		oss << "size=" << std::dec << view->size() << ", view={";

		ViewMap::const_iterator it = view->begin();
		for (int i=0; it != view->end(); ++it, ++i)
		{
			oss << it->first->getNodeName();
			if (it->second)
			{
				oss << " I=" << it->second->getIncarnationNumber() << " S=" << static_cast<int>(it->second->getNodeStatus());
				AttributeMap_SPtr attr = it->second->getAttributeMap();
				if (attr)
				{
					oss << " #A=" << attr->size();
				}
				else
				{
					oss << " A=Null";
				}
			}
			else
			{
				oss << " MetaData=Null";
			}

			if (i < (int)view->size()-1)
			{
				oss << ", ";
			}
		}

		oss << "}";
	}
	else
	{
		oss << "view=null";
	}

	return oss.str();
}


String ViewChangeEvent::viewMapEPToString(ViewMap_SPtr view)
{
	std::ostringstream oss;

	if (view)
	{
		oss << "size=" << std::dec << view->size() << ", view={";

		ViewMap::const_iterator it = view->begin();
		for (int i=0; it != view->end(); ++it, ++i)
		{
			oss << it->first->toString();
			if (it->second)
			{
				oss << " I=" << it->second->getIncarnationNumber() << " S=" << static_cast<int>(it->second->getNodeStatus());
			}

			if (i < (int)view->size()-1)
			{
				oss << ", ";
			}
		}

		oss << "}";
	}
	else
	{
		oss << "view=null";
	}

	return oss.str();
}

//=== ChangeOfMetaDataEvent ===

ChangeOfMetaDataEvent::ChangeOfMetaDataEvent(ViewMap_SPtr view) :
			MembershipEvent(Change_of_Metadata), _view(view)
{
	//std::cout << "ChangeOfMetaDataEvent()" << std::endl;
}

ChangeOfMetaDataEvent::~ChangeOfMetaDataEvent()
{
	//std::cout << "~ChangeOfMetaDataEvent()" << std::endl;
}

ViewMap_SPtr ChangeOfMetaDataEvent::getView()
{
	return _view;
}

String ChangeOfMetaDataEvent::viewMapToString(ViewMap_SPtr view, AttributeValue::ToStringMode mode)
{
	std::ostringstream oss;

	if (view)
	{
		oss << "size=" << std::dec << view->size() << ", view={";

		ViewMap::const_iterator it = view->begin();
		for (int i=0; it != view->end(); ++it, ++i)
		{
			oss << it->first->getNodeName() << ' ';
			if (it->second)
			{
				oss << it->second->toString(mode);
			}

			if (i < (int)view->size()-1)
			{
				oss << ", ";
			}
		}

		oss << "}";
	}
	else
	{
		oss << " view=null";
	}

	return oss.str();
}

String ChangeOfMetaDataEvent::toString() const
{
	String s = MembershipEvent::toString();
	s.append(" ");
	s.append(ViewChangeEvent::viewMapToString(_view));

	return s;
}

//=== ForeignZoneMembershipEvent ====================================

ForeignZoneMembershipEvent::ForeignZoneMembershipEvent(
			const int64_t requestID, const String& zoneBusName,
			boost::shared_ptr<ViewMap > view, const bool lastEvent) :
			MembershipEvent(Foreign_Zone_Membership),
			requestID_(requestID),
			zoneBusName_(zoneBusName),
			_view(view),
			lastEvent_(lastEvent),
			errorCode_(Error_Code_Unspecified),
			errorMessage_()
{
}

ForeignZoneMembershipEvent::ForeignZoneMembershipEvent(
			const int64_t requestID, const String& zoneBusName,
			ErrorCode errorCode, const String& errorMessage, bool lastEvent) :
			MembershipEvent(Foreign_Zone_Membership),
			requestID_(requestID),
			zoneBusName_(zoneBusName),
			_view(),
			lastEvent_(lastEvent),
			errorCode_(errorCode),
			errorMessage_(errorMessage)
{
}

ForeignZoneMembershipEvent::~ForeignZoneMembershipEvent()
{
}

int64_t ForeignZoneMembershipEvent::getRequestID() const
{
	return requestID_;
}

const String& ForeignZoneMembershipEvent::getZoneBusName() const
{
	return zoneBusName_;
}

ViewMap_SPtr ForeignZoneMembershipEvent::getView()
{
	return _view;
}

bool ForeignZoneMembershipEvent::isLastEvent() const
{
	return lastEvent_;
}

ErrorCode ForeignZoneMembershipEvent::getErrorCode() const
{
	return errorCode_;
}

const String& ForeignZoneMembershipEvent::getErrorMessage() const
{
	return errorMessage_;
}

bool ForeignZoneMembershipEvent::isError() const
{
	return (errorCode_ != Error_Code_Unspecified);
}

String ForeignZoneMembershipEvent::toString() const
{
	std::ostringstream oss;
	oss << MembershipEvent::toString();
	oss << " ReqID=" << requestID_ << " zone=" << zoneBusName_;

	if (isError())
	{
		oss << " Error=" << SpiderCastEvent::errorCodeName[errorCode_] << " " << errorMessage_;
	}
	else
	{
		oss << " View-size=";
		if (_view)
		{
			oss << _view->size();
		}
		else
		{
			oss << "empty";
		}
	}

	return oss.str();
}

//=== ZoneCensusEvent ===============================================
ZoneCensusEvent::ZoneCensusEvent(const int64_t requestID, ZoneCensus_SPtr census, const bool full) :
		MembershipEvent(Zone_Census),
		requestID_(requestID),
		census_(census),
		full_(full)
{
}

ZoneCensusEvent::~ZoneCensusEvent()
{
}

int64_t ZoneCensusEvent::getRequestID() const
{
	return requestID_;
}

ZoneCensus_SPtr ZoneCensusEvent::getZoneCensus()
{
	return census_;
}

bool ZoneCensusEvent::isFullCensus() const
{
	return full_;
}

String ZoneCensusEvent::toString() const
{
	std::ostringstream oss;
	oss << MembershipEvent::toString();
	oss << " ReqID=" << requestID_;

	oss << " size=";
	if (census_)
	{
		oss << census_->size();
	}
	else
	{
		oss << "empty";
	}

	 oss << " full=" << std::boolalpha << full_;

	return oss.str();
}

String ZoneCensusEvent::zoneCensusToString(ZoneCensus_SPtr census)
{
	std::ostringstream oss;
	oss << "ZoneCensus(";
	if (census)
	{
		oss << census->size() << ")={";
		ZoneCensus::const_iterator it = census->begin();
		while (it != census->end())
		{
			oss << it->first << " " << it->second;
			if (++it != census->end())
			{
				oss << "; ";
			}
		}
		oss << "}";
	}
	else
	{
		oss << "empty)";
	}

	return oss.str();
}

}//event
}//spdr
