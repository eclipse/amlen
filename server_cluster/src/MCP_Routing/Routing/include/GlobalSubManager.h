/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#ifndef GLOBALSUBMANAGER_H_
#define GLOBALSUBMANAGER_H_

#include "RouteLookup.h"
#include "SubCoveringFilterEventListener.h"

namespace mcp {

class GlobalSubManager : public RouteLookup, public SubCoveringFilterEventListener {
public:
	GlobalSubManager();
	virtual ~GlobalSubManager();

};

} /* namespace mcp */

#endif /* GLOBALSUBMANAGER_H_ */
