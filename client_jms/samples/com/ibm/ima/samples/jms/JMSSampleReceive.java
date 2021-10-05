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
package com.ibm.ima.samples.jms;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

/**
 * Class to encapsulate the JMS message consumer logic.
 * 
 * This class contains the doSubscribe method for subscribing to JMS messages.
 * 
 */

public class JMSSampleReceive {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /**
     * receive messages sent to the destination
     * 
     * @param config
     *            JMSSample instance containing configuration settings
     * 
     * @throws JMSException
     *             the JMS exception
     */
    public static void doReceive(JMSSample config) throws JMSException {
        int recvcount = 0;

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

        MessageConsumer consumer = null;

        // Parameter checks in JMSSample ensure isDurable is only set for topics
        if (config.isDurable) {
            consumer = session.createDurableSubscriber((Topic) dest, config.clientId);
        } else {
            consumer = session.createConsumer(dest);
        }

        config.connection.start();
        config.println("Client " + config.clientId + " ready to consume messages from " + config.destType + ": '"
                        + config.destName + ((config.jndi_context == null)? "'." : "'('" + JMSSample.getDestName(dest) + "')."));

        do {
            // consumer.receive will block for 10 seconds waiting for a message
            // to arrive
            Message msg = null;
            msg = consumer.receive((config.timeout == 0) ? 10000 : config.timeout * 1000);
            if (msg != null) {
                recvcount++;
                if (recvcount == 1)
                    config.println("Client " + config.clientId + " received first message on " + config.destType + " '"
                                    + getDestName(msg) + "'");

                if (config.verbose) {
                    // Only expect to receive text messages, but check before cast.
                    if (msg instanceof TextMessage)
                        config.println("Client " + config.clientId + " received TextMessage " + recvcount + " on "
                                        + config.destType + " '" + getDestName(msg) + "': "
                                        + ((TextMessage) msg).getText());
                    else
                        config.println("Client " + config.clientId + " received Message " + recvcount + " on "
                                        + config.destType + " '" + getDestName(msg));

                }
            } else if (config.timeout > 0) {
                break;
            }
        } while (recvcount < config.count);

        config.println("Client " + config.clientId + " received " + recvcount + " messages.");

        config.connection.stop();
        consumer.close();

        if (config.deleteDurableSubscription) {
            session.unsubscribe(config.clientId);
            config.println("Client " + config.clientId + " removed durable subscription.");
        }

        session.close();
    }

    /**
     * Gets the topic name from a message.
     * 
     * @param msg
     *            The message from which the topic name is extracted.
     * 
     * @throws JMSException
     *             the jMS exception
     */
    public static String getDestName(Message msg) throws JMSException {
        String destName = null;
        Destination dest = msg.getJMSDestination();
        if ((dest != null) && (dest instanceof Queue)) {
            destName = ((Queue) dest).getQueueName();
        } else if ((dest != null) && (dest instanceof Topic)) {
            destName = ((Topic) dest).getTopicName();
        }
        return destName;
    }

}
