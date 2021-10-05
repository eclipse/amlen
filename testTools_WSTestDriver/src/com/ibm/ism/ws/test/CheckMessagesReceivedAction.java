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

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class CheckMessagesReceivedAction extends ApiCallAction {
	private final 	String 					_structureID;
	private final	Integer					_messages;
	private final	String					_comparator;
	private final	Map<String, Integer>	_messagesPerTopic;


	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CheckMessagesReceivedAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("connection_id");
		if (_structureID == null) {
			throw new RuntimeException("connection_id is not defined for "
					+ this.toString());
		}
		String temp = _actionParams.getProperty("messageCount");
		if (null == temp) {
			_messages = null;
		} else {
			_messages = new Integer(temp);
		}
		if (null != _messages) {
			_comparator = _actionParams.getProperty("compare", "==");
		} else {
			_comparator = null;
		}
		temp = _actionParams.getProperty("messageCounts");
		if (null == temp) {
			_messagesPerTopic = null;
		} else {
			_messagesPerTopic = new HashMap<String, Integer>();
			String[] substrings = temp.split(":");
			for (int i = 0; i<substrings.length; i++) {
				String[] pieces = substrings[i].split("=");
				if (2 != pieces.length) {
					throw new RuntimeException(" Invalid substring in 'messageCounts', value is '"+substrings[i]+"' shoud be 'path=count'");
				} else {
					_messagesPerTopic.put(pieces[0], new Integer(pieces[1]));
				}
			}
		}
		if (null == _messages && (null == _messagesPerTopic || 0 == _messagesPerTopic.size())) {
			throw new RuntimeException("compareTo is not defined for "
					+ this.toString());
		}
	}

	protected boolean invokeApi() throws IsmTestException {
		MyConnection con = (MyConnection)_dataRepository.getVar(_structureID);
		if (null == con) {
			throw new IsmTestException("ISMTEST"+Constants.CHECKMESSAGESRECEIVED,
					"Connection "+_structureID+" is not found");
		}
		try {
		if (null != _messages) {
			int value = con.getMessagesReceived();
			if ("==".equals(_comparator)) {
				if (_messages != value) {
					throw new IsmTestException(("ISMTEST"+Constants.CHECKMESSAGESRECEIVED+1)
							, "Expected "+_messages+" messages, received "+value);
				}
			} else if ("!=".equals(_comparator)) {
				if (_messages == value) {
					throw new IsmTestException(("ISMTEST"+Constants.CHECKMESSAGESRECEIVED+2)
							, "Expected other than "+_messages+" messages, received "+value);
				}
			} else if (">".equals(_comparator)) {
				if (_messages <= value) {
					throw new IsmTestException(("ISMTEST"+Constants.CHECKMESSAGESRECEIVED+3)
							, "Expected more than "+_messages+" messages, received "+value);
				}
			} else if ("<".equals(_comparator)) {
				if (_messages >= value) {
					throw new IsmTestException(("ISMTEST"+Constants.CHECKMESSAGESRECEIVED+4)
							, "Expected less than "+_messages+" messages, received "+value);
				}
			} else if (">=".equals(_comparator)) {
				if (_messages < value) {
					throw new IsmTestException(("ISMTEST"+Constants.CHECKMESSAGESRECEIVED+5)
							, "Expected at least "+_messages+" messages, received "+value);
				}
			} else if ("<=".equals(_comparator)) {
				if (_messages > value) {
					throw new IsmTestException(("ISMTEST"+Constants.CHECKMESSAGESRECEIVED+6)
							, "Expected at most "+_messages+" messages, received "+value);
				}
			} else {
				throw new IsmTestException(("ISMTEST"+Constants.CHECKMESSAGESRECEIVED+7)
						, "Unknown comparator '"+_comparator+"'");
			}
		}
		} catch (IsmTestException e) {
			Map <String, Integer> mpt = con.getMessagesPerTopic();
			Iterator<String> iter = mpt.keySet().iterator();
			while (iter.hasNext()) {
				String key = iter.next();
				this._trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,
						"ISMTEST"+Constants.CHECKMESSAGESRECEIVED+9,
						"Topic '"+key+"' received "+con.getMessagesReceived(key)+" messages.");
			}
			throw e;
		}
		if (_trWriter.willLog(LogLevel.LOGLEV_INFO)) {
			Map <String, Integer> mpt = con.getMessagesPerTopic();
			Iterator<String> iter = mpt.keySet().iterator();
			while (iter.hasNext()) {
				String key = iter.next();
				this._trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,
						"ISMTEST"+Constants.CHECKMESSAGESRECEIVED+9,
						"Topic '"+key+"' received "+con.getMessagesReceived(key)+" messages.");
			}
		}
		if (null != _messagesPerTopic) {
			Set<String> keys = _messagesPerTopic.keySet();
			Iterator<String> iter = keys.iterator();
			while (iter.hasNext()) {
				String key = iter.next();
				Integer value = _messagesPerTopic.get(key);
				if (value != con.getMessagesReceived(key)) {
					throw new IsmTestException(("ISMTEST"+Constants.CHECKMESSAGESRECEIVED+8)
							, "Expected "+value+" messages for topic "+key+", received "+con.getMessagesReceived(key));
				}
			}
		}
		
		return true;
	}

}
