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

import java.util.List;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class UnsubscribeAction extends ApiCallAction {
	private final String _connectionID;
	private final String _topic;
	private final String[] _topics;
    private final int _expectedrclist[];
    private final List<MqttUserProp> _userprops;
	

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public UnsubscribeAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		
		super(config, dataRepository, trWriter);
		
		_connectionID = _actionParams.getProperty("connection_id");
		if (_connectionID == null) {
			throw new RuntimeException("connection_id is not defined for "
					+ this.toString());
		}
		_topic = _apiParameters.getProperty("topic");
		// If topic is null, must specifies topics (multiple topic subscribe request)
		if (null == _topic) {
			String temp = _apiParameters.getProperty("topics");
			if (null == temp) {
				throw new RuntimeException("ISMTEST"+(Constants.SUBSCRIBE)
						+ " Either 'topic' or 'topics' must be supplied for Subscribe");
			}
			_topics = temp.split(":");
		} else {
			_topics = null;
		}
        String temp = _apiParameters.getProperty("expectedrc");
		String temp2 = _apiParameters.getProperty("expectedrcv5");

	    if (null == temp) {
			if (null == temp2) {
				_expectedrclist = null;
			} else {
				_expectedrclist = new int[_topics == null ? 1 : _topics.length];
				String[] rclist = temp2.split(":");
				for (int i = 0; i < _expectedrclist.length; i++) {
					if (i < rclist.length) {
						_expectedrclist[i] = Integer.parseInt(rclist[i]);
					} else {
						_expectedrclist[i] = -1; // don't care
					}
				}
			}
	        
	    } else {
            _expectedrclist = new int[_topics == null ? 1 : _topics.length];
            String[] rclist = temp.split(":");
            for (int i = 0; i<_expectedrclist.length; i++) {
                if (i < rclist.length) {
                    _expectedrclist[i] = Integer.parseInt(rclist[i]);
                } else {
                    _expectedrclist[i] = -1; // don't care
                }
            }
		}

	    _userprops = getUserProps("userprop");
	}

	protected boolean invokeApi() throws IsmTestException {
		final MyConnection con =  (MyConnection)_dataRepository.getVar(_connectionID);
		if (con == null) 
		    throw(new IsmTestException("ISMTEST"+(Constants.UNSUBSCRIBE), "Cannot find connection named '"+_connectionID+"'"));
        setConnection(con);
        
        int [] rclist;
		if (null != _topics) {
			rclist = con.unsubscribe(_topics, _userprops);
		} else {
			rclist = con.unsubscribe(_topic, _userprops);
		}
	
	    boolean result = true;
	    if (version >= 5) {
	        /* TODO: fix rclist.length check when pahov5 is fixed */
            if (null != rclist && rclist.length > 0 && null != _expectedrclist) {
                for (int i = 0; i < _expectedrclist.length; i++) {
                    if (i < _expectedrclist.length) {
                        System.out.println("rclist=" + rclist.length);
                        if (-1 != _expectedrclist[i] && rclist[i] != _expectedrclist[i]) {
                            result = false;
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
                                , "ISMTEST"+(Constants.UNSUBSCRIBE+4)
                                , "Expected return code for element "+i
                                  +" to be "+_expectedrclist[i]
                                  +" was "+rclist[i]);
                        } else {
							_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST" + (Constants.UNSUBSCRIBE + 5),
									"Return code match, expected: " + _expectedrclist[i] + ", received: " + rclist[i]);
						}
						if (rclist[i] >= 128) {
							_trWriter.writeTraceLine(LogLevel.LOGLEV_WARN, "ISMTEST"+(Constants.UNSUBSCRIBE+7), "Warning: An Unsubscribe reason code >= 128, unsubscribe was unsuccessful");
						}
                    } else {
						
                        return result;
                    }
                }
            } else {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST" + (Constants.UNSUBSCRIBE + 6),
						"Unsubscribe action not checking return code.");
			}
		}
		
		if (version < 5 && rclist != null) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST" + (Constants.UNSUBSCRIBE + 6),
					"MQTT version less than 5, Unsubscribe action not checking return code.");
		}
    
        return result;
	}

}
