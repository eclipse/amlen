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
 * UpdateDatabase.cpp
 *
 *  Created on: 16/05/2010
 */

#include "UpdateDatabase.h"

namespace spdr
{

ScTraceComponent* UpdateDatabase::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"UpdateDatabase",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

UpdateDatabase::UpdateDatabase(const String& instID, const String& nodeName,
			const SpiderCastConfigImpl& config) :
		ScTraceContext(tc_, instID, nodeName),
		crcEnabled_(config.isCRCMemTopoMsgEnabled())
{
	Trace_Entry(this,"UpdateDatabase()");
}

UpdateDatabase::~UpdateDatabase()
{
	Trace_Entry(this,"~UpdateDatabase()");
}

bool UpdateDatabase::addToLeft(const String& nodeName, NodeVersion ver, spdr::event::NodeStatus status)
{
	Trace_Entry(this, "addToLeft()",
			"name", nodeName,
			"version", ver.toString());

	bool modified = false;

	std::pair<NodeName2VersionStatusMap::iterator, bool> res = leftMap.insert(
			NodeName2VersionStatusMap::value_type(nodeName, std::make_pair(ver,status)));

	if (!res.second)
	{
		if (res.first->second.first < ver)
		{
			res.first->second.first = ver;
			res.first->second.second = status;
			modified = true;
		}
	}
	else
	{
		modified = true;
	}

	Trace_Exit<bool>(this,  "addToLeft()", modified);

	return modified;
}

bool UpdateDatabase::addToAlive(NodeIDImpl_SPtr id, NodeVersion ver)
{
	Trace_Entry(this, "addToAlive()",
				"id", NodeIDImpl::stringValueOf(id),
				"version", ver.toString());

	bool modified = false;

	std::pair<NodeID2VersionMap::iterator, bool> res = aliveMap.insert(
			NodeID2VersionMap::value_type(id, ver));

	if (!res.second)
	{
		if (res.first->second < ver)
		{
			res.first->second = ver;
			modified = true;
		}
	}
	else
	{
		modified = true;
	}

	Trace_Exit<bool>(this, "addToAlive()", modified);

	return modified;
}

bool UpdateDatabase::addToRetained(NodeIDImpl_SPtr id, NodeVersion ver, spdr::event::NodeStatus status)
{
	Trace_Entry(this, "addToRetained()",
				"id", NodeIDImpl::stringValueOf(id),
				"version", ver.toString(),
				"status", ScTraceBuffer::stringValueOf(status));

	bool modified = false;

	std::pair<NodeID2VersionStatusMap::iterator, bool> res = retainedMap.insert(
			NodeID2VersionStatusMap::value_type(id, std::make_pair(ver,status)));

	if (!res.second)
	{
		if (res.first->second.first <= ver)
		{
			res.first->second.first = ver;
			res.first->second.second = status;
			modified = true;
		}
	}
	else
	{
		modified = true;
	}

	Trace_Exit<bool>(this, "addToRetained()", modified);

	return modified;
}

bool UpdateDatabase::addToSuspect(StringSPtr reporter, StringSPtr suspect,
			NodeVersion suspectVer)
{
	Trace_Entry(this, "addToSuspect()",
					"reporter", *reporter,
					"suspect", *suspect,
					"suspect.version", suspectVer.toString());

	bool modified = false;
	Suspicion suspicion(reporter,suspect,suspectVer);
	std::pair<SuspicionSet::iterator, bool> res = suspicionSet.insert(suspicion);
	if (!res.second)
	{
		if (res.first->getSuspectedNodeVersion() < suspectVer)
		{
			suspicionSet.erase(res.first);
			suspicionSet.insert(suspicion);
			modified = true;
		}
	}
	else
	{
		modified = true;
	}

	Trace_Exit<bool>(this, "addToSuspect()", modified);

	return modified;
}

bool UpdateDatabase::empty()
{
	//Trace_Entry(this, "empty()");

	bool empty = (leftMap.empty() && aliveMap.empty() && suspicionSet.empty()) && retainedMap.empty();

	if (!empty)
	{
		Trace_Dump(this,"empty()","false, updates pending");
	}

	//Trace_Exit<bool>(this,"empty()",empty);

	return empty;
}

void UpdateDatabase::clear()
{
	Trace_Entry(this, "clear()");

	leftMap.clear();
	aliveMap.clear();
	suspicionSet.clear();
	retainedMap.clear();

	Trace_Exit(this, "clear()");
}

void UpdateDatabase::writeToMessage(SCMessage_SPtr msg)
{
	using namespace std;

	Trace_Entry(this, "writeToMessage()");

	std::ostringstream oss;

	ByteBuffer&  bb = *(msg->getBuffer());

	bb.writeInt( (int)leftMap.size() );
	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		oss << "#Left=" << leftMap.size() << ", ";
	}

	for (NodeName2VersionStatusMap::iterator it = leftMap.begin(); it != leftMap.end(); ++it )
	{
		bb.writeString(it->first);
		msg->writeNodeVersion(it->second.first);
		bb.writeInt(static_cast<int>(it->second.second));

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			oss << it->first << " " << it->second.first.toString() << " " << it->second.second << ", ";
		}
	}

	bb.writeInt( (int)aliveMap.size() );
	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		oss << "#Alive=" << aliveMap.size() << ", ";
	}

	for (NodeID2VersionMap::iterator it = aliveMap.begin(); it != aliveMap.end(); ++it)
	{
		msg->writeNodeID(it->first);
		msg->writeNodeVersion(it->second);

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			oss << it->first->getNodeName() << " " << it->second.toString() << ", ";
		}
	}

	bb.writeInt( (int)suspicionSet.size() );
	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		oss << "#Susp=" << suspicionSet.size() << ", ";
	}

	for (SuspicionSet::iterator it = suspicionSet.begin(); it != suspicionSet.end(); ++it)
	{
		bb.writeString(*(it->getReporting()));
		bb.writeString(*(it->getSuspected()));
		msg->writeNodeVersion(it->getSuspectedNodeVersion());

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			oss << *(it->getReporting()) << " " << *(it->getSuspected()) << " " << it->getSuspectedNodeVersion().toString() << ", ";
		}
	}

	//AttRetain write retained history nodes
	bb.writeInt( (int)retainedMap.size() );
	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		oss << "#Retain=" << retainedMap.size() << ", ";
	}

	for (NodeID2VersionStatusMap::iterator it = retainedMap.begin(); it != retainedMap.end(); ++it)
	{
		msg->writeNodeID(it->first);
		msg->writeNodeVersion(it->second.first);
		bb.writeInt(it->second.second);

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			oss << it->first->getNodeName() << " " << it->second.first.toString() << " " << it->second.second << ", ";
		}
	}

	msg->updateTotalLength();
	if (crcEnabled_)
	{
		msg->writeCRCchecksum();
	}

	Trace_Event(this, "writeToMessage()", oss.str(), "size", msg->getBuffer()->getDataLength());

	Trace_Exit(this, "writeToMessage()");
}

}
