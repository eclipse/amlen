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
 * ScTrConstants.h
 *
 *  Created on: 16/06/2010
 */

#ifndef SCTRCONSTANTS_H_
#define SCTRCONSTANTS_H_

#include <boost/cstdint.hpp>

namespace spdr
{
namespace trace
{

/*
 * Trace related constants
 */
class ScTrConstants
{
public:

	//=== component name ===
	static const char* ScTr_Component_Name;// = "SpiderCastCpp"

	//=== sub-component names ===
	// When adding a subcomponent remember to update the layer-ID and mask
	static const char* ScTr_SubComponent_Core;// = "Core";
	static const char* ScTr_SubComponent_Comm;// = "Comm";
	static const char* ScTr_SubComponent_Mem;// = "Mem";
	static const char* ScTr_SubComponent_Topo;// = "Topo";
	static const char* ScTr_SubComponent_Hier;// = "Hier";
	static const char* ScTr_SubComponent_Route;// = "Route";
	static const char* ScTr_SubComponent_Msgn;// = "Msgn";
	static const char* ScTr_SubComponent_DHT;// = "DHT";
	static const char* ScTr_SubComponent_Trace;// = "Trace";
	static const char* ScTr_SubComponent_Util;// = "Util";
	static const char* ScTr_SubComponent_API; // = "API";
	static const char* ScTr_SubComponent_TEST; // = "TEST";
	static const char* ScTr_SubComponent_App; // = "App";

	/*
	 * Layer ID - a sequential numbering of layers
	 * The layer ID bitmap is the higher 24 bits of the trace-config
	 */
	enum LayerID
	{
		Layer_ID_Core = 8,
		Layer_ID_CommAdapter,
		Layer_ID_Membership,
		Layer_ID_Topology,
		Layer_ID_Hierarchy,
		Layer_ID_Route,
		Layer_ID_Msgn,
		Layer_ID_DHT,
		Layer_ID_Trace,
		Layer_ID_Util,
		Layer_ID_API,
		Layer_ID_TEST,
		Layer_ID_App,
		Layer_ID_Max
	};

	/*
	 * Layer Mask - A bit for each layer, according to the order defined in LayerID
	 */
	static const uint32_t Layer_Mask     			= 0xFFFFFF00;

	static const uint32_t Layer_Mask_Core 			= 0x01 << Layer_ID_Core;
	static const uint32_t Layer_Mask_CommAdapter 	= 0x01 << Layer_ID_CommAdapter;
	static const uint32_t Layer_Mask_Membership 	= 0x01 << Layer_ID_Membership;
	static const uint32_t Layer_Mask_Topology 		= 0x01 << Layer_ID_Topology;
	static const uint32_t Layer_Mask_Hierarchy 		= 0x01 << Layer_ID_Hierarchy;
	static const uint32_t Layer_Mask_Route	 		= 0x01 << Layer_ID_Route;
	static const uint32_t Layer_Mask_Msgn	 		= 0x01 << Layer_ID_Msgn;
	static const uint32_t Layer_Mask_DHT	 		= 0x01 << Layer_ID_DHT;
	static const uint32_t Layer_Mask_Trace 			= 0x01 << Layer_ID_Trace;
	static const uint32_t Layer_Mask_Util 			= 0x01 << Layer_ID_Util;
	static const uint32_t Layer_Mask_API 			= 0x01 << Layer_ID_API;
	static const uint32_t Layer_Mask_TEST 			= 0x01 << Layer_ID_TEST;
	static const uint32_t Layer_Mask_App 			= 0x01 << Layer_ID_App;

	/*
	 * The log level is the lower byte of the trace-config
	 */
	static const uint32_t Level_Mask 		    = 0x000000FF;

	/*
	 * NLS resource bundle name
	 */
	static const char* ScTr_ResourceBundle_Name;// = "spdr_nls.props";

private:
	ScTrConstants();
	~ScTrConstants();
	ScTrConstants(const ScTrConstants&);
	ScTrConstants& operator=(const ScTrConstants&);

}; //class

}// namespace trace
}// namespace spdr

#endif /* SCTRCONSTANTS_H_ */
