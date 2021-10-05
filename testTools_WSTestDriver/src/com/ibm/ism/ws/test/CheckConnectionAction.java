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

public class CheckConnectionAction extends ApiCallAction {
	private final 	String 					_structureID;
	private final	boolean					_isConnected;
	private final   int                     _reasonCode;   
	private final   String                  _reason;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CheckConnectionAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("connection_id");
		if (_structureID == null) {
			throw new RuntimeException("connection_id is not defined for "
					+ this.toString());
		}
		_isConnected = Boolean.parseBoolean(_actionParams.getProperty("isConnected", "true"));
		/*
		 * If a reasonCode is specified, the connection reason code must match.
		 * This test is only valid for MQTTv5 and above
		 */
		_reasonCode = Integer.parseInt(_actionParams.getProperty("reasonCode", "-1"));
		_reason = _actionParams.getProperty("reason");
	}

	protected boolean invokeApi() throws IsmTestException {
		MyConnection con = (MyConnection)_dataRepository.getVar(_structureID);
		if (null == con) {
			throw new IsmTestException("ISMTEST"+Constants.CHECKCONNECTION,
					"Connection "+_structureID+" is not found");
		}
		setConnection(con);
		
    	if (_isConnected != con.isConnected()) {
    		throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTION+1)
    			, "Expected connection.isConnected to be "+_isConnected+" but is "+con.isConnected());
		}
    	if (_reasonCode >= 0 && version >= 5 && con.getReasonCode() != _reasonCode) {
            throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTION+2),
                "Expected connection reasonCode to be " + _reasonCode + " but got " + con.getReasonCode());
    	} else if (_reasonCode >= 0 && version >= 5 && con.getReasonCode() == _reasonCode) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST" + (Constants.CHECKCONNECTION + 6),
					"Connection " + _structureID + " reason code match, expected " + _reasonCode + ", got " + con.getReasonCode());
			}

    	if (_reason != null && version >= 5) {
    	    String reason = con.getReason();
    	    if (reason == null || reason.indexOf(_reason) < 0) {
                throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTION+3),
    	            "Expected reason string to include \"" + _reason + "\" but got \"" + reason + "\"");
    	    } else {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST" + (Constants.CHECKCONNECTION + 6),
						"Connection " + _structureID + " reason string match, expected \"" + _reason + "\", got \"" + reason + "\"");
					}
			}
		
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST" + (Constants.CHECKCONNECTION + 6),
				"Connection " + _structureID + " match");

		return true;
	}

}
