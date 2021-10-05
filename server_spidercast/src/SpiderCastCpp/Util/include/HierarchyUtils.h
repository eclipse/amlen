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
 * HierarchyUtils.h
 *
 *  Created on: Jan 10, 2011
 */

#ifndef HIERARCHYUTILS_H_
#define HIERARCHYUTILS_H_

#include <string>
#include <set>

#include "NodeIDImpl.h"

namespace spdr
{

typedef std::set<NodeIDImpl_SPtr,spdr::SPtr_Less<NodeIDImpl> > NodeIDImpl_Set;

class HierarchyUtils
{
private:
	HierarchyUtils();
	virtual ~HierarchyUtils();
public:
	//=== Attribute keys ===

	static const std::string hierarchy_AttributeKeyPrefix; 					// ".hier."

	static const std::string delegateState_AttributeKey; 					// ".hier.del.state"
	static const std::string delegateSupervisor_AttributeKeyPrefix; 			// ".hier.del.supervisors:"

	static const std::string supervisorState_AttributeKey; 					// ".hier.sup.state";
	static const std::string supervisorGuardedBaseZone_AttributeKeyPrefix; 	// ".hier.sup.gbz:";

	enum ReplyType
	{
		ACCEPT = 1,
		REJECT,
		REDIRECT
	};
	static const std::string ReplyTypeName[];

	enum DelegateState
	{
		delegateState_Candidate = 1, 	//attempting to connect
		delegateState_Connected	= 2		//already connected
	};
};
}
#endif /* HIERARCHYUTILS_H_ */
