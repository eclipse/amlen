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

package com.ibm.ima.resources.data;

import java.util.Arrays;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;



/**
 * The REST representation of a the LogLevel
 *
 */
public class LogLevel extends  AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	
    @SuppressWarnings("unused")
    private final static String CLAS = LogLevel.class.getCanonicalName();

    private final static String MQCONNECTIVITY_LOG_TYPE = "MQConnectivityLog";

    public enum ValidLogLevel {
    	MIN,NORMAL,MAX;
    	
        /**
         * @return the default log level as a string
         */
        public static String getDefaultLogLevel() {
            return NORMAL.name();
        }
    }

    @JsonProperty("LogLevel")
    private String LogLevel = ValidLogLevel.getDefaultLogLevel();
    
    private String LogType;

    /**
     * @return the logLevel
     */
    @JsonProperty("LogLevel")
    public String getLogLevel() {
        return LogLevel;
    }
    
    /**
     * @return the logType
     */
    @JsonIgnore
    public String getLogType() {
        return LogType;
    }

    /**
     * @param logLevel the logLevel to set
     */
    @JsonProperty("LogLevel")
    public void setLogLevel(String logLevel) {
        LogLevel = logLevel;
    }
    
    /**
     * @param logType the logType to set
     */
    @JsonIgnore
    public void setLogType(String logType) {
    	LogType = logType;
    }
    
    @Override
	public String toString() {
		return "LogLevel [LogLevel=" + LogLevel + ", LogType=" + LogType + "]";
	}

    @Override
    @JsonIgnore
    public ValidationResult validate() {
        ValidationResult vr = new ValidationResult();
        if (LogLevel == null || trimUtil(LogLevel).length() == 0) {
            vr.setResult(VALIDATION_RESULT.VALUE_EMPTY);
            return vr;
        }
        try {
            ValidLogLevel.valueOf(LogLevel);
        } catch (Exception e) {
            vr.setResult(VALIDATION_RESULT.INVALID_ENUM);
            vr.setParams(new Object[] {LogLevel, Arrays.toString(ValidLogLevel.values())});
        }

        return vr;
    }

    @JsonIgnore
    public boolean isMQConnectivityLogType() {
        return MQCONNECTIVITY_LOG_TYPE.equals(this.LogType);
    }

}
