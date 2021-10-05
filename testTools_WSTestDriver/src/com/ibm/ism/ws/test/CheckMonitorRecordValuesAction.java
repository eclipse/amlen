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


public class CheckMonitorRecordValuesAction extends ApiCallAction {
	private final String	_connectionID;
	private final String	_recordType;
	private final String	_monitorRecordPrefix;
	private final int		_monitorRecordIndex;
	private final String	_monitorRecordKey;
	private final String[]  _values;
	
	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CheckMonitorRecordValuesAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
        _connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null ){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
        _recordType = _actionParams.getProperty("recordType");
        if(_recordType == null ){
        	throw new RuntimeException("recordType is not defined for " + this.toString());
        }
        _monitorRecordPrefix = _actionParams.getProperty("monitorRecordPrefix");
        _monitorRecordIndex = Integer.valueOf(_actionParams.getProperty("monitorRecordIndex", "-1"));
        _monitorRecordKey = _actionParams.getProperty("monitorRecordKey");
        if (null == _monitorRecordPrefix && null == _monitorRecordKey) {
        	throw new RuntimeException("Must define either monitorRecordPrefix or monitorRecordKey for "+this.toString());
        }
        String temp = _actionParams.getProperty("values");
        if (null == temp) {
        	throw new RuntimeException("values is not defined for " + this.toString());
        }
        _values = temp.split(":");
       
	}

	protected boolean invokeApi() throws IsmTestException {
		
		final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);			
		if(con == null){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.VALIDATEMONITORRECORDS), 
					"Action {0}: Failed to locate the IsmWSConnection object ({1}).",_id, _connectionID);
			return false;
		}
		MonitorRecord.ObjectType type = MonitorRecord.getMessageType(_recordType);
		MonitorRecord record = null;
		if (null != _monitorRecordPrefix) {
			if (-1 == _monitorRecordIndex) {
				int i=0;
				MonitorRecord nextRecord = (MonitorRecord)_dataRepository.getVar(_monitorRecordPrefix+i);
				while (null != nextRecord) {
					if (nextRecord.getMessageType() == type) {
						record = nextRecord;
					}
					i++;
					nextRecord = (MonitorRecord)_dataRepository.getVar(_monitorRecordPrefix+i);
				}
				if (null == record) {
					throw new IsmTestException("ISMTEST"+(Constants.CHECKMONITORRECORDVALUE)
						, "No records found of type "+_recordType);
				}
			} else {
				String key = _monitorRecordPrefix + _monitorRecordIndex;
				record = (MonitorRecord)_dataRepository.getVar(key);
				if (null == record) {
					throw new IsmTestException("ISMTEST"+(Constants.CHECKMONITORRECORDVALUE)
						, "Could not retrieve MonitorRecord "+key);
				}
			}
		} else {
			record = (MonitorRecord)_dataRepository.getVar(_monitorRecordKey);
			if (null == record) {
				throw new IsmTestException("ISMTEST"+(Constants.CHECKMONITORRECORDVALUE)
					, "Could not retrieve MonitorRecord "+_monitorRecordKey);
			}

		}
		if (record.getMessageType() != type) {
			throw new IsmTestException("ISMTEST"+(Constants.CHECKMONITORRECORDVALUE+1)
					, "Expected record of type "+_recordType
					+", actual record is type "+record.getMessageTypeString());
		}
		record.validateValues(type, _values);

		return true;
	}

}
