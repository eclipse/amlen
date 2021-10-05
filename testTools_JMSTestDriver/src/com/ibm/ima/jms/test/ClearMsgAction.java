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
import javax.jms.Message;

import org.w3c.dom.Element;

public class ClearMsgAction extends JmsMessageAction {
    private final boolean _clearBody;
    private final boolean _clearProps;
    
    protected ClearMsgAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _clearBody = Boolean.parseBoolean(_apiParameters.getProperty("clearBody", "false"));
        _clearProps = Boolean.parseBoolean(_apiParameters.getProperty("clearProps", "false"));
    }

    @Override
    protected boolean invokeMessageApi(Message msg) throws JMSException {
        if(_clearBody)
            msg.clearBody();
        if(_clearProps)
            msg.clearProperties();
        return true;
    }


}
