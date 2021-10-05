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
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


/*
 * Simple stand-alone topic subscriber
 * 
 * There is one positional argument which can be set when running this test:
 * 1. The topic name (default = RequestT)
 * 1. The number of messages to receive (default = 10)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class SimpleTSubscriber implements Runnable{
	int  action;
	static final int RECV = 1;
	   
	static int  count = 10;
	static String topicname = "RequestT";

	static ConnectionFactory   fact;
	static Connection          conn;

	static boolean recvdone;
	
	static int  recvcount = 0;
	
	
	/*
	 * Create the object which is used to create the receive threads
	 */
    public SimpleTSubscriber(int action) {
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
    	} catch (Exception ex) {
    		ex.printStackTrace(System.err);
    		recvdone = true;
    	}
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
     * Create topic administered object using IMA provider
     */
    public static Topic getTopic() throws JMSException
    {
        Topic dest = ImaJmsFactory.createTopic(topicname);
        // Currently IMA supports only MQTT QoS 0 for pub/sub
        ((ImaProperties)dest).put("DisableACK", true); 
        return dest;
    }
    
    /*
     * Send messages
     */
    public static void doCheckDone() throws JMSException {
    	synchronized (SimpleTSubscriber.class) {
        	while (!recvdone) {
        		try { SimpleTSubscriber.class.wait(500); } catch (Exception e) {}
        	}
    	}
    }
    
    /*
     * Receive messages
     */
    public void doReceive() throws JMSException {
    	Session sess = conn.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
    	Topic requestT = getTopic();
    	MessageConsumer cons = sess.createConsumer(requestT);
    	conn.start();
    	int i = 0;
    	
    	do {
    		Message msg = cons.receive(100000);
    		if (msg == null) {
    			throw new JMSException("Timeout in receive message: "+i);
    	    }
    		if(msg instanceof TextMessage) {
    		    TextMessage tmsg = (TextMessage)msg;
    		    System.out.println("Topic: "+tmsg.getJMSDestination());
    		    System.out.println("Received message "+i+": "+tmsg.getText());
    		} else {
    		    System.out.println("Received message "+i);
    		}
    		
    		recvcount++;
    		i++;
    	}while(recvcount < count);
    	synchronized (SimpleTSubscriber.class) {
    		recvdone = true;
    		if(recvcount < count){
    			System.err.println("doReceive: Not all messages were received: " + recvcount);
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
    	System.out.println("SimpleTSubscriber  topicname count");
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
    	    conn.setClientID("ssub_"+Utils.uniqueString("ssubscriber"));
    	    System.out.println("Client ID is: " + conn.getClientID());
    	} catch (JMSException jmse) {
    		jmse.printStackTrace(System.out);
    		return;
    	}
    	
    	/*
    	 * Start the receive threads 
    	 */
    	new Thread(new SimpleTSubscriber(RECV)).start();
    	
       try {
            doCheckDone();
        } catch (Exception ex) {
            ex.printStackTrace(System.out);
        }
    	
    	/*
    	 * Print out statistics
    	 */
    	synchronized (SimpleTSubscriber.class) {
        	System.out.println("Received "+recvcount+" messages");
		}
    	try {
    	    conn.close();
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }
    
}
