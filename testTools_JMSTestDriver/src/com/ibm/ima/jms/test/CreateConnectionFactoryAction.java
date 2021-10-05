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
package com.ibm.ima.jms.test;

import javax.jms.ConnectionFactory;
import javax.jms.JMSException;

import org.w3c.dom.Element;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CreateConnectionFactoryAction extends ApiCallAction {
    private final String   _structureID;
    private String         _jndiName;
    private final String   _ldapPrefix;
    private final String   _type;
    private final TrWriter _trWriter;
    private final String   _logLevel;
    private final String   _logFile;
    private final String   _keyStore;
    private final String   _keyStorePasswd;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateConnectionFactoryAction(Element config,
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _trWriter = trWriter;
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }

        _ldapPrefix = getJndiNamePrefix();

        String jndiName = _actionParams.getProperty("jndiName");
        if (jndiName != null && jndiName.length()>0)
        {
            if (_ldapPrefix == null || _ldapPrefix.length()<1)
                _jndiName = jndiName;
            else
                _jndiName = _ldapPrefix+jndiName;
        }
        
        // Valid type values are topic, queue, or default
        // The type paramether is ignored if _jndiName is specified
        _type = _actionParams.getProperty("type","default");
        _logLevel = _actionParams.getProperty("loglevel");
        _logFile = _actionParams.getProperty("logfile");
        _keyStore = _actionParams.getProperty("keyStore");
        _keyStorePasswd = _actionParams.getProperty("keyStorePassword");
    }

    protected boolean invokeApi() throws JMSException {
        final ConnectionFactory factory;
        int logLevel = 0;
        
        if (_logLevel != null && _logFile != null) {
            logLevel = Integer.parseInt(_logLevel);
            if (logLevel < 1 || logLevel > 9) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD9006", 
                        "Action {0}: Trace Level must be between 1 and 9.", _id);
                return false;
            }
            System.setProperty("IMATraceLevel", _logLevel);
            System.setProperty("IMATraceFile", _logFile);
        }
        
        if (_keyStore != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9007",
                        "Action {0}: Setting keyStore location.", _id);
            System.setProperty("javax.net.ssl.keyStore", _keyStore);
            System.setProperty("javax.net.ssl.trustStore", _keyStore);
        }

        if (_keyStorePasswd != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9008",
                        "Action {0}: Setting keyStorePassword.", _id);
            System.setProperty("javax.net.ssl.keyStorePassword", _keyStorePasswd);
            System.setProperty("javax.net.ssl.trustStorePassword", _keyStorePasswd);
        }

        if (_jndiName != null) {
            try {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9006", "Beginning JNDI Lookup of connection factory");
                factory = (ConnectionFactory) jndiLookup(_jndiName, _trWriter);
            } catch (JmsTestException e) {
                _trWriter.writeException(e);
                return false;
            }
        } else {
            if(_type.equals("default")) 
            {
                //_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"RMTD9100","Action {0} calling ImaJmsFactory.createConnectionFactory().",this.toString());
                factory = ImaJmsFactory.createConnectionFactory();
            }
            else if(_type.equals("topic"))
            {
                //_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"RMTD9101","Action {0} calling ImaJmsFactory.createTopicConnectionFactory().",this.toString());
                factory = ImaJmsFactory.createTopicConnectionFactory();
            }
            else if(_type.equals("queue"))
            {
                //_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"RMTD9102","Action {0} calling ImaJmsFactory.createQueueConnectionFactory().",this.toString());
                factory = ImaJmsFactory.createQueueConnectionFactory();
            }
            else
                throw new RuntimeException("type "+_type + " is not valid.  Valid types are default, topic and queue.");
                
        }
        _dataRepository.storeVar(_structureID, factory);
        return true;
    }

}
