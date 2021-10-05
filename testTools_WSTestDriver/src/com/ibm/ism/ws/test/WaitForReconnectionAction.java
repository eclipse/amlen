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


public class WaitForReconnectionAction extends ApiCallAction {
	private final String 		_connectionID;
	private final int			_waitTime;
	
	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public WaitForReconnectionAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
        _connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null ){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
        _waitTime = Integer.parseInt(_actionParams.getProperty("waitTime", "10000"));
       
	}

	protected boolean invokeApi() throws IsmTestException {
		
		if(_connectionID != null){
			final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);			
			if(con == null){
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.WAITPENDING), 
						"Action {0}: Failed to locate the IsmWSConnection object ({1}).",_id, _connectionID);
				return false;
			}
            setConnection(con);
			
			if (con.waitForReconnection(_waitTime*1000)) {
				return true;
			}
			
		}
		
		return false;
	}

}
