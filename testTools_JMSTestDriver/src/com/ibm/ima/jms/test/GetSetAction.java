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
import java.util.Arrays;
import java.util.Random;

import javax.jms.JMSException;
import javax.jms.Message;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

abstract class GetSetAction extends JmsMessageAction implements JmsMessageUtils{
    private static final Random RANDOM = new Random(); 
    private final JmsActionType _actionType;
    private final String        _value;
    private final JmsValueType  _valueType;
    private final String        _verifyValue;
    private final String        _inputStream; 
    private final String        _outputStream; 
    private boolean             _verifyNull = false;
    
    protected GetSetAction(Element config, JmsActionType actionType, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _actionType = actionType;
        _value = _apiParameters.getProperty("value");
        _valueType = JmsValueType.valueOf(_apiParameters.getProperty("valueType","String"));
        _verifyValue = _actionParams.getProperty("verifyValue");
        if((_verifyValue != null) && _verifyValue.equals(""))
            _verifyNull = true;
        _inputStream = _actionParams.getProperty("inputStream");
        _outputStream = _actionParams.getProperty("outputStream");
    }
    
    protected GetSetAction(Element config, JmsActionType actionType, JmsValueType valueType ,
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _actionType = actionType;
        _value = _apiParameters.getProperty("value");
        _valueType = valueType;
        _verifyValue = _actionParams.getProperty("verifyValue");
        if((_verifyValue != null) && _verifyValue.equals(""))
            _verifyNull = true;
        _inputStream = _actionParams.getProperty("inputStream");
        _outputStream = _actionParams.getProperty("outputStream");
    }

    @Override
    protected boolean invokeMessageApi(Message msg) throws JMSException {
        if(_actionType == JmsActionType.SET)
            return set(msg,_valueType);
        Object obj = get(msg,_valueType);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "JMST8001", 
                "Action {0}: Value got from the message is: {1}", _id, String.valueOf(obj));
        if(obj != null){
            if(_value != null)
                _dataRepository.storeVar(_value, obj);
        }
        return verify(obj);
    }
    
    abstract protected Object get(Message msg, JmsValueType type) throws JMSException;
    abstract protected boolean set(Message msg, JmsValueType type) throws JMSException;

    private TestInputStream getInputStream() throws IOException {
        TestInputStream is = null;
        if(_inputStream != null){
            is = (TestInputStream) _dataRepository.getVar(_inputStream);
            if(is == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Failed to locate input stream ({0}).", _inputStream);
                throw new IOException("Failed to locate input stream");
            }            
        }
        return is;
    }
    private TestOutputStream getOutputStream() throws IOException {
        TestOutputStream stream = null;
        if(_outputStream != null){
            stream = (TestOutputStream) _dataRepository.getVar(_outputStream);
            if(stream == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Failed to locate input stream ({0}).", _outputStream);
                throw new IOException("Failed to locate output stream");
            }            
        }
        return stream;
    }
    private boolean verify(Object value){
        try {
            TestInputStream     is = getInputStream();
            TestOutputStream     os = getOutputStream();
            switch(_valueType){
            case Boolean:{
                boolean expectedValue;
                boolean realValue = ((Boolean)value).booleanValue();
                if(os != null)
                    os.writeBoolean(realValue);
                if(is != null){
                    expectedValue = is.readBoolean();
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                if(_verifyValue != null){
                    expectedValue = Boolean.parseBoolean(_verifyValue);
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                return true;
            }
            case Byte:{
                byte expectedValue;
                byte realValue = ((Byte)value).byteValue();
                if(os != null)
                    os.writeByte(realValue);
                if(is != null){
                    expectedValue = is.readByte();
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                if(_verifyValue != null){
                    expectedValue = Byte.parseByte(_verifyValue);
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                return true;
            }
            case ByteArray:{
                byte[] expectedValue;
                byte[] realValue = ((byte[])value);
                if(os != null)
                    os.write(realValue);
                if(is != null){
                    int length = is.readInt();
                    expectedValue = new byte[length];
                    is.read(expectedValue);
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (Arrays.equals(realValue, expectedValue));
                }
                return true;
            }
            case Short:{
                short expectedValue;
                short realValue = ((Short)value).shortValue();
                if(os != null)
                    os.writeShort(realValue);
                if(is != null){
                    expectedValue = is.readShort();
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                if(_verifyValue != null){
                    expectedValue = Short.parseShort(_verifyValue);
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                return true;
            }
            case Float:{
                float expectedValue;
                float realValue = ((Float)value).floatValue();
                if(os != null)
                    os.writeFloat(realValue);
                if(is != null){
                    expectedValue = is.readFloat();
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                if(_verifyValue != null){
                    expectedValue = Float.parseFloat(_verifyValue);
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                return true;
            }
            case Integer:{
                int expectedValue;
                int realValue = ((Integer)value).intValue();
                if(os != null)
                    os.writeInt(realValue);
                if(is != null){
                    expectedValue = is.readInt();
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                if(_verifyValue != null){
                    expectedValue = Integer.parseInt(_verifyValue);
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                return true;
            }
            case Counter:{
                int realValue = ((Integer)value).intValue();
                if(os != null)
                    os.writeInt(realValue);
                if(_verifyValue != null){
                    int expectedValue = getCounterValue(_verifyValue);
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                return true;
            }
            case Long:{
                long expectedValue;
                long realValue = ((Long)value).longValue();
                if(os != null)
                    os.writeLong(realValue);
                if(is != null){
                    expectedValue = is.readLong();
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                if(_verifyValue != null){
                    expectedValue = Long.parseLong(_verifyValue);
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                return true;
            }
            case Double:{
                double expectedValue;
                double realValue = ((Double)value).doubleValue();
                if(os != null)
                    os.writeDouble(realValue);
                if(is != null){
                    expectedValue = is.readDouble();
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                if(_verifyValue != null){
                    expectedValue = Double.parseDouble(_verifyValue);
                    if (expectedValue != realValue)
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue == realValue);
                }
                return true;
            }
            case UTF8:
            case String:{
                String expectedValue;
                String realValue = (String)value;
                if(os != null)
                    os.writeUTF(realValue);
                if(is != null){
                    expectedValue = is.readUTF();
                    if (!expectedValue.equals(realValue))
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                    return (expectedValue.equals(realValue));
                }
                if(_verifyValue != null){
                    if(!_verifyNull)
                    {
                        expectedValue = _verifyValue;
                        if (!expectedValue.equals(realValue))
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: EXPECTED: \"{1}\" || RECEIVED: \"{2}\"", this.toString(), expectedValue, realValue);
                        return (expectedValue.equals(realValue));
                    }
                    return(((realValue == null) || realValue.equals(""))? true : false);
                }
                return true;
            }
            case Object:
            default:{
                Object expectedValue;
                Object realValue = value;
                if(os != null)
                    os.writeObject(realValue);
                if(is != null){
                    expectedValue = is.readObject();
                    return (expectedValue.equals(realValue));
                }
                if(_verifyNull){
                    return(((realValue == null) || realValue.equals(""))? true : false);
                }
                if(_verifyValue != null){
                    expectedValue = _dataRepository.getVar(_verifyValue);
                    if(expectedValue == null){
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                                "Failed to locate the object ({0}).", _verifyValue);
                        return false;
                    }
                    return expectedValue.equals(realValue);
                }
                return true;
            }
            }            
        } catch (Throwable e) {
            _trWriter.writeException(e);
            return false;
        }
    }
    boolean    booleanValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            TestOutputStream os = getOutputStream();
            boolean v = Boolean.parseBoolean(_value);
            if(os != null)
                os.writeBoolean(v);
            return v;
        }
        return is.readBoolean();
    }
    byte    byteValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            TestOutputStream os = getOutputStream();
            byte v = Byte.parseByte(_value);
            if(os != null)
                os.writeByte(v);
            return v;
        }
        return is.readByte();
    }
    byte[]    byteArrayValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            TestOutputStream os = getOutputStream();
            int length = Integer.parseInt(_value);
            byte[] result = new byte[length];
            RANDOM.nextBytes(result);
            if(os != null){
                os.writeInt(length);
                os.write(result);
            }
            return result;
        }else{
            int length = is.readInt();
            byte[] result = new byte[length];
            is.read(result,0,length);
            return result;
        }
    }
    short    shortValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            TestOutputStream os = getOutputStream();
            short v = Short.parseShort(_value);
            if(os != null)
                os.writeShort(v);
            return v;
        }
        return is.readShort();
    }
    float    floatValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            TestOutputStream os = getOutputStream();
            float v = Float.parseFloat(_value);
            if(os != null)
                os.writeFloat(v);
            return v;
        }
        return is.readFloat();
    }
    int    intValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            TestOutputStream os = getOutputStream();
            int v = Integer.parseInt(_value);
            if(os != null)
                os.writeInt(v);
            return v;
        }
        return is.readInt();
    }
    long    longValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            TestOutputStream os = getOutputStream();
            long v = Long.parseLong(_value);
            if(os != null)
                os.writeLong(v);
            return v;
        }
        return is.readLong();
    }
    double    doubleValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            TestOutputStream os = getOutputStream();
            double v = Double.parseDouble(_value);
            if(os != null)
                os.writeDouble(v);
            return v;
        }
        return is.readDouble();
    }
    String    stringValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            TestOutputStream os = getOutputStream();
            if(os != null)
                os.writeUTF(_value);
            return _value;
        }
        return is.readUTF();
    }
    Object    objectValue() throws IOException{
        TestInputStream is = getInputStream();
        if(is == null){
            Object obj = _dataRepository.getVar(_value);
            if(obj == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Failed to locate the object ({0}).",    _value);
                throw new IOException("Object not found");
            }
            return obj;
        }
        return is.readObject();
    }
    int counterValue() throws IOException{
        int v = getCounterValue(_value);
        TestOutputStream os = getOutputStream();
        if(os != null){
            os.writeInt(v);
        }
        return v;
    }
}
