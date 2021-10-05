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
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.ExceptionListener;
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
 * Test a point-to-point send and receive with acknowledge
 * 
 * There are three positional arguments which can be set when running this test:
 * 1. The size of the message (default = 128)
 * 2. The number of messages (default = 100)
 * 3. The name of the topic (default = "RequestT")
 * 
 * The location of the IMA server can be specified using two environment variables:
 * IMAServer - The IP address of the server (default = 127.0.0.1)
 * IMAPort   - The port number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class TopicListenerTest implements ExceptionListener {
	static int  msglen = 128;    
	static int  count = 100;
	static String topicName = "RequestT";

	static ConnectionFactory   fact;
	static Connection          conn;
	static volatile boolean recvdone1;
	static volatile boolean recvdone2;
	
	static int  sendcount;
	static int  recvcount1;
	static int  recvcount2;
	
    /*
     * Send messages
     */
    public static long doSend(Session sess, MessageProducer prod) throws JMSException {
    	prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
    	conn.start();
    	//conn2.start();
    	try { Thread.sleep(100); } catch (Exception e) {}
    	
    	long startTime = System.currentTimeMillis();
    	
    	BytesMessage bmsg = sess.createBytesMessage();
    	byte [] msgbytes = new byte[msglen];
    	
    	for (int i=0; i<count; i++) {
    		bmsg.clearBody();
    		msgbytes[0] = (byte)i;
    		bmsg.writeBytes(msgbytes);
    		prod.send(bmsg);
    		sendcount++;
    		if (sendcount%500 == 0)
    			try { Thread.sleep(3);} catch (Exception e) {} 
    	}
    	
    	int wait = 800;
    	while (!(recvdone1 && recvdone2) && wait-- > 0) {
    		long sleepTime = 50;
    		long curTime = System.currentTimeMillis();
    		long wakeUpTime = curTime + sleepTime;

    		while (wakeUpTime > curTime) {
    			try { Thread.sleep(wakeUpTime - curTime); } catch (Exception e) {}
    			curTime = System.currentTimeMillis();
    		}
    	}
    	long endTime = System.currentTimeMillis();
    	if (!recvdone1) {
    		System.out.println("Recv(1) - Messages were not all received");
    	}
    	if (!recvdone2) {
            System.out.println("Recv(2) - Messages were not all received");
    	}
        return endTime-startTime;  	
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
    }
    
    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg)  {
    	if (msg != null)
            System.out.println(msg);
    	System.out.println("PtPListenerTest  msglen  count ");
    }
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	long millis = 0;
    	Session sendSess = null;
    	Session recvSess1 = null;
    	Session recvSess2 = null;
    	MessageProducer prod = null;
    	MessageConsumer cons1 = null;
    	MessageConsumer cons2 = null;
    	Topic prodT;
    	Topic consT1;
    	Topic consT2;
    	
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
    	    conn = fact.createConnection();
        	sendSess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
        	recvSess1 = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
        	recvSess2 = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
        	
        	prodT = sendSess.createTopic(topicName); 
        	consT1 = recvSess1.createTopic(topicName);
        	((ImaProperties)consT1).put("DisableACK", true);
        	consT2 = recvSess2.createTopic(topicName);
//        	((ImaProperties)consT2).put("DisableACK", true);
        	prod = sendSess.createProducer(prodT);
        	
        	conn.setExceptionListener(new TopicListenerTest());
        	
        	cons1 = recvSess1.createConsumer(consT1);
        	MessageListener msgListener1 = new TopicListenerTest().new TopicMessageListener();
        	((TopicMessageListener)msgListener1).setInstance(1);
			cons1.setMessageListener(msgListener1);
			
			cons2 = recvSess2.createConsumer(consT2);
			MessageListener msgListener2 = new TopicListenerTest().new TopicMessageListener();
            ((TopicMessageListener)msgListener2).setInstance(2);
            cons2.setMessageListener(msgListener2);
    	} catch (JMSException jmse) {
    		jmse.printStackTrace(System.out);
    		return;
    	}
    	
    	/*
    	 * Run the send
    	 */
    	try {
    	    millis = doSend(sendSess, prod);
    	} catch (Exception ex) {
    		ex.printStackTrace(System.out);
    	}
    	
    	/*
    	 * Print out statistics
    	 */
    	System.out.println("Time = "+millis);
    	System.out.println("Send = "+sendcount);
    	System.out.println("Recv(1) = "+recvcount1);
    	System.out.println("Recv(2) = "+recvcount2);
    	try {
    	    conn.close();
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }

    class TopicMessageListener implements MessageListener {
        int recvcount;
        int inst;
        boolean done = false;
        
        void setInstance(int inst) {
            this.inst = inst;
        }

        public void onMessage(Message msg) {
            if(inst == 1) {
                recvcount = recvcount1;
                done = recvdone1;
            } else {
                recvcount = recvcount2;
                done = recvdone2;
            }
            /* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
            BytesMessage bmsg = (BytesMessage)msg;
            byte [] b = new byte [1];
            try {
                bmsg.readBytes(b);
                if (bmsg.getBodyLength() != msglen || b[0] != (byte)recvcount) {
                    throw new JMSException("Message received is not the correct message: expected("+recvcount+") found("+b[0]+") expected length("+msglen+") found length("+bmsg.getBodyLength()+")");
                }
            }
            catch (JMSException jmse) {
                throw new RuntimeException(jmse);
            }
            recvcount++;
            if (recvcount == count) {
                done = true;
            }
            if(inst == 1) {
                recvcount1 = recvcount;
                recvdone1 = done;
            } else {
                recvcount2 = recvcount;
                recvdone2 = done;
            }
        }
    }

	@Override
	public void onException(JMSException arg0) {

		System.out.println("Exception - " + arg0.getMessage());
	}
}
