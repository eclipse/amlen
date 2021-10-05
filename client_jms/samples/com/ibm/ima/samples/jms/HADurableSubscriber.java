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
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/**
 * Standalone topic subscriber sample for high availability
 * 
 * This application is intended for use with HATopicPublisher.  Both applications
 * demonstrate how IBM MessageSight JMS client applications can detect that a connection
 * to the server has been lost and can reconnect to the server in order to continue.
 * 
 * To fail over to a backup server when the connection to the primary is lost, the client application must 
 * recreate all of the JMS objects required for the subscription.
 * 
 * There are four positional arguments required to run this application and one optional argument:
 * 1. Server list - the list of server host names or IP addresses where IBM MessageSight is running in a high availability configuration
 * 2. Server port - the connection port for the IBM MessageSight server
 * 3. Topic name - the name of the topic from which messages will be received
 *                 NOTE: This topic name must match the name used for the HATopicPublisher sample
 * 4. Message count - the number of messages to receive
 * 5. Optional receivetimeout - the number of seconds to wait to receive the first message from the server.  The default is 10 seconds.
 * 
 * IMPORTANT NOTE:  This sample does not remove the durable subscription so that messages sent by HATopicPublisher 
 * while HADurableSubscriber is not active will be received when HADurableSubscriber becomes active.  If you prefer
 * to remove the durable topic before this program exits, uncomment the lines in the main program that close the 
 * consumer and unsubscribe the session.
 * 
 * Example command lines for the program:
 *      java com.ibm.ima.samples.jms.HADurableSubscriber "server1.mycompany.com, server2.mycompany.com" 16102 durableTopic 2000
 *      java com.ibm.ima.samples.jms.HADurableSubscriber "server1.mycompany.com, server2.mycompany.com" 16102 durableTopic 2000 60
 * 
 * To see the usage statement for the program, run it with no arguments:
 *      java com.ibm.ima.samples.jms.HADurableSubscriber
 * 
 */
public class HADurableSubscriber implements Runnable {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    final static long forcetimeout = 120000;    /* The number of milliseconds to wait before giving up
                                                 * on waiting for all messages to be received. 
                                                 * Timeout is forced in 2 minutes (120 seconds). */
    final static int connectTimeout = 120000;   /* Apply connection retry logic for up to two minutes before giving up. */
    
    static String serverlist = null;            /* A comma delimited list of server host names or IP addresses
                                                 * where IBM MessageSight is running. */ 
    static int serverport;                      /* The connection port for IBM MessageSight. */
    static String topicname = null;             /* The topic name for the durable subscription. */
    static int count;                           /* The number of messages to receive. */
    static int receivetimeout = 10000;          /* The number of milliseconds to wait for the first receive request to succeed. */

    static ConnectionFactory fact;              /* The connection factory used to create the connection object. */
    static Connection conn;                     /* The connection object used to connect to IBM MessageSight. */
    static Session sess;                        /* The session object used to create the durable subscriber. */
    static MessageConsumer cons;                /* The durable subscriber (consumer) object that receives messages. */

    static boolean recvdone;                    /* Flag that indicates whether the application has finished
                                                 * receiving.  This is set to true when all of the messages
                                                 * have been received or under certain error conditions.
                                                 */

    static int recvcount = 0;                   /* A count of the number of messages received so far. */

    static long lastrecvtime = System.currentTimeMillis();  /* Tracks the last time a message was received. 
                                                             * This time is used to determine whether a 
                                                             * forced timeout is needed. */


    /**
     * Print simple syntax help.
     * 
     * @param msg   Syntax help message. 
     * 
     */
    public static void syntaxhelp(String msg) {
        System.err.println("java com.ibm.ima.samples.jms.HADurableSubscriber serverlist serverport topicname count [receivetimeout]");
        System.err.println("  Where the positional arguments are:");
        System.err.println("      serverlist A String representing list of MessageSight appliance host names or IP addresses.  This list can be comma or space delimited.");
        System.err.println("      serverport An integer value representing the port number for connections to the MessageSight appliance.");
        System.err.println("      topicname A String representing the durable subscription topic name.");
        System.err.println("      count An integer value representing the number of messages to receive.");
        System.err.println("      receivetimeout (Optional) The number of seconds to wait for the first message to be received.  The default value is 10 seconds.");
        System.err.println();
        System.err.println("  Examples:");
        System.err.println("      java com.ibm.ima.samples.jms.HADurableSubscriber \"server1.mycompany.com, server2.mycompany.com\" 16102 durableTopic 2000");
        System.err.println("      java com.ibm.ima.samples.jms.HADurableSubscriber \"server1.mycompany.com, server2.mycompany.com\" 16102 durableTopic 2000 60");
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
     *      topicname           durable subscription topic name
     *      count               number of messages to receive from the durable topic
     *      [receivetimeout]    number of seconds to wait to receive the first message
     *      
     */
    public static void main(String[] args) {

        parseArgs(args);

        /*
         * Connect to server
         */
        boolean connected = doConnect();

        if (!connected)
            throw new RuntimeException("Failed to connect!");

        /*
         * Start the receive thread.
         */
        new Thread(new HADurableSubscriber()).start();

        /*
         * Check to see if we have finished receiving messages.
         */
        doCheckDone();

        
        /*
         * Uncomment the following lines if you wish to remove the durable subscription
         * when we have finished receiving messages. Note that this will cause 
         * IBM MessageSight to discard any remaining messages that might exist on 
         * the durable topic.
         */
        //try {
        //    cons.close();
        //    sess.unsubscribe("subscription1");
        //} catch (Exception ex) {
        //    ex.printStackTrace();
        //}
         

        /*
         * Print out the number of message received.
         */
        synchronized (HADurableSubscriber.class) {
            System.out.println("Received " + recvcount + " messages");
        }

        /*
         * Close the connection.
         */
        try {
            conn.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    /**
     * Parse command line arguments.
     * 
     * @param args      Command line arguments from the main program. 
     * 
     *  Summary of command line arguments:
     *      serverlist          list of server host names or IP addresses 
     *      serverport          connection port for the IBM MessageSight server
     *      topicname           durable subscription topic name
     *      count               number of messages to receive from the durable topic
     *      [receivetimeout]    number of seconds to wait to receive the first message
     *      
     */
    public static void parseArgs(String[] args) {
        /* 
         * Provide syntax help if too few or too many arguments appear on the command line.
         */
        if (args.length < 4) {
            if ( args.length == 0)
                syntaxhelp(null);
            syntaxhelp("Too few command line arguments (" + args.length + ")");
        } 
        
        if (args.length > 5)
            syntaxhelp("Too many command line arguments (" + args.length + ")");

        /* The first required argument is a list of server host names or IP addresses.  This can be a 
         * comma delimited list.  The host names or IP addresses in the list must match those where
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
         * The name of the subscription topic is the third required argument. This name must match the topic
         * name specified in HATopicPublisher.
         */
        topicname = args[2];
        
        /* 
         * The number of messages to receive is the fourth required argument.
         */
        try {
            count = Integer.valueOf(args[3]);
        } catch (Exception e) {
            syntaxhelp(args[3] + " is not a valid count value. Count (number of messages) must be an integer value.");
        }
        
        /* 
         * The fifth argument is optional and specifies how many seconds to wait to receive the first message. The default is 10 seconds.
         */
        if (args.length > 4) {
            int timeout;
            try {
                timeout = Integer.valueOf(args[4]);
                receivetimeout = timeout * 1000;
            } catch (Exception e) {
                syntaxhelp(args[4] + " is not a valid timeout value. Receivetimeout (number of seconds) must be an integer value.");
            }
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
     * Run the subscriber thread.
     * 
     * @see java.lang.Runnable#run()
     */
    public void run() {
        try {
            doReceive();
        } catch (Exception ex) {
            ex.printStackTrace(System.err);
            /* 
             * If an exception is thrown while preparing to receive messages, check to see
             * whether the client application is still connected to the server.  If it is not, then 
             * attempt to reconnect and continue receiving.
             */
            if (!recvdone && isConnClosed()) {
                try {                   
                    this.reconnectAndContinueReceiving();
                } catch (JMSException jex) {
                    /* 
                     * If an exception is thrown while attempting to handle the previous exception, 
                     * then reconnect and start a new receive thread.
                     */
                    reconnectAndStartNewReceiveThread();
                }
            } else {
                recvdone = true;
            }
        }
    }

    /**
     * Check to see if we are finished receiving messages.
     */
    public static void doCheckDone() {
        boolean done = false;
        while (!done) {
            synchronized (HADurableSubscriber.class) {
                if (!recvdone) {
                    try {
                        HADurableSubscriber.class.wait(500);
                    } catch (Exception e) {
                    }
                    if (System.currentTimeMillis() - lastrecvtime > forcetimeout)
                        recvdone = true;
                }
                done = recvdone;
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
            conn.setClientID("HASubscriber");
            sess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
            System.out.println("Client ID is: " + conn.getClientID());

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
     * Receive messages via durable subscription.
     */
    public void doReceive() throws JMSException {       
        Topic durabletopic = getTopic();
        cons = sess.createDurableSubscriber(durabletopic, "subscription1");
        conn.start();
        int i, firstmsg;
        i = firstmsg = recvcount;

        do {
            int timeout = 10000;
            if (i == firstmsg)
                timeout = receivetimeout;
            try {
                Message msg = cons.receive(timeout);
                if (msg == null) {
                    throw new JMSException("Timeout in receive message: " + i);
                } else {
                    synchronized (HADurableSubscriber.class) {
                        recvcount++;
                        lastrecvtime = System.currentTimeMillis();
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
                /* 
                 * If an exception is thrown while messages are received, check to see whether the connection
                 * has closed.  If it has closed, then attempt to reconnect and continue receiving.
                 */
                if (!recvdone && isConnClosed()) {
                    this.reconnectAndContinueReceiving();
                } else {
                    synchronized (HADurableSubscriber.class) {
                        /* 
                         * If all messages have already been received or if the connection remains active despite
                         * the exception, the error should not be addressed with an attempt to reconnect.
                         * Set recvdone to true in order to exit this sample application.
                         */
                        recvdone = true;
                    }
                }
                break;
            }
        } while (recvcount < count);
        synchronized (HADurableSubscriber.class) {
            recvdone = true;
            if (recvcount < count)
                System.err.println("doReceive: Not all messages were received: " + recvcount);
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
                 * We cannot determine whether the connection is closed so return false
                 */
                return false;
            }
        } else {
            return true;
        }
    }
    
    /**
     * Reconnect and continue receiving messages.
     */
    public void reconnectAndContinueReceiving() throws JMSException {
        System.out.println("Connection is closed.  Attempting to reconnect and continue receiving.");
        boolean reconnected = doConnect();

        if (reconnected) {
            doReceive();
        } else {
            /* 
             * If we fail to reconnect in a timely manner, then set recvdone to true in order to 
             * exit from this sample application.
             */
            synchronized (HADurableSubscriber.class) {
                System.err.println("Timed out while attempting to reconnect to the server.");
                recvdone = true;
            }
        }
    }
    
    /**
     * Reconnect to the server and start a new receive thread to continue receiving messages.
     */
    public static void reconnectAndStartNewReceiveThread() {
        System.out.println("Connection is closed.  Attempting to reconnect and continue receiving.");
        boolean reconnected = doConnect();

        if (reconnected) {
            new Thread(new HADurableSubscriber()).start();
        } else {
            synchronized (HADurableSubscriber.class) {
                System.err.println("Timed out while attempting to reconnect to the server.");
                recvdone = true;
            }
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
         * must contain the list of IBM MessageSight server host names or IPs so that the client
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
     * Get topic administered object.
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
