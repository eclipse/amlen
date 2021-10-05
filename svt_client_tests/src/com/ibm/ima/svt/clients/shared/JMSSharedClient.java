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
package com.ibm.ima.svt.clients.shared;

/*
 * This creates a shared JMS Client using a JMSConnection. Clients have a message listener
 * which increments a count onReceive. This JMS Client is to be used for checking 'fairness'
 * with various 'MessageCache' settings.
 * 
 * The client may be expanded in the future to output specific messages to a file to ensure
 * message integrity.
 */

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.sql.Timestamp;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.TextMessage;

import com.ibm.ima.jms.ImaSubscription;
import com.ibm.ima.jms.impl.ImaMessageConsumer;

public class JMSSharedClient {
    int 				count = 0;
    static int 			globalCount = 0;
    JMSConnection 		connSettings;
    MessageConsumer    	consumer;
    String 				fileName;
    static File			file;
    static PrintWriter  output;
    
    public int getGlobalCount() {
		return globalCount;
	}
	
    /*
     * The message listener increments both local and global counts.
     * If a PrintWriter exists, a timestamp, consumer ID and message payload will be output to a text file.
     */
	MessageListener messageListener = new MessageListener() {
		public synchronized void onMessage(Message arg0) {
			if (output!=null){
				String cid = ((ImaMessageConsumer)consumer).getInfo();
				try {
					output.println(new Timestamp(new java.util.Date().getTime()) + " " + cid + " : " + ((TextMessage) arg0).getText());
					output.flush();
				} catch (JMSException e) {
					e.printStackTrace();
				}
			}
			count++;
			globalCount++;
		}
	};
	
	/*
	 * Constructor without file output
	 */
    public JMSSharedClient(JMSConnection conn){
    	connSettings = conn;
    }
	
    /*
     * Constructor with file output
     */
    public JMSSharedClient(JMSConnection conn, String filename){
    	connSettings = conn;
    	try {
    		file = new File(filename+"_output.txt");
			output = new PrintWriter(new FileWriter(file));
		} catch (IOException e) {
			e.printStackTrace();
		}
    }
    
    /*
     * Can create the shared consumer to be either Non-Durable or Durable.
     * All clients are assigned the message listener by default.
     */
    void createSharedNonDurableSubscriber(){
    	try {
    		consumer = ((ImaSubscription)connSettings.session).createSharedConsumer(connSettings.topic, connSettings.topicName);
    		consumer.setMessageListener(messageListener);
		} catch (JMSException e) {
			e.printStackTrace();
		}
    }
    
    void createSharedDurableSubscriber(){
    	try {
    		consumer = ((ImaSubscription)connSettings.session).createSharedDurableConsumer(connSettings.topic, connSettings.topicName);
    		consumer.setMessageListener(messageListener);
		} catch (JMSException e) {
			e.printStackTrace();
		}
    }
    
    public void connect(){
    	try {
			connSettings.connection.start();
		} catch (JMSException e) {
			e.printStackTrace();
		}
    }
    
    public void disconnect(){
    	try {
			connSettings.connection.close();
		} catch (JMSException e) {
			e.printStackTrace();
		}
    } 
}
