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

import java.io.UnsupportedEncodingException;
import java.util.Random;

import javax.jms.BytesMessage;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Session;

import org.w3c.dom.Element;
import java.util.zip.CRC32;
import java.util.zip.Checksum;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

/**
 * This class is used for sending variable length BYTES messages.
 * A random seed value is used, unless the seed is specified.
 *
 * The messages:
 * writeLong( crc value of data )
 * writeBytes( data )
 *
 * <Action name="SendCRC" type="SendMessageCRC">
 *     <ActionParameter name="producer_id">producer1_dest1</ActionParameter>
 *     <ActionParameter name="session_id">session1_tx_cf1</ActionParameter>
 *     <ActionParameter name="num_messages">100000</ActionParameter>
 *     <ActionParameter name="max_length">1048576</ActionParameter>
 *     <!--ActionParameter name="seed">8007756947271441634</ActionParameter-->
 * </Action>
 *
 *
 * TODO: add support for more message types.
 *
 */
public class SendMessageCRCAction extends ApiCallAction {
    private final String  _producerID;
    private final String  _sessionID;
    private final int     _numMessages;
    private final int     _maxLength;
    private final long    _seed;
    private final boolean _variableInterval;
    private final int     _rate; // msg per second

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public SendMessageCRCAction(Element config, DataRepository dataRepository,
                                TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _producerID = _actionParams.getProperty("producer_id");
        if(_producerID == null){
            throw new RuntimeException("producer_id is not defined for " + this.toString());
        }
        _sessionID = _actionParams.getProperty("session_id");
        _numMessages = Integer.parseInt(_actionParams.getProperty("num_messages", "100"));
        _maxLength = Integer.parseInt(_actionParams.getProperty("max_length", "512"));
        _variableInterval = Boolean.parseBoolean(_actionParams.getProperty("variable_interval", "false"));
        _seed = Long.parseLong(_actionParams.getProperty("seed", "0"));
        _rate = Integer.parseInt(_actionParams.getProperty("rate", "0"));

        if (_variableInterval && _rate != 0)
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "Both Variable Interval and Rate specified in CRC test, this does not make any sense");
    }

    protected boolean invokeApi() throws JMSException {
        final MessageProducer producer = (MessageProducer) _dataRepository.getVar(_producerID);
        final Session session = (Session) _dataRepository.getVar(_sessionID);

        if(producer == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Failed to locate the MessageProducer object ({0}).", _producerID);
            return false;
        }

        if(session == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Failed to locate the Session object ({0}).", _sessionID);
            return false;
        }

        Random r;
        if (_seed != 0) {
            r = new Random(_seed);
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2083", "Action {0}: Random seed value: {1}", _id, _seed);
        } else {
            Random rndSeed = new Random();
            long seed = rndSeed.nextLong();
            r = new Random(seed);
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2083", "Action {0}: Random seed value: {1}", _id, seed);
        }
        int i=0;

        long lastMessageSendTime = 0;
        long interval = (long)((1.0 / (float)_rate) * 1000.0);

        try {
            if (producer.getDestination() != null) {
                for (i=0; i<_numMessages; i++) {
                    BytesMessage msg = session.createBytesMessage();
                    int msgLen = r.nextInt(_maxLength);
                    if (msgLen <= 0)
                        msgLen = 512;
                    byte [] msgBytes = new byte[msgLen];
                    String token = Integer.toString(msgLen, 36);
                    StringBuffer sbuf = new StringBuffer();
                    while (sbuf.length() < msgLen) {
                        sbuf.append(token);
                    }
                    String payload = sbuf.substring(0,msgLen-1);
                    msgBytes = payload.getBytes("UTF-8");
                    Checksum checksum = new CRC32();
                    checksum.update(msgBytes,0,msgBytes.length);
                    long lngChecksum = checksum.getValue();
                    msg.writeLong(lngChecksum);
                    msg.writeBytes(msgBytes,0,msgBytes.length);

                    if(_variableInterval)
                        try { Thread.sleep(r.nextInt(100)); } catch(InterruptedException iex) {} // sleep for 0 - .5 seconds

                    else if (_rate != 0) {
                        while (System.currentTimeMillis() - lastMessageSendTime < interval)
                            { /* do nothing */ }
                    }
                    lastMessageSendTime = System.currentTimeMillis();
                    producer.send(msg);
                }
            }
        } catch (UnsupportedEncodingException e) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2085", "Sent {0} messages", i);
        }
        return true;
    }
}
