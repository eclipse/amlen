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
 * HierarchyUtils.cpp
 *
 *  Created on: Jan 10, 2011
 */

#include "HierarchyUtils.h"

namespace spdr
{

const std::string HierarchyUtils::hierarchy_AttributeKeyPrefix = ".hier.";

const std::string HierarchyUtils::delegateState_AttributeKey = ".hier.del.state";
const std::string HierarchyUtils::delegateSupervisor_AttributeKeyPrefix =
		".hier.del.supervisor:";

const std::string HierarchyUtils::supervisorState_AttributeKey =
		".hier.sup.state";
const std::string HierarchyUtils::supervisorGuardedBaseZone_AttributeKeyPrefix =
		".hier.sup.gbz:";

const std::string HierarchyUtils::ReplyTypeName[] =
{ "NONE", "ACCEPT", "REJECT", "REDIRECT" };

HierarchyUtils::HierarchyUtils()
{
	//
}

HierarchyUtils::~HierarchyUtils()
{
	//
}
}
