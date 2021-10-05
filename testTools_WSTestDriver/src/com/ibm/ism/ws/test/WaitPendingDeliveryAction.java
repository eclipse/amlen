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


import java.util.Date;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;


public class WaitPendingDeliveryAction extends ApiCallAction {
	private final String 		_connectionID;
	private final int			_waitTime;
	private final int			_maxPendingMsgs;
	
	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public WaitPendingDeliveryAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
        _connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null ){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
        _waitTime = Integer.parseInt(_actionParams.getProperty("waitTime", "10000"));
        _maxPendingMsgs = Integer.parseInt(_actionParams.getProperty("maxPendingMsgs", "0"));
       
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
			
			if (con.isConnected() == false) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.WAITPENDING+1),
						"Action {0}: Connection object ({1}) is not connected.", _id, _connectionID);
			}
			long curTime = (new Date()).getTime();
			long endTime = curTime + _waitTime;
			int tokens; 
			try {
				do {
					tokens = con.countPendingDeliveryTokens();
					if (tokens <= _maxPendingMsgs) break;
					Thread.sleep(10);
					curTime = (new Date()).getTime();
				} while (curTime < endTime);
			} catch (Exception ie) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.WAITPENDING+2)
						, "Action {0}: Connection object ({1}) received Exception: ({2})"
						, _id, _connectionID, ie.getMessage());
				throw new IsmTestException("ISMTEST"+(Constants.WAITPENDING+2)
						, ie.getMessage(), ie);
			}
			if (tokens > _maxPendingMsgs) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.WAITPENDING+3)
						, "Action [0]: Wait for pending delivery tokens expired with {1} tokens still outstanding"
						, _id, tokens);
				con.checkPendingDeliveryTokens();
			}

			
		}
		
		return true;
	}

}
