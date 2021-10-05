/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
 * System Name : MQ Low Latency Messaging
 * File        : ApiCallAction.java
 * Author      :  
 *-----------------------------------------------------------------
 */

package com.ibm.ima.jms.test;

import java.util.Properties;

import javax.jms.JMSException;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

/**
 * <p>
 * 
 */
public abstract class ApiCallAction extends Action {

    protected final int _expectedRC;
    protected final String _expectedFailureReason;
    protected final Properties _apiParameters;

    protected ApiCallAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _expectedRC = Integer.parseInt(getConfigAttribute(config, "rc", "0"));
        if (_expectedRC != 0) {
            _expectedFailureReason = config.getAttribute("reason");
        } else {
            _expectedFailureReason = null;
        }
        _apiParameters = new Properties();
        parseApiParameters(config);
    }

    protected void checkExpectedResult(JMSException result) {
        if (_expectedRC == 0) {
            if (result != null) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2011",
                        "Action {0}: Call to ISM API failed. Expected result is: SUCCESS. Real result was {1}",
                        _id, result);
                throw new RuntimeException("Result of the ISM API is not as expected.", result);
            }
            return;
        }
        if (result == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2012",
                    "Action {0}: Call to ISM API failed. Expected result is: FAILURE with reason {1}. Real result was SUCCESS",
                    _id, _expectedFailureReason);
            throw new RuntimeException("Result of the ISM API is not as expected.");
        }

        if ((_expectedFailureReason != null)
                && (!_expectedFailureReason.equalsIgnoreCase(result.getErrorCode()))) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2013",
                    "Action {0}: Call to ISM API failed. Expected result is: FAILURE with reason {1}. Real result was FAILURE with reason {2}.",
                    _id, _expectedFailureReason, result.getErrorCode());
            throw new RuntimeException("Result of the ISM API is not as expected.", result);
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
                    _trWriter.writeTraceLine(
                            LogLevel.LOGLEV_WARN,
                            "RMTD3011",
                            "Action {0}: Value for API parameter {1} is not set.",
                            _id, paramName);
                }
            }
        };
        TestUtils.walkXmlElements(config, "ApiParameter", processor);
    }

    protected final boolean run() {
        boolean rc;
        try {
            try {
                rc = invokeApi();
                if (rc) {
                    checkExpectedResult(null);
                }
            } catch (JMSException e) {
                checkExpectedResult(e);
                _trWriter.writeTraceLine(
                        LogLevel.LOGLEV_INFO,
                        "RMTD5200",
                        "Action {0}: API call failed as expected. The reason is: error_code={1} : {2}",
                        _id, e.getErrorCode(), e.getMessage());
                rc = true;
            }
        } catch (Throwable e) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2100",
                    "Action {0} failed. The reason is: {1}", _id,
                    e.getMessage());
            _trWriter.writeException(e);
            rc = false;
        }
        return rc;
    }

    protected abstract boolean invokeApi() throws JMSException;

}
