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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.ImaProperties;

/*
 * Stand-alone topic publisher enabled for high availability
 * 
 * This application is intended for use with RecoverableDurableSubscriber.  Both applications
 * demonstrate how JMS client applications for IBM Messaging Appliance can detect that a connection
 * to the server has been lost and can reconnect to the server in order to continue.
 * 
 * There are four positional arguments which can be set in this application:
 * 1. The topic name (default = RequestT)
 * 2. The message text (default = message<n> 
 *     - where n is the index of the message
 * 3. The number of messages (default = 5000)
 * 4. Whether to enable debug output (default = false)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class RecoverableTopicPublisher {
    static boolean debug = false;
    static int count = 5000;
    static String topicname = "RequestT";
    static String msgtext = "message";

    static ConnectionFactory fact;
    static Connection conn;

    static int sendcount = 0;
    static boolean senddone = false;

    /*
     * Create connection factory administered object using IMA provider
     */
    public static ConnectionFactory getCF() throws JMSException {
        ConnectionFactory cf = ImaJmsFactory.createConnectionFactory();
        /*
         * NOTE: For high availability configurations, the IMAServer setting
         * should contain the list of server hosts or IPs so that each can be
         * tried when connections are established or when they need to be
         * reestablished.
         */
        String server = System.getenv("IMAServer");
        String port = System.getenv("IMAPort");
        if (server == null)
            server = "127.0.0.1";
        if (port == null)
            port = "16102";
        ((ImaProperties) cf).put("Server", server);
        ((ImaProperties) cf).put("Port", port);
        return cf;
    }

    /*
     * Send messages
     */
    public static void doSend() throws JMSException {
        Session sess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
        Topic requestT = sess.createTopic(topicname);
        MessageProducer prod = sess.createProducer(requestT);
        conn.start();
        try {
            Thread.sleep(100);
        } catch (Exception e) {
        }

        TextMessage tmsg;
        int i = sendcount;
        while (sendcount < count && !senddone) {
            try {
                tmsg = sess.createTextMessage(msgtext + i);
                if (debug)
                    System.out.println("Sending message: " + ImaJmsObject.toString(tmsg, "b"));
                prod.send(tmsg);
                synchronized (RecoverableTopicPublisher.class) {
                    sendcount++;
                    i++;
                    if (sendcount == count)
                        senddone = true;
                    if (sendcount % 500 == 0)
                        try {
                            RecoverableTopicPublisher.class.wait(3);
                        } catch (Exception e) {
                        }
                }
            } catch (Exception ex) {
                ex.printStackTrace(System.err);
                if (!senddone && isConnClosed()) {
                    reconnectAndContinueSending();
                } else {
                    senddone = true;
                }
                break;
            }
        }
    }

    /*
     * Reconnect and continue processing
     */
    public static void reconnectAndContinueSending() throws JMSException {
        boolean reconnected = false;
        long startretrytime = System.currentTimeMillis();
        long currenttime = startretrytime;
        while (!reconnected && ((currenttime - startretrytime) < 60000)) {
            try {
                Thread.sleep(5000);
            } catch (InterruptedException iex) {
            }
            reconnected = doConnect();
            currenttime = System.currentTimeMillis();
        }
        if (reconnected)
            doSend();
        else
            senddone = true;
    }

    public static boolean doConnect() {
        /*
         * Create connection factory and connection
         */
        try {
            fact = getCF();
            conn = fact.createConnection();
            conn.setClientID("spub_" + Utils.uniqueString("spublisher"));
            System.out.println("Client ID is: " + conn.getClientID());
            return true;
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
            return false;
        }
    }

    /*
     * The createDurable() function is included for demonstration purposes only.
     * Normally we would not create a subscription from within an application
     * that is intended strictly for publishing messages.
     */
    public static void createDurable() {
        /*
         * Create a durable subscription for the topic to be published.
         */
        try {
            ConnectionFactory dfact = getCF();
            Connection dconn = dfact.createConnection();
            /*
             * When messages are received, the consumer client application must
             * specify "RecoverableSubscriber" and the client ID.
             */
            dconn.setClientID("RecoverableSubscriber");
            Session dsess = dconn.createSession(false, Session.AUTO_ACKNOWLEDGE);
            /*
             * When messages are received, the consumer client application must
             * specify the same topic name.
             */
            Topic dtopic = dsess.createTopic(topicname);
            /*
             * When messages are received, the consumer client application must
             * specify "subscription1" as the durable subscription name.
             */
            MessageConsumer dcons = dsess.createDurableSubscriber(dtopic, "subscription1");
            dconn.close();
        } catch (Exception ex) {
            System.out.println("Failure occurred during attempt to create durable subscripbion.  Published messages might not be preserved.");
            ex.printStackTrace();
        }

    }

    /*
     * When the JMS client detects that the connection to the server has been
     * broken, it marks the connection object close. This method checks the
     * state of the connection object using a provider-specific check. This
     * function is not portable to other JMS providers.
     */
    public static boolean isConnClosed() {
        if (conn != null)
            return ((ImaProperties) conn).getBoolean("isClosed", false);
        else
            return true;
    }

    /*
     * Parse arguments
     */
    public static void parseArgs(String[] args) {
        if (args.length > 0) {
            topicname = args[0];
        }
        if (args.length > 1) {
            msgtext = args[1];
        }
        if (args.length > 2) {
            try {
                count = Integer.valueOf(args[2]);
            } catch (Exception e) {
                syntaxhelp("");
                return;
            }
        }
        if (args.length > 3)
            try {
                debug = Boolean.valueOf(args[3]);
            } catch (Exception e) {
                syntaxhelp("");
                return;
            }
    }

    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg) {
        if (msg != null)
            System.out.println(msg);
        System.out.println("SimpleTPublisher topicname messagetext count debug");
    }

    /*
     * Main method
     */
    public static void main(String[] args) {
        int connretry = 0;
        parseArgs(args);

        /*
         * First, assure a durable subscription exists for the topic so that
         * messages are preserved on the server.
         */
        createDurable();

        /*
         * Connect to server
         */
        boolean connected = doConnect();

        while (!connected && connretry < 5) {
            connretry++;
            connected = doConnect();
        }

        if (!connected)
            throw new RuntimeException("Failed to connect to server!");

        /*
         * Run the send
         */
        try {
            doSend();
        } catch (Exception ex) {
            ex.printStackTrace(System.out);
        }

        /*
         * Print out statistics
         */
        synchronized (RecoverableTopicPublisher.class) {
            if (sendcount < count)
                System.out.println("Not all " + count + " messages were sent.");
            System.out.println("Sent " + sendcount + " messages");
        }

        try {
            conn.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
