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
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/*
 * Stand-alone topic subscriber enabled for high availability
 * 
 * This application is intended for use with RecoverableTopicPublisher.  Both applications
 * demonstrate how JMS client applications for IBM Messaging Appliance can detect that a connection
 * to the server has been lost and can reconnect to the server in order to continue.
 * 
 * There are two positional arguments which can be set when running this application:
 * 1. The topic name (default = RequestT)
 * 2. The number of messages to receive (default = 5000)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class RecoverableDurableSubscriber implements Runnable {
    static int count = 5000;
    static String topicname = "RequestT";

    static ConnectionFactory fact;
    static Connection conn;

    static boolean recvdone;

    static int recvcount = 0;

    public void run() {
        try {
            doReceive();
        } catch (Exception ex) {
            ex.printStackTrace(System.err);
            if (!recvdone && isConnClosed()) {
                try {
                    this.reconnectAndContinueReceiving();
                } catch (JMSException jex) {
                    reconnectAndStartNewReceiveThread();
                }
            } else {
                recvdone = true;
            }
        }
    }

    /*
     * Reconnect and start a new receive thread to continue processing
     */
    public static void reconnectAndStartNewReceiveThread() {
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
            new Thread(new RecoverableDurableSubscriber()).start();
        else
            synchronized (RecoverableDurableSubscriber.class) {
                recvdone = true;
            }
    }

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
     * Create topic administered object using IMA provider
     */
    public static Topic getTopic() throws JMSException {
        Topic dest = ImaJmsFactory.createTopic(topicname);
        return dest;
    }

    /*
     * Check to see if we are finished receiving messages
     */
    public static void doCheckDone() throws JMSException {
        synchronized (RecoverableDurableSubscriber.class) {
            while (!recvdone) {
                try {
                    RecoverableDurableSubscriber.class.wait(500);
                } catch (Exception e) {
                }
            }
        }
    }

    /*
     * Receive messages
     */
    public void doReceive() throws JMSException {
        Session sess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
        Topic requestT = getTopic();
        MessageConsumer cons = sess.createDurableSubscriber(requestT, "subscription1");
        conn.start();
        int i = recvcount;

        do {
            try {
                Message msg = cons.receive(10000);
                if (msg == null) {
                    throw new JMSException("Timeout in receive message: " + i);
                } else {
                    synchronized (RecoverableDurableSubscriber.class) {
                        recvcount++;
                    }
                    if (msg instanceof TextMessage) {
                        TextMessage tmsg = (TextMessage) msg;
                        System.out.println("Received message " + i + ": " + tmsg.getText());
                    } else {
                        System.out.println("Received message " + i);
                    }
                    i++;
                }
            } catch (Exception ex) {
                ex.printStackTrace(System.err);
                if (!recvdone && isConnClosed()) {
                    this.reconnectAndContinueReceiving();
                } else {
                    synchronized (RecoverableDurableSubscriber.class) {
                        recvdone = true;
                    }
                }
                break;
            }
        } while (recvcount < count);
        synchronized (RecoverableDurableSubscriber.class) {
            recvdone = true;
            if (recvcount < count) {
                System.err.println("doReceive: Not all messages were received: " + recvcount);
            }
        }
        /*
         * Uncomment the following lines if you wish to close the subscription
         * when recvdone is true. Note that this will cause the server to
         * discard any remaining messages that might exist on the durable topic.
         */
        // cons.close();
        // sess.unsubscribe("subscription1");
    }

    public static boolean doConnect() {
        /*
         * Create connection factory and connection
         */
        try {
            fact = getCF();
            conn = fact.createConnection();
            conn.setClientID("RecoverableSubscriber");
            System.out.println("Client ID is: " + conn.getClientID());
            return true;
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
            return false;
        }
    }

    public static boolean isConnClosed() {
        if (conn != null)
            return ((ImaProperties) conn).getBoolean("isClosed", false);
        else
            return true;
    }

    /*
     * Reconnect and continue processing
     */
    public void reconnectAndContinueReceiving() throws JMSException {
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
            doReceive();
        else
            synchronized (RecoverableDurableSubscriber.class) {
                recvdone = true;
            }
    }

    /*
     * Parse arguments
     */
    public static void parseArgs(String[] args) {
        if (args.length > 0) {
            topicname = args[0];
        }
        if (args.length > 1) {
            try {
                count = Integer.valueOf(args[1]);
            } catch (Exception e) {
                syntaxhelp("");
                return;
            }
        }
    }

    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg) {
        if (msg != null)
            System.out.println(msg);
        System.out.println("SimpleTSubscriber  topicname count");
    }

    /*
     * Main method
     */
    public static void main(String[] args) {
        int connretry = 0;
        parseArgs(args);

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
         * Start the receive threads
         */
        new Thread(new RecoverableDurableSubscriber()).start();

        /*
         * Check to see if we have finished receiving messages
         */
        try {
            doCheckDone();
        } catch (Exception ex) {
            ex.printStackTrace(System.out);
        }

        /*
         * Print out statistics
         */
        synchronized (RecoverableDurableSubscriber.class) {
            System.out.println("Received " + recvcount + " messages");
        }

        try {
            conn.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
