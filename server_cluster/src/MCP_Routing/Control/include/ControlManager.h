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

#ifndef CONTROLMANAGER_H_
#define CONTROLMANAGER_H_

#include "SubCoveringFilterPublisher.h"
#include "EngineEventCallback.h"
#include "ForwardingControl.h"
#include "RemoteServerRecord.h"

namespace mcp
{

class ControlManager
{
public:
	ControlManager();
	virtual ~ControlManager();

	/**
	 * Store the subscription patterns that are assigned to the wild-card
	 * Bloom filter, to facilitate recovery of the LocalSubManager.
	 *
	 * This vector should contain all the patterns that are assigned to
	 * the WC-BF.
	 *
	 * Call this before publishing the new pattern base or update.
	 *
	 * @param patterns
	 * @return
	 */
	virtual MCPReturnCode storeSubscriptionPatterns(
			const std::vector<SubscriptionPattern_SPtr>& patterns) = 0;

	virtual void executeViewNotifyTask() = 0;

	virtual void executePublishRemovedServersTask() = 0;

	virtual void executeAdminDeleteRemovedServersTask(RemoteServerVector& pendingRemovals) = 0;

	virtual void executePublishRestoredNotInViewTask() = 0;

	virtual void executeRequestAdminMaintenanceModeTask(int errorRC, int restartFlag) = 0;

	virtual void onTaskFailure(const std::string& component, const std::string& errorMessage, int rc) = 0;

};

} /* namespace mcp */

#endif /* CONTROLMANAGER_H_ */
