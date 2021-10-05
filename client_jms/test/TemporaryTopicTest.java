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

import java.util.HashMap;

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TemporaryTopic;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/*
 * Test an echoed send and receive with DisableACK
 * 
 * There are four positional arguments which can be set when running this test:
 * 1. The size of the message (default = 128)
 * 2. The number of messages (default = 100)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class TemporaryTopicTest implements Runnable {
    int action;
    static final int RECV = 1;
    static final int ECHO = 2;

    static TemporaryTopic tempResponseT;
    static HashMap<Topic, MessageProducer> replytoProds = new HashMap<Topic, MessageProducer>();

    static int msglen = 128;
    static int count = 100;

    static ConnectionFactory fact;
    static Connection conn;
    static boolean recvdone;
    static boolean echodone;

    static int sendcount = 0;
    static int echocount = 0;
    static int recvcount = 0;

    static int readyToStart = 0;

    /*
     * Create the object which is used to create the receive threads
     */
    public TemporaryTopicTest(int action) {
        this.action = action;
    }

    /*
     * We only get one run, but choose which function to execute in this thread
     * 
     * @see java.lang.Runnable#run()
     */
    public void run() {
        try {
            if (action == RECV)
                doReceive();
            else
                doEcho();
        } catch (Exception ex) {
            ex.printStackTrace(System.err);
            recvdone = true;
            echodone = true;
        }
    }

    /*
     * Send messages
     */
    public static long doSend() throws JMSException {
        Session sess = conn.createSession(false, 0);
        tempResponseT = sess.createTemporaryTopic();
        Topic requestT = sess.createTopic("RequestT");
        MessageProducer prod = sess.createProducer(requestT);
        prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        new Thread(new TemporaryTopicTest(RECV)).start();

        /* Wait until other 2 threads are ready */
        synchronized (TemporaryTopicTest.class) {
            while (readyToStart < 2) {
                try {
                    TemporaryTopicTest.class.wait(50);
                } catch (Exception e) {
                }
            }
        }

        conn.start();

        long startTime = System.currentTimeMillis();

        BytesMessage bmsg = sess.createBytesMessage();
        byte[] msgbytes = new byte[msglen];

        for (int i = 0; i < count; i++) {
            bmsg.clearBody();
            msgbytes[0] = (byte) i;
            bmsg.writeBytes(msgbytes);
            bmsg.setJMSReplyTo(tempResponseT);
            prod.send(bmsg);
            synchronized (TemporaryTopicTest.class) {
                sendcount++;
                if (sendcount % 500 == 0)
                    try {
                        TemporaryTopicTest.class.wait(3);
                    } catch (Exception e) {
                    }
            }
        }
        synchronized (TemporaryTopicTest.class) {
            int wait = 1000;
            while ((wait-- > 0) && (!recvdone || !echodone)) {
                long currentTime = System.currentTimeMillis();
                long wakeupTime = currentTime + 50;

                while (currentTime < wakeupTime) {
                    try {
                        TemporaryTopicTest.class.wait(50);
                    } catch (Exception e) {
                    }
                    currentTime = System.currentTimeMillis();
                }
            }
            long endTime = System.currentTimeMillis();
            if (!recvdone || !echodone) {
                System.out.println("Messages were not all received");
            }
            return endTime - startTime;
        }
    }

    /*
     * Receive messages
     */
    public void doReceive() throws JMSException {
        Session sess = conn.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
        ((ImaProperties) tempResponseT).put("DisableACK", true);
        MessageConsumer cons = sess.createConsumer(tempResponseT);
        byte[] b = new byte[1];
        int i = 0;

        /* Let the main thread know that the connection can be started */
        synchronized (TemporaryTopicTest.class) {
            readyToStart++;
        }

        do {
            Message msg = cons.receive(1000);
            if (msg == null) {
                throw new JMSException("Timeout in receive message: " + i);
            }
            /* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
            BytesMessage bmsg = (BytesMessage) msg;
            bmsg.readBytes(b);
            if (bmsg.getBodyLength() != msglen || b[0] != (byte) i) {
                throw new JMSException("Message received is not the correct message: " + i);
            }
            recvcount++;
            i++;
        } while (recvcount < count);
        synchronized (TemporaryTopicTest.class) {
            recvdone = true;
            if (recvcount < count) {
                System.err.println("doReceive: Not all messages were received: " + recvcount);
            }
        }
    }

    /*
     * Echo messages
     */
    public void doEcho() throws JMSException {
        Session sess = conn.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
        Topic responseT;
        Topic requestT = sess.createTopic("RequestT");
        ((ImaProperties) requestT).put("DisableACK", true);
        MessageConsumer cons = sess.createConsumer(requestT);

        /* Let the main thread know that the connection can be started */
        synchronized (TemporaryTopicTest.class) {
            readyToStart++;
        }

        byte[] b = new byte[1];
        int i = 0;

        do {
            Message msg = cons.receive(1000);
            if (msg == null) {
                throw new JMSException("Timeout in echo message: " + i);
            }

            /* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
            BytesMessage bmsg = (BytesMessage) msg;
            bmsg.readBytes(b);
            if (bmsg.getBodyLength() != msglen || b[0] != (byte) i) {
                throw new JMSException("Message to echo is not the correct message: " + i);
            }
            responseT = (Topic) bmsg.getJMSReplyTo();
            echocount++;
            MessageProducer prod = getReplyToProd(sess, responseT);
            prod.send(msg);
            i++;
        } while (echocount < count);
        synchronized (TemporaryTopicTest.class) {
            echodone = true;
            if (echocount < count) {
                System.err.println("doEcho: Not all messages were received: " + echocount);
            }
        }
    }

    /*
     * Check to see of the replyto producer is already cached. If not, create a new producer and cache it.
     */
    MessageProducer getReplyToProd(Session sess, Topic topic) throws JMSException {
        if (replytoProds.containsKey(topic))
            return (replytoProds.get(topic));
        MessageProducer prod = sess.createProducer(topic);
        prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        replytoProds.put(topic, prod);
        return prod;
    }

    /*
     * Parse arguments
     */
    public static void parseArgs(String[] args) {
        if (args.length > 0) {
            try {
                msglen = Integer.valueOf(args[0]);
            } catch (Exception e) {
                syntaxhelp("Message length not valid");
                return;
            }
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
        System.out.println("TopicTest  msglen  count ");
    }

    /*
     * Main method
     */
    public static void main(String[] args) {
        long millis = 0;

        parseArgs(args);
        /*
         * Create connection factory and connection
         */
        try {
            fact = ImaJmsFactory.createConnectionFactory();
            String server = System.getenv("IMAServer");
            String port = System.getenv("IMAPort");
            String transport = System.getenv("IMATransport");
            if (server == null)
                server = "127.0.0.1";
            if (port == null)
                port = "16102";
            ((ImaProperties) fact).put("Server", server);
            ((ImaProperties) fact).put("Port", port);
            if (transport != null)
                ((ImaProperties) fact).put("Protocol", transport);
            ((ImaProperties) fact).put("DisableMessageTimestamp", true);
            ((ImaProperties) fact).put("DisableMessageID", true);
            conn = fact.createConnection();
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
            return;
        }

        /*
         * Start the echo thread
         */
        new Thread(new TemporaryTopicTest(ECHO)).start();

        /*
         * Run the send
         */
        try {
            millis = doSend();
        } catch (Exception ex) {
            ex.printStackTrace(System.out);
        }

        /*
         * Print out statistics
         */
        synchronized (TemporaryTopicTest.class) {
            System.out.println("Time = " + millis);
            System.out.println("Send = " + sendcount);
            System.out.println("Echo = " + echocount);
            System.out.println("Recv = " + recvcount);
        }
        try {
            conn.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
