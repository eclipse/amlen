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
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/*
 * Test a point-to-point send and receive with acknowledge
 * 
 * There are three positional arguments which can be set when running this test:
 * 1. The size of the message (default = 128)
 * 2. The number of messages (default = 100)
 * 3. The name of the topic (default = "RequestT")
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class SelectTest implements Runnable {
    static int msglen = 128;
    static int count = 100;
    static String topicName = "RequestT";
    static String propName = "\u554A";

    static ConnectionFactory fact;
    static Connection conn;
    static volatile boolean recvdone;

    static int sendcount;
    static int recvcount;
    static MessageConsumer cons = null;

    /*
     * Send messages
     */
    public static long doSend(Session sess, MessageProducer prod) throws JMSException {
        prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        conn.start();
        try {
            Thread.sleep(100);
        } catch (Exception e) {
        }

        long startTime = System.currentTimeMillis();

        BytesMessage bmsg = sess.createBytesMessage();
        byte[] msgbytes = new byte[msglen];

        for (int i = 0; i < count * 3; i++) {
            bmsg.clearBody();
            msgbytes[0] = (byte) (i / 3);
            bmsg.writeBytes(msgbytes);
            switch (i % 3) {
            case 0:
                bmsg.setIntProperty(propName, 1);
                break;
            case 1:
                bmsg.setIntProperty(propName, 2);
                break;
            case 2:
                bmsg.setStringProperty(propName, "fred");
                break;
            }
            prod.send(bmsg);
            sendcount++;
            if (sendcount % 500 == 0)
                try {
                    Thread.sleep(3);
                } catch (Exception e) {
                }
        }

        int wait = count * 10 + 100;
        while (!recvdone && wait-- > 0) {
            long sleepTime = 50;
            long curTime = System.currentTimeMillis();
            long wakeUpTime = curTime + sleepTime;

            while (wakeUpTime > curTime) {
                try {
                    Thread.sleep(wakeUpTime - curTime);
                } catch (Exception e) {
                }
                curTime = System.currentTimeMillis();
            }
        }
        long endTime = System.currentTimeMillis();
        if (!recvdone) {
            System.out.println("Recv(1) - Messages were not all received");
        }
        return endTime - startTime;
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
            topicName = args[2];
        }
    }

    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg) {
        if (msg != null)
            System.out.println(msg);
        System.out.println("SelectTest  msglen  count ");
    }

    /*
     * Main method
     */
    public static void main(String[] args) {
        long millis = 0;
        Session sendSess = null;
        Session recvSess = null;
        MessageProducer prod = null;

        Topic prodT;
        Topic consT;

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
                ((ImaProperties) fact).put("Transport", transport);

            ((ImaProperties) fact).put("DisableMessageTimestamp", true);
            ((ImaProperties) fact).put("DisableMessageID", true);
            conn = fact.createConnection();
            sendSess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
            recvSess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);

            prodT = sendSess.createTopic(topicName);
            consT = recvSess.createTopic(topicName);
            ((ImaProperties) consT).put("DisableACK", true);

            prod = sendSess.createProducer(prodT);
            // cons = recvSess.createConsumer(consT, "true");
            cons = recvSess.createConsumer(consT, propName + " = 1");
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
            return;
        }

        new Thread(new SelectTest()).start();

        /*
         * Run the send
         */
        try {
            millis = doSend(sendSess, prod);
        } catch (Exception ex) {
            ex.printStackTrace(System.out);
        }

        /*
         * Print out statistics
         */
        System.out.println("Time = " + millis);
        System.out.println("Send = " + sendcount);
        System.out.println("Recv = " + recvcount);
        try {
            conn.close();
            System.out.println("close conection");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void run() {
        try {
            while (recvcount < count) {
                Message msg = cons.receive();
                BytesMessage bmsg = (BytesMessage) msg;
                byte[] b = new byte[1];
                bmsg.readBytes(b);
                if (bmsg.getBodyLength() != msglen || b[0] != (byte) recvcount) {
                    throw new JMSException("Message received is not the correct message: expected(" + recvcount
                                    + ") found(" + b[0] + ") expected length(" + msglen + ") found length("
                                    + bmsg.getBodyLength() + ")");
                }
                recvcount++;
                // System.out.println("receive "+ recvcount);
            }
        } catch (Exception e) {
            e.printStackTrace(System.err);
        }
        recvdone = true;
    }
}
