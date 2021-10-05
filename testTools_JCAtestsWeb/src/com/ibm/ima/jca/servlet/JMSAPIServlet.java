/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.jca.servlet;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.QueueBrowser;
import javax.jms.QueueConnection;
import javax.jms.QueueConnectionFactory;
import javax.jms.QueueReceiver;
import javax.jms.QueueSender;
import javax.jms.QueueSession;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.Queue;
import javax.jms.TopicConnection;
import javax.jms.TopicConnectionFactory;
import javax.jms.TopicPublisher;
import javax.jms.TopicSession;
import javax.jms.TopicSubscriber;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.naming.InitialContext;
import javax.naming.NamingException;

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.ImaSubscription;

/**
 * Just a servlet that calls various JMS
 * send and receive API's in the topic, queue, shared and unified domains.
 * Each type of connection operates on its own thread.
 * 
 * Error checking isn't really done beyond catching exceptions.
 * Objects are successfully created or it fails.
 * 
 */
@WebServlet("/JMSAPIServlet")
public class JMSAPIServlet extends HttpServlet {
    private static final long serialVersionUID = 1L;
    
    private ConnectionFactory basecf;
    private ConnectionFactory cf;
    private ConnectionFactory clientidcf;
    private TopicConnectionFactory tcf;
    private QueueConnectionFactory qcf;
    
    private Topic topic;
    private Topic sharedtopic;
    private Queue queue;
    private Destination dest;
    
    private ServletOutputStream o;
    
    /**
     * @see HttpServlet#HttpServlet()
     */
    public JMSAPIServlet() {
        super();
    }
    
    /**
     * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
     */
    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
        o = response.getOutputStream();
        o.println("[JMSAPIServlet] Processing GET request ... ");                       
        
        String hostName = parseURLParam(request.getQueryString());
        if(hostName == null)
            throw new RuntimeException("Test number not provided, url params: " + request.getQueryString());
        
        runTests(hostName);
    }
    
    private String parseURLParam(String params) throws IOException {
        if(params == null)
            return null;
        Matcher m = Pattern.compile("param=([\\w]+)").matcher(params);
        if(!m.find())
            return null;
        return m.group(1);
    }
    
    private void runTests(String hostName) throws IOException {
        try {
            lookupJNDI(hostName);
            
        	Thread[] threads = new Thread[4];
        	QueueAPITester qt = new QueueAPITester();
        	threads[0] = new Thread(qt);
        	TopicAPITester tt = new TopicAPITester();
        	threads[1] = new Thread(tt);
        	UnifiedAPITester ut = new UnifiedAPITester();
        	threads[2] = new Thread(ut);
        	SharedSubOutboundAPITester st = new SharedSubOutboundAPITester();
        	threads[3] = new Thread(st);
			
			for (Thread t : threads) {
				t.start();
			}
        	
        	while (!qt.isDone())
        	    Thread.sleep(250);
        	while (!tt.isDone())
        	    Thread.sleep(250);
        	while (!ut.isDone())
        	    Thread.sleep(250);
        	while (!st.isDone())
        		Thread.sleep(250);
        	o.println(ut.buf.toString());
        	o.println(tt.buf.toString());
        	o.println(qt.buf.toString());
        	o.println(st.buf.toString());
        	o.println("JMSAPITester Finished");
        } catch (Exception e) {
            o.println("JMSAPITester FAILED");
            e.printStackTrace();
        }
        
    }
    
    private void lookupJNDI(String hostName) throws NamingException, JMSException {
    	// need a way to know whether these should have the java:/ prefix for JBoss
        Hashtable<String, String> env = new Hashtable<String, String>();
        InitialContext context = new InitialContext(env);
        basecf = (ConnectionFactory) context.lookup("JMS_BASE_CF");
        cf = (ConnectionFactory) context.lookup("JMS_API_CF");
        tcf = (TopicConnectionFactory) context.lookup("JMS_API_TCF");
        qcf = (QueueConnectionFactory) context.lookup("JMS_API_QCF");
        clientidcf = (ConnectionFactory) context.lookup("JMS_CLIENTID_CF");
        topic = (Topic) context.lookup("jmsapiservlettopic");
        sharedtopic = (Topic) context.lookup("jmsapiservletsharedtopic");
        queue = (Queue) context.lookup("jmsapiservletqueue");
        dest = (Destination) context.lookup("jmsapiservletdest");
    }
    
    private void processMsg(String caller, StringBuffer buf, Message msg) throws IOException, JMSException {
        buf.append(caller + " received >> " + ((msg instanceof TextMessage) ? ((TextMessage)msg).getText() : "something went wrong") + '\n');
    }
    
    public void trace(StringBuffer buf, String msg) {
        buf.append(msg + '\n');
    }
    
    public class QueueAPITester implements Runnable {
        QueueConnection qc;
        QueueSession qs;
        TextMessage m;
        
        ArrayList<QueueSender> senders;
        ArrayList<QueueReceiver> receivers;
        ArrayList<QueueBrowser> browsers;
        
        StringBuffer buf;
        boolean done;
        
        public QueueAPITester() {
            senders = new ArrayList<QueueSender>();
            receivers = new ArrayList<QueueReceiver>();
            browsers = new ArrayList<QueueBrowser>();
            done = false;
            buf = new StringBuffer();
        }
        
        public void run() {
            try {
                qc = qcf.createQueueConnection();
                try {
                    /* Apparently it is illegal to call setClientID on a Java EE client application
                     * connection */
                    qc.setClientID("jmsapiclient3");
                    trace(buf, "FAILED: QueueConnection setClientID completed");
                } catch (JMSException e) {
                    trace(buf, "QueueConnection correctly failed to call setClientID");
                }
                trace(buf, "QueueConnection created");
                
                qs = qc.createQueueSession(true, Session.AUTO_ACKNOWLEDGE);
                m = qs.createTextMessage("Sample for queue");
                
                createSenders();
                createReceivers();
                
                sendMessages();
                receiveMessages();
                
                closeReceivers();
                qc.close();
                
                trace(buf, "QueueAPITester complete");
                done = true;
            } catch (Exception e) {
                e.printStackTrace();
                try {
                    if (qc != null)
                        qc.close();
                    trace(buf, "QueueAPITester FAILED!");
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
                done = true;
            }
        }
        
        public void createSenders() throws JMSException {
            senders.add(qs.createSender(queue));
            senders.add(qs.createSender(null));
        }
        
        public void createReceivers() throws JMSException {
            receivers.add(qs.createReceiver(queue));
            receivers.add(qs.createReceiver(queue, "true"));
            browsers.add(qs.createBrowser(queue));
            browsers.add(qs.createBrowser(queue, "true"));
        }
        
        public void closeReceivers() throws JMSException {
        	for (QueueReceiver qr : receivers) {
        		qr.close();
        	}
        }
        private void sendMessages() throws JMSException, IOException {
            for (int i=0; i<10; i++) {
                for (QueueSender qs : senders) {
                    if (qs.getDestination() != null) {
                        qs.send(m);
                        trace(buf, "Message sent with basic QueueSender send API");
                        qs.send(m, DeliveryMode.NON_PERSISTENT, 5, 0);
                        trace(buf, "Message sent with multiple parameter QueueSender send API");
                    } else {
                        qs.send(queue, m);
                        trace(buf, "Message sent with basic anonymous QueueSender send API");
                        qs.send(queue, m, DeliveryMode.NON_PERSISTENT, 5, 0);
                        trace(buf, "Message sent with multiple parameter anonymous QueueSender send API");
                    }
                }
            }
        }
        
        private void receiveMessages() throws JMSException, IOException {
            Message msg = null;
            for (QueueBrowser qb : browsers) {
                Enumeration<?> e = qb.getEnumeration();
                if (e.hasMoreElements()) {
                    msg = (Message) e.nextElement();
                    processMsg("Browser", buf, msg);
                }
            }
            for (QueueReceiver qr : receivers) {
                msg = qr.receive();
                processMsg("Receiver", buf, msg);
                msg = qr.receive(5000);
                processMsg("Receiver", buf, msg);
                int count = 0;
                while ((msg = qr.receiveNoWait()) == null) {
                	try {
                    	Thread.sleep(500);
                    } catch (InterruptedException e) {
                    
                    }
                	count++;
                	if (count > 50) {
                		break;
                	}
                }
                processMsg("Receiver", buf, msg);
            }
        }
        
        public boolean isDone() {
            return done;
        }
    }
    
    public class TopicAPITester implements Runnable {
        TopicConnection tc;
        TopicSession ts;
        TextMessage m;
        
        ArrayList<TopicPublisher> publishers;
        ArrayList<TopicSubscriber> subscribers;
        
        StringBuffer buf;
        boolean done;
        
        public TopicAPITester() {
            publishers = new ArrayList<TopicPublisher>();
            subscribers = new ArrayList<TopicSubscriber>();
            done = false;
            buf = new StringBuffer();
        }
        
        public void run() {
            try {
                tc = tcf.createTopicConnection();
                try {
                    /* Apparently it is illegal to call setClientID on a Java EE client application
                     * connection */
                    tc.setClientID("jmsapiclient2");
                    trace(buf, "FAILED: TopicConnection setClientID completed");
                } catch (JMSException e) {
                    trace(buf, "TopicConnection correctly failed to call setClientID");
                }
                trace(buf, "TopicConnection created");
                
                ts = tc.createTopicSession(true, Session.AUTO_ACKNOWLEDGE);
                m = ts.createTextMessage("Sample for topic");
                
                createPublishers();
                createSubscribers();
                
                publishMessages();
                receiveMessages();
                
                try {
                    tc.stop();
                    trace(buf, "FAILED: TopicConnection.stop() completed");
                } catch (Exception e) {
                    trace(buf, "Caught exception on TopicConnection.stop()");
                }
                
                try {
                    tc.start();
                    trace(buf, "TopicConnection.start() was ignored");
                } catch (Exception e) {
                    trace(buf, "FAILED: Caught exception on TopicConnection.start()");
                }
                
                closeSubscribers();
                tc.close();
                
                trace(buf, "TopicAPITester complete");
                done = true;
            } catch (Exception e) {
                e.printStackTrace();
                try {
                    if (tc != null)
                        tc.close();
                    trace(buf, "TopicAPITester FAILED!");
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
                done = true;
            }
        }
        
        public void createPublishers() throws JMSException {
            publishers.add(ts.createPublisher(topic));
            publishers.add(ts.createPublisher(null));
        }
        
        public void createSubscribers() throws JMSException {
            subscribers.add(ts.createSubscriber(topic));
            subscribers.add(ts.createSubscriber(topic, "true", false));
        }
        
        public void closeSubscribers() throws JMSException {
        	for (TopicSubscriber ts : subscribers) {
        		ts.close();
        	}
        }
        
        private void publishMessages() throws JMSException, IOException {
            for (int i=0; i<10; i++) {
                for (TopicPublisher tp : publishers) {
                    if (tp.getDestination() != null) {
                        tp.send(m);
                        trace(buf, "Message sent with basic TopicPublisher send API");
                        tp.send(m, DeliveryMode.NON_PERSISTENT, 5, 0);
                        trace(buf, "Message sent with multiple parameter TopicPublisher send API");
                    } else {
                        tp.send(topic, m);
                        trace(buf, "Message sent with basic anonymous TopicPublisher send API");
                        tp.send(topic, m, DeliveryMode.NON_PERSISTENT, 5, 0);
                        trace(buf, "Message sent with multiple parameter anonymous TopibPublisher send API");
                    }
                }
            }
        }
        
        private void receiveMessages() throws JMSException, IOException {
            Message msg = null;
            for (TopicSubscriber ts : subscribers) {
                msg = ts.receive();
                processMsg("Subscriber", buf, msg);
                msg = ts.receive(5000);
                processMsg("Subscriber", buf, msg);
                msg = ts.receiveNoWait();
                processMsg("Subscriber", buf, msg);
            }
        }
        
        public boolean isDone() {
            return done;
        }
    }
    
    public class SharedSubOutboundAPITester implements Runnable {
    	Connection clientidC;
    	Connection noclientidC1;
    	Connection noclientidC2;
    	Session clientidS;
    	Session noclientidS1;
    	Session noclientidS2;
    	TextMessage m;
    	
    	// ND=NonDurable D=Durable
    	// NG=NonGlobal G=Global
    	ArrayList<MessageConsumer> NDNGconsumers;
    	ArrayList<MessageConsumer> DNGconsumers;
    	ArrayList<MessageConsumer> NDGconsumers;
    	ArrayList<MessageConsumer> DGconsumers;
    	MessageProducer producer;
    	
    	StringBuffer buf;
    	boolean done;
    	
    	public SharedSubOutboundAPITester() {
    		NDNGconsumers = new ArrayList<MessageConsumer>();
    		DNGconsumers = new ArrayList<MessageConsumer>();
    		NDGconsumers = new ArrayList<MessageConsumer>();
    		DGconsumers = new ArrayList<MessageConsumer>();
    		producer = null;
    		done = false;
    		buf = new StringBuffer();
    	}

		public void run() {
    		try {
    			clientidC = clientidcf.createConnection();
	    		trace(buf, "clientidC Connection created");
	    		
	    		noclientidC1 = basecf.createConnection();
	    		trace(buf, "noclientidC1 Connection created");
	    		
	    		noclientidC2 = basecf.createConnection();
	    		trace(buf, "noclientidC2 Connection created");
	    		
	    		((ImaProperties)topic).put("clientMessageCache", "0");
	    		
	    		clientidS = clientidC.createSession(true, Session.AUTO_ACKNOWLEDGE);
	    		noclientidS1 = noclientidC1.createSession(true, Session.AUTO_ACKNOWLEDGE);
	    		noclientidS2 = noclientidC2.createSession(true, Session.AUTO_ACKNOWLEDGE);
	    		
	    		m = clientidS.createTextMessage("Sample for sharedsuboutbound");
	    		
	    		producer = clientidS.createProducer(sharedtopic);
	    		createConsumers();
	    		
	    		produceMessages();
	    		consumeMessages();
	    		
	    		closeConsumers();
	    		
	    		clientidC.close();
	    		noclientidC1.close();
	    		noclientidC2.close();
	    		
	    		trace(buf, "SharedSubOutboundAPITester complete");
	    		done = true;
	    		
    		} catch (Exception e) {
    			e.printStackTrace();
    			trace(buf, "SharedSubOutboundAPITester FAILED!");
    			try {
    				if (clientidC != null)
    					clientidC.close();
    				if (noclientidC1 != null)
    					noclientidC1.close();
    				if (noclientidC2 != null)
    					noclientidC2.close();
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
                done = true;
    		}
    	}

    	public void createConsumers() throws JMSException {
    		// Create 5 consumers on the same non-durable shared subscription.
    		// Not global
    		for (int i=0; i<5; i++) {
    			NDNGconsumers.add(((ImaSubscription)clientidS).createSharedConsumer(sharedtopic, 
    				"STNonGlobalNonDurableSub"));
    		}
    		
    		// Create 5 consumers on the same durable shared subscription.
    		// Not global
    		for (int i=0; i<5; i++) {
    			DNGconsumers.add(((ImaSubscription)clientidS).createSharedDurableConsumer(sharedtopic, 
    				"STNonGlobalDurableSub"));
    		}
    		
    		// Create a consumer with noclientidC1 and with noclientidC2 on the same
    		// non-durable shared global subscription
    		NDGconsumers.add(((ImaSubscription)noclientidS1).createSharedConsumer(sharedtopic, 
    				"STGlobalNonDurableSub"));
    		NDGconsumers.add(((ImaSubscription)noclientidS2).createSharedConsumer(sharedtopic, 
    				"STGlobalNonDurableSub"));
    		
    		// Create a consumer with noclientidC1 and with noclientidC2 on the same
    		// durable shared global subscription
    		DGconsumers.add(((ImaSubscription)noclientidS1).createSharedDurableConsumer(sharedtopic, 
    				"STGlobalDurableSub"));
    		DGconsumers.add(((ImaSubscription)noclientidS2).createSharedDurableConsumer(sharedtopic, 
    				"STGlobalDurableSub"));
    	}
    	
    	public void closeConsumers() throws JMSException {
    		for (int i=0; i<5; i++) {
    			NDNGconsumers.get(i).close();
    		}
    		//clientidS.unsubscribe("STNonGlobalNonDurableSub");
    		
    		// Create 5 consumers on the same durable shared subscription.
    		// Not global
    		for (int i=0; i<5; i++) {
    			DNGconsumers.get(i).close();
    		}
    		clientidS.unsubscribe("STNonGlobalDurableSub");
    		
    		// Create a consumer with noclientidC1 and with noclientidC2 on the same
    		// non-durable shared global subscription
    		NDGconsumers.get(0).close();
    		NDGconsumers.get(1).close();
    		
    		// Create a consumer with noclientidC1 and with noclientidC2 on the same
    		// durable shared global subscription
    		DGconsumers.get(0).close();
    		DGconsumers.get(1).close();
    		//noclientidS1.unsubscribe("STGlobalNonDurableSub");
    		//noclientidS2.unsubscribe("STGlobalNonDurableSub");
    		//noclientidS1.unsubscribe("STGlobalDurableSub");
    		noclientidS2.unsubscribe("STGlobalDurableSub");
    	}
    	
    	public void produceMessages() throws JMSException {
    		for (int i=0; i<100; i++) {
    			producer.send(m);
    		}
    	}
    	
    	public void consumeMessages() throws JMSException, IOException {
    		Message rm = null;
    		int received = 0;
    		while (received < 100) {
    		//for (int i=0; i<20; i++) {
	    		for (MessageConsumer mc : NDNGconsumers) {
	    			rm = mc.receive(500);
	    			if (rm != null) {
	    				processMsg("NDNGconsumer", buf, rm);
	    				received++;
	    			}
	    		}
    		}
    		received = 0;
    		while (received < 100) {
    		//for (int i=0; i<20; i++) {
	    		for (MessageConsumer mc : DNGconsumers) {
	    			rm = mc.receive(500);
	    			if (rm != null) {
	    				processMsg("DNGconsumer", buf, rm);
	    				received++;
	    			}
	    		}
    		}
    		received = 0;
    		while (received < 100) {
    		//for (int i=0; i<20; i++) {
    			for (MessageConsumer mc : NDGconsumers) {
    				rm = mc.receive(500);
	    			if (rm != null) {
	    				processMsg("NDGconsumer", buf, rm);
	    				received++;
	    			}
    			}
    		}
    		received = 0;
    		while (received < 100) {
    		//for (int i=0; i<50; i++) {
    			for (MessageConsumer mc : DGconsumers) {
    				rm = mc.receive(500);
	    			if (rm != null) {
	    				processMsg("DGconsumer", buf, rm);
	    				received++;
	    			}
    			}
    		}
    		for (MessageConsumer mc : NDNGconsumers) {
    			if (mc.receive(500) != null) {
    				trace(buf, "Failure! NDNGconsumers received too many messages.");
    			}
    		}
    		for (MessageConsumer mc : DNGconsumers) {
    			if (mc.receive(500) != null) {
    				trace(buf, "Failure! DNGconsumers received too many messages.");
    			}
    		}
    		for (MessageConsumer mc : NDGconsumers) {
    			if (mc.receive(500) != null) {
    				trace(buf, "Failure! NDGconsumers received too many messages.");
    			}
    		}
    		for (MessageConsumer mc : DGconsumers) {
    			if (mc.receive(500) != null) {
    				trace(buf, "Failure! DGconsumers received too many messages.");
    			}
    		}
    	}
    	
    	public boolean isDone() {
			return done;
		}
    }
    
    public class UnifiedAPITester implements Runnable {
        Connection c;
        Session s;
        TextMessage m;
        
        ArrayList<MessageConsumer> consumers;
        ArrayList<MessageProducer> producers;
        
        StringBuffer buf;
        boolean done;
        
        public UnifiedAPITester() {
            consumers = new ArrayList<MessageConsumer>();
            producers = new ArrayList<MessageProducer>();
            done = false;
            buf = new StringBuffer();
        }
        
        public void run() {
            try {
                c = cf.createConnection();
                try {
                    /* Apparently it is illegal to call setClientID on a Java EE client application
                     * connection */
                    c.setClientID("jmsapiclient1");
                    trace(buf, "FAILED: Connection setClientID completed");
                } catch (JMSException e) {
                    trace(buf, "Connection correctly failed to call setClientID");
                }
                trace(buf, "Connection created");
                
                s = c.createSession(true, Session.AUTO_ACKNOWLEDGE);
                m = s.createTextMessage("Sample for unified");
                
                createProducers();
                createConsumers();
                
                produceMessages();
                receiveMessages();
                
                closeConsumers();
                c.close();
                
                trace(buf, "UnifiedAPITester complete");
                done = true;
            } catch (Exception e) {
                e.printStackTrace();
                try {
                    if (c != null)
                        c.close();
                    trace(buf, "UnifiedAPITester FAILED!");
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
                done = true;
            }
        }
        
        public void createProducers() throws JMSException {
            producers.add(s.createProducer(dest));
            producers.add(s.createProducer(null));
        }
        
        public void createConsumers() throws JMSException {
            consumers.add(s.createConsumer(dest));
            consumers.add(s.createConsumer(dest, "true"));
            consumers.add(s.createConsumer(dest, "true", false));
        }
        
        public void closeConsumers() throws JMSException {
        	for (MessageConsumer c : consumers) {
        		c.close();
        	}
        }
        
        private void produceMessages() throws JMSException, IOException {
            for (int i=0; i<10; i++) {
                for (MessageProducer p : producers) {
                    if (p.getDestination() != null) {
                        p.send(m);
                        trace(buf, "Message sent with basic MessageProducer send API");
                        p.send(m, DeliveryMode.NON_PERSISTENT, 5, 0);
                        trace(buf, "Message sent with multiple parameter MessageProducer send API");
                    } else {
                        p.send(dest, m);
                        trace(buf, "Message sent with basic anonymous MessageProducer send API");
                        p.send(dest, m, DeliveryMode.NON_PERSISTENT, 5, 0);
                        trace(buf, "Message sent with multiple parameter anonymous MessageProducer send API");
                    }
                }
            }
        }
        
        private void receiveMessages() throws JMSException, IOException {
            Message msg = null;
            for (MessageConsumer mc : consumers) {
                msg = mc.receive();
                processMsg("Consumer", buf, msg);
                msg = mc.receive(5000);
                processMsg("Consumer", buf, msg);
                msg = mc.receiveNoWait();
                processMsg("Consumer", buf, msg);
            }
        }
        
        public boolean isDone() {
            return done;
        }
    }

}
