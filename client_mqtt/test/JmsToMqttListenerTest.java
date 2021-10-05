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
import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListener;
import com.ibm.ism.mqtt.IsmMqttMessage;


/*
 * Test JMS send to MQTT asyn receive
 * 
 * There are four positional arguments which can be set when running this test:
 * 1. The number of messages per sender (default = 100)
 * 2. The name of the topic (default = "RequestT")
 * 3. The number of JMS senders on the topic (default = 1)
 * 4. The number of MQTT receivers on the topic (default = 1)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer    - The IP address of the server (default = 127.0.0.1)
 * IMAJmsPort   - The JMS port number of the server (default = 16102)
 * IMAMqttPort  - The MQTT port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class JmsToMqttListenerTest implements Runnable {
	static final int SENDER = 1;
	static final int RECEIVER = 2;
	    
	static int  count = 100; //Messages published per publisher
	static String topicName = "RequestT";
	static int numConsumers = 1;
	static int numPublishers = 1;
	
	static ConnectionFactory   fact;
	static Connection[]        sendconn;
	static IsmMqttConnection[]        recvconn;
	
	static volatile boolean[] recvdone;
	static volatile boolean[] senddone;
	
	static int[]  sendcount;
	static int[]  recvcount;
	
	int insttype=0; //Sender or receiver
	int senderInst=0; //Which sender does this class represent
	int receiverInst=0; //Which receiver does this class represent
	IsmMqttConnection conn;
	MessageProducer producer;
	Session senderSess;
	
	/*
	 * Wait for all the senders and receivers to finish
	 */
	void waitForMessagesReceived() {
    	int wait = 15000;    		
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
				try { Thread.sleep(50); } catch (Exception e) {}
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
    public long doSend() throws JMSException {    	
        producer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        try { Thread.sleep(100); } catch (Exception e) {}
        
        long startTime = System.currentTimeMillis();
        
        TextMessage tmsg = senderSess.createTextMessage();
    	
    	for (int i=0; i<count; i++) {
    	    tmsg.clearBody();
            tmsg.setText("This is message "+i);
    	    
            producer.send(tmsg);
    		sendcount[senderInst]++;
    		if (sendcount[senderInst]%500 == 0)
    			try { Thread.sleep(3);} catch (Exception e) {} 
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
    
    public JmsToMqttListenerTest(int insttype, int inst, IsmMqttConnection conn) {
    	//Used for creating producers
    	this.insttype = insttype;
    	if(insttype == SENDER)
    		this.senderInst = inst;
    	else // InstType == RECEIVER
    		this.receiverInst = inst;
		this.conn = conn;
	}
    
    public JmsToMqttListenerTest(int senderInst, Session session,
            MessageProducer messageProducer) {
        //Used for creating producers
        this.insttype = SENDER;
        this.senderInst = senderInst;
        this.senderSess = session;
        this.producer = messageProducer;
    }
    
    
    public JmsToMqttListenerTest() {
		//Used for creating receivers
	}
	public void run() {
		try {
			if(insttype == SENDER)
				doSend();
		} catch (JMSException e) {
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
    			numPublishers = Integer.valueOf(args[2]);
    		} catch (Exception e) {
    			syntaxhelp("");
    			return;
    		}
    	}    	
    	if(args.length > 3) {
    		try {
    			numConsumers = Integer.valueOf(args[3]);
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
    	System.out.println("JmsToMqttListenerTest  count [numProducers] [numConsumers] ");
    }
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	long millis = 0;
    	Session[] sendSess = null;
        MessageProducer[] prod = null;
        Topic[] prodT;
    	
    	parseArgs(args);
    	/*
    	 * Create connections
    	 */
    		int jmsport;
    		int mqttport;
    		
    	    String server = System.getenv("IMAServer");
    	    String jmsportstr   = System.getenv("IMAJmsPort");
    	    String mqttportstr   = System.getenv("IMAMqttPort");
            if (server == null) server = "127.0.0.1";
            if (jmsportstr == null) 
                jmsport = 16102;
            else
                jmsport = Integer.parseInt(jmsportstr);
            if (mqttportstr == null) 
            	mqttport = 16102;
            else
            	mqttport = Integer.parseInt(mqttportstr);
           
            
            /*Setup the arrays that depend on the number of consumers */
            recvcount = new int[numConsumers];
            recvdone = new boolean[numConsumers];
            recvconn = new IsmMqttConnection[numConsumers];
            
            /*Setup the arrays that depend on the number of producers */
        	senddone = new boolean[numPublishers];
        	sendcount = new int[numPublishers]; 
        	sendconn = new Connection[numPublishers];
            sendSess = new Session[numPublishers];
            prodT = new Topic[numPublishers];   
            prod = new MessageProducer[numPublishers];
       
            try {
                fact = ImaJmsFactory.createConnectionFactory();
                ((ImaProperties)fact).put("Server", server);
                ((ImaProperties)fact).put("Port", jmsport);
            
                for(int i = 0; i < numPublishers; i++) {
                    sendconn[i] = fact.createConnection();
                    sendconn[i].setClientID("send"+i);
                    sendSess[i] = sendconn[i].createSession(false, Session.AUTO_ACKNOWLEDGE);
                    prodT[i] = sendSess[i].createTopic(topicName); 
                    prod[i] = sendSess[i].createProducer(prodT[i]);         
                } 
            } catch (JMSException jmse){
                throw new RuntimeException("Failed creating JMS senders.  Cannot continue.",jmse);
            }
    	    
        	for(int i = 0; i < numConsumers; i++) {
        		recvconn[i] = new IsmMqttConnection(server, mqttport, "/mqtt", "recvLstnrClient"+i);
        		recvconn[i].setEncoding(IsmMqttConnection.BINARY);
        		recvconn[i].setVerbose(false);
        	}
    	
    	/*
    	 * Start the connections
    	 */
    	try {
    		for(int i=0; i < numPublishers; i++) {
    			sendconn[i].start();
    		}
    		for(int i=0; i < numConsumers; i++) {
        		JmsToMqttListenerTest recvInst = new JmsToMqttListenerTest(RECEIVER, i, recvconn[i]);
        		MqttTopicMessageListener listener = recvInst.new MqttTopicMessageListener(i);
        		recvconn[i].setListener(listener);
    			recvconn[i].connect();
    			recvconn[i].subscribe(topicName, 0); 			
    		}
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    	
    	/*
    	 * If we have multiple senders, start threads to
    	 * do all but one send...
    	 */
    	for(int i=1; i<numPublishers; i++) {
    		new Thread(new JmsToMqttListenerTest(i, sendSess[i], prod[i])).start();
    	}
    	
    	/* Run a sender on the main thread... */
    	try {
    		JmsToMqttListenerTest t = new JmsToMqttListenerTest(0, sendSess[0], prod[0]);
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
    			//sendconn[i].unsubscribe(topicName);
    			sendconn[i].close();
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
    		System.out.println("Disconnect: rc="+rc);
    		if (e != null) 
    			e.printStackTrace(System.out);
    			
    	}
        
        public void onMessage(IsmMqttMessage msg) {
            String msgstr = msg.getPayload();
            
            /* Validate the message */
            //If there are multiple producers we can't tell which message we should be receiving
            //So only check in the case there is 1 producer
            if(numPublishers == 1) {
                if (!msgstr.equals("This is message "+recvcount[receiverInst])) {
                    throw new RuntimeException("Message received is not the correct message: expected(This is message "+recvcount[receiverInst]+") found("+msgstr+")");
                }
            }
            
            recvcount[receiverInst]++;
            if (recvcount[receiverInst] == numPublishers * count) {
                recvdone[receiverInst] = true;
                System.out.println("Receiver "+receiverInst+" complete");
            }
        }
    }
}
