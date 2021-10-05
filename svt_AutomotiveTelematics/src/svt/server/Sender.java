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
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class Sender {
    public String action = null;
    public Connection connection = null;
    Session session = null;
    Destination dest = null;
    MessageProducer producer = null;
    public String clientId = null;
    public String userName = null;
    public char[] password = null;
    public int throttleWaitMSec = 0;
    public String payload = "Sample Message";
    public boolean persistence = false;
    public boolean isDurable = false;
    public boolean deleteDurableSubscription = false;
    public String scheme = null;
    // public String port = null;
    // public String server = null;
    public String defaultTopicName = "/JMSSample";
    public String destName = null;
    public String destType = null;
    public boolean verbose = false;
    public boolean disableACKS = false;
    public long timeout = 0;

    Sender() {
        super();
    }

    Sender(String clientId) {
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

    void connect(String server, String port) {
        if (this.clientId == null) {
            this.clientId = "S_" + MyPid() + "_" + MyTid();
        }
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

            connection.setClientID(clientId);

            session = connection.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);

        } catch (Throwable e) {
            e.printStackTrace();
            return;
        }

    }

    public void close() {
        try {
            if (producer != null) {
                producer.close();
                producer = null;
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

    public void doSendString(String destType, String destName, String payload) throws JMSException {
        this.destType = destType;
        this.destName = destName;
        this.payload = payload;
        doSend();
    }

    private void doSend() throws JMSException {

        if (producer == null) {
            if (("topic".equals(destType)) && (destName != null))
                dest = session.createTopic(destName);
            else
                dest = session.createQueue(destName);

            if (dest == null) {
                System.err.println("ERROR:  Unable to create " + destType + ":  " + destName);
                System.exit(1);
            }

            producer = session.createProducer(dest);
        }

        // Parameter checks in JMSSample ensure persistence is only set for topics
        if (persistence) {
            producer.setDeliveryMode(DeliveryMode.PERSISTENT);
        } else {
            producer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        }

        TextMessage tmsg = session.createTextMessage(payload);

        System.out.println("Client '" + clientId + "' ready to send to " + destType + ": '" + destName + ".");

        producer.send(tmsg);

        System.out.println("Client " + clientId + " sent " + payload + " to " + destType + " " + destName);

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
