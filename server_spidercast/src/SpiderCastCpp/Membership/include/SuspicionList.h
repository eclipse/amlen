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
 * SuspicionList.h
 *
 *  Created on: 17/05/2010
 */

#ifndef SUSPICIONLIST_H_
#define SUSPICIONLIST_H_

#include <list>

#include "Definitions.h"
#include "NodeVersion.h"
#include "SCMessage.h"

namespace spdr
{
class SuspicionList
{
public:
	SuspicionList();
	virtual ~SuspicionList();

	int size() const;
	void clear();

	/**
	 * Add suspicion to list.
	 *
	 * @param reporter
	 * @param suspect_ver
	 * @return true if it is a new suspicion, i.e. new reporter or existing reporter with newer version.
	 */
	bool add(StringSPtr reporter, NodeVersion suspect_ver);

	void deleteOlder(NodeVersion suspect_ver);

	/**
	 * Write the suspicions to a membership message, in the format:
	 * reporter-name (string)
	 * suspect-name (string)
	 * suspect-version (NodeVersion)
	 *
	 * @param suspectName
	 * @param msg
	 */
	void writeToMessage(const String& suspectName, SCMessage& msg) const;

	String toString() const;


private:
	/* A list of reporter-name and NodeVersion pairs. */
	typedef std::list<std::pair<StringSPtr, NodeVersion > > NameVerList;
	NameVerList list;
};

}//namespace spdr
#endif /* SUSPICIONLIST_H_ */
