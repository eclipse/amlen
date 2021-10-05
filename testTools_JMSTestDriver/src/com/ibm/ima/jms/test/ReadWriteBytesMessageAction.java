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
import java.io.UnsupportedEncodingException;

import javax.jms.BytesMessage;
import javax.jms.JMSException;
import javax.jms.Message;

import org.w3c.dom.Element;

public class ReadWriteBytesMessageAction extends GetSetAction {
    
    protected ReadWriteBytesMessageAction(Element config,JmsActionType actionType, 
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, actionType,dataRepository, trWriter);
    }
    
    @Override
    protected Object get(Message msg, JmsValueType type) throws JMSException {
        final BytesMessage bMsg;
        if (msg instanceof BytesMessage) {
            bMsg = (BytesMessage) msg;
        }else{
            return null;
        }
            
        switch(type){
        case Boolean:
            return Boolean.valueOf(bMsg.readBoolean());
        case Byte:
            return Byte.valueOf(bMsg.readByte());
        case ByteArray:{
            int length = bMsg.readInt();
            byte[] ba = new byte[length];
            if(bMsg.readBytes(ba, length) != length)
                return null;
            return ba;
        }
        case Short:
            return Short.valueOf(bMsg.readShort());
        case Float:
            return Float.valueOf(bMsg.readFloat());
        case Integer:
        case Counter:
            return Integer.valueOf(bMsg.readInt());
        case Long:
            return Long.valueOf(bMsg.readLong());
        case Double:
            return Double.valueOf(bMsg.readDouble());
        case UTF8:
            return bMsg.readUTF();
        case String:
            byte[] bytes = new byte[(int)bMsg.getBodyLength()];
            bMsg.readBytes(bytes);
            String string = null;
            try {
                string = new String(bytes, "UTF-8");
            } catch (UnsupportedEncodingException e) {    // Should not get here
                JMSException jmse = new JMSException("What do you mean, UTF-8 is an unsupported encoding?");
                jmse.setLinkedException(e);
                ((Exception)jmse).initCause(e);
                throw jmse;
            }
            return string;//bMsg.readUTF();
        case Object:
        default:
            throw new RuntimeException("readObject is not supported for BytesMessage");
        }
    }
    
    @Override
    protected boolean set(Message msg, JmsValueType type) throws JMSException {
        final BytesMessage bMsg;
        if (msg instanceof BytesMessage) {
            bMsg = (BytesMessage) msg;
        }else{
            return false;
        }
        try{
            switch(type){
            case Boolean:
                bMsg.writeBoolean(booleanValue());
                break;
            case Byte:
                bMsg.writeByte(byteValue());
                break;
            case ByteArray:{
                byte[] ba = byteArrayValue();
                bMsg.writeInt(ba.length);
                bMsg.writeBytes(ba);
                break;
            }
            case Short:
                bMsg.writeShort(shortValue());
                break;
            case Float:
                bMsg.writeFloat(floatValue());
                break;
            case Integer:
                bMsg.writeInt(intValue());
                break;
            case Long:
                bMsg.writeLong(longValue());
                break;
            case Double:
                bMsg.writeDouble(doubleValue());
                break;
            case UTF8:
                bMsg.writeUTF(stringValue());
                break;
            case String:
                byte[] bytes = null;
                try {
                    bytes = stringValue().getBytes("UTF-8");
                } catch (UnsupportedEncodingException e) {    // Should not get here
                    JMSException jmse = new JMSException("What do you mean, UTF-8 is an unsupported encoding?");
                    jmse.setLinkedException(e);
                    ((Exception)jmse).initCause(e);
                    throw jmse;
                }
                bMsg.writeBytes(bytes);
            case Counter:
                bMsg.writeInt(counterValue());
            case Object:
            default:
                bMsg.writeObject(objectValue());
                break;
            }
        }catch (IOException e) {
            _trWriter.writeException(e);
            return false;
        }
        return true;
    }

}
