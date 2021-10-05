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

package com.ibm.ima.jms.impl;

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnection;


/*
 * Test an echoed send and receive with commit
 */
public class TimerTest implements Runnable {
	int  action;
	
	static int  msglen = 100;
	static int  count = 1000000;
	static int  thread_count = 1;
	

	static ConnectionFactory   fact;
	Connection          conn;
	
	int  sendcount;
	int  recvcount;
	
	static int donecount = 0;
	static long [] threadtime;
	static int [] threadcount;
	
	static Object statlock = new Object();
	
	/*
	 * Create the object which is used to create the receive threads
	 */
    public TimerTest(int action) {
    	this.action = action;
    }

    /*
     * We only get one run, but choose which function to execute in this thread
     * @see java.lang.Runnable#run()
     */
    public void run() {
    	doSend();
    }
    
    
    /*
     * 
     * Send messages
     */
    public void doSend() {
    	long startTime = System.currentTimeMillis();
    	try {
    		fact = ImaJmsFactory.createConnectionFactory();
    		String server = System.getenv("IMAServer");
     	    String port   = System.getenv("IMAPort");
    	    String transport = System.getenv("IMATransport");
    		if (server != null)
    	    	((ImaProperties)fact).put("Server", server);
    	    if (port != null)
    	    	((ImaProperties)fact).put("Port", port);
    	    if (transport != null)
    	    	((ImaProperties)fact).put("Protocol", transport);
    	    ((ImaProperties)fact).put("DisableMessageTimestamp", true);
            ((ImaProperties)fact).put("DisableMessageID", true);
    	    conn = fact.createConnection();
    	    if ("junit".equals(transport)) {
          	    ((ImaConnection)conn).client = new ImaLoopbackClient();
          	    ((ImaConnection)conn).clientid = "TestClient";
    	    }    
	    	Session sess = conn.createSession(false, 0);
	    	Queue requestQ = sess.createQueue("RequestQ"); 
	    	MessageProducer prod = sess.createProducer(requestQ);
	    	MessageConsumer cons = sess.createConsumer(requestQ);
	    	prod.setDeliveryMode(DeliveryMode.PERSISTENT);
	    	conn.start();
	       	byte [] b = new byte [1];
	    	
	    	startTime = System.currentTimeMillis();
	    	
	    	BytesMessage bmsg = sess.createBytesMessage();
	    	BytesMessage rmsg;
	    	
	    	byte [] msgbytes = new byte[msglen];
	       	bmsg.setIntProperty("fred", 4);
	       	bmsg.setDoubleProperty("pi", 3.1415926535);
	    	
	    	for (int i=0; i<count; i++) {
	    		bmsg.clearBody();
	    		msgbytes[0] = (byte)i;
	    		bmsg.writeBytes(msgbytes);
	    		prod.send(bmsg);
	    		sendcount++;
	    		
	    		rmsg = (BytesMessage)cons.receive(500);
	    		bmsg.readBytes(b);
	    		if (rmsg.getBodyLength() != msglen || b[0] != (byte)i) {
	    			throw new JMSException("Message received is not the correct message: "+i);
	    		} 
	    		recvcount++;
	    	}
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    	
    	long endTime = System.currentTimeMillis();
    	
    	try {conn.close();} catch (Exception e) {}
    	
  //  	System.out.println("thread"+action+" sendcount="+sendcount+" recvcount="+recvcount+" time="+(endTime-startTime));
       	synchronized (statlock) {
    		donecount++;
    		threadcount[action] = recvcount;
    		threadtime[action] = endTime-startTime;
    		statlock.notify();
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
    	if (args.length > 2) {
    		try { 
    			thread_count = Integer.valueOf(args[2]);
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
    	System.out.println("TimerTest  msglen  count");
    }
    
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	long millis = 0;
    	int  i;
    	
    	parseArgs(args);
    	threadtime = new long[thread_count];
    	threadcount = new int[thread_count];
    	
    	for (i=0; i<thread_count; i++) {
    		new Thread(new TimerTest(i)).start();
    	}
   
    	while (donecount < thread_count) {
    	    synchronized (statlock) {
    	    	try { statlock.wait(); } catch(InterruptedException e) {}
        	}	
    	}
    	
    	int totalcount = 0;
    	for (i=0; i<thread_count; i++) {
    		millis += threadtime[i];
    		if (threadcount[i] != count)
    			System.out.println("Count ["+i+"] = " + threadcount[i]);
    		totalcount += threadcount[i];
    	}
    	
    	/*
    	 * Print out statistics
    	 */
    	System.out.println("Threads = " + thread_count);
    	System.out.println("Time    = "+millis);
    	System.out.println("Count   = " + count);
    	System.out.println("Rate    = "+(int)((((double)totalcount/(double)millis)*thread_count)*1000.0)+ " / second");
    }
}
