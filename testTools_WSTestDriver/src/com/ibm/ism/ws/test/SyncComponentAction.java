/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
/*-----------------------------------------------------------------
 * System Name : Internet Scale Messaging
 * File        : SyncComponentAction.java
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

/**
 * 
 */
class SyncComponentAction extends Action {
	private final List<String> component_list;
	private final String component_name;
	private final int timeout;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	SyncComponentAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);

		String component_list_string = _actionParams
				.getProperty("component_list");
		component_list = new ArrayList<String>();
		StringTokenizer strtok = new StringTokenizer(component_list_string, ";");
		while (strtok.hasMoreTokens()) {
			component_list.add(strtok.nextToken());
		}

		component_name = _actionParams.getProperty("component_name");
		timeout = Integer.parseInt(_actionParams
				.getProperty("timeout", "20000"));
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.ibm.mds.rm.test.Action#run()
	 */
	@Override
	protected final boolean run() {
		try {
			String condition;
			// invoke "init" and "set" operation for component_name
			condition = component_name + "_sync-component_ready";
			IsmSyncClient.invokeSyncRequest(IsmSyncClient.INIT_REQUEST,
					condition, -1, timeout, _trWriter);
			condition = component_name + "_sync-component_ready";
			IsmSyncClient.invokeSyncRequest(IsmSyncClient.SET_REQUEST,
					condition, 1, timeout, _trWriter);

			// invoke "wait" operations for all other components
			for (int i = 0; i < component_list.size(); i++) {
				condition = component_list.get(i) + "_sync-component_ready";
				IsmSyncClient.invokeSyncRequest(IsmSyncClient.WAIT_REQUEST,
						condition, 1, timeout, _trWriter);
			}
			return true;
		} catch (Throwable e) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.SYNCCOMPONENT),
					"Action {0} failed. The reason is: {1}", _id,
					e.getMessage());
			_trWriter.writeException(e);
			return false;
		}
	}

}

