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

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/*
 * Test an echoed send and receive with commit
 * 
 * There are four positional arguments which can be set when running this test:
 * 1. The size of the message (default = 128)
 * 2. The number of messages (default = 100)
 * 3. The number of messages to send before a commit (default = 1)
 * 4. The number of messages to echo before a commit (default = 1)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class CommitTest implements Runnable {
    int action;
    static final int RECV = 1;
    static final int ECHO = 2;

    static int msglen = 128;
    static int count = 100;
    static int sendcommit = 1;
    static int echocommit = 1;

    static ConnectionFactory fact;
    static Connection conn;
    static boolean recvdone;
    static boolean echodone;

    static int sendcount = 0;
    static int echocount = 0;
    static int recvcount = 0;

    /*
     * Create the object which is used to create the receive threads
     */
    public CommitTest(int action) {
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
        Session sess = conn.createSession(true, 0);
        Queue requestQ = sess.createQueue("RequestQ");
        MessageProducer prod = sess.createProducer(requestQ);
        prod.setDeliveryMode(DeliveryMode.PERSISTENT);
        conn.start();
        int commitcount = 0;
        try {
            Thread.sleep(100);
        } catch (Exception e) {
        }

        long startTime = System.currentTimeMillis();

        BytesMessage bmsg = sess.createBytesMessage();
        byte[] msgbytes = new byte[msglen];

        for (int i = 0; i < count; i++) {
            bmsg.clearBody();
            msgbytes[0] = (byte) i;
            bmsg.writeBytes(msgbytes);
            prod.send(bmsg);
            if (++commitcount == sendcommit) {
                sess.commit();
                commitcount = 0;
            }
            synchronized (CommitTest.class) {
                sendcount++;
                if (sendcount % 500 == 0)
                    try {
                        CommitTest.class.wait(3);
                    } catch (Exception e) {
                    }
            }
        }
        sess.commit();
        synchronized (CommitTest.class) {
            int wait = 1000;
            while ((wait-- > 0) && (!recvdone || !echodone)) {
                try {
                    CommitTest.class.wait(50);
                } catch (Exception e) {
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
        Session sess = conn.createSession(echocommit != 0, Session.DUPS_OK_ACKNOWLEDGE);
        Queue responseQ = sess.createQueue("ResponseQ");
        MessageConsumer cons = sess.createConsumer(responseQ);
        byte[] b = new byte[1];
        int commitcount = 0;
        int i = 0;

        do {
            Message msg = cons.receive(500);
            if (msg == null) {
                throw new JMSException("Timeout in receive message: " + i);
            }
            /* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
            BytesMessage bmsg = (BytesMessage) msg;
            bmsg.readBytes(b);
            if (bmsg.getBodyLength() != msglen || b[0] != (byte) i) {
                throw new JMSException("Message received is not the correct message. Expected: " + (byte) i
                                + " Found: " + b[0]);
            }
            if (++commitcount == echocommit) {
                sess.commit();
                commitcount = 0;
            }
            recvcount++;
            i++;
        } while (recvcount < count);
        sess.commit();
        synchronized (CommitTest.class) {
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
        Session sess = conn.createSession(echocommit != 0, Session.DUPS_OK_ACKNOWLEDGE);
        Queue requestQ = sess.createQueue("RequestQ");
        Queue responseQ = sess.createQueue("ResponseQ");
        MessageConsumer cons = sess.createConsumer(requestQ);
        MessageProducer prod = sess.createProducer(responseQ);
        prod.setDeliveryMode(DeliveryMode.PERSISTENT);
        byte[] b = new byte[1];
        int commitcount = 0;
        int i = 0;

        do {
            Message msg = cons.receive(500);
            if (msg == null) {
                throw new JMSException("Timeout in echo message: " + i);
            }

            /* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
            BytesMessage bmsg = (BytesMessage) msg;
            bmsg.readBytes(b);
            if (bmsg.getBodyLength() != msglen || b[0] != (byte) i) {
                throw new JMSException("Message to echo is not the correct message. Expected: " + (byte) i + " Found: "
                                + b[0]);
            }
            echocount++;
            prod.send(msg);
            if (++commitcount == echocommit) {
                sess.commit();
                commitcount = 0;
            }
            i++;
        } while (echocount < count);
        sess.commit();
        synchronized (CommitTest.class) {
            echodone = true;
            if (echocount < count) {
                System.err.println("doEcho: Not all messages were received: " + echocount);
            }
        }
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
        if (args.length > 2) {
            try {
                sendcommit = Integer.valueOf(args[2]);
                if (sendcommit < 1)
                    sendcommit = 1;
            } catch (Exception e) {
                syntaxhelp("Send commit count not valid");
                return;
            }
        }
        if (args.length > 3) {
            try {
                echocommit = Integer.valueOf(args[3]);
                if (echocommit < 1)
                    echocommit = 1;
            } catch (Exception e) {
                syntaxhelp("Echo commit count not valid");
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
        System.out.println("CommitTest  msglen  count send_commit  echo_commit ");
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
         * Start the receive threads
         */
        new Thread(new CommitTest(RECV)).start();
        new Thread(new CommitTest(ECHO)).start();

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
        synchronized (CommitTest.class) {
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
