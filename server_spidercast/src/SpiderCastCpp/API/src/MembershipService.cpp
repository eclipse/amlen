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
 * MembershipService.cpp
 *
 *  Created on: 28/01/2010
 */

#include "MembershipService.h"

namespace spdr
{

const Const_Buffer
	MembershipService::EmptyAttributeValue = std::make_pair(0, (const char*) 0);

MembershipService::MembershipService()
{
}

MembershipService::~MembershipService()
{
}

}
