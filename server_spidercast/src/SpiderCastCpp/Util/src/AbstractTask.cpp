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
 * AbstractTask.cpp
 *
 *  Created on: 03/03/2010
 */

#include "AbstractTask.h"

namespace spdr
{
const String AbstractTask::stateName[] =
	{ 		String("X"),
			String("VIRGIN"),
			String("SCHEDULED"),
			String("EXECUTED"),
			String("CANCELED") };
//just make the Eclipse auto-build happy
}
