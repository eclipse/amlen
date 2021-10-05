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

import javax.jms.Destination;
import javax.jms.JMSException;


import org.w3c.dom.Element;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CreateDestinationAction extends ApiCallAction {
    private final String   _structureID;
    private String         _jndiName;
    private final String   _ldapPrefix;
    private final String   _type;
    private final TrWriter _trWriter;
    private final String   _name;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateDestinationAction(Element config,
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
        
        // Valid type values are topic or queue
        // The type parameter is ignored if _jndiName is specified
        _type = _actionParams.getProperty("type");
        _name = _apiParameters.getProperty("name",null);

        //TODO: We need to throw an exception if _jndiName, and _name
        // are not set.  In some form, the Name property must be
        // set when a destination is created.
    }

    protected boolean invokeApi() throws JMSException {
        final Destination destination;
        if (_jndiName != null) {
            try {
                destination = (Destination) jndiLookup(_jndiName, _trWriter);
            } catch (JmsTestException e) {
                _trWriter.writeException(e);
                return false;
            }
        } else {
            String name = null;
            try {
                name = EnvVars.replace(_name);
            } catch (Exception e) {
                throw new JMSException(e.getMessage(), e.getMessage());
            }
            if(_type.equals("topic"))
            {
                if(name != null)
                {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"RMTD9212","Action {0} calling ImaJmsFactory.createTopic(String "+name+").",this.toString());
                    destination = ImaJmsFactory.createTopic(name);
                }
                else
                {
                    //TODO: Log an error message here.  Empty call not supported.
                    //_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"RMTD9210","Action {0} calling ImaJmsFactory.createTopic().",this.toString());
                    //destination = ImaJmsFactory.createTopic();
                    return false;
                }
            }
            else if(_type.equals("queue"))
            {
                if(name != null)
                {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"RMTD9222","Action {0} calling ImaJmsFactory.createQueue(String "+name+").",this.toString());
                    destination = ImaJmsFactory.createQueue(name);
                }
                else
                {
                    //TODO: Log an error message here.  Empty call not supported.
                    //_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"RMTD9220","Action {0} calling ImaJmsFactory.createQueue().",this.toString());
                    //destination = ImaJmsFactory.createQueue();
                    return false;
                }
            }
            else
                throw new RuntimeException("type "+_type + " is not valid.  Valid types are topic and queue.");
        }
        _dataRepository.storeVar(_structureID, destination);
        return true;
    }

}
