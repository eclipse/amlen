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

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttMessage;


/*
 * Test an echoed MQTT send and receive
 * 
 * There one argument which can be set when running this test:
 * 1. The number of messages (default = 100)
 * 2. (Optional) QoS level (default = 0)
 * 
 * The location of the ISM server can be specified using two environment variables:
 * ISMServer - The IP address of the server (default = 127.0.0.1)
 * ISMPort   - The MQTT port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class MqttEchoTest implements Runnable{
	int  action;
	static final int RECV = 1;
	static final int ECHO = 2;
	static String server = null;
	static String port = null;
	static boolean requestTSubscribed = false;
	static boolean responseTSubscribed = false;
	 
	static int  count = 10000;
	static int  qos   = 0;
	
	static boolean recvdone;
	static boolean echodone;
	
	static int  sendcount = 0;
	static int  echocount = 0;
	static int  recvcount = 0;
	
	
	/*
	 * Create the object which is used to create the receive threads
	 */
    public MqttEchoTest(int action) {
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
    public static long doSend() throws IOException {
    	IsmMqttConnection conn = new IsmMqttConnection(server, Integer.parseInt(port), "/mqtt", "testsend");
    	conn.setEncoding(IsmMqttConnection.BINARYWS);
    	conn.connect();
    	conn.setVerbose(false);
    	
    	// Assure no messages are sent until topic is subscribed.  Otherwise, program will appear to hang.
    	while (!requestTSubscribed) {
    		try { Thread.sleep(5); } catch (Exception e) {}
    	}
    	
    	long startTime = System.currentTimeMillis();
    	
    	String msgstr = new String("This is message ");
    	
    	for (int i=0; i<count; i++) {
    		conn.publish("RequestT", msgstr+i, qos, false);
    		synchronized (MqttEchoTest.class) {
        		sendcount++;
        		if (sendcount%500 == 0)
        			try { MqttEchoTest.class.wait(3);} catch (Exception e) {} 
			}
    	}
    	synchronized (MqttEchoTest.class) {
        	int wait = 15000;
        	while ((wait-- > 0) && (!recvdone || !echodone)) {
        		try { MqttEchoTest.class.wait(50); 
        		} catch (Exception e) {
        		    //System.out.println("wait interrupted");
        		}
        	}
        	long endTime = System.currentTimeMillis();
        	if (!recvdone) {
        		System.out.println("Messages were not all received");
        	}
            if (!echodone) {
            	System.out.println("Messages were not all echoed");
            }
    		conn.disconnect();
            return endTime-startTime;  	
    	}
    }
    
    /*
     * Receive messages
     */
    public void doReceive() throws IOException {
    	IsmMqttConnection conn = new IsmMqttConnection(server, Integer.parseInt(port), "/mqtt", "testrecv");
    	conn.setEncoding(IsmMqttConnection.BINARY);
    	conn.connect();
    	conn.setVerbose(false);
    	conn.subscribe("ResponseT", qos);
    	// Tell sender that the subscriber is ready to receive on this topic
    	responseTSubscribed = true;
    	int i = 0;
    	
    	do {
    		//IsmMqttMessage msg = conn.receive(500);
    		IsmMqttMessage msg = conn.receive();
    		if (msg == null) {
    			throw new RuntimeException("Timeout in receive message: "+i);
    	    }
      		/* Validate the message */
    		String msgstr = msg.getPayload();
    		if (!msgstr.equals("This is message "+i)) {
    			conn.unsubscribe("ResponseT");
    			conn.disconnect();
    			throw new RuntimeException("Message received is not the correct message.  Expected: This is message "+i+" Found: "+msgstr);
    		}	
    		recvcount++;
    		i++;
    	}while(recvcount < count);
    	synchronized (MqttEchoTest.class) {
    		recvdone = true;
    		conn.unsubscribe("ResponseT");
    		conn.disconnect();
    		if(recvcount < count){
    			System.err.println("doReceive: Not all messages were received: " + recvcount);
    		}
    	}
    }
    
    /*
     * Echo messages
     */
    public void doEcho() throws IOException {
    	IsmMqttConnection conn = new IsmMqttConnection(server, Integer.parseInt(port), "/mqtt", "testecho");
    	conn.setEncoding(IsmMqttConnection.BINARY);
    	conn.connect();
    	conn.setVerbose(false);
    	conn.subscribe("RequestT", qos);
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
      		/* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
    		msgstr = msg.getPayload();
    		if (!msgstr.equals("This is message "+i)) {
    			conn.unsubscribe("RequestT");
        		conn.disconnect();
    			throw new RuntimeException("Message echoed is not the correct message.  Expected: This is message "+i+" Found: "+msgstr);
    		} 
    		echocount++;
    		conn.publish("ResponseT", msgstr, qos, false);
    		i++;
    	}while(echocount < count);
    	synchronized (MqttEchoTest.class) {
    		echodone = true;
    		conn.unsubscribe("RequestT");
    		conn.disconnect();
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
    			count = Integer.valueOf(args[0]);
    		} catch (Exception e) {
    			syntaxhelp("");
    			return;
    		}
        }
    	if (args.length > 1) {
    		try { 
    			qos = Integer.valueOf(args[1]);
    		} catch (Exception e) {
    			syntaxhelp("");
    			return;
    		}
        }    	
    	if (args.length > 2) {
    		syntaxhelp("Too many command line arguments");
        }
    }
    
    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg)  {
    	if (msg != null)
            System.out.println(msg);
    	System.out.println("MqttEchoTest  count ");
    }
    
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	long millis = 0;
    	
    	parseArgs(args);

    	/*
    	 * Get server info
    	 */
	    server = System.getenv("IMAServer");
	    if (server == null) {
	        server = "127.0.0.1";
	    }
	    port   = System.getenv("IMAPort");
	    if (port == null) {
	        port = "16102";
	    }
  	
    	/*
    	 * Start the receive threads 
    	 */
    	new Thread(new MqttEchoTest(RECV)).start();
    	new Thread(new MqttEchoTest(ECHO)).start();
    	
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
    	synchronized (MqttEchoTest.class) {
        	System.out.println("Time = "+millis);
        	System.out.println("Send = "+sendcount);
        	System.out.println("Echo = "+echocount);
        	System.out.println("Recv = "+recvcount);
		}
    }
    
}
