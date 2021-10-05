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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.ImaProperties;

/**
 * Standalone topic publisher sample for high availability
 * 
 * This application is intended for use with HADurableSubscriber.  Both applications
 * demonstrate how IBM MessageSight JMS client applications can detect that a connection
 * to the server has been lost and can reconnect to the server in order to continue.  This client application
 * publishes text messages to the specified topic.
 * 
 * To fail over to a backup server when the connection to the primary is lost, the client application must 
 * recreate all of the JMS objects required for publishing messages.
 * 
 * There are five positional arguments required to run this application and three optional arguments:
 * 1. Server list - the list of server host names or IP addresses where IBM MessageSight is running in a high availability configuration
 * 2. Server port - the connection port for the IBM MessageSight server
 * 3. Topic name - the name of the topic to which messages will be published
 *                 NOTE: This topic name must match the name used for the HADurableSubscriber sample
 * 4. Message count - the number of messages to publish
 * 5. Message text - the message to publish 
 *     - Each message will have a numeric value (representing the message index) appended to it
 * 6. Optional message rate - (default 5000) sets the number of messages per second to send
 * 7. Optional initialize durable subscription flag - (default true) determines whether to open a durable subscription
 *                                            for the publish topic to assure that the published messages 
 *                                            will be saved on the server until HADurableSubscriber is run
 *                                            NOTE: If this flag is not set to true, then published messages
 *                                            will only be preserved on the server if the subscription was previously
 *                                            initialized.
 * 8. Optional debug flag - (default false) determines whether to enable debug output provided in this sample
 * 
 * Example command lines for the program:
 *      java com.ibm.ima.samples.jms.HATopicPublisher "server1.mycompany.com, server2.mycompany.com" 16102 haTopic 2000 "Test message"
 *      java com.ibm.ima.samples.jms.HATopicPublisher "server1.mycompany.com, server2.mycompany.com" 16102 haTopic 2000 "Test message" 10000
 *      java com.ibm.ima.samples.jms.HATopicPublisher "server1.mycompany.com, server2.mycompany.com" 16102 haTopic 2000 "Test message" 5000 false
 *      java com.ibm.ima.samples.jms.HATopicPublisher "server1.mycompany.com, server2.mycompany.com" 16102 haTopic 2000 "Test message" 5000 true true
 *      
 * To see the usage statement for the program, run it with no arguments:
 *      java com.ibm.ima.samples.jms.HATopicPublisher   
 *      
 */
public class HATopicPublisher {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    final static int connectTimeout = 120000;   /* Apply connection retry logic for up to two minutes before giving up. */
    
    static String serverlist = null;            /* A comma delimited list of server host names or IP addresses
                                                 * where IBM MessageSight is running. */ 
    static int serverport;                      /* The connection port for IBM MessageSight. */
    static String topicname = null;             /* The topic name for the publish topic. */
    static int count;                           /* The number of messages to send. */
    static String msgtext = null;               /* The base message text for each message. The message index
                                                 * will be appended to this text. */
    static int msgsPerSecond = 5000;            /* The number of messages per second to send. */
    static boolean initds = true;               /* The optional flag that controls whether a durable subscription
                                                 * is initialized for the publish topic to assure published 
                                                 * messages are preserved on the server until they are received
                                                 * by a subscriber. */
    static boolean debug = false;               /* The optional debug flag that controls whether to 
                                                 * print the debug output available in this application. */

    static ConnectionFactory fact;              /* The connection factory used to create the connection object. */
    static Connection conn;                     /* The connection object used to connect to IBM MessageSight. */

    static int sendcount = 0;                   /* A count of the number of messages published so far. */
    static boolean senddone = false;            /* Flag that indicates whether the application has finished
                                                 * publishing.  This is set to true when all of the messages
                                                 * have been sent or under certain error conditions.
                                                 */

    
    /**
     * Print simple syntax help.
     * 
     * @param msg   Syntax help message. 
     * 
     */
    public static void syntaxhelp(String msg) {
        System.err.println("java com.ibm.ima.samples.jms.HATopicPublisher serverlist serverport topicname count messagetext [msgspersec] [initdurablesub] [debug]");
        System.err.println("  Where the five required the positional arguments are:");
        System.err.println("      serverlist A String representing list of MessageSight appliance host names or IP addresses.  This list can be comma or space delimited.");
        System.err.println("      serverport An integer value representing the port number for connections to the MessageSight appliance.");
        System.err.println("      topicname A String representing the name of the topic destination for published messages.");
        System.err.println("      count An integer value representing the number of messages to publish.");
        System.err.println("      messagetext A String representing the content of the published messages.");
        System.err.println();
        System.err.println("  And the remaining three positional arguments are optional:");
        System.err.println("      msgspersec An integer value representing the number of messages to send per second. The default value is 5000.");
        System.err.println("      initdurablesub A boolean value indicating whether a durable subscription should be initialized for the specified topic. The default value is true.");
        System.err.println("      debug A boolean value indicating whether to provide debug output from the application. The default value is false.");
        System.err.println();
        System.err.println("  Examples:");
        System.err.println("      java java com.ibm.ima.samples.jms.HATopicPublisher \"server1.mycompany.com, server2.mycompany.com\" 16102 durableTopic 2000 \"Test message\"");
        System.err.println("      java java com.ibm.ima.samples.jms.HATopicPublisher \"server1.mycompany.com, server2.mycompany.com\" 16102 durableTopic 2000 \"Test message\" 10000");
        System.err.println("      java java com.ibm.ima.samples.jms.HATopicPublisher \"server1.mycompany.com, server2.mycompany.com\" 16102 durableTopic 2000 \"Test message\" 5000 false");
        System.err.println("      java java com.ibm.ima.samples.jms.HATopicPublisher \"server1.mycompany.com, server2.mycompany.com\" 16102 durableTopic 2000 \"Test message\" 5000 true true");
        if (msg != null)
            System.err.println("\n" + msg);
        System.exit(1);
    }
    
    /**
     * Main method.
     * 
     * @param args      Command line arguments.  See parseArgs() for details.
     * 
     *  Summary of command line arguments:
     *      serverlist          list of server host names or IP addresses
     *      serverport          connection port for the IBM MessageSight server
     *      topicname           publish topic name
     *      count               number of messages to publish       
     *      messagetext         base text content of published messages
     *      [msgspersec]        (optional) message rate in messages per second (default 5000)
     *      [initdurablesub]    (optional) flag to initialize the durable subscription (default true)
     *      [debug]             (optional) flag for printing debug output
     *
     */
    public static void main(String[] args) {
        parseArgs(args);

        /*
         * First, initialize the durable subscription if the initds flag is set to true.
         */
        if (initds)
            initDurableSubscription();

        /*
         * Connect to server
         */
        boolean connected = doConnect();

        if (!connected)
            throw new RuntimeException("Failed to connect!");

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
        if (sendcount < count)
            System.out.println("Not all " + count + " messages were sent.");
        System.out.println("Sent " + sendcount + " messages");

        try {
            conn.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    /**
     * Parse arguments
     * 
     * @param args      Command line arguments to parse.
     * 
     *  Summary of command line arguments:
     *      serverlist          list of server host names or IP addresses
     *      serverport          connection port for the IBM MessageSight server
     *      topicname           publish topic name
     *      count               number of messages to publish       
     *      messagetext         base text content of published messages
     *      [msgspersec]        (optional) message rate in messages per second (default 5000)
     *      [initdurablesub]    (optional) flag to initialize the durable subscription (default true)
     *      [debug]             (optional) flag for printing debug output
     *  
     */
    public static void parseArgs(String[] args) {
        /* 
         * Provide syntax help if too few or too many arguments appear on the command line. 
         */
        if (args.length < 5) {
            if (args.length == 0)
                syntaxhelp("");
            else
                syntaxhelp("Too few command line arguments (" + args.length + ")");
        }
        if (args.length > 8)
            syntaxhelp("Too many command line arguments (" + args.length + ")");
        
        /* 
         * The first required argument is a list of server host names or IP addresses.  This can be a 
         * comma delimited list.  The host names or IP addresses in the list must match those where the
         * IBM MessageSight is running in a high availability configuration.
         */
        serverlist = args[0];
        
        /* 
         * The second required argument is the connection port for IBM MessageSight. 
         */
        try {
            serverport = Integer.valueOf(args[1]);
        } catch (Exception ex) {
            syntaxhelp(args[1] + " is not a valid server port.  Server port must be an integer value.");
        }
        
        /* 
         * The name of the publish topic is the third required argument. This name must match the topic
         * name specified in HADurableSubscriber. 
         */
        topicname = args[2];
        
        /* 
         * The number of messages to publish is the fourth required argument. 
         */
        try {
            count = Integer.valueOf(args[3]);
        } catch (Exception e) {
            syntaxhelp(args[3] + " is not a valid count value. Count (number of messages) must be an integer value.");
        }
        
        /* 
         * The fifth required argument is the base text content of each message. 
         */
        msgtext = args[4] + " ";
        
        /* 
         * The sixth argument is optional and sets the message rate for sending messages as messages per second. 
         * This argument is required if you wish to use the seventh argument.
         */
        if (args.length > 5) {
            try {
                msgsPerSecond = Integer.valueOf(args[5]);
            } catch (Exception ex) {
                syntaxhelp(args[5] + " is not a valid message rate.  Messages per second must be a positive integer value.");
            }
            if (msgsPerSecond < 1) {
                System.err.println("WARNING: " + msgsPerSecond + "is not a valid message rate. Using default rate of 5000.");
                msgsPerSecond = 5000;
            }
        }
            
        /* 
         * The seventh command line argument is optional and determines whether a durable subscription (matching 
         * the subscription used in HADurableSubscriber) is initialized prior to publishing messages.  It is true
         * by default.  Initializing the subscription assures that messages published to the topic will be preserved
         * even if the durable subscriber is not active while messages are being published.
         * This argument is required if you wish to use the eighth argument.
         */
        if (args.length > 6)
            try {
                initds = Boolean.valueOf(args[6]);
            } catch (Exception e) {
                syntaxhelp(args[6] + " is not a valid flag setting for initializing durable subscription. Use true or false.");
                return;
            }
        
        /* 
         * The eighth command line argument is optional and determines whether debug output provided in this sample
         * is printed.  It is false by default.
         */
        if (args.length > 7)
            try {
                debug = Boolean.valueOf(args[7]);
            } catch (Exception e) {
                syntaxhelp(args[7] + " is not a valid flag setting for enabling debug output. Use true or false.");
                return;
            }
    }

    /**
     * Initialize the durable subscription. This allows for the messages to be persisted
     * on the server when the subscriber is not connected to the server.
     */
    public static void initDurableSubscription() {
        /*
         * Create a durable subscription for the topic to be published.
         */
        try {
            ConnectionFactory dfact = getCF();
            Connection dconn = dfact.createConnection();
            /*
             * When messages are received, the consumer client application must
             * specify "HASubscriber" and the client ID.
             */
            dconn.setClientID("HASubscriber");
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
            System.out.println("Initialized durable subscription for clientID " + dconn.getClientID() + ", durable name subscription1, topic "+dtopic);
            dcons.close();
            dconn.close();
        } catch (Exception ex) {
            /* 
             * We should only get an exception here if the HADurableSubscriber application is running and thus,
             * the HASubscriber client ID is in use.  This exception can be safely ignored because it will mean
             * that the subscription is already initialized.
             */
        }

    }
    
    /**
     * Establish a connection to the server.  Connection attempts are retried until successful or until
     * the specified timeout for retries is exceeded.
     */
    public static boolean doConnect() {
        int connattempts = 1;
        boolean connected = false;
        long starttime = System.currentTimeMillis();
        long currenttime = starttime;
        
        /* 
         * Try for up to connectTimeout milliseconds to connect.
         */
        while (!connected && ((currenttime - starttime) < connectTimeout)) {
            try { Thread.sleep(5000); } catch (InterruptedException iex) {}
            System.out.println("Attempting to connect to server (attempt " + connattempts + ").");
            connected = connect();
            connattempts++;
            currenttime = System.currentTimeMillis();
        }
        return connected;
    }

    /**
     * Publish messages to specified topic.
     */
    public static void doSend() throws JMSException {
        Session sess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
        System.out.println("Client ID is: " + conn.getClientID());
        Topic durableTopic = getTopic();
        MessageProducer prod = sess.createProducer(durableTopic);
        conn.start();
        long startTime = System.currentTimeMillis();

        TextMessage tmsg;
        int msgcnt = 0;
        while ((sendcount < count) && !senddone) {
            try {
                tmsg = sess.createTextMessage(msgtext + sendcount);
                if (debug)
                    System.out.println("Sending message: " + sendcount + " " + ImaJmsObject.toString(tmsg, "b"));
                prod.send(tmsg);
                sendcount++;
                msgcnt++;
                if (sendcount == count)
                    senddone = true;
            } catch (Exception ex) {
                ex.printStackTrace(System.err);
                /* 
                 * If an exception is thrown while messages are sent, check to see whether the connection
                 * has closed.  If it has closed, then attempt to reconnect and continue sending.
                 */
                if (!senddone && isConnClosed()) {
                    reconnectAndContinueSending();
                } else {
                    /* 
                     * If all messages have already been sent or if the connection remains active despite
                     * the exception, the error should not be addressed with an attempt to reconnect.
                     * Set senddone to true in order to exit this sample application.
                     */
                    senddone = true;
                }
                break;
            }
            
            long currTime = System.currentTimeMillis();
            double elapsed = (double) (currTime - startTime);
            double projected = ((double) msgcnt / (double) msgsPerSecond) * 1000.0;
            if (elapsed < projected) {
                double sleepInterval = projected - elapsed;
                try {
                    Thread.sleep((long) sleepInterval);
                } catch (InterruptedException e) {
                }
            }
        }
    }
    
    /**
     * Connect to the server
     * 
     * @return Success (true) or failure (false) of the attempt to connect.
     */
    public static boolean connect() {
        /*
         * Create connection factory and connection
         */
        try {
            fact = getCF();
            conn = fact.createConnection();
            
            /* 
             * Check that we are using IBM MessageSight JMS administered objects before calling
             * provider specific check on property values. 
             */
            if (fact instanceof ImaProperties) {
                /* 
                 * For high availability, the connection factory stores the list of IBM MessageSight server
                 * host names or IP addresses.  Once connected, the connection object contains only the host name or IP 
                 * to which the connection is established.
                 */
                System.out.println("Connected to " + ((ImaProperties)conn).getString("Server"));
            }

            /* 
             * Connection has succeeded.  Return true.
             */
            return true;
        } catch (JMSException jmse) {
            System.err.println(jmse.getMessage());
            /* 
             * Connection has failed.  Return false. 
             */
            return false;
        }
    }

    /**
     * Check to see if connection is closed.
     * 
     * When the IBM MessageSight JMS client detects that the connection to the server has been
     * broken, it marks the connection object closed. 
     * 
     * @return True if connection is closed or if the connection object is null, false otherwise.
     * 
     */
    public static boolean isConnClosed() {
        if (conn != null) {
            /* 
             * Check the IBM MessageSight JMS isClosed connection property
             * to determine whether the connection state is closed.
             * This mechanism for checking whether the connection is closed 
             * is a provider-specific feature of the IBM MessageSight 
             * JMS client.  So check first that the IBM MessageSight JMS 
             * client is being used.
             */
            if (conn instanceof ImaProperties) {
                return ((ImaProperties) conn).getBoolean("isClosed", false);
            } else {
                /* 
                 * We cannot determine whether the connection is closed so return false.
                 */
                return false;
            }
        } else {
            return true;
        }
    }

    /**
     * Reconnect and continue sending messages.
     */
    public static void reconnectAndContinueSending() throws JMSException {
        System.out.println("Connection is closed.  Attempting to reconnect and continue sending.");
        boolean reconnected = doConnect();

        if (reconnected) {
            doSend();
        } else {
            /* 
             * If we fail to reconnect in a timely manner, then set senddone to true in order to 
             * exit from this sample application.
             */
            System.err.println("Timed out while attempting to reconnect to the server.");
            senddone = true;
        }
    }
    
    /**
     * Get connection factory administered object.
     * 
     * @return ConnectionFactory object.
     * 
     * Note: In production applications, this method would retrieve a
     *       ConnectionFactory object from a JNDI repository.
     */
    public static ConnectionFactory getCF() throws JMSException {
        ConnectionFactory cf = ImaJmsFactory.createConnectionFactory();
        /*
         * NOTE: For high availability configurations, the serverlist
         * must contain the list of IBM MessageSight server host names or IP addresses so that the client
         * can attempt to connect to alternate hosts if connection (or reconnection)
         * to the primary fails.
         */
        ((ImaProperties) cf).put("Server", serverlist);
        ((ImaProperties) cf).put("Port", serverport);
        
        /* 
         * After setting properties on an administered object, it is a best practice to
         * run the validate() method to assure all specified properties are recognized
         * by the IBM MessageSight JMS client. 
         */
        ImaJmsException errors[] = ((ImaProperties) cf).validate(true);
        if (errors != null)
            throw new RuntimeException("Invalid properties provided for the connection factory.");
        return cf;
    }
    
    /**
     * Create topic administered object.
     * 
     * @return Topic object.
     * 
     * Note: In production applications, this method would retrieve a
     *       Topic object from a JNDI repository.
     */
    public static Topic getTopic() throws JMSException {
        Topic dest = ImaJmsFactory.createTopic(topicname);
        return dest;
    }
}
