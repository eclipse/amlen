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
 * MembershipServiceConfig.cpp
 *
 *  Created on: Feb 20, 2011
 */

#include <sstream>

#include "MembershipServiceConfig.h"

namespace spdr
{

MembershipServiceConfig::MembershipServiceConfig(const PropertyMap& properties) :
		BasicConfig(properties),
		fzmRequestTimeoutMillis()
{
	//using namespace config::membership_service;

	fzmRequestTimeoutMillis = getOptionalInt32Property(
			config::HierarchyForeignZoneMemberhipTimeOut_PROP_KEY,
			config::HierarchyForeignZoneMemberhipTimeOut_DEFVALUE);
	{
		std::ostringstream oss;
		oss << fzmRequestTimeoutMillis;
		prop.setProperty(config::HierarchyForeignZoneMemberhipTimeOut_PROP_KEY, oss.str());
	}
}

MembershipServiceConfig::~MembershipServiceConfig()
{
}

}
