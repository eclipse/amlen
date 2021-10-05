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
import java.util.Random;

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListener;
import com.ibm.ism.mqtt.IsmMqttMessage;

/*
 * Test MQTT send and receive
 * 
 * There are four positional arguments which can be set when running this test:
 * 1. The number of messages to send from each sender (default = 100)
 * 2. The name of the topic (default = "RequestT")
 * 3. The number of receivers on the topic (default = 1)
 * 4. The QoS (default = 0)
 * 5. Number of messages before disconnect/connect (default = 10)
 * 
 * For QoS 0, not disconnect/re-connects are done.
 * For QoS 1, all un-acked messages are discarded on disconnect.
 * For QoS 2, all sent messages have to be received prior to disconnect.  
 * 
 * The location of the ISM server can be specified using two environment variables:
 * ISMServer - The IP address of the server (default = 127.0.0.1)
 * ISMPort   - The port MQTT number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class DurableMqttListenerTest implements Runnable {
	static final int SENDER = 1;
	static final int RECEIVER = 2;
	    
	static int  count = 100; //Messages published per publisher
	static String topicName = "RequestT";
	static int numConsumers = 1;
	static int numPublishers = 1;
	
	static IsmMqttConnection[]        sendconn;
	static IsmMqttConnection[]        recvconn;
	
	static volatile boolean[] recvdone;
	static volatile boolean[] senddone;
	static volatile boolean[] reconnect;
	
	static int[]  sendcount;
	static int[]  recvcount;
	
	static int  qos = 0;

	static int  reconnectPeriod = 10;
	
	int insttype=0; //Sender or receiver
	int senderInst=0; //Which sender does this class represent
	int receiverInst=0; //Which receiver does this class represent
	IsmMqttConnection conn;
	
	/*
	 * Wait for all the senders and receivers to finish
	 */
	void waitForMessagesReceived() {
        final int WAIT_TIME = 50;
        int waitsPerSecond = 1000 / WAIT_TIME;
        int wait = 10 * waitsPerSecond;     		// 75 seconds
    	boolean allDone = false;
    	
		while (wait-- > 0 && !allDone) {
			allDone = true;
			//Check whether all 
  			for(int i = 0; i < senddone.length; i++) {
				if(!senddone[i]) {
					allDone = false;
					break;
				}
			}
  			if(allDone) {
  				for(int i = 0; i < recvdone.length; i++) {
  					if(!recvdone[i]) {
  						allDone = false;
  						break;
  					}
  				}
  			}
			if(!allDone) {
				long curTime = System.currentTimeMillis();
				long wakeUpTime = curTime + WAIT_TIME;
				while (wakeUpTime - curTime > 0) {
					try {
						Thread.sleep(wakeUpTime - curTime);
					} catch (InterruptedException ex) { }
					curTime = System.currentTimeMillis();
				}
			}
		}
		
		for(int i=0; i<recvdone.length; i++) {
			if(!recvdone[i]) {
				System.out.println("Recv("+i+") - Messages were not all received");
			}
		}
	}
	
	/*
     * Send messages
     */
    public long doSend() throws IOException {    	
    	try { Thread.sleep(100); } catch (Exception e) {}
    	
    	Random generator = new Random();
    	
    	long startTime = System.currentTimeMillis();
    	
    	for (int i=0; i<count; i++) {
    		conn.publish(topicName, "This is message "+i, qos, false);
    		sendcount[senderInst]++;
    		if (sendcount[senderInst]%500 == 0) {
    			try { Thread.sleep(3);} catch (Exception e) {}
    		}

    		if (qos > 0 && sendcount[senderInst] < numPublishers * count) {

    			if (sendcount[senderInst]%reconnectPeriod == 0) {
    				int cons = generator.nextInt(numConsumers);
    				System.out.println("Disconnecting and reconnecting consumer #" + 
    						cons + " at count " + recvcount[cons] + 
    						" (" + sendcount[senderInst] + ")");
    				
    				synchronized (DurableMqttListenerTest.class) {
    					reconnect[cons] = false;
    					recvconn[cons].disconnect();
    				}
   					System.out.println("Disconnected: " + (sendcount[senderInst] - recvcount[cons]));

   					synchronized (DurableMqttListenerTest.class) {
   						while (!reconnect[cons])
   							try { DurableMqttListenerTest.class.wait(50); } catch (InterruptedException e) { }
   					}
       				recvconn[cons].connect();
   					recvcount[cons] = sendcount[senderInst];
       				System.out.println("Connected: " + (sendcount[senderInst] - recvcount[cons]));
    			}
    		}
    	}
    	
    	senddone[senderInst] = true;
    	System.out.println("Done sender "+senderInst);
    	
    	if(senderInst == 0) {
    		//We're running on the main thread and we'll wait for all the other senders and
    		//receivers to finish
    		waitForMessagesReceived();
    	}
    	long endTime = System.currentTimeMillis();
        return endTime-startTime;  	
    }
    
    public DurableMqttListenerTest(int insttype, int inst, IsmMqttConnection conn) {
    	this.insttype = insttype;
    	if(insttype == SENDER)
    		this.senderInst = inst;
    	else // InstType == RECEIVER
    		this.receiverInst = inst;
		this.conn = conn;
	}
    
	public void run() {
		try {
			if(insttype == SENDER)
				doSend();
		} catch (IOException e) {
			e.printStackTrace();
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
    		topicName = args[1];
    	}
    	if(args.length > 2) {
    		try {
    			numConsumers = Integer.valueOf(args[2]);
    		} catch (Exception e) {
    			syntaxhelp("");
    			return;
    		}
    	}
    	
    	if(args.length > 3) {
    		try {
    			qos = Integer.valueOf(args[3]);
    		} catch (Exception e) {
    			syntaxhelp("");
    			return;
    		}
    	}
    	if(args.length > 4) {
    		try {
    			reconnectPeriod = Integer.valueOf(args[4]);
    		} catch (Exception e) {
    			syntaxhelp("");
    			return;
    		}
    	}    	
    	
    	if(args.length > 5) {
    		syntaxhelp("Too many command line arguments");
    	}
    }

    
    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg)  {
    	if (msg != null)
            System.out.println(msg);
    	System.out.println("DurableMqttListenerTest  count [topic] [numConsumers] [qos] [reconnectPeriod]");
    }
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	long millis = 0;
    	
    	parseArgs(args);

    	int port;
		
	    String server = System.getenv("ISMServer");
	    String portstr   = System.getenv("ISMPort");
	    String protocol   = "mqtt";
        if (server == null) server = "127.0.0.1";
        if (portstr == null) 
        	port = 16102;
        else
        	port = Integer.parseInt(portstr);
           
        /*Setup the arrays that depend on the number of consumers */
        recvcount = new int[numConsumers];
        recvdone = new boolean[numConsumers];
        reconnect = new boolean[numConsumers];
        recvconn = new IsmMqttConnection[numConsumers];
        
        /*Setup the arrays that depend on the number of producers */
    	senddone = new boolean[numPublishers];
    	sendcount = new int[numPublishers];
    	sendconn = new IsmMqttConnection[numPublishers];     
    	
    	/*
    	 * Create connections
    	 */
	    for(int i = 0; i < numPublishers; i++) {
    	    sendconn[i] = new IsmMqttConnection(server, port, "/"+protocol, "durSendLstnrClient"+i);
//    	    sendconn[i].setEncoding(IsmMqttConnection.BINARY);
    	    sendconn[i].setVerbose(false);
        }
    	for(int i = 0; i < numConsumers; i++) {
    		recvconn[i] = new IsmMqttConnection(server, port, "/"+protocol, "durRecvLstnrClient"+i);
//    		recvconn[i].setEncoding(IsmMqttConnection.BINARY);
    		recvconn[i].setVerbose(false);
    		recvconn[i].setClean(false);
    	}
    	
    	/*
    	 * Start the connections
    	 */
    	try {
    		for(int i=0; i < numPublishers; i++) {
    			sendconn[i].connect();
    		}
    		for(int i=0; i < numConsumers; i++) {
        		DurableMqttListenerTest recvInst = new DurableMqttListenerTest(RECEIVER, i, recvconn[i]);
        		MqttTopicMessageListener listener = recvInst.new MqttTopicMessageListener(i);
        		recvconn[i].setListener(listener);
    			recvconn[i].connect();
    			recvconn[i].subscribe(topicName, qos); 			
    		}
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    	
    	/*
    	 * If we have multiple senders, start threads to
    	 * do all but one send...
    	 */
    	for(int i=1; i<numPublishers; i++) {
    		new Thread(new DurableMqttListenerTest(SENDER, i, sendconn[i])).start();
    	}
    	
    	/* Run a sender on the main thread... */
    	try {
    		DurableMqttListenerTest t = new DurableMqttListenerTest(SENDER, 0, sendconn[0]);
    	    millis = t.doSend();
    	} catch (Exception ex) {
    		ex.printStackTrace(System.out);
    	}
    	
    	/*
    	 * Print out statistics
    	 */
    	System.out.println("Time = "+millis);
    	for(int i = 0; i < sendcount.length; i++) {
    	    System.out.println("Send("+i+") = "+sendcount[i]);
    	}
    	for(int i = 0; i < recvcount.length; i++) {
    		System.out.println("Recv("+i+") = "+recvcount[i]);
    	}
    	
    	try {
    		for(int i=0; i < numPublishers; i++) {
    			sendconn[i].disconnect();
    		}
    		for(int i=0; i < numConsumers; i++) {
    			recvconn[i].unsubscribe(topicName);
    			recvconn[i].disconnect();
    		}
    	} catch (Exception e) {
    		//e.printStackTrace();
    	}
    }

    class MqttTopicMessageListener implements IsmMqttListener {
    	int inst;

    	MqttTopicMessageListener(int inst) {
    		this.inst = inst;
    	}

    	public void onDisconnect(int rc, Throwable e) {
    		synchronized (DurableMqttListenerTest.class) {
    			reconnect[inst] = true;
    		}
    		System.out.println("Disconnect(" + inst + "): rc="+rc);
//    		if (e != null) 
//    			e.printStackTrace(System.out);  			
    	}

    	public void onMessage(IsmMqttMessage msg) {          
    		String msgstr = msg.getPayload();

    		/* Validate the message */
    		//If there are multiple producers we can't tell which message we should be receiving
    		//So only check in the case there is 1 producer
    		if(numPublishers == 1) {
    			if (!msgstr.equals("This is message "+recvcount[inst])) {
    				throw new RuntimeException("Message received is not the correct message: expected(This is message "+recvcount[inst]+") found("+msgstr+")");

//    				System.out.println(("durRecvLstnrClient"+inst) + 
//    						" - Message received is not the correct message: expected(This is message "+recvcount[inst]+") found("+msgstr+")");
    			}
    		}

    		recvcount[inst]++;
//    		System.out.println("onMessage: " + msg.getPayload() + " - " + ("durRecvLstnrClient"+inst) + " - " + recvcount[inst]);    		

    		if (recvcount[inst] == numPublishers * count) {
    			recvdone[inst] = true;
    			System.out.println("Receiver "+inst+" complete");
    		}
    	}
    }
}
