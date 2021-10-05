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
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


/*
 * Simple stand-alone topic publisher
 * 
 * There is one positional argument which can be set when running this test:
 * 1. The topic name (default = RequestT)
 * 2. The message text (default = message<n> 
 *     - where n is the index of the message
 * 3. The number of messages (default = 10)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class SimpleTPublisher {    
	static int  count = 10;
	static String topicname = "RequestT";
	static String msgtext = "message";

	static ConnectionFactory   fact;
	static Connection          conn;

	
	static int  sendcount = 0;
	
	
	/*
	 * Create the object which is used to create the receive threads
	 */
    public SimpleTPublisher(int action) {
    }
    
    /*
     * Create connection factory administered object using IMA provider
     */
    public static ConnectionFactory getCF() throws JMSException
    {
        ConnectionFactory cf = ImaJmsFactory.createConnectionFactory();
        String server = System.getenv("IMAServer");
        String port   = System.getenv("IMAPort");
        if (server == null)
            server = "127.0.0.1";
        if (port == null)
            port = "16102";
        ((ImaProperties)cf).put("Server", server);
        ((ImaProperties)cf).put("Port", port);
        return cf;
    }
    
    /*
     * Send messages
     */
    public static void doSend() throws JMSException {
    	Session sess = conn.createSession(false, 0);
    	Topic requestT = sess.createTopic(topicname); 
    	MessageProducer prod = sess.createProducer(requestT);
    	prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
    	conn.start();
    	try { Thread.sleep(100); } catch (Exception e) {}

    	TextMessage tmsg;
    	
    	for (int i=0; i<count; i++) {
    	    tmsg = sess.createTextMessage(msgtext+i);
    	    prod.send(tmsg);
    		synchronized (SimpleTPublisher.class) {
        		sendcount++;
        		if (sendcount%500 == 0)
        			try { SimpleTPublisher.class.wait(3);} catch (Exception e) {} 
			}
    	}
    }
    
    /*
     * Parse arguments
     */
    public static void parseArgs(String [] args) {
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
    public static void syntaxhelp(String msg)  {
    	if (msg != null)
            System.out.println(msg);
    	System.out.println("SimpleTPublisher topicname messagetext count");
    }
    
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	
    	parseArgs(args);
    	/*
    	 * Create connection factory and connection
    	 */
    	try {
    	    fact = getCF();
    	    conn = fact.createConnection();
    	    conn.setClientID("spub_"+Utils.uniqueString("spublisher"));
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
    	} catch (Exception ex) {
    		ex.printStackTrace(System.out);
    	}
    	
    	/*
    	 * Print out statistics
    	 */
    	synchronized (SimpleTPublisher.class) {
        	System.out.println("Sent "+sendcount+" messages");
		}
    	try {
    	    conn.close();
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }
    
}
