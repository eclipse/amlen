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

import java.util.Enumeration;
import java.util.HashMap;

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
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


/*
 * Simple stand-alone topic subscriber that echoes received messages to the replyto specified in the message
 * This program can be used as the echo program with the interchange_client.html JavaScript page.
 * 
 * There is one positional argument which can be set when running this test:
 * 1. The topic name (default = RequestT)
 * 1. The number of messages to receive (default = 9)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class SimpleEchoSubscriber implements Runnable{
	int  action;
	static final int RECV = 1;
	   
	static int  count = 9;
	static String topicname = "RequestT";

	static ConnectionFactory   fact;
	static Connection          conn;
	static HashMap<Topic,MessageProducer> replytoProds = new HashMap<Topic,MessageProducer>();

	static boolean recvdone;
	
	static int  recvcount = 0;
	static int  echocount = 0;
	
	
	/*
	 * Create the object which is used to create the receive threads
	 */
    public SimpleEchoSubscriber(int action) {
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
     * Check to see if all messages have been received
     */
    public static void doCheckDone() throws JMSException {
    	synchronized (SimpleEchoSubscriber.class) {
        	while (!recvdone) {
        		try { SimpleEchoSubscriber.class.wait(500); } catch (Exception e) {}
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
     * Receive messages
     */
    public void doReceive() throws JMSException {
    	Session sess = conn.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
    	Topic requestT = getTopic();
    	MessageConsumer cons = sess.createConsumer(requestT);
    	Topic responseT = null;
    	conn.start();
    	
    	int i = 0;
    	
    	do {
    		Message msg = cons.receive(10000);
    		if (msg == null) {
    			throw new JMSException("Timeout in receive message: "+i);
    	    }
    		if(msg instanceof TextMessage) {
    		    TextMessage tmsg = (TextMessage)msg;
    		    System.out.println("Received message "+i+": "+tmsg.getText());
    		} else if(msg instanceof StreamMessage) {
                StreamMessage smsg = (StreamMessage)msg;
                System.out.print("Received message "+i+" [ ");
                Object thisobj = null;
                do {
                    try {
                        thisobj = smsg.readObject();
                    } catch (Exception ex) {
                        thisobj = null;
                    }
                    System.out.print(thisobj != null ? thisobj+" " : "");
                } while(thisobj != null);
                System.out.println("]");
    		} else if(msg instanceof MapMessage) {
    		    MapMessage mmsg = (MapMessage)msg;
                System.out.print("Received message "+i+": ");
                Enumeration names = mmsg.getMapNames();
                while(names.hasMoreElements()) {
                    String thisname = (String)names.nextElement();
                    Object thisobj = mmsg.getObject(thisname);
                    if (thisobj instanceof Byte) {
                        System.out.print("Element: "+thisname+" Value: "+thisobj + "  ");                        
                    } else if (thisobj instanceof String) {
                        System.out.print("Element: "+thisname+" Value: "+thisobj + "  ");
                    } else if (thisobj instanceof Integer) {
                        System.out.print("Element: "+thisname+" Value: "+(Integer)thisobj +"  ");
                    }
                }
                System.out.println();
    		} else {
    		    System.out.println("Received message "+i);
    		}
    		
    		recvcount++;
    		i++;
    		
            responseT = (Topic)msg.getJMSReplyTo();
            echocount++;
            MessageProducer prod = getReplyToProd(sess, responseT);
            prod.send(msg);
    	}while(recvcount < count);
    	synchronized (SimpleEchoSubscriber.class) {
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
    	System.out.println("SimpleEchoSubscriber  topicname count");
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
    	    conn.setClientID("esub_"+Utils.uniqueString("esubscriber"));
    	    System.out.println("Client ID is: " + conn.getClientID());
    	} catch (JMSException jmse) {
    		jmse.printStackTrace(System.out);
    		return;
    	}
    	
    	/*
    	 * Start the receive threads 
    	 */
    	new Thread(new SimpleEchoSubscriber(RECV)).start();
    	
       try {
            doCheckDone();
        } catch (Exception ex) {
            ex.printStackTrace(System.out);
        }
    	
    	/*
    	 * Print out statistics
    	 */
    	synchronized (SimpleEchoSubscriber.class) {
        	System.out.println("Received "+recvcount+" messages");
		}
    	try {
    	    conn.close();
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }
    
}
