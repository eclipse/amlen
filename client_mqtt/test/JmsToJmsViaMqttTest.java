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



import java.io.IOException;
import java.util.HashMap;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TemporaryTopic;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttMessage;


/*
 * Test a JMS send and receive echoed via MQTT and using temporary topic and replyto
 * 
 * There is one argument which can be set when running this test:
 * 1. The number of messages (default = 100)
 * 
 * The location of the ISM server can be specified using two environment variables:
 * ISMServer    - The IP address of the server (default = 127.0.0.1)
 * ISMJmsPort   - The JMS port number of the server (default = 16102)
 * ISMMqttPort  - The MQTT port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class JmsToJmsViaMqttTest implements Runnable{
	int  action;
	static final int RECV = 1;
	static final int ECHO = 2;
	
	static TemporaryTopic tempResponseT;
    static HashMap<Topic,MessageProducer> replytoProds = new HashMap<Topic,MessageProducer>();
	
	static int  msglen = 128;    
	static int  count = 100;

	static ConnectionFactory   fact;
	static Connection          conn;
	static String              server;
	static String              mqttport;
	static boolean recvdone;
	static boolean echodone;
	static boolean requestTSubscribed = false;
	static boolean responseTSubscribed = false;
	
	static int  sendcount = 0;
	static int  echocount = 0;
	static int  recvcount = 0;
	
	
	/*
	 * Create the object which is used to create the receive threads
	 */
    public JmsToJmsViaMqttTest(int action) {
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
    	Session sess = conn.createSession(false, 0);
    	tempResponseT = sess.createTemporaryTopic();
    	Topic requestT = sess.createTopic("RequestT"); 
    	MessageProducer prod = sess.createProducer(requestT);
    	prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);    	
    	new Thread(new JmsToJmsViaMqttTest(RECV)).start();
    	conn.start();
    	try { Thread.sleep(100); } catch (Exception e) {}
    	        
        // Assure no messages are sent until topic is subscribed.  Otherwise, program will appear to hang.
        while (!requestTSubscribed) {
            try { Thread.sleep(5); } catch (Exception e) {}
        }
        
    	long startTime = System.currentTimeMillis();
    	
    	TextMessage tmsg = sess.createTextMessage();
        
    	for (int i=0; i<count; i++) {
    		tmsg.clearBody();
    		tmsg.setText("This is message "+i);
    		tmsg.setJMSReplyTo(tempResponseT);
    		prod.send(tmsg);
    		synchronized (JmsToJmsViaMqttTest.class) {
        		sendcount++;
        		if (sendcount%500 == 0)
        			try { JmsToJmsViaMqttTest.class.wait(3);} catch (Exception e) {} 
			}
    	}
    	synchronized (JmsToJmsViaMqttTest.class) {
        	int wait = 1000;
        	while ((wait-- > 0) && (!recvdone || !echodone)) {
        		long curTime = System.currentTimeMillis();
        		long wakeupTime = curTime + 50;
        		while (curTime < wakeupTime) {
        			try { JmsToJmsViaMqttTest.class.wait(50); } catch (Exception e) {}
        			Thread.yield();
        			curTime = System.currentTimeMillis();
        		}
        	}
        	long endTime = System.currentTimeMillis();
        	if (!recvdone || !echodone) {
        		System.out.println("Messages were not all received");
        	}
            return endTime-startTime;  	
    	}
    }
    
    /*
     * Receive messages
     */
    public void doReceive() throws JMSException {
    	Session sess = conn.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
    	((ImaProperties)tempResponseT).put("DisableACK", true);
    	MessageConsumer cons = sess.createConsumer(tempResponseT);    	
    	// Tell sender that the subscriber is ready to receive on this topic
        responseTSubscribed = true;
    	int i = 0;
    	
    	do {
    		Message msg = cons.receive(500);
    		if (msg == null) {
    			throw new JMSException("Timeout in receive message: "+i);
    	    }
      		/* Validate the message */
            TextMessage tmsg = (TextMessage)msg;
            try {
                String tstr = tmsg.getText();

                if (!tstr.equals("This is message "+i)) {
                    throw new JMSException("Message received is not the correct message: expected(This is message "+i+") found("+tstr+")");
                }
            }
            catch (JMSException jmse) {
                throw new RuntimeException(jmse);
            }	
    		recvcount++;
    		i++;
    	}while(recvcount < count);
    	synchronized (JmsToJmsViaMqttTest.class) {
    		recvdone = true;
    		if(recvcount < count){
    			System.err.println("doReceive: Not all messages were received: " + recvcount);
    		}
    	}
    }
    
    /*
     * Echo messages
     */
    public void doEcho() throws IOException {
        IsmMqttConnection conn = new IsmMqttConnection(server, Integer.parseInt(mqttport), "/mqtt", "testecho");
        //conn.setEncoding(IsmMqttConnection.BINARY);
        conn.connect();
        conn.setVerbose(false);
        conn.subscribe("RequestT", 0);
        // Tell sender that the subscriber is ready to receive on this topic
        requestTSubscribed = true;
        int i = 0;
        String msgstr = null;
        
        // Assure no messages are sent until topic is subscribed.  Otherwise, program will appear to hang.
        while (!responseTSubscribed) {
            try { Thread.sleep(5); } catch (Exception e) {}
        }
        
        do {
            //IsmMqttMessage msg = conn.receive(500);
            IsmMqttMessage msg = conn.receive();
            if (msg == null) {
                throw new RuntimeException("Timeout in receive message: "+i);
            }
            /* Validate the message */
            msgstr = msg.getPayload();
            if (!msgstr.equals("This is message "+i)) {
                System.out.println("Message echoed is not the correct message.  Expected: This is message "+i+" Found: "+msgstr);
                // TODO: Fix this.  This call to unsubscribe never returns.
                conn.unsubscribe("RequestT");
                conn.disconnect();
                throw new RuntimeException("Message echoed is not the correct message.  Expected: This is message "+i+" Found: "+msgstr);
            } 
            echocount++;
            conn.publish(msg.getReplyTo(),msgstr);
            i++;
        }while(echocount < count);
        synchronized (JmsToJmsViaMqttTest.class) {
            echodone = true;
            conn.unsubscribe("RequestT");
            conn.disconnect();
            if(echocount < count){
                System.err.println("doEcho: Not all messages were received: " + echocount);
            }
        }
    }

    /*
     * Check to see of the replyto producer is already cached.  If not, create a new producer and cache it.
     */
    MessageProducer getReplyToProd(Session sess, Topic topic) throws JMSException {
        if(replytoProds.containsKey(topic))
            return(replytoProds.get(topic));
        MessageProducer prod = sess.createProducer(topic);
        prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        replytoProds.put(topic,prod);
        return prod;   
    }
    
    /*
     * Parse arguments
     */
    public static void parseArgs(String [] args) {
    	if (args.length > 0) {
    		try { 
    			count = Integer.valueOf(args[0]);
    		} catch (Exception e) {
    			syntaxhelp("");
    			return;
    		}
        }
    	if (args.length > 1) {
    	    syntaxhelp("Too many args.");
    	    return;
    	}
    }
    
    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg)  {
    	if (msg != null)
            System.out.println(msg);
    	System.out.println("JmsToJmsViaMqttTest  count ");
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
    	    server = System.getenv("ISMServer");
    	    String jmsport   = System.getenv("ISMJmsPort");
    	    mqttport   = System.getenv("ISMMqttPort");
    	    if (server != null)
    	    	((ImaProperties)fact).put("Server", server);
    	    if (jmsport != null)
    	    	((ImaProperties)fact).put("Port", jmsport);
    	    if(mqttport == null)
    	        mqttport = "16102";
    	    ((ImaProperties)fact).put("DisableMessageTimestamp", true);
            ((ImaProperties)fact).put("DisableMessageID", true);
    	    conn = fact.createConnection();
    	} catch (JMSException jmse) {
    		jmse.printStackTrace(System.out);
    		return;
    	}
    	
    	/*
    	 * Start the echo thread 
    	 */
    	new Thread(new JmsToJmsViaMqttTest(ECHO)).start();
    	
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
    	synchronized (JmsToJmsViaMqttTest.class) {
        	System.out.println("Time = "+millis);
        	System.out.println("Send = "+sendcount);
        	System.out.println("Echo = "+echocount);
        	System.out.println("Recv = "+recvcount);
		}
    	try {
    	    conn.close();
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    } 
}
