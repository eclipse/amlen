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
 * MembershipDefinitions.h
 *
 *  Created on: 31/01/2010
 */

#ifndef MEMBERSHIPDEFINITIONS_H_
#define MEMBERSHIPDEFINITIONS_H_

#include "NodeIDImpl.h"
#include "NodeInfo.h"
#include "VirtualID.h"

namespace spdr
{

//typedef std::map<NodeIDImpl_SPtr , NodeInfo, NodeIDImpl_SPtr::Less> MembershipViewMap;
typedef boost::unordered_map<NodeIDImpl_SPtr, NodeInfo, NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals > MembershipViewMap;

//typedef std::set<NodeIDImpl_SPtr, NodeIDImpl::SPtr_Less> MembershipRing; //By NodeID
typedef std::map<util::VirtualID_SPtr, NodeIDImpl_SPtr, util::VirtualID::SPtr_Less> MembershipRing; //By VirtualID

//typedef boost::unordered_map< NodeIDImpl_SPtr, NodeInfo, NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals > NodeHistoryMap;
typedef std::map< NodeIDImpl_SPtr, NodeInfo, NodeIDImpl::SPtr_Less > NodeHistoryMap;

} //namespace spdr
#endif /* MEMBERSHIPDEFINITIONS_H_ */
