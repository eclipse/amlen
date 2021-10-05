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
package svt.server;

import java.lang.management.ManagementFactory;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class Receiver {
    public String action = null;
    public Connection connection = null;
    Session session = null;
    Destination dest = null;
    MessageConsumer consumer = null;
    public String clientId = null;
    public String userName = null;
    public char[] password = null;
    public int throttleWaitMSec = 0;
    public int count = 1;
    public String payload = "Sample Message";
    public boolean persistence = false;
    public boolean isDurable = true;
    public boolean deleteDurableSubscription = false;
    public String scheme = null;
    // public String port = null;
    // public String server = null;
    public String defaultTopicName = "/JMSSample";
    public String destName = null;
    public String destType = null;
    public boolean verbose = false;
    public boolean disableACKS = false;
    public long timeout = 30000;

    Receiver() {
        super();
    }

    Receiver(String clientId) {
        super();
        this.clientId = clientId;
    }

    static private long MyTid() {
        return Thread.currentThread().getId();
    }

    static private String MyPid() {
        String nameOfRunningVM = ManagementFactory.getRuntimeMXBean().getName();
        int p = nameOfRunningVM.indexOf('@');
        String pid = nameOfRunningVM.substring(0, p);
        return pid;
    }

    void connect(String server, String port, String destType, String destName) {
        if (clientId == null) {
            clientId = "R_" + MyPid() + "_" + MyTid();
        }
        this.destType = destType;
        this.destName = destName;

        try {
            ConnectionFactory connectionFactory = null;
            connectionFactory = ImaJmsFactory.createConnectionFactory();

            if (server != null)
                ((ImaProperties) connectionFactory).put("Server", server);
            if (port != null)
                ((ImaProperties) connectionFactory).put("Port", port);

            if ("tcps".equals(scheme))
                ((ImaProperties) connectionFactory).put("Protocol", "tcps");

            ((ImaProperties) connectionFactory).put("DisableACK", (disableACKS == false) ? "false" : "true");

            ImaJmsException[] exceptions = ((ImaProperties) connectionFactory).validate(false);
            if (exceptions != null) {
                for (ImaJmsException e : exceptions)
                    System.out.println(e.getMessage());
                return;
            }

            if (password != null) {
                connection = connectionFactory.createConnection(userName, new String(password));

                // clear passwords after they are no longer needed.
                java.util.Arrays.fill(password, '\0');
            } else
                connection = connectionFactory.createConnection();

            System.out.printf("connection.setClientID(%s)\n", clientId);

            connection.setClientID(clientId);

            session = connection.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);

            // The JMS Destination administered objects are loaded from a JNDI
            // repository when specified in the command line arguments.
            // Otherwise the JMS Destination is created from the session object by specifying the destination name.

            if (("topic".equals(destType)) && (destName != null))
                dest = session.createTopic(destName);
            else if (("queue".equals(destType)) && (destName != null))
                dest = session.createQueue(destName);
            else
                throw new Exception("invalid destType:  " + destType);

            if (dest == null) {
                System.err.println("ERROR:  Unable to create " + destType + ":  " + destName);
                System.exit(1);
            }

            if (isDurable) {
                System.out.printf("session.createDurableSubscriber((Topic) '%s', '%s')\n", destName, clientId);
                consumer = session.createDurableSubscriber((Topic) dest, clientId);
            } else {
                System.out.printf("session.createConsumer((Topic) %s\n", dest);
                consumer = session.createConsumer(dest);
            }

            // moved from doReceiveMessage
            // connection.start();

        } catch (Throwable e) {
            e.printStackTrace();
            return;
        }

    }

    public void close() {
        try {
            if (consumer != null) {
                consumer.close();
                consumer = null;
            }
            connection.close();

            if (deleteDurableSubscription) {
                session.unsubscribe(clientId);
                System.out.println("Client " + clientId + " removed durable subscription.");
            }

            session.close();
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    public String doReceiveUser() throws JMSException {
        String user = null;
        Message message = null;
        String delims = "[/]+";
        String[] destTokens = null;
        String destName = null;

        message = doReceiveMessage();

        if (message != null) {
            Destination dest = message.getJMSDestination();
            if (dest instanceof Topic) {
                destName = ((Topic) dest).getTopicName();
            } else if (dest instanceof Queue) {
                destName = ((Queue) dest).getQueueName();
            }

            destTokens = destName.split(delims);
            user = destTokens[4];
        }

        return user;
    }

    public String doReceiveBody() throws JMSException {
        String body = null;
        Message message = null;

        message = doReceiveMessage();

        if (message instanceof TextMessage) {
            body = ((TextMessage) message).getText();
        }

        return body;
    }

    public Message doReceiveMessage() throws JMSException {
        // Parameter checks in JMSSample ensure isDurable is only set for topics

        connection.start();
        System.out.println("Client " + clientId + " ready to consume messages from " + destType + ": '" + destName
                        + "'.");

        // consumer.receive will block for 10 seconds waiting for a message
        // to arrive
        Message message = null;
        message = consumer.receive((timeout == 0) ? 10000 : timeout * 1000);
        if (message != null) {
            if (verbose) {
                // Only expect to receive text messages, but check before cast.
                if (message instanceof TextMessage)
                    System.out.println("Client " + clientId + " received TextMessage on " + destType + " '"
                                    + getDestName(message) + "': " + ((TextMessage) message).getText());
                else
                    System.out.println("Client " + clientId + " received Message on " + destType + " '"
                                    + getDestName(message));

            }
        }

        connection.stop();

        return message;
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
