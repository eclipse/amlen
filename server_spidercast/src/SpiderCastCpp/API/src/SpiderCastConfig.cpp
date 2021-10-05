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
 * SpiderCastConfig.cpp
 *
 *  Created on: 28/01/2010
 */

#include <iostream>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "SpiderCastConfig.h"

namespace spdr
{
using namespace std;

SpiderCastConfig::SpiderCastConfig(const PropertyMap& properties) :
		BasicConfig(properties)
{
	//cout << "SpiderCastConfig()" << endl;
}

SpiderCastConfig::SpiderCastConfig(const SpiderCastConfig& config) :
		BasicConfig(config.prop)
{
	//cout << "SpiderCastConfig(SpiderCastConfig&)" << endl;
}

SpiderCastConfig::~SpiderCastConfig()
{
	//cout << "~SpiderCastConfig()" << endl;
}

String SpiderCastConfig::toString() const
{
	return BasicConfig::toString();
}
}
