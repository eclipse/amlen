/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.svt.clients.shared;

import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;

/**
 * Class that encapsulates the IBM MessageSight message send logic.
 * 
 * This class contains the doPublish method for sending messages to a destination.
 * 
 */
public class JMSSampleSend {

    /**
     * Send message to the specified destination
     * 
     * @param config
     *            JMSSample instance containing configuration settings
     * 
     * @throws JMSException
     *             the JMS exception
     */
    public static void doSend(JMSSample config) throws JMSException {

        Session session = config.connection.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);

        // The JMS Destination administered objects are loaded from a JNDI
        // repository when specified in the command line arguments.
        // Otherwise the JMS Destination is created from the session object by specifying the destination name.

        Destination dest = null;
        if (config.jndi_context != null) {
            dest = (Destination) config.retrieveFromJndi(config.jndi_context, config.destName);

            if (dest == null) {
                System.err.println("ERROR:  Unable to retrieve JNDI " + config.destType + ":  " + config.destName);
                System.exit(1);
            }

        } else {
            if ("topic".equals(config.destType))
                dest = session.createTopic(config.destName);
            else
                dest = session.createQueue(config.destName);

            if (dest == null) {
                System.err.println("ERROR:  Unable to create " + config.destType + ":  " + config.destName);
                System.exit(1);
            }
        }

        MessageProducer producer = session.createProducer(dest);

        // Parameter checks in JMSSample ensure persistence is only set for topics
        if (config.persistence) {
            producer.setDeliveryMode(DeliveryMode.PERSISTENT);
        } else {
            producer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        }

        TextMessage tmsg = session.createTextMessage(config.payload);

        config.println("Client '" + config.clientId + "' ready to send to " + config.destType + ": '" + config.destName
                        + ".");
        long startTime = System.currentTimeMillis();
        long sent_count = 0;

        for (int i = 0; i < config.count; i++) {
            if (config.verbose)
                config.println(i + ": Client '" + config.clientId + " sending \"" + config.payload + "\" to "
                                + config.destType + " " + config.destName);

            tmsg = session.createTextMessage(config.payload + " " + i);
            producer.send(tmsg);
            sent_count++;

            if (config.throttleWaitMSec > 0) {
                long currTime = System.currentTimeMillis();
                double elapsed = (double) (currTime - startTime);
                double projected = ((double) i / (double) config.throttleWaitMSec) * 1000.0;
                if (elapsed < projected) {
                    double sleepInterval = projected - elapsed;
                    try {
                        Thread.sleep((long) sleepInterval);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }

        config.println("Client " + config.clientId + " sent " + sent_count + " messages to " + config.destType + " "
                        + config.destName);

        config.connection.close();
    }

}
