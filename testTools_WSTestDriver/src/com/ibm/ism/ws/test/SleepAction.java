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

public class SleepAction extends ApiCallAction {
	private final 	long 	_waitTime;
	private final	String	_connectionID;
	private final	String	_topic;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public SleepAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		String temp = _actionParams.getProperty("waitTime");
		if (null == temp) throw new RuntimeException("waitTime not defined for "
				+this.toString());
		_waitTime = Long.valueOf(temp);
		_connectionID = _actionParams.getProperty("connection_id");
		if (null != _connectionID) {
			_topic = _actionParams.getProperty("topic");
			// If topic not specified, then terminate Sleep on loss of connection only
			//if (null == _topic) throw new RuntimeException("topic must be specified if connection_id is specified for "
			//		+this.toString());
			
		} else {
			_topic = null;
		}
	}

	long curtime() {
		return new Date().getTime();
	}
	
	private long min(long one, long two) {
		if (one < two) return one;
		return two;
	}
	
	protected boolean invokeApi() throws IsmTestException {
		MyConnection con = null;
		if (null != _connectionID) {
			con = (MyConnection)_dataRepository.getVar(_connectionID);
			if(con == null){
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECEIVEMESSAGE)
					, "Action {0}: Failed to locate the Connection object ({1}).",_id, _connectionID);
				return false;
			}
	        setConnection(con);
			con.setFlag(_topic);
		}
		long time = curtime() + _waitTime;
		do {
			long timeToSleep = min(1000L, time - curtime());
			try {
				Thread.sleep(timeToSleep);
			} catch (InterruptedException e) {}
		} while (curtime() < time && (null == con ? true : con.getFlag()));
		if (null != con) {
			con.clearFlag();
		}
		return true;
	}

}
