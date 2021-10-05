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
 * MembershipServiceConfig.h
 *
 *  Created on: Feb 20, 2011
 */

#ifndef MEMBERSHIPSERVICECONFIG_H_
#define MEMBERSHIPSERVICECONFIG_H_

#include "SpiderCastConfig.h"
#include "MembershipService.h"

namespace spdr
{

/**
 * Configuration parameters of the membership service.
 */
class MembershipServiceConfig: public BasicConfig
{
public:
	MembershipServiceConfig(const PropertyMap& properties);

	virtual ~MembershipServiceConfig();

	int getFZMRequestTimeoutMillis() const
	{
		return fzmRequestTimeoutMillis;
	}

private:
	int fzmRequestTimeoutMillis;
};

}

#endif /* MEMBERSHIPSERVICECONFIG_H_ */
