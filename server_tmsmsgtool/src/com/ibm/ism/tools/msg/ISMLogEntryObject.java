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
package com.ibm.ism.tools.msg;

import java.util.ArrayList;
import java.util.Collection;

public class ISMLogEntryObject implements Comparable<ISMLogEntryObject> {

	// public static enum LOG_ENTRY_COMPONENT_TYPE{RMM,RUM, DCS, LLMD, SHARED,
	// UTIL, LLMC, LLMQ, LLMJ, ISM_TRANSPORT};
	public static enum LOG_ENTRY_COMPONENT_TYPE {
		TRANSPORT, UTILS, PROTOCOL, ENGINE, STORE, ADMIN, GUI, MONITORING, MQCONNECTIVITY, PROXY, NONE
	};

	public static enum LOG_POINT_TYPE {
		SIMPLE, COMPOSITE, ERROR_CODE
	};

	private String handleName;
	private String _fileName;
	private int _lineNum;
	private String logLevel;
	private String category;
	private String msgKey;
	private int msgNumber;
	private String flag;
	private String instanceName;
	private String component;

	private String errorCodeName;
	private int 	errorCode;

	public int getErrorCode() {
		return errorCode;
	}

	public void setErrorCode(int errorCode) {
		this.errorCode = errorCode;
	}

	public String getErrorCodeName() {
		return errorCodeName;
	}

	public void setErrorCodeName(String errorCodeName) {
		this.errorCodeName = errorCodeName;
	}

	public String getComponent() {
		return component;
	}

	public void setComponent(String component) {
		this.component = component;
	}

	private boolean isValid = true;

	private boolean ignoreDup = false;

	LOG_ENTRY_COMPONENT_TYPE _type;
	LOG_ENTRY_COMPONENT_TYPE _subtype = null;
	LOG_POINT_TYPE logpointtype;
	ArrayList<ISMLogEntryObject> usedByOtherList = new ArrayList<ISMLogEntryObject>();
	ArrayList<ISMLogMsgObject> logMsgObjects;

	public ISMLogEntryObject(String fileName, int lineNum) {
		_fileName = fileName;
		_lineNum = lineNum;
		_type = LOG_ENTRY_COMPONENT_TYPE.TRANSPORT;
		logMsgObjects = new ArrayList<ISMLogMsgObject>();
	}

	public ISMLogEntryObject(String fileName, int lineNum,
			LOG_ENTRY_COMPONENT_TYPE componenttype, LOG_POINT_TYPE logPointType) {
		_fileName = fileName;
		_lineNum = lineNum;
		_type = componenttype;
		logpointtype = logPointType;
		logMsgObjects = new ArrayList<ISMLogMsgObject>();
	}

	public ISMLogEntryObject(String fileName, int lineNum,
			LOG_ENTRY_COMPONENT_TYPE componenttype,
			LOG_ENTRY_COMPONENT_TYPE componentsubtype,
			LOG_POINT_TYPE logPointType) {
		_fileName = fileName;
		_lineNum = lineNum;
		_type = componenttype;
		_subtype = componentsubtype;
		logpointtype = logPointType;
		logMsgObjects = new ArrayList<ISMLogMsgObject>();
	}

	public ISMLogEntryObject(String imsgKey, LOG_ENTRY_COMPONENT_TYPE compType,
			LOG_POINT_TYPE logptype) {
		logMsgObjects = new ArrayList<ISMLogMsgObject>();
		_type = compType;
		logpointtype = logptype;
		setMsgKey(imsgKey);
	}

	public ISMLogEntryObject(String imsgKey, LOG_ENTRY_COMPONENT_TYPE compType,
			LOG_ENTRY_COMPONENT_TYPE subcomptype, LOG_POINT_TYPE logptype) {
		logMsgObjects = new ArrayList<ISMLogMsgObject>();
		_type = compType;
		_subtype = subcomptype;
		logpointtype = logptype;
		setMsgKey(imsgKey);
	}

	/**
	 * Get the log point type. It is either COMPOSITE or SIMPLE.
	 *
	 * @return LOG_POINT_TYPE type
	 */
	public LOG_POINT_TYPE getLogPointType() {
		return logpointtype;
	}

	/**
	 * Get the handle name
	 *
	 * @return handle name string
	 */
	public String getHandleName() {
		return handleName;
	}

	/**
	 * Set the log handle name
	 *
	 * @param handleName
	 */
	public void setHandleName(String handleName) {
		this.handleName = handleName;
	}

	/**
	 * Get the Log Level
	 *
	 * @return log level string
	 */
	public String getLogLevel() {
		return logLevel;
	}

	/**
	 * Set Log level
	 *
	 * @param logLevel
	 *            String
	 */
	public void setLogLevel(String logLevel) {
		this.logLevel = logLevel;
	}

	/**
	 * Get message key
	 *
	 * @return
	 */
	public String getMsgKey() {
		return msgKey;
	}

	/**
	 * Set Message Key
	 *
	 * @param msgKey
	 *            integer
	 */
	public void setMsgKey(int msgKey) {

		setMsgKey(String.format("%04d",msgKey));


	}

	/**
	 * Set Message Key string
	 *
	 * @param msgID
	 *            string
	 */
	public void setMsgKey(String msgID) {
		if (!isValid) {
			msgNumber = -1;
			msgKey = "";
			return;
		}
		this.msgKey = msgID.trim();

		try {
			// msgNumberS = msgID.substring(msgID
			// .indexOf(Constants.ISM_MSG_KEY_PREFIX)
			// + Constants.ISM_MSG_KEY_PREFIX.length());
			msgNumber = Integer.parseInt(msgID);
			if(logpointtype == LOG_POINT_TYPE.ERROR_CODE){
				errorCode=msgNumber;
			}

		} catch (Exception exception) {
			System.out.println("ERROR: Invalid MsgKey: " + msgID);
			exception.printStackTrace();
		}
	}

	/**
	 * Add log message object into the list for this message key
	 *
	 * @param llmLogMsgObj
	 */
	public void addLogMsg(ISMLogMsgObject llmLogMsgObj) {
		if(logMsgObjects.contains(llmLogMsgObj)==false){
			logMsgObjects.add(llmLogMsgObj);
		}
	}

	/**
	 * Get log message objects
	 *
	 * @return the list of LogMsgObject
	 */
	public Collection<ISMLogMsgObject> getLogMsgObjects() {
		return logMsgObjects;
	}

	/**
	 * Get Component Type
	 *
	 * @return LOG_ENTRY_COMPONENT type
	 */
	public LOG_ENTRY_COMPONENT_TYPE getComponentType() {
		return _type;
	}

	/**
	 * Get all the message text for this message key
	 *
	 * @return messages string
	 */
	public String getAllMsgText() {
		StringBuffer stringBuf = new StringBuffer();

		for (ISMLogMsgObject msgObj : logMsgObjects) {
			stringBuf.append(msgObj.getMsgText());
			stringBuf.append(System.getProperty("line.separator"));
		}
		//return stringBuf.toString().trim();
		return stringBuf.toString();
	}

	/**
	 * Get the file name that contains this log point
	 *
	 * @return
	 */
	public String getFileName() {
		return _fileName;
	}

	/**
	 * Set file name for this log point
	 *
	 * @param name
	 */
	public void setFileName(String name) {
		_fileName = name;
	}

	/**
	 * Get line number
	 *
	 * @return the line number (integer)
	 */
	public int getLineNum() {
		return _lineNum;
	}

	/**
	 * Set the line number
	 *
	 * @param num
	 *            the line number
	 */
	public void setLineNum(int num) {
		_lineNum = num;
	}

	/**
	 * Get the log message category
	 *
	 * @return
	 */
	public String getCategory() {
		if (category != null)
			return category;
		String retCat = null;
		if (_subtype == LOG_ENTRY_COMPONENT_TYPE.TRANSPORT) {
			retCat = LOG_ENTRY_COMPONENT_TYPE.TRANSPORT.name();
		}
		return retCat;
	}

	/**
	 * Set message categroy
	 *
	 * @param icat
	 */
	public void setCategory(String icat) {
		if (icat != null)
			icat = icat.trim();
		category = icat;

	}

	/**
	 * Set message flag
	 *
	 * @param iflag
	 */
	public void setFlag(String iflag) {
		flag = iflag;
	}

	/**
	 * Get message flag
	 *
	 * @return
	 */
	public String getFlag() {
		return flag;
	}

	/**
	 * Get instance name
	 *
	 * @return instance name string
	 */
	public String getInstanceName() {
		return instanceName;
	}

	/**
	 * Set Instance name
	 *
	 * @param instanceName
	 *            string
	 */
	public void setInstanceName(String instanceName) {
		this.instanceName = instanceName;
	}

	public int getMessageSuffix() {
		int retmsgnum = -1;

		retmsgnum = msgNumber;

		return retmsgnum;
	}

	public boolean isIgnoreDup() {
		return ignoreDup;
	}

	public void setIgnoreDup(boolean ignoreDup) {
		this.ignoreDup = ignoreDup;
	}

	public LOG_ENTRY_COMPONENT_TYPE getSubComponentType() {
		return _subtype;
	}

	public void setSubComponentType(LOG_ENTRY_COMPONENT_TYPE _subtype) {
		this._subtype = _subtype;
	}

	/**
	 * Validate the message strings
	 *
	 * @param msg
	 * @return
	 */
	public boolean validateMsg(String msg) {
		if (getAllMsgText() == null || msg == null) {
			return false;
		}
		if (getAllMsgText().compareTo(msg) == 0) {
			return true;
		}
		return false;

	}

	/**
	 * Return the string with neccessary parameters
	 */
	public String toString() {
		StringBuffer stringBuf = new StringBuffer();

		stringBuf.append("File: " + _fileName + "|");
		stringBuf.append("LineNumber: " + _lineNum + "|");
		stringBuf.append("ComponentType: " + _type.toString() + "|");
		stringBuf.append("Category: " + category + "|");
		stringBuf.append("LogLevel: " + logLevel + "|");
		stringBuf.append("MsgKey: " + msgKey + "|");
		stringBuf.append("MsgText: " + getAllMsgText() + "");

		return stringBuf.toString();
	}

	public boolean equals(Object object) {
		if (object instanceof ISMLogEntryObject) {
			ISMLogEntryObject inputObj = (ISMLogEntryObject) object;
			// if(this.getMsgKey().compareTo(inputObj.getMsgKey())==0){
			// return true;
			// }

			if ((this.getSubComponentType() == inputObj.getSubComponentType())) {
				if (this.getMessageSuffix() == inputObj.getMessageSuffix()) {
					return true;
				}
			}
		}
		return false;

	}

	public int hashCode() {
		String key = this.getMsgKey();
		if (key == null) {
			System.out.println("Unknown key=" + this);
			key = "";
		}
		return key.hashCode();
	}

	public int compareTo(ISMLogEntryObject o) {
		String key = this.getMsgKey();
		if (key == null) {
			System.out.println("Unknown key=" + this);
			key = "";
		}
		return key.compareTo(o.getMsgKey());
	}

	public char getSeverity() {
		char retch = 'I';
		if (logLevel.indexOf("WARN") >= 0) {
			retch = 'W';
		} else if (logLevel.indexOf("ERROR") >= 0) {
			retch = 'E';
		} else if (logLevel.indexOf("INFO") >= 0) {
			retch = 'I';
		} else if (logLevel.indexOf("STATUS") >= 0) {
			retch = 'S';
		} else if (logLevel.indexOf("FATAL") >= 0) {
			retch = 'F';
		} else if (logLevel.indexOf("EVENT") >= 0) {
			retch = 'V';
		} else if (logLevel.indexOf("DEBUG") >= 0) {
			retch = 'D';
		} else if (logLevel.indexOf("TRACE") >= 0) {
			retch = 'T';
		} else if (logLevel.indexOf("XTRACE") >= 0) {
			retch = 'X';
		} else if (logLevel.equals("")) {
			retch = '\0';
		}

		return retch;
	}

	public String getTMSMessageKey(boolean isPGMKey) {
		return getTMSMessageKey(isPGMKey, getSeverity());
	}

	public String getTMSMessageKey(boolean isPGMKey, char sev) {
		StringBuffer sb = new StringBuffer();
		sb.append(Constants.ISM_MSG_KEY_PREFIX + String.format("%04d",getMessageSuffix()));
		return sb.toString();
	}

	public String getTMSMessage() {
		StringBuffer sb = new StringBuffer();

		sb.append(getAllMsgText());

		return sb.toString();
	}

	public boolean isNeedInTMS() {
		boolean retVal = false;

		if (!isValid || !isInRange()) {
			return false;
		}
		if(this.logpointtype!=LOG_POINT_TYPE.ERROR_CODE){
			switch (getSeverity()) {
			case 'E':
			case 'W':
			case 'I':
				// case 'S':
			case 'F':
			case '\0':
				retVal = true;
				break;
			default:
				retVal = false;
			}
		}else{
			retVal=true;

		}

		return retVal;
	}

	public boolean isValid() {
		if (isValid) {
			return isInRange();
		}
		return isValid;
	}

	public void setValid(boolean isValid) {
		this.isValid = isValid;
	}

	public boolean isInRange() {
		boolean retVal = false;

		int min = 0;
		int max = 0;

		if (_subtype == LOG_ENTRY_COMPONENT_TYPE.TRANSPORT) {
			min = Constants.TRANSPORT_MSG_KEY_MIN;
			max = Constants.TRANSPORT_MSG_KEY_MAX;

		} else if (_subtype == LOG_ENTRY_COMPONENT_TYPE.UTILS) {
			min = Constants.UTILS_MSG_KEY_MIN;
			max = Constants.UTILS_MSG_KEY_MAX;

		} else if (_subtype == LOG_ENTRY_COMPONENT_TYPE.PROTOCOL) {
			min = Constants.PROTOCOL_MSG_KEY_MIN;
			max = Constants.PROTOCOL_MSG_KEY_MAX;

		} else if (_subtype == LOG_ENTRY_COMPONENT_TYPE.ENGINE) {
			min = Constants.ENGINE_MSG_KEY_MIN;
			max = Constants.ENGINE_MSG_KEY_MAX;

		} else if (_subtype == LOG_ENTRY_COMPONENT_TYPE.ADMIN) {
			min = Constants.ADMIN_MSG_KEY_MIN;
			max = Constants.ADMIN_MSG_KEY_MAX;

		} else if (_subtype == LOG_ENTRY_COMPONENT_TYPE.STORE) {
			min = Constants.STORE_MSG_KEY_MIN;
			max = Constants.STORE_MSG_KEY_MAX;

		}else if (_subtype == LOG_ENTRY_COMPONENT_TYPE.MONITORING) {
			min = Constants.MONITORING_MSG_KEY_MIN;
			max = Constants.MONITORING_MSG_KEY_MAX;

		}else if (_subtype == LOG_ENTRY_COMPONENT_TYPE.MQCONNECTIVITY) {
			min = Constants.MQCONNECTIVITY_MSG_KEY_MIN;
			max = Constants.MQCONNECTIVITY_MSG_KEY_MAX;

		}else if (_subtype == LOG_ENTRY_COMPONENT_TYPE.PROXY) {
			min = Constants.PROXY_MSG_KEY_MIN;
			max = Constants.PROXY_MSG_KEY_MAX;

		}

		if (min != 0 && max != 0) {
			if (msgNumber < min || msgNumber > max) {
				retVal = false;
			} else {
				retVal = true;
			}
		} else {
			retVal = false;
		}

		return retVal;
	}

	/**
	 * Get the LOG_ENTRY_COMPONENT_TYPE object based on the component type
	 * string
	 *
	 * @param compTypeStr
	 * @return LOG_ENTRY_COMPONENT_TYPE. Return null if the type can't be found.
	 */
	public static LOG_ENTRY_COMPONENT_TYPE getComponentType(String compTypeStr) {
		LOG_ENTRY_COMPONENT_TYPE type = null;
		if (compTypeStr == null)
			return type;
		if (compTypeStr.equals("TRANSPORT")) {
			type = LOG_ENTRY_COMPONENT_TYPE.TRANSPORT;
		}

		return type;

	}

}
