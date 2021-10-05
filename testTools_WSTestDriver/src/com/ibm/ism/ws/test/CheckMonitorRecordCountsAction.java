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

import com.ibm.ism.ws.test.TrWriter.LogLevel;


public class CheckMonitorRecordCountsAction extends ApiCallAction {
	private final String	_connectionID;
	private final String	_recordCounts;
	
	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CheckMonitorRecordCountsAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
        _connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null ){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
        _recordCounts = _actionParams.getProperty("recordCounts");
        if(_recordCounts == null ){
        	throw new RuntimeException("recordCounts is not defined for " + this.toString());
        }
       
	}

	protected boolean invokeApi() throws IsmTestException {
		
		final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);			
		if(con == null){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.VALIDATEMONITORRECORDS), 
					"Action {0}: Failed to locate the IsmWSConnection object ({1}).",_id, _connectionID);
			return false;
		}
		String[] pieces = _recordCounts.split(":");
		for (int i=0; i<pieces.length; i++) {
			String[] subPieces = pieces[i].split("=");
			if (2 != subPieces.length) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.VALIDATEMONITORRECORDS+1), 
						"Action {0}: recordCounts part {1} has wrong form {2}",_id, i, pieces[i]);
				return false;
			}
			MonitorRecord.ObjectType type = MonitorRecord.getMessageType(subPieces[0]);
			if (con.getMonitorRecordCount(type) < Long.valueOf(subPieces[1])) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.VALIDATEMONITORRECORDS+2)
						, "Action {0}: expected {1} monitor messages of type {2}, got {3}"
						,_id, subPieces[1], MonitorRecord.getMessageType(subPieces[0]), con.getMonitorRecordCount(type));
				return false;
			}
		}

		return true;
	}

}
