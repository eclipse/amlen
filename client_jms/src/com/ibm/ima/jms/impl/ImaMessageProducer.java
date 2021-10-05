/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.jms.impl;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.QueueSender;
import javax.jms.TopicPublisher;


/**
 * Implement the MessgeProducer interface for the IBM MessageSight JMS client.
 *
 * A message producer can either set the destination when it is created, or specify it on
 * each send.  
 * 
 * This method is extended in the JMS 2.0 implementation
 */
public class ImaMessageProducer extends ImaProducer implements MessageProducer, TopicPublisher, QueueSender {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    public ImaMessageProducer(ImaSession session, Destination dest, int domain) throws JMSException {
        super(session, dest, domain);
    }
}
