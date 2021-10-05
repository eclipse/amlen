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

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


/*
 * Test a topic-based send and receive
 * 
 * There are five positional arguments which can be set when running this test:
 * 1. The size of the message (default = 128)
 * 2. The number of messages (default = 100)
 * 3. The name of the topic (default = "RequestT")
 * 4. The number of publishers on the topic (default = 1)
 * 5. The number of consumers on the topic (default = 2)
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class TopicListenerTestCfg implements Runnable {
	static int  msglen = 128;    
	static int  count = 100; //Messages published per publisher
	static String topicName = "RequestT";
	static int numConsumers = 2;
	static int numPublishers = 1;
	
	static private final String CLIENT_ID = "TopicListenerTestCfg";
	
	static ConnectionFactory   fact;
	static Connection[]        sendconn;
	static Connection[]        recvconn;
	
	static volatile boolean[] recvdone;
	static volatile boolean[] senddone;
	
	static int[]  sendcount;
	static int[]  recvcount;
	
	int senderInst=0; //Which Producer does this class represent
	Session senderSess;
	MessageProducer producer;
	
	/*
	 * Wait for all the senders and receivers to finish
	 */
	void waitForMessagesReceived() {
        final int WAIT_TIME = 50;
        int waitsPerSecond = 1000 / WAIT_TIME;
        int wait = 75 * waitsPerSecond;     		// 75 seconds
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
    public long doSend() throws JMSException {    	
    	producer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
    	try { Thread.sleep(100); } catch (Exception e) {}
    	
    	long startTime = System.currentTimeMillis();
    	
    	BytesMessage bmsg = senderSess.createBytesMessage();
    	byte [] msgbytes = new byte[msglen];
    	
    	for (int i=0; i<count; i++) {
    		bmsg.clearBody();
    		msgbytes[0] = (byte)i;
    		bmsg.writeBytes(msgbytes);
    		producer.send(bmsg);
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
    
    public TopicListenerTestCfg(int senderInst, Session session,
			MessageProducer messageProducer) {
    	//Used for creating producers
		this.senderInst = senderInst;
		this.senderSess = session;
		this.producer = messageProducer;
	}
    public TopicListenerTestCfg() {
		//Used for creating receivers
	}
	public void run() {
		try {
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
    		topicName = args[2];
    	}
    	if(args.length > 3) {
    		try {
    			numPublishers = Integer.valueOf(args[3]);
    		} catch (Exception e) {
    			syntaxhelp("");
    			return;
    		}
    	}    	
    	if(args.length > 4) {
    		try {
    			numConsumers = Integer.valueOf(args[4]);
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
    	System.out.println("TopicListenerTestCfg  msglen  count [numProducers] [numConsumers] ");
    }
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	long millis = 0;
    	Session[] sendSess = null;
    	Session[] recvSess = null;
    	MessageProducer[] prod = null;
    	MessageConsumer[] cons = null;
    	Topic[] prodT;
    	Topic[] consT;
    	
    	parseArgs(args);
    	/*
    	 * Create connection factory and connection
    	 */
    	try {
    	    fact = ImaJmsFactory.createConnectionFactory();
    	    String server = System.getenv("IMAServer");
    	    String port   = System.getenv("IMAPort");
    	    String transport = System.getenv("IMATransport");
    	    String recvBuffer = System.getenv("IMARecvBuffer");
    	    String sendBuffer = System.getenv("IMASendBuffer");
            if (server == null)
                server = "127.0.0.1";
            if (port == null)
                port = "16102";
            ((ImaProperties)fact).put("Server", server);
            ((ImaProperties)fact).put("Port", port);
    	    if (transport != null)
    	    	((ImaProperties)fact).put("Protocol", transport);
    	    if (recvBuffer != null)
    	    	((ImaProperties)fact).put("RecvBuffer", recvBuffer);
    	    if (sendBuffer != null)
    	    	((ImaProperties)fact).put("SendBuffer", sendBuffer);

    	    ((ImaProperties)fact).put("DisableMessageTimestamp", true);
            ((ImaProperties)fact).put("DisableMessageID", true);
            
            /*Setup the arrays that depend on the number of consumers */
            recvcount = new int[numConsumers];
            recvdone = new boolean[numConsumers];
            recvconn = new Connection[numConsumers];
            recvSess = new Session[numConsumers];
            consT    = new Topic[numConsumers];
            cons     = new MessageConsumer[numConsumers];
            
            /*Setup the arrays that depend on the number of producers */
        	senddone = new boolean[numPublishers];
        	sendcount = new int[numPublishers];
        	sendconn = new Connection[numPublishers];
            sendSess = new Session[numPublishers];
        	prodT = new Topic[numPublishers];   
        	prod = new MessageProducer[numPublishers];
            
    	    for(int i = 0; i < numPublishers; i++) {
    	    	senddone[i] = false;
    	    	((ImaProperties)fact).put("ClientID", CLIENT_ID + "_SEND_" + i);
        	    sendconn[i] = fact.createConnection();
    	    	sendSess[i] = sendconn[i].createSession(false, Session.AUTO_ACKNOWLEDGE);
            	prodT[i] = sendSess[i].createTopic(topicName); 
            	prod[i] = sendSess[i].createProducer(prodT[i]);    	    
            }
        	for(int i = 0; i < numConsumers; i++) {
        		recvdone[i] = false;
        		((ImaProperties)fact).put("ClientID", CLIENT_ID + "_RECV_" + i);
        		
        		recvconn[i] = fact.createConnection();
        		recvSess[i] = recvconn[i].createSession(false, Session.CLIENT_ACKNOWLEDGE);

        		consT[i] = recvSess[i].createTopic(topicName);
//        		((ImaProperties)consT[i]).put("DisableACK", true);
        		
        		cons[i] = recvSess[i].createConsumer(consT[i]);
        		
            	MessageListener msgListener = new TopicListenerTestCfg().new TopicMessageListener();
            	((TopicMessageListener)msgListener).setInstance(i);
    			cons[i].setMessageListener(msgListener);
        	}
			
    	} catch (JMSException jmse) {
    		jmse.printStackTrace(System.out);
    		return;
    	}
    	
    	/*
    	 * Start the connections
    	 */
    	try {
    		for(int i=0; i < numPublishers; i++) {
    			sendconn[i].start();
    		}
    		for(int i=0; i < numConsumers; i++) {
    			recvconn[i].start();
    		}
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    	
    	/*
    	 * If we have multiple senders, start threads to
    	 * do all but one send...
    	 */
    	for(int i=1; i<numPublishers; i++) {
    		new Thread(new TopicListenerTestCfg(i, sendSess[i], prod[i])).start();
    	}
    	
    	/* Run a sender on the main thread... */
    	try {
    		TopicListenerTestCfg t = new TopicListenerTestCfg(0, sendSess[0], prod[0]);
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
    			sendconn[i].close();
    		}
    		for(int i=0; i < numConsumers; i++) {
    			recvconn[i].close();
    		}
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }

    class TopicMessageListener implements MessageListener {
        int inst;
        
        void setInstance(int inst) {
            this.inst = inst;
        }

        public void onMessage(Message msg) {
            /* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
            BytesMessage bmsg = (BytesMessage)msg;
            try {
                msg.acknowledge();
            } catch (JMSException e) {
                e.printStackTrace();
            }
            byte [] b = new byte [1];
            try {
                bmsg.readBytes(b);
                
                //If there are multiple producers we can't tell which message we should be receiving
                //So only check in the case there is 1 producer
                if(numPublishers == 1) {
                	if (bmsg.getBodyLength() != msglen || b[0] != (byte)recvcount[inst]) {
                		throw new JMSException("Message received is not the correct message: expected("+recvcount[inst]+") found("+b[0]+") expected length("+msglen+") found length("+bmsg.getBodyLength()+")");
                	}
                }
            }
            catch (JMSException jmse) {
                throw new RuntimeException(jmse);
            }
            recvcount[inst]++;
            if (recvcount[inst] == numPublishers * count) {
                recvdone[inst] = true;
                System.out.println("Receiver "+inst+" complete");
            }
        }
    }
}
