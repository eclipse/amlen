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
 * File        : ApiCallAction.java
 * Author      : 
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import java.util.List;
import java.util.Properties;
import java.util.Vector;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;
import com.ibm.ism.ws.test.IsmTestException;

/*
 */
public abstract class ApiCallAction extends Action {

	protected final int _expectedRC;
	protected final int _expectedRCv5;
	protected final String _expectedFailureReason;
	protected final String _expectedFailureReasonV5;
	protected final Properties _apiParameters;
	                MyConnection connect;
	                MyConnectionKafka connectKafka;
	protected       int version = 4;

	/*
	 * If the attribute rcv5 exists, it is used instead of rc when the MQTT version is
	 * greater or equal to 5.  If the attribute reasonv5 exists, it is used insted of reason
	 * when the MQTT version is greater or equal 5.
	 */
	protected ApiCallAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_expectedRC = Integer.parseInt(getConfigAttribute(config, "rc", "0"));
		if (_expectedRC != 0) {
			_expectedFailureReason = config.getAttribute("reason");
		} else {
			_expectedFailureReason = null;
		}
		_expectedRCv5 = Integer.parseInt(getConfigAttribute(config, "rcv5", ""+_expectedRC));
	    if (_expectedRCv5 != 0) {
	        String reason = config.getAttribute("reasonv5");
	        if (reason == null || reason.length() == 0)
	            reason = _expectedFailureReason;
	        _expectedFailureReasonV5 = reason;
	    } else {
	        _expectedFailureReasonV5 = null;
	    }
	    
		_apiParameters = new Properties();
		parseApiParameters(config);
	}
	
	/*
	 * Set the connection associated with the API call.  
	 * This can be null
	 */
	protected void setConnection(MyConnection conn) {
	    connect = conn;
	    if (connect != null) {
	        version = connect.getVersion();
	    }
	}

	protected void checkExpectedResult(IsmTestException result) {
	    int xrc = version >= 5 ? _expectedRCv5 : _expectedRC;
	    String xreason = version >=5 ? _expectedFailureReasonV5 : _expectedFailureReason;
		if (xrc == 0) {
			if (result != null) {
			    String ec = result.getErrorCode();
		        if (!"ISMTEST4016".equals(ec) && !"ISMTEST4017".equals(ec)) {
				    _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,	"ISMTEST"+Constants.APICALL,
								"Action {0}: Call failed. Expected result is: SUCCESS. Real result was {1}",
								_id, result);
				    throw new RuntimeException("Result of the action is not as expected.", result);
		        }    
			}
		} else {
    		if (result == null) {
    			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,	"ISMTEST"+(Constants.APICALL+1),
    							"Action {0}: Call failed. Expected result is: FAILURE with reason {1}. Real result was SUCCESS",
    							_id, xreason );
    			throw new RuntimeException("Result of the action is not as expected.");
    		}
    
    		if (xreason != null && !xreason.equalsIgnoreCase(result.getErrorCode())) {
    			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,	"ISMTEST"+(Constants.APICALL+2),
    							"Action {0}: Call failed. Expected result is: FAILURE with reason {1}. Real result was FAILURE with reason {2} {3}.",
    							_id, xreason, result.getErrorCode(), result.getMessage());
    			throw new RuntimeException("Result of the action is not as expected.", result);
    		}
		}
	}

	private void parseApiParameters(Element config) {
		TestUtils.XmlElementProcessor processor = new TestUtils.XmlElementProcessor() {
			public void process(Element element) {
				String paramName = element.getAttribute("name");
				String text = element.getTextContent();
				if (text != null) {
					_apiParameters.setProperty(paramName, text);
				} else {
					_trWriter
							.writeTraceLine(
									LogLevel.LOGLEV_WARN,
									"ISMTEST"+(Constants.APICALL+3),
									"Action {0}: Value for API parameter {1} is not set.",
									_id, paramName);
				}
			}
		};
		TestUtils.walkXmlElements(config, "ApiParameter", processor);
	}
	
	/*
	 * Get user properties from the api parameters.
	 * The user properties are in the form name.# wher e# is a set of contiguous
	 * integers starting with 0.
	 */
	protected List<MqttUserProp> getUserProps(String name) {
	    List<MqttUserProp> list = null;
	    for (int index = 0; ; index++) {
	        String item = name + '.' + index;
	        String entry = _apiParameters.getProperty(item);
	        if (entry == null)
	            break;
	        int pos = entry.indexOf('=');
	        if (pos <= 0) {             
	            _trWriter.writeTraceLine(LogLevel.LOGLEV_WARN, "ISMTEST"+(Constants.APICALL+3),
                        "Action {0}: Value for user property {1} is not correct.", _id, entry);
	            break;
	        }
	        if (list == null)
	            list  = (List<MqttUserProp>) new Vector<MqttUserProp>();
	        list.add(new MqttUserProp(entry.substring(0, pos), entry.substring(pos+1)));
	    }
	    return list;
	}
	
	protected List<String> getStringList(String name) {
		List<String> list = null;
		for (int index = 0; ; index++) {
			String item = name + '.' + index;
			String entry = _apiParameters.getProperty(item);
			if(entry == null)
				break;
			if(list == null)
				list = (List<String>) new Vector<String>();
			list.add(entry);
		}
		return list;
	}

	protected final boolean run() {
		boolean rc;
		try {
			try {
				rc = invokeApi();
				if (rc) {
					checkExpectedResult(null);
				}
			} catch (IsmTestException e) {
				checkExpectedResult(e);
				_trWriter.writeTraceLine(
						LogLevel.LOGLEV_INFO,
						"ISMTEST"+(Constants.APICALL+4),
						"Action {0}: API call failed as expected. The reason is: error_code={1} : {2}",
						_id, e.getErrorCode(), e.getMessage());
				rc = true;
			}
		} catch (Throwable e) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.APICALL+5),
					"Action {0} failed. The reason is: {1}", _id,
					e.getMessage());
			_trWriter.writeException(e);
			rc = false;
		}
		return rc;
	}

	protected abstract boolean invokeApi() throws IsmTestException;

}
