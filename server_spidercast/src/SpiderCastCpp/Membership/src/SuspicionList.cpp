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
 * SuspicionList.cpp
 *
 *  Created on: 17/05/2010
 */

#include "SuspicionList.h"

namespace spdr
{
SuspicionList::SuspicionList() :
		list()
{
}

SuspicionList::~SuspicionList()
{
}

int SuspicionList::size() const
{
	return list.size();
}

void SuspicionList::clear()
{
	list.clear();
}

bool SuspicionList::add(StringSPtr reporter, NodeVersion suspect_ver)
{
	bool is_new = false;
	bool found = false;

	for (NameVerList::iterator it = list.begin(); it != list.end(); ++it)
	{
		if (*((*it).first) == *reporter)
		{
			if ((*it).second < suspect_ver)
			{
				(*it).second = suspect_ver;
				is_new = true;
			}
			found = true;
		}
	}

	if (!found)
	{
		list.push_back(std::make_pair(reporter, suspect_ver));
		is_new = true;
	}

	return is_new;
}

void SuspicionList::deleteOlder(NodeVersion suspect_ver)
{
	NameVerList::iterator it = list.begin();
	while (it != list.end())
	{
		if ((*it).second < suspect_ver)
		{
			it = list.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void SuspicionList::writeToMessage(const String& suspectName, SCMessage& msg) const
{
	for (NameVerList::const_iterator it = list.begin(); it!=list.end(); ++it)
	{
		ByteBuffer& bb = *(msg.getBuffer());
		bb.writeString(*(it->first)); //reporter
		bb.writeString(suspectName); //suspect
		msg.writeNodeVersion(it->second); //suspect-version
	}
}

String SuspicionList::toString() const
{
	std::ostringstream oss;
	oss << "[";
	for (NameVerList::const_iterator it = list.begin(); it!=list.end(); ++it)
	{
		oss << 	"Rep=" << (*(it->first)) << " SusVer=" << it->second.toString() << " "; //suspect-version
	}
	oss << "]";
	return oss.str();
}

}
