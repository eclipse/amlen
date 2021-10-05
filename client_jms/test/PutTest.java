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
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/*
 * Test a point-to-point send and receive with acknowledge
 * 
 * There are three positional arguments which can be set when running this test:
 * 1. The size of the message (default = 128)
 * 2. The number of messages (default = 100)
 * 3. The name of the queue (default = "RequestQ")
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class PutTest {
    static int msglen = 128;
    static int count = 100;
    static String queueName = "RequestQ";

    static ConnectionFactory fact;
    static Connection conn;

    static int sendcount;

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

        for (int i = 0; i < count; i++) {
            bmsg.clearBody();
            msgbytes[0] = (byte) i;
            bmsg.writeBytes(msgbytes);
            prod.send(bmsg);
            sendcount++;
            if (sendcount % 500 == 0)
                try {
                    Thread.sleep(3);
                } catch (Exception e) {
                }
        }

        long endTime = System.currentTimeMillis();

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
            queueName = args[2];
        }
    }

    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg) {
        if (msg != null)
            System.out.println(msg);
        System.out.println("PtPListenerTest  msglen  count ");
    }

    /*
     * Main method
     */
    public static void main(String[] args) {
        long millis = 0;
        Session sendSess = null;
        MessageProducer prod = null;
        Queue prodQ;

        parseArgs(args);
        /*
         * Create connection factory and connection
         */
        try {
            fact = ImaJmsFactory.createConnectionFactory();
            String server = System.getenv("IMAServer");
            String port = System.getenv("IMAPort");
            String transport = System.getenv("IMATransport");
            String recvBuffer = System.getenv("IMARecvBuffer");
            String sendBuffer = System.getenv("IMASendBuffer");
            if (server == null)
                server = "127.0.0.1";
            if (port == null)
                port = "16102";
            ((ImaProperties) fact).put("Server", server);
            ((ImaProperties) fact).put("Port", port);
            if (transport != null)
                ((ImaProperties) fact).put("Protocol", transport);
            if (recvBuffer != null)
                ((ImaProperties) fact).put("RecvBuffer", recvBuffer);
            if (sendBuffer != null)
                ((ImaProperties) fact).put("SendBuffer", sendBuffer);

            ((ImaProperties) fact).put("DisableMessageTimestamp", true);
            ((ImaProperties) fact).put("DisableMessageID", true);
            conn = fact.createConnection();
            sendSess = conn.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
            prodQ = sendSess.createQueue(queueName);
            prod = sendSess.createProducer(prodQ);
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
            return;
        }

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
        try {
            conn.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
