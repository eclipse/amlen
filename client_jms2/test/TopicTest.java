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
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSConsumer;
import javax.jms.JMSContext;
import javax.jms.JMSException;
import javax.jms.JMSProducer;
import javax.jms.Message;
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
public class TopicTest implements Runnable{
    int  action;
    static final int RECV = 1;
    static final int ECHO = 2;
    
    static int  msglen = 128;    
    static int  count = 100;

    static ConnectionFactory   fact;
    static JMSContext          context;
    static boolean recvdone;
    static boolean echodone;
    
    static int  sendcount = 0;
    static int  echocount = 0;
    static int  recvcount = 0;
    
    static boolean echoStarted = false;
    static boolean recvStarted = false;
    
    /*
     * Create the object which is used to create the receive threads
     */
    public TopicTest(int action) {
        this.action = action;
    }
    
    
    /*
     * We only get one run, but choose which function to execute in this thread
     * @see java.lang.Runnable#run()
     */
    public void run() {
        try {
        if (action==RECV)
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
        Topic requestT = context.createTopic("RequestT"); 
        JMSProducer prod = context.createProducer();
        prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);

        context.start();

        /* Don't start sending until all consumers have been created */
        synchronized (TopicTest.class) {
            while (!echoStarted || !recvStarted) {
                try { TopicTest.class.wait(); } catch (InterruptedException e) { }
            }
        }
        
        long startTime = System.currentTimeMillis();
        
        BytesMessage bmsg = context.createBytesMessage();
        byte [] msgbytes = new byte[msglen];
        
        for (int i=0; i<count; i++) {
            bmsg.clearBody();
            msgbytes[0] = (byte)i;
            bmsg.writeBytes(msgbytes);
            prod.send(requestT, bmsg);
            synchronized (TopicTest.class) {
                sendcount++;
                if (sendcount%500 == 0)
                    try { TopicTest.class.wait(3);} catch (Exception e) {} 
            }
        }

        int wait = 1000;
        long sleepTime = 50;
        while ((wait-- > 0) && (!recvdone || !echodone)) {
            long curTime = System.currentTimeMillis();
            long wakeUpTime = curTime + sleepTime;

            while (wakeUpTime > curTime) {
                try { Thread.sleep(wakeUpTime - curTime); } catch (Exception e) {}
                curTime = System.currentTimeMillis();
            }
        }
        long endTime = System.currentTimeMillis();
        if (!recvdone || !echodone) {
            System.out.println("Messages were not all received");
        }
        return endTime-startTime;      
    }
    
    /*
     * Receive messages
     */
    public void doReceive() throws JMSException {
        Topic responseT = context.createTopic("ResponseT");
        ((ImaProperties)responseT).put("DisableACK", true);
        JMSConsumer cons = context.createConsumer(responseT);
        byte [] b = new byte [1];
        int i = 0;
        
        recvStarted = true;
        synchronized (TopicTest.class) {
            TopicTest.class.notify();
        }
        
        do {
            Message msg = cons.receive(5000);
            if (msg == null) {
                throw new JMSException("Timeout in receive message: "+i);
            }
              /* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
            BytesMessage bmsg = (BytesMessage)msg;
            bmsg.readBytes(b);
            if (bmsg.getBodyLength() != msglen || b[0] != (byte)i) {
                throw new JMSException("Message received is not the correct message: "+i);
            }    
            recvcount++;
            i++;
        }while(recvcount < count);
        synchronized (TopicTest.class) {
            recvdone = true;
            if(recvcount < count){
                System.err.println("doReceive: Not all messages were received: " + recvcount);
            }
        }
    }
    
    /*
     * Echo messages
     */
    public void doEcho() throws JMSException {
        JMSContext context = this.context.createContext(JMSContext.DUPS_OK_ACKNOWLEDGE);
        Topic requestT = context.createTopic("RequestT");
        ((ImaProperties)requestT).put("DisableACK", true);
        Topic responseT = context.createTopic("ResponseT"); 
        JMSConsumer cons = context.createConsumer(requestT);
        JMSProducer prod = context.createProducer();
        prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        byte [] b = new byte [1];
        int i = 0;
           
        echoStarted = true;
        synchronized (TopicTest.class) {
            TopicTest.class.notify();
        }
           
        do {
            Message msg = cons.receive(500);
            if (msg == null) {
                throw new JMSException("Timeout in echo message: "+i);
            }
            
            /* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
            BytesMessage bmsg = (BytesMessage)msg;
            bmsg.readBytes(b);
            if (bmsg.getBodyLength() != msglen || b[0] != (byte)i) {
                throw new JMSException("Message to echo is not the correct message: "+i);
            }
            echocount++;
            prod.send(responseT, msg);
            i++;
        }while(echocount < count);
        synchronized (TopicTest.class) {
            echodone = true;
            if(echocount < count){
                System.err.println("doEcho: Not all messages were received: " + echocount);
            }
        }
    }
    
    /*
     * Parse arguments
     */
    public static void parseArgs(String [] args) {
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
    public static void syntaxhelp(String msg)  {
        if (msg != null)
            System.out.println(msg);
        System.out.println("TopicTest  msglen  count ");
    }
    
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
        long millis = 0;
        
        parseArgs(args);
        /*
         * Create connection factory and connection
         */
        try {
        fact = ImaJmsFactory.createConnectionFactory();
        String server = System.getenv("IMAServer");
        String port   = System.getenv("IMAPort");
        String transport = System.getenv("IMATransport");
        if (server == null)
            server = "127.0.0.1";
        if (port == null)
            port = "16102";
        ((ImaProperties)fact).put("Server", server);
        ((ImaProperties)fact).put("Port", port);
        if (transport != null)
            ((ImaProperties)fact).put("Protocol", transport);
        ((ImaProperties)fact).put("DisableMessageTimestamp", true);
        ((ImaProperties)fact).put("DisableMessageID", true);
        } catch (JMSException jex) {
            jex.printStackTrace(System.out);
            return;
        }
        context = fact.createContext();
        context.setClientID("fred");

        
        /*
         * Start the receive threads 
         */
        new Thread(new TopicTest(RECV)).start();
        new Thread(new TopicTest(ECHO)).start();
        
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
        synchronized (TopicTest.class) {
            System.out.println("Time = "+millis);
            System.out.println("Send = "+sendcount);
            System.out.println("Echo = "+echocount);
            System.out.println("Recv = "+recvcount);
        }
        context.close();
    }
    
}
