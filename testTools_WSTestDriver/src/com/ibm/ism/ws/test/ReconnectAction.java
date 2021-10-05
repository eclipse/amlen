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
 * File        : WsConnectionAction.java
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import java.io.IOException;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class ReconnectAction extends ApiCallAction {
	/*
	private final String _structureID;
	*/
	private final String	_connectionID;
	

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public ReconnectAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		
		super(config, dataRepository, trWriter);
		/*
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
		*/
		
		_connectionID = _actionParams.getProperty("connection_id");
		if (_connectionID == null) {
			throw new RuntimeException("connection_id is not defined for "
					+ this.toString());
		}
	}

	protected boolean invokeApi() throws IsmTestException {
		final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);			
		if(con == null){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECONNECT)
				, "Action {0}: Failed to locate the IsmWSConnection object ({1}).",_id, _connectionID);
			return false;
		}
        setConnection(con);
		
		if (con.isConnected() == true) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECONNECT+1)
				, "Action {0}: Connection object ({1}) is already connected.", _id, _connectionID);
		}
		try {
			con.connect(con.getMyOpts());
		}catch (IOException ie) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECONNECT+2),
					"Action {0}: Failed to create the Connection object ({1}).",
					_id,_connectionID);

			throw(new IsmTestException("ISMTEST"+(Constants.RECONNECT+2)
					, ie.getLocalizedMessage(), ie));
		}
		return true;
	}

}
