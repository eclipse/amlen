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
package com.ibm.ima.sample.outboundmsg;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Properties;

import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.TopicConnection;
import javax.jms.TopicConnectionFactory;
import javax.naming.InitialContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * Servlet implementation class SendMsg
 */
public class SendMsg extends HttpServlet {
	private static final long serialVersionUID = 1L;

    /**
     * Default constructor. 
     */
    public SendMsg() {
		try {
			
		} catch (Exception e) {
			e.printStackTrace();
		}
    }

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		String msg = request.getParameter("msg");
		if (msg == null) { msg = "default message"; }
		
		String size = request.getParameter("size");
		if (size != null) {
			msg = "";
			for (int i = 0; i < Integer.parseInt(size); i++) {
				msg += "a";
			}
		}
		
		String topic = request.getParameter("topic");
		if (topic == null) { topic = "outbound"; }
		
		String repeat = request.getParameter("repeat");
		if (repeat == null) { repeat = "1"; }
		int times = Integer.parseInt(repeat);
		
		PrintWriter out = response.getWriter();
		
		try {
			InitialContext jndi = new InitialContext(new Properties());
			TopicConnectionFactory conFactory;
			conFactory = (TopicConnectionFactory)jndi.lookup("TopicConnectionFactory");
			TopicConnection connection = conFactory.createTopicConnection();
	        Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
	        MessageProducer producer = session.createProducer(session.createTopic(topic));
	        out.print("Publishing message...   topic: " + topic + "    payload: " + msg + "\n");
	        
	        producer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
	        for (int i = 0; i < times; i++) {
	        	TextMessage message = session.createTextMessage(msg);
		        producer.send(message);
	        }
	        out.print("Done-- Sent " + times + " message(s)!\n");
	        session.close();
	        connection.close();
	        out.print("Connection closed.\n");
	        
		} catch (Exception e) {
			e.printStackTrace(out);
		}
	}

}
