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

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class GetSetMessagePropertyAction extends GetSetAction {
    private final JmsPropertyType _propertyType;
    private final String          _propertyName;
    
    protected GetSetMessagePropertyAction(Element config,JmsActionType actionType, 
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, actionType,dataRepository, trWriter);
        _propertyType = JmsPropertyType.valueOf(_apiParameters.getProperty("propertyType","Common"));
        if(_propertyType == JmsPropertyType.Common ){
            
            _propertyName = _apiParameters.getProperty("propertyName");
            if (_propertyName == null) {
                throw new RuntimeException("propertyName is not defined for "
                        + this.toString());
            }
        }else{
            _propertyName = null;
        }
    }
    
    @Override
    protected Object get(Message msg, JmsValueType type) throws JMSException {
        switch(_propertyType){
        case CorrelationID:
            return msg.getJMSCorrelationID();
        case DeliveryMode:
            return new Integer(msg.getJMSDeliveryMode());
        case Destination:
            return msg.getJMSDestination();
        case Expiration:
            return new Long(msg.getJMSExpiration());
        case MessageID:
            return msg.getJMSMessageID();
        case Priority:
            return new Integer(msg.getJMSPriority());
        case Redelivered:
            return new Boolean(msg.getJMSRedelivered());
        case ReplyTo:
            return msg.getJMSReplyTo();
        case Timestamp:
            return new Long(msg.getJMSTimestamp());
        case MessageType:
            return msg.getJMSType();
        case Common:
        default:
            return getMessageProperty(msg,type);
        }
    }
    
    private Object getMessageProperty(Message msg, JmsValueType type) throws JMSException {
        if(!msg.propertyExists(_propertyName))
            return null;
        switch(type){
        case Boolean:
            return Boolean.valueOf(msg.getBooleanProperty(_propertyName));
        case Byte:
            return Byte.valueOf(msg.getByteProperty(_propertyName));
        case Short:
            return Short.valueOf(msg.getShortProperty(_propertyName));
        case Float:
            return Float.valueOf(msg.getFloatProperty(_propertyName));
        case Integer:
        case Counter:
            return Integer.valueOf(msg.getIntProperty(_propertyName));
        case Long:
            return Long.valueOf(msg.getLongProperty(_propertyName));
        case Double:
            return Double.valueOf(msg.getDoubleProperty(_propertyName));
        case String:
            return msg.getStringProperty(_propertyName);
        case Object:
        default:
            return msg.getObjectProperty(_propertyName);
        }
    }
    
    @Override
    protected boolean set(Message msg, JmsValueType type) throws JMSException {
        try {
            switch(_propertyType){
            case CorrelationID:
                msg.setJMSCorrelationID(stringValue());
                break;
            case DeliveryMode:
                msg.setJMSDeliveryMode(intValue());
                break;
            case Destination:{
                    Destination dest = (Destination) objectValue();
                    msg.setJMSDestination(dest);
                }
                break;
            case Expiration:
                msg.setJMSExpiration(longValue());
                break;
            case MessageID:
                msg.setJMSMessageID(stringValue());
                break;
            case Priority:
                msg.setJMSPriority(intValue());
                break;
            case Redelivered:
                msg.setJMSRedelivered(booleanValue());
                break;
            case ReplyTo:{
                    Destination dest = (Destination) objectValue();
                    msg.setJMSReplyTo(dest);
                }
                break;
            case Timestamp:
                msg.setJMSTimestamp(longValue());
                break;
            case MessageType:
                msg.setJMSType(stringValue());
                break;
            case Common:
            default:
                return setMessageProperty(msg,type,stringValue());
            }            
        } catch (IOException e) {
            _trWriter.writeException(e);
            return false;
        }
        return true;
    }
    
    private boolean setMessageProperty(Message msg, JmsValueType type,
            String value) throws JMSException {
        switch(type){
        case Boolean:
            msg.setBooleanProperty(_propertyName,Boolean.parseBoolean(value));
            break;
        case Byte:
            msg.setByteProperty(_propertyName,Byte.parseByte(value));
            break;
        case Short:
            msg.setShortProperty(_propertyName,Short.parseShort(value));
            break;
        case Float:
            msg.setFloatProperty(_propertyName,Float.parseFloat(value));
            break;
        case Integer:
            msg.setIntProperty(_propertyName,Integer.parseInt(value));
            break;
        case Long:
            msg.setLongProperty(_propertyName,Long.parseLong(value));
            break;
        case Double:
            msg.setDoubleProperty(_propertyName,Double.parseDouble(value));
            break;
        case String:
            msg.setStringProperty(_propertyName,value);
            break;
        case Counter:
            msg.setIntProperty(_propertyName, getCounterValue(value));
            break;
        case Object:
        default:
            Object obj = _dataRepository.getVar(value);
            if(obj == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Failed to locate the object ({0}).",    value);
                return false;
            }
            msg.setObjectProperty(_propertyName,obj);
            break;
        }
        return true;
    }

}
