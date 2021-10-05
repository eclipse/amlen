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

import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;

import org.w3c.dom.Element;

public class GetSetMessageMapAction extends GetSetAction {
    private final String _propertyName;
    
    protected GetSetMessageMapAction(Element config,JmsActionType actionType, 
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, actionType,dataRepository, trWriter);
        _propertyName = _apiParameters.getProperty("propertyName");
        if (_propertyName == null) {
            throw new RuntimeException("propertyName is not defined for "
                    + this.toString());
        }
    }
    
    @Override
    protected Object get(Message msg, JmsValueType type) throws JMSException {
        final MapMessage mMsg;
        if (msg instanceof MapMessage) {
            mMsg = (MapMessage) msg;
        }else{
            return null;
        }
            
        if(!mMsg.itemExists(_propertyName))
            return null;
        switch(type){
        case Boolean:
            return Boolean.valueOf(mMsg.getBoolean(_propertyName));
        case Byte:
            return Byte.valueOf(mMsg.getByte(_propertyName));
        case ByteArray:
            return mMsg.getBytes(_propertyName);
        case Short:
            return Short.valueOf(mMsg.getShort(_propertyName));
        case Float:
            return Float.valueOf(mMsg.getFloat(_propertyName));
        case Integer:
        case Counter:
            return Integer.valueOf(mMsg.getInt(_propertyName));
        case Long:
            return Long.valueOf(mMsg.getLong(_propertyName));
        case Double:
            return Double.valueOf(mMsg.getDouble(_propertyName));
        case String:
            return mMsg.getString(_propertyName);
        case Object:
        default:
            return mMsg.getObject(_propertyName);
        }
    }
    
    @Override
    protected boolean set(Message msg, JmsValueType type) throws JMSException {
        final MapMessage mMsg;
        if (msg instanceof MapMessage) {
            mMsg = (MapMessage) msg;
        }else{
            return false;
        }
        try {
            switch(type){
            case Boolean:
                mMsg.setBoolean(_propertyName,booleanValue());
                break;
            case Byte:
                mMsg.setByte(_propertyName,byteValue());
                break;
            case ByteArray:
                mMsg.setBytes(_propertyName,byteArrayValue());
                break;
            case Short:
                mMsg.setShort(_propertyName,shortValue());
                break;
            case Float:
                mMsg.setFloat(_propertyName,floatValue());
                break;
            case Integer:
                mMsg.setInt(_propertyName,intValue());
                break;
            case Counter:
                mMsg.setInt(_propertyName,counterValue());
                break;
            case Long:
                mMsg.setLong(_propertyName,longValue());
                break;
            case Double:
                mMsg.setDouble(_propertyName,doubleValue());
                break;
            case String:
                mMsg.setString(_propertyName,stringValue());
                break;
            case Object:
            default:
                mMsg.setObject(_propertyName,objectValue());
            }            
        } catch (IOException e) {
            _trWriter.writeException(e);
            return false;
        }
        return true;
    }

}
