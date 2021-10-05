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

import com.ibm.ima.jms.test.TrWriter.LogLevel;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;

import org.w3c.dom.Element;

public class GetReplyToDestAction extends JmsMessageAction implements JmsMessageUtils{
    private final String   _structureID;
    private final TrWriter _trWriter;

    protected GetReplyToDestAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);

        _trWriter = trWriter;
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
    }

    protected boolean invokeMessageApi(Message msg) throws JMSException {
        _trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD9213", "Action {0} Calling msg.getJMSReplyTo()", this.toString());
        Destination dest = msg.getJMSReplyTo();
        if (dest != null) {
            _dataRepository.storeVar(_structureID, dest);
        }
        return true;
    }
}
