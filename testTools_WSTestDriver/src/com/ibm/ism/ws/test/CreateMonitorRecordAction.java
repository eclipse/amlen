/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.ws.test;

import org.w3c.dom.Element;

public class CreateMonitorRecordAction extends ApiCallAction {
	private final 	String 	_structureID;
	private final	String	_connectionID;
	private final	String	_monitorID;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CreateMonitorRecordAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
        _connectionID = _actionParams.getProperty("connection_id");
		_monitorID = _actionParams.getProperty("monitor_id");
		if (null == _monitorID) {
			throw new RuntimeException("monitor_id is not defined for "
					+ this.toString());
		}
	}

	protected boolean invokeApi() throws IsmTestException {
		MyMessage msg = (MyMessage)_dataRepository.getVar(_structureID);
		if (null == msg) throw new IsmTestException("ISMTEST"+Constants.CREATEMONITORRECORD,
				"Could not retrieve message "+_structureID);
		
		MonitorRecord monitor = new MyMonitorRecord(msg).getMonitorRecord();
		_dataRepository.storeVar(_monitorID, monitor);
		if (null != _connectionID) {
			MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);
			if (null != con) {
				con.countMonitorRecords(monitor);
			}
			setConnection(con);
		}
		return true;
	}

}
