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

import java.io.IOException;
import java.io.Serializable;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.ObjectMessage;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class GetSetMessageObjectAction extends GetSetAction {
    private final JmsValueType _objValueType;
    
    protected GetSetMessageObjectAction(Element config,JmsActionType actionType, 
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, actionType, dataRepository, trWriter);
        _objValueType = JmsValueType.valueOf(_actionParams.getProperty("objValueType","Object"));
    }
    
    @Override
    protected Object get(Message msg, JmsValueType type) throws JMSException {
        
        if (msg instanceof ObjectMessage) {
            ObjectMessage oMsg = (ObjectMessage) msg;
            return oMsg.getObject();
        }
        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4004", 
                "Action {0}: The getObject API is not valid for message of type: ", _id, msg.getClass().getSimpleName());
        return null;
    }
    
    @Override
    protected boolean set(Message msg, JmsValueType type) throws JMSException {
        if (msg instanceof ObjectMessage) {
            ObjectMessage oMsg = (ObjectMessage) msg;
            try {
            
                switch(_objValueType){
                case Boolean:
                    oMsg.setObject(booleanValue());
                    break;
                case Byte:
                    oMsg.setObject(byteValue());
                    break;
                case ByteArray:{
                    byte[] ba = byteArrayValue();
                    oMsg.setObject(ba);
                    break;
                }
                case Short:
                    oMsg.setObject(shortValue());
                    break;
                case Float:
                    oMsg.setObject(floatValue());
                    break;
                case Integer:
                    oMsg.setObject(intValue());
                    break;
                case Long:
                    oMsg.setObject(longValue());
                    break;
                case Double:
                    oMsg.setObject(doubleValue());
                    break;
                case UTF8:
                case String:
                    oMsg.setObject(stringValue());
                    break;
                case Counter:
                    oMsg.setObject(counterValue());
                case Object:
                default:
                    oMsg.setObject((Serializable)objectValue());
                    break;
                }
            } catch (IOException e) {
                _trWriter.writeException(e);
                return false;
            }
            
            return true;
        }
        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4005", 
                "Action {0}: The getObject API is not valid for message of type: ", _id, msg.getClass().getSimpleName());
        return false;
    }

}
