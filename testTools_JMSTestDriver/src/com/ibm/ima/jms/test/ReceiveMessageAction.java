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

import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.StringTokenizer;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;


public class ReceiveMessageAction extends ApiCallAction {
    private final String          _consumerID;
    private final String          _listenerID;
    private final String          _structureID;
    private final long            _timeout; 
    private final String          _msgBodyType;
    private final boolean         _logTurboFlowname;
    private Map<Integer, Integer> _turboFlowLabels = null;
    private Map<String, Integer>  _turboFlowBitmaps = null;
    
    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public ReceiveMessageAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        // _structurID is used for the message in this case because the message is instantiated when received
        _structureID = _actionParams.getProperty("structure_id");
        if(_structureID == null){
            throw new RuntimeException("structure_id is not defined for " + this.toString());
        }
        _consumerID = _actionParams.getProperty("consumer_id");
        _listenerID = _actionParams.getProperty("listener_id");
        if((_listenerID == null) && (_consumerID == null) ){
            throw new RuntimeException("Both consumer_id and listener_id are not defined for " + this.toString());
        }
        if((_listenerID != null) && (_consumerID != null) ){
            throw new RuntimeException("Both consumer_id and listener_id are defined for " + this.toString());
        }
        _timeout = Long.parseLong(_apiParameters.getProperty("timeout", "0"));
        _msgBodyType = _apiParameters.getProperty("msgBodyType");
        _logTurboFlowname = Boolean.parseBoolean(_actionParams.getProperty("logTurboFlowname", "0"));
        String tfLabels = _actionParams.getProperty("turboFlowLabels");
        if (null != tfLabels) {
            _turboFlowLabels = parseLabels(tfLabels);
        }
        String tfBitmaps = _actionParams.getProperty("turboFlowBitmaps");
        if (null != tfBitmaps) {
            _turboFlowBitmaps = parseBitmaps(tfBitmaps);
        }
    }

    protected boolean invokeApi() throws JMSException {
        final Message msg;
        
        if(_consumerID != null){
            final MessageConsumer consumer = (MessageConsumer) _dataRepository.getVar(_consumerID);            
            if(consumer == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                        "Action {0}: Failed to locate the MessageConsumer object ({1}).",_id, _consumerID);
                return false;
            }
            if(_timeout == 0)
                msg = consumer.receive();
            else {
                long delta = 0;
                Date before = new Date();
                msg = consumer.receive(_timeout);
                if (null == msg) {
                    delta = (new Date()).getTime() - before.getTime();
                    if (delta > 2*_timeout) {
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2043", 
                                "Action {0}: failed after {1} milliseconds, timeout was {2} milliseconds", 
                                _consumerID, delta, _timeout);
                    }
                }
            }
        }else{
            final JmsMessageListener listener = (JmsMessageListener) _dataRepository.getVar(_listenerID);
            if(listener == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                        "Action {0}: Failed to locate the MessageListener object ({1}).", _id,_listenerID);
                return false;
            }
            long delta = 0;
            Date before = new Date();
//            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "Calling receive for listener {0}",_listenerID);
            msg = listener.receive(_timeout);
            if (null == msg) {
                delta = (new Date()).getTime() - before.getTime();
                if (delta > 2*_timeout) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2043", 
                            "Action {0}: failed after {1} milliseconds, timeout was {2} milliseconds", 
                            _consumerID, delta, _timeout);
                }
            }
        }
        if(msg == null) {
            throw new JMSException("Null message from receive()","JMSTDNullMsg");
        } else {
            if (_logTurboFlowname) {
                String name = (String)msg.getObjectProperty("JMS_Flowname");
                if (null != name) {
                    Integer i = (Integer)_dataRepository.getVar(name);
                    if (null == i) {
                        i = new Integer(1);
                    } else {
                        i = i + 1;
                    }
                    _dataRepository.storeVar(name, i);
                }
            } else if (null != _turboFlowLabels) {
                Integer label = (Integer)msg.getObjectProperty("JMS_Label");
                if (null == label) { 
                    throw new RuntimeException("Expecting Turboflow labels, but message has none");
                }
                Integer count = _turboFlowLabels.get(label);
                if (null == count) {
                    throw new RuntimeException("Not expecting turboflow label '"+label.toString()+"'");
                }
                if (count >= 0) {
                    count -= 1;
                    if (count < 0) {
                        throw new RuntimeException("Received too many occurrances of turboflow label '"+label+"'");
                    }
                    _turboFlowLabels.put(label, count);
                }
            } else if (null != _turboFlowBitmaps) {
                String label = (String)msg.getObjectProperty("JMS_Bitmap");
                if (null == label) { 
                    throw new RuntimeException("Expecting Turboflow labels, but message has none");
                }
                Integer count = _turboFlowBitmaps.get(label);
                if (null == count) {
                    throw new RuntimeException("Not expecting turboflow bitmap '"+label.toString()+"'");
                }
                if (count >= 0) {
                    count -= 1;
                    if (count < 0) {
                        throw new RuntimeException("Received too many occurrances of turboflow bitmap '"+label+"'");
                    }
                    _turboFlowBitmaps.put(label, count);
                }
            }
        }
        _dataRepository.storeVar(_structureID, msg);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2042", "Action {0}: Received message, JMSid={1}", _id, msg.getJMSMessageID());
        
        if (_msgBodyType != null) {
            if (!msg.getClass().toString().endsWith(_msgBodyType)) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: Expected Message Body Type = {1}, but was Message Body Type = {2}", _id, _msgBodyType, msg.getClass().toString());
                throw new JMSException("The message received was not of the expected type", "JMSTDWrongMsgType");
            }
        }
        
        return true;
    }
    
    private Map<Integer, Integer> parseLabels(String labels) {
        Map<Integer, Integer> labelSet = new HashMap<Integer, Integer>();
        StringTokenizer tokens = new StringTokenizer(labels, " \t");
        while (tokens.hasMoreTokens()) {
            String next = tokens.nextToken();
            int pos;
            if (-1 != (pos = next.indexOf(","))) {
                if (0 == pos) {
                    throw new RuntimeException("Invalid label,count = '"+next+"'");
                }
                String value = next.substring(0, pos); 
                String count = next.substring(pos + 1);
                labelSet.put(new Integer(value), new Integer(count));
            } else {
                labelSet.put(new Integer(next), new Integer(-1));
            }
        }
        if (0 ==labelSet.size()) {
            return null;
        }
        return labelSet;
    }
    
    private Map<String, Integer> parseBitmaps(String labels) {
        Map<String, Integer> labelSet = new HashMap<String, Integer>();
        StringTokenizer tokens = new StringTokenizer(labels, " \t");
        while (tokens.hasMoreTokens()) {
            String next = tokens.nextToken();
            int pos;
            if (-1 != (pos = next.indexOf(","))) {
                if (0 == pos) {
                    throw new RuntimeException("Invalid label,count = '"+next+"'");
                }
                String value = next.substring(0, pos); 
                String count = next.substring(pos + 1);
                labelSet.put(value, new Integer(count));
            } else {
                labelSet.put(next, new Integer(-1));
            }
        }
        if (0 ==labelSet.size()) {
            return null;
        }
        return labelSet;
    }

}
