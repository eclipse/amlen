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

import javax.jms.JMSException;
import javax.jms.TextMessage;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CompareMessageAction extends ApiCallAction {
    public final String _structureID1;
    public final String _structureID2;

    public CompareMessageAction(Element config,
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID1 = _actionParams.getProperty("structure_id1");
        _structureID2 = _actionParams.getProperty("structure_id2");
        if (_structureID1 == null || _structureID2 == null) {
            throw new RuntimeException("structure_id1 and structure_id2 must both be defined for "+this.toString());
        }
    }

    protected boolean invokeApi() throws JMSException {
        final TextMessage msg1 = (TextMessage) _dataRepository.getVar(_structureID1);
        final TextMessage msg2 = (TextMessage) _dataRepository.getVar(_structureID2);

        if (msg1 == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Action {0}: Failed to locate the message object ({1}).", _id, _structureID1);
        }
        if (msg2 == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Action {0}: Failed to locate the message object ({1}).", _id, _structureID2);
        }

        if ((msg1 instanceof TextMessage) && (msg2 instanceof TextMessage)) {
            String msg1Text = msg1.getText();
            String msg2Text = msg2.getText();

            if (!msg1Text.equals(msg2Text)) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2072", "Action {0}: {1} text differs from {2} text.", _id, _structureID1, _structureID2);
                return false;
            }
        } else {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2071", "Action {0}: Only implemented for TEXT messages.", _id);
            return false;
        }
        return true;
    }
}
