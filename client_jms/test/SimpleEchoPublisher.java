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
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.TemporaryTopic;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/*
 * Simple stand-alone topic publisher that specifies an replyto topic and provide a receiver to receive
 * from the replyto topic
 * This program was written to facilitate testing this functionality with JavaScript web pages that support
 * replyto on received messages.  It sends JMS TextMessages, StreamMessages and MapMessages.
 * 
 * There is one positional argument which can be set when running this test:
 * 1. The topic name (default = RequestT)
 * 2. The message text (default = message<n>) 
 *     - where n is the index of the message
 * 3. The number of messages for each of the three types (default = 3 of each)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class SimpleEchoPublisher implements Runnable {
    static int count = 3;
    static String topicname = "RequestT";
    static String msgtext = "message";

    static TemporaryTopic tempResponseT;

    static ConnectionFactory fact;
    static Connection conn;

    static int sendcount = 0;
    static int recvcount = 0;
    static boolean recvdone;

    /*
     * Create the object which is used to create the receive threads
     */
    public SimpleEchoPublisher() {
    }

    /*
     * We only get one run, but choose which function to execute in this thread
     * 
     * @see java.lang.Runnable#run()
     */
    public void run() {
        try {
            doReceive();
        } catch (Exception ex) {
            ex.printStackTrace(System.err);
            recvdone = true;
        }
    }

    /*
     * Create connection factory administered object using IMA provider
     */
    public static ConnectionFactory getCF() throws JMSException {
        ConnectionFactory cf = ImaJmsFactory.createConnectionFactory();
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
        Session sess = conn.createSession(false, 0);
        Topic requestT = sess.createTopic(topicname);
        tempResponseT = sess.createTemporaryTopic();
        MessageProducer prod = sess.createProducer(requestT);
        prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        new Thread(new SimpleEchoPublisher()).start();
        try {
            Thread.sleep(1000);
        } catch (Exception e) {
        }

        TextMessage tmsg;
        StreamMessage smsg;
        MapMessage mmsg;

        for (int i = 0; i < count; i++) {
            tmsg = sess.createTextMessage(msgtext + i);
            tmsg.setJMSReplyTo(tempResponseT);
            tmsg.setJMSCorrelationID("corrid" + i);
            prod.send(tmsg);
            synchronized (SimpleEchoPublisher.class) {
                sendcount++;
                if (sendcount % 500 == 0)
                    try {
                        SimpleEchoPublisher.class.wait(3);
                    } catch (Exception e) {
                    }
            }
        }
        for (int i = 0; i < count; i++) {
            smsg = sess.createStreamMessage();
            smsg.writeInt(i);
            smsg.writeInt(i);
            smsg.writeInt(i);
            smsg.setJMSReplyTo(tempResponseT);
            smsg.setJMSCorrelationID("streamcorrid" + i);
            prod.send(smsg);
            synchronized (SimpleEchoPublisher.class) {
                sendcount++;
                if (sendcount % 500 == 0)
                    try {
                        SimpleEchoPublisher.class.wait(3);
                    } catch (Exception e) {
                    }
            }
        }
        for (int i = 0; i < count; i++) {
            mmsg = sess.createMapMessage();
            mmsg.setInt("MapMessage" + i, i);
            mmsg.setInt("MapMessage" + (i + 1), i);
            mmsg.setInt("MapMessage" + (i + 2), i);
            mmsg.setJMSReplyTo(tempResponseT);
            mmsg.setJMSCorrelationID("mapcorrid" + i);
            prod.send(mmsg);
            synchronized (SimpleEchoPublisher.class) {
                sendcount++;
                if (sendcount % 500 == 0)
                    try {
                        SimpleEchoPublisher.class.wait(3);
                    } catch (Exception e) {
                    }
            }
        }
    }

    /*
     * Receive messages
     */
    public void doReceive() throws JMSException {
        Session sess = conn.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
        ((ImaProperties) tempResponseT).put("DisableACK", true);
        MessageConsumer cons = sess.createConsumer(tempResponseT);
        conn.start();
        int i = 0;

        do {
            Message msg = cons.receive(10000);
            if (msg == null) {
                throw new JMSException("Timeout in receive message: " + i);
            }
            if (msg instanceof TextMessage) {
                TextMessage tmsg = (TextMessage) msg;
                System.out.println("Received message " + i + ": " + tmsg.getText() + ", corrid: "
                                + tmsg.getJMSCorrelationID());
                if (recvcount < count) {
                    if (!tmsg.getJMSCorrelationID().equals(new String("corrid" + i))) {
                        System.out.println("*** correlation ID mismatch: expected = corrid" + i + ", actual = "
                                        + tmsg.getJMSCorrelationID());
                    }
                } else if (recvcount > count && recvcount < 2 * count) {
                    if (!tmsg.getJMSCorrelationID().equals(new String("streamcorrid" + i))) {
                        System.out.println("*** correlation ID mismatch: expected = streamcorrid" + i + ", actual = "
                                        + tmsg.getJMSCorrelationID());
                    }
                } else if (recvcount > 2 * count && recvcount < 3 * count) {
                    if (!tmsg.getJMSCorrelationID().equals(new String("mapcorrid" + i))) {
                        System.out.println("*** correlation ID mismatch: expected = mapcorrid" + i + ", actual = "
                                        + tmsg.getJMSCorrelationID());
                    }
                }
            } else {
                System.out.println("Received message " + i);
            }

            recvcount++;
            i++;
            if (i == count)
                i = 0;
        } while (recvcount < 3 * count);
        synchronized (SimpleTSubscriber.class) {
            recvdone = true;
            if (recvcount < count) {
                System.err.println("doReceive: Not all messages were received: " + recvcount);
            }
        }
    }

    /*
     * Check to see if all messages have been received
     */
    public static void doCheckDone() throws JMSException {
        synchronized (SimpleEchoSubscriber.class) {
            while (!recvdone) {
                try {
                    SimpleEchoSubscriber.class.wait(500);
                } catch (Exception e) {
                }
            }
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
    }

    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg) {
        if (msg != null)
            System.out.println(msg);
        System.out.println("SimpleTPublisher topicname messagetext count");
    }

    /*
     * Main method
     */
    public static void main(String[] args) {

        parseArgs(args);
        /*
         * Create connection factory and connection
         */
        try {
            fact = getCF();
            conn = fact.createConnection();
            conn.setClientID("epub_" + Utils.uniqueString("epublisher"));
            System.out.println("Client ID is: " + conn.getClientID());
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
            return;
        }

        /*
         * Run the send
         */
        try {
            doSend();
            doCheckDone();
        } catch (Exception ex) {
            ex.printStackTrace(System.out);
        }

        /*
         * Print out statistics
         */
        synchronized (SimpleEchoPublisher.class) {
            System.out.println("Sent " + sendcount + " messages");
            System.out.println("Received " + recvcount + " messages");
        }
        try {
            conn.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
