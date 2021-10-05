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
 * SpiderCastEvent.cpp
 *
 *  Created on: 17/02/2010
 */

#include "SpiderCastEvent.h"

namespace spdr
{

namespace event
{

const String SpiderCastEvent::eventTypeName[] =
{
		"None",

		//=== Error & Warning Events ===
		// These events are delivered to the SpiderCastEventListener
		"Fatal_Error",
		"Warning_Connection_Refused",
		"Warning_Message_Refused",

		//=== Topology Events ===
		// These events are delivered to the SpiderCastEventListener
		"Connectivity",

		//=== Membership Events ===
		"View_Change",
		"Node_Join",
		"Node_Leave",
		"Change_of_Metadata",//
		"Foreign_Zone_Membership",
		"Zone_Census",//

		//=== P2P Events ===
		"P2P_Stream_Broke"
};

const String SpiderCastEvent::errorCodeName[] =
{ 		"None", //
		"Thread_Exit_Abnormally",
		"Network_Failure",
		"Component_Failure",
		"Duplicate_Local_Node_Detected",

		"Connection_Refused_Handshake_Parse_Error",
		"Connection_Refused_Incompatible_Version",
		"Connection_Refused_Incompatible_BusName",
		"Connection_Refused_Wrong_TargetName",

		"Message_Refused_Parse_error",
		"Message_Refused_Incompatible_Version",
		"Message_Refused_Incompatible_BusName",
		"Message_Refused_Incompatible_TargetName",

		"Duplicate_Remote_Node_Suspicion",

		"Foreign_Zone_Membership_Request_Timeout",
		"Foreign_Zone_Membership_Unreachable",
		"Foreign_Zone_Membership_No_Attributes",

		"Error_Code_Max"
};

SpiderCastEvent::~SpiderCastEvent(){}

}

}
