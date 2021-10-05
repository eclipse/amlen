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
package com.ibm.ima.jms.test;

import java.util.ArrayList;
import java.util.Random;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.Topic;

import org.w3c.dom.Element;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.test.TrWriter.LogLevel;

/* 
  Example Usage:
 
 <Action name=Fuzzy type="JMSFuzzAction">
 	<ActionParameter name="port">16102</ActionParameter>
 	<ActionParameter name="duration">10</ActionParameter>
 </Action>
 
 */

/**
 * This action is designed to really abuse the JMS protocol by 
 * creating/deleting consumers/producers, and sending/receiving messages
 * at random times on different threads. This is an attempt to make sure
 * the server can't get caught in a bad state, even if the client program
 * is really bad.
 *
 */

public class FuzzTestAction extends Action {
	
	private final int port;
	private final int duration; // in seconds
	
	private final int NUM_THREADS = 16; // 1.5x NUM_THREADS threads are produced

	FuzzTestAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		
		port = Integer.parseInt(_actionParams.getProperty("port"));
		duration = Integer.parseInt(_actionParams.getProperty("duration"));
	}

	@Override
	protected boolean run() {
		log("<Fuzz> beginning!");
		log("<Fuzz> duration: " + duration + " seconds");
		
		try {
			ConnectionFactory factory = ImaJmsFactory.createConnectionFactory();
			((ImaProperties)factory).put("Port", port);
			String server = EnvVars.replace("``A1_IPv4_1``");
			((ImaProperties)factory).put("Server", server);
			
			ImaJmsException[] problems = ((ImaProperties)factory).validate(ImaProperties.WARNINGS);
			if (problems != null)
				return err(problems[0].getMessage());
			
			Topic fuzztopic = ImaJmsFactory.createTopic("fuzzTopic");
			
			Connection connection = factory.createConnection();
			Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
			
			long stopTime = (1000 * duration) + System.currentTimeMillis();		
						
			ThingContainer container = new ThingContainer();
			ThingCreator[] creators = new ThingCreator[NUM_THREADS/2];
			ThingDoer[] doers = new ThingDoer[NUM_THREADS];			
			
			/* create and kick-off creators */
			for (int i=0; i<creators.length; i++)
				creators[i] = new ThingCreator(container, fuzztopic, session, stopTime);
			
			Thread[] creator_threads = new Thread[NUM_THREADS/2];
			for (int i=0; i<creator_threads.length; i++)
				creator_threads[i] = new Thread(creators[i]);
			
			for (int i=0; i<creator_threads.length; i++)
				creator_threads[i].start();
			
			/* create and kick-off doers */
			for (int i=0; i<doers.length; i++)
				doers[i] = new ThingDoer(container, fuzztopic, session, stopTime);
			
			Thread[] doer_threads = new Thread[NUM_THREADS];
			for (int i=0; i<doer_threads.length; i++)
				doer_threads[i] = new Thread(doers[i]);
			
			for (int i=0; i<doer_threads.length; i++)
				doer_threads[i].start();		
			
			/* wait for creators and doers to finish */
			for (int i=0; i<creator_threads.length; i++)
				while (!creators[i].isDone()) 
					Thread.sleep(250);				
			
			for (int i=0; i<doer_threads.length; i++)
				while (!doers[i].isDone())
					Thread.sleep(250);
		
			log("<Fuzz> consumers: " + container.consumerMetrics());
			log("<Fuzz> producers: " + container.producerMetrics());
			
			connection.close();
			
		} catch (Exception e) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "JMSFuzzAction failed", new Object());
			e.printStackTrace();
			return false;
		}
		
		log("<Fuzz> completed successfully");
		return true;
	}

	private void log(String msg) {
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", msg, new Object());
	}
	
	private boolean err(String msg) {
		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", msg, new Object());
		return false;
	}
	
	class ThingDoer implements Runnable {
		private Session session;
		private long stopTime;
		private ThingContainer container;
		private Topic destination;
		private boolean done;
		private int numConsumers;
		
		public ThingDoer(ThingContainer container, Topic destination, Session session, long stopTime) {
			this.session = session;
			this.stopTime = stopTime;
			this.container = container;
			this.destination = destination;
			this.numConsumers = 0;
			done = false;
		}
		
		public synchronized boolean isDone() {
			return done;
		}
		
		public void run() {
			Random rand = new Random();
			while (System.currentTimeMillis() < stopTime) {
				try {
					int n = Math.abs(rand.nextInt()) % 100;
					if (n < 5) { /* 5% chance create consumer */
						if (numConsumers < 1000) {
							container.add(session.createConsumer(destination));
							log("<TD> created consumer #" + container.consumers.size());
							numConsumers++;
						}
					} else if (n < 15) { /* 10% chance create producer */
						container.add(session.createProducer(destination));
						log("<TD> created producer #" + container.producers.size());
					} else if (n < 45) { /* 30% chance send messages */
						/* 50% chance for each producer to send a message */
						container.sendMessages(session, destination);
						log("<TD> sent messages");
					} else if (n < 75) { /* 30% chance rx messages */
						/* 50% chance for each consumer to rx a message */
						container.receiveMessages(session, destination);
						log("<TD> received messages");
					} else if (n < 80) { /* 5% close some consumers */
						container.closeSomeConsumers();
						log("<TD> closed some consumers");
					} else if (n < 85) { /* 5% close some producers */
						container.closeSomeProducers();
						log("<TD> closed some producers");
					} else { /* .1% chance we close the session */
						if (rand.nextInt() == 42) { /* 1/100 * 1/100 */
							session.close();
							log("<TD> closed the session");
						} else
							log("<TD> did nothing, this time");
					}
				} catch(Exception e) {
					err(e.getMessage());
				}
			}
			done = true;
		}
	}
	
	class ThingCreator implements Runnable {
		private Session session;
		private long stopTime;
		private ThingContainer container;
		private Topic destination;
		private boolean done;
		
		public ThingCreator(ThingContainer container, Topic destination, Session session, long stopTime) {
			this.session = session;
			this.stopTime = stopTime;
			this.container = container;
			this.destination = destination;
			done = false;
		}
		
		public void run() {

			Random rand = new Random();
			while (System.currentTimeMillis() < stopTime) {
				try {
					if (rand.nextBoolean()) {
						container.add(session.createConsumer(destination));
						log("<TC> created consumer #" + container.consumers.size());
					}
					else {
						container.add(session.createProducer(destination));
						log("<TC> created producer #" + container.producers.size());
					}
				} catch(JMSException jexe) {
					err(jexe.getMessage());
				}
			}
			done = true;
		}
		
		public synchronized boolean isDone() {
			return done;
		}
	}

	class ThingContainer {
		private ArrayList<MessageConsumer> consumers;
		private ArrayList<MessageProducer> producers;
		

		public ThingContainer() {
			consumers = new ArrayList<MessageConsumer>(1000);
			producers = new ArrayList<MessageProducer>(10000);
		}
		
		public void add(MessageConsumer consumer) {
			synchronized(consumers) {
				consumers.add(consumer);
			}
		}
		
		public void add(MessageProducer producer) {
			synchronized(producers) {
				producers.add(producer);
			}
		}
		
		public void closeSomeConsumers() throws JMSException {
			Random rand = new Random();
			for (int i=0; i<consumers.size(); i++) {
				if (Math.abs(rand.nextInt()) % 50 == 1) {
					consumers.get(i).close();
					log("consumer #" + i + "got closed");
				}
			}
		}
		
		public void closeSomeProducers() throws JMSException {
			Random rand = new Random();
			for (int i=0; i<producers.size(); i++) {
				if (Math.abs(rand.nextInt()) % 50 == 1) {
					producers.get(i).close();
					log("producer #" + i + "got closed");
				}
			}
		}
		
		public void sendMessages(Session s, Topic destination) throws JMSException {
			Random rand = new Random();
			for (int i=0; i<producers.size(); i++) {
				if (rand.nextBoolean()) {
					producers.get(i).send(s.createTextMessage("abc123"));
					log("producer #" + i + " sent message");
				}
			}
		}
		
		public void receiveMessages(Session s, Topic destination) throws JMSException {
			Random rand = new Random();
			for (int i=0; i<consumers.size(); i++) {
				if (rand.nextBoolean()) {
					consumers.get(i).receive(1); /* must not block */
					log("consumer #" + i + " rxd message");
				}
			}
		}
		
		public String consumerMetrics() {
			String r = null;
			synchronized(consumers) {
				r = "" + consumers.size();
			}
			return r;
		}
		
		public String producerMetrics() {
			String r = null;
			synchronized(producers) {
				r = "" + producers.size();
			}
			return r;
		}
	}
}


