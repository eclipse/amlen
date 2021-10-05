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


import java.util.Arrays;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class GetSetMessageTextAction extends ApiCallAction {
	private final 	String 				_msgID;
	
	public GetSetMessageTextAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_msgID = _actionParams.getProperty("message_id");
		if (_msgID == null) {
			throw new RuntimeException("message_id is not defined for "
					+ this.toString());
		}
	}
	
	protected boolean invokeApi() throws IsmTestException {
		
		MyMessage msg = (MyMessage) _apiParameters.get("message_id");
		try {
			if ( msg.getType() == MyMessage.TEXT) {
				return verify(msg.getPayload(),_apiParameters.getProperty("verifyValue")) ;
			}
			else if (msg.getType() == MyMessage.BINARY) {
				return verify(msg.getPayloadBytes(), (byte[])_apiParameters.get("verifyValue"));
			} 
		} catch (Exception e) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.GETSETMESSAGETEST),
					"Exception "+e.getMessage()+" was thrown trying to get payload");
			throw new IsmTestException("ISMTEST4003", 
					"Exception "+e.getMessage()+" was thrown trying to get payload", e);
		}
		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.GETSETMESSAGETEST+1), 
				"Action {0}: The getText API is not valid for message of type: {1}", _id, msg.getClass().getSimpleName());
		return false;
	}
	
	private boolean verify(String received, String ori){
		return (ori.equals(received));
	}
	
	private boolean verify(byte[] received, byte[] ori){
		return Arrays.equals(ori, received);
	}
	
}
