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


import java.io.IOException;
import java.util.List;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class CloseConnectionAction extends ApiCallAction {
	private final String _connID;
	        final int _rc;
	        final String _reason;
	        final List<MqttUserProp> _userprops;
	        final long    _sessionExpire;  /* uint32_t */

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CloseConnectionAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_connID = _actionParams.getProperty("connection_id");
		if (_connID == null) {
			throw new RuntimeException("connection_id is not defined for "
					+ this.toString());
		}
	    _userprops = getUserProps("userprop");
	    _rc = Integer.parseInt(_apiParameters.getProperty("rc", "0"));
	    _reason = _apiParameters.getProperty("reason");
	    _sessionExpire = Long.valueOf(_apiParameters.getProperty("sessionExpire", "-1")); 
	}

	protected boolean invokeApi() throws IsmTestException {
		final MyConnection con = (MyConnection)_dataRepository.getVar(_connID);
		if (con == null) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+Constants.CLOSECONNECTION,
					"Action {0}: Failed to locate the Connection object ({1}).",_id, _connID);
			return false;
		}
		setConnection(con);
		
		try {
			con.disconnect(_rc, _reason, _sessionExpire, _userprops);
		} catch (IOException ie) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.CLOSECONNECTION+1),
					"Action {0}: Failed to disconnect the Connection object ({1}).",
					_id, _connID);
			throw(new IsmTestException("ISMTEST"+(Constants.CLOSECONNECTION+1), ie.getLocalizedMessage(), ie));
		}
		return true;
	}

}
