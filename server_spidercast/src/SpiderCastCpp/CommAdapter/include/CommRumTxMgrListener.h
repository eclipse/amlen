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
 * CommRumTxMgrListener.h
 *
 *  Created on: Nov 8, 2015
 */

#ifndef COMMRUMTXMGRLISTENER_H_
#define COMMRUMTXMGRLISTENER_H_

#include "rumCapi.h"

namespace spdr
{

class CommRumTxMgrListener
{
public:
	virtual ~CommRumTxMgrListener(){};
	virtual void onStreamNotPresent(rumStreamID_t sid) = 0;
};

} /* namespace mcp */

#endif /* COMMRUMTXMGRLISTENER_H_ */
