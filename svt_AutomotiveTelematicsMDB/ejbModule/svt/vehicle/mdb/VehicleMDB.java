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
package svt.vehicle.mdb;

import javax.annotation.Resource;
import javax.ejb.ActivationConfigProperty;
import javax.ejb.MessageDriven;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.Message;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

/**
 * Message-Driven Bean implementation class for: VehicleMDB
 * 
 */
@MessageDriven(activationConfig = { @ActivationConfigProperty(propertyName = "destinationType", propertyValue = "javax.jms.Queue") })
public class VehicleMDB implements MessageListener {

	@Resource(name = "jms/replyCF", type = javax.jms.ConnectionFactory.class)
	private ConnectionFactory replyCF;

	/**
	 * Default constructor.
	 */
	public VehicleMDB() {
		// TODO Auto-generated constructor stub

	}

	/**
	 * @see MessageListener#onMessage(Message)
	 */
	public void onMessage(Message message) {
		try {

			// sys out that MDB had been driven
			System.out.println("Sample MDB invoked");

			// get the destination to send a response to, hope its not null...
			// Destination replyDestination = message.getJMSReplyTo();
			Topic dest = (Topic) message.getJMSDestination();
			if (dest == null) {
				throw new Exception(
						"message.getJMSDestionation() returned null");
			}

			String incoming_topic = dest.getTopicName();
			if (incoming_topic == null) {
				throw new Exception("dest.getTopicName() returned null");
			}

			// create the required JMS connections etc
			Connection replyConnection = replyCF.createConnection();
			Session replySession = replyConnection.createSession(true, 0);

			// create a response message and populate it
			TextMessage replyMessage = replySession.createTextMessage();

			String text = "";
			if (message instanceof TextMessage) {
				text = ((TextMessage) message).getText();
			} else if (message instanceof javax.jms.BytesMessage) {
				int length = (int) ((javax.jms.BytesMessage) message)
						.getBodyLength();
				byte[] bytes = new byte[length];
				((javax.jms.BytesMessage) message).readBytes(bytes, length);
				text = new String(bytes, "UTF8");

			} else {
				System.out.println(message.getJMSType());
				System.out.println(message);
				throw new Exception("Unexpected message received.");
			}

			String delims = "[|]+";
			String[] token = text.split(delims);

			if ((token[0] == null) || (token[0].equals(""))) {
				throw new Exception("No Action string specified");
			} else if (token[0].equals("ACCOUNT_INFO")) {
				replyMessage
						.setText("54662817|Thomas J. Watson|(800) 426-4968|11501 Burnet Road|Austin|TX|78758-3407|Platnum Member");
			} else if (token[0].equals("RESERVATION_LIST")) {
				replyMessage.setText("10/10/2012|11/04/2012|11/26/2012");
			} else if (token[0].equals("RESERVATION_INFO")) {
				replyMessage
						.setText("F2881763329|10/10/2012|16:00|Ft. Lauderdale International Airport|600 Terminal Drive|Ft Lauderdale,FL 33315");
			} else if (token[0].equals("QUERY_ALERTS")) {
				replyMessage.setText("No alerts exist.");
			} else if (token[0].equals("RECORD_GPS")) {
				replyMessage.setText("GPS Coordinates recorded.");
			} else if (token[0].equals("RECORD_ERRORCODE")) {
				replyMessage.setText("Errorcode recorded");
			} else if (token[0].equals("SYSTEM_UPDATE")) {
				replyMessage.setText("System Update unsuported.");
			} else if (token[0].equals("UNLOCK")) {
				System.out.println("UNLOCK sent to " + token[0] + " from "
						+ incoming_topic);
				replyMessage.setText("UNLOCK");
			} else if (token[0].equals("LOCK")) {
				System.out.println("LOCK sent to " + token[0] + " from "
						+ incoming_topic);
				replyMessage.setText("LOCK");
			} else if (token[0].equals("GET_DIAGNOSTICS")) {
				replyMessage.setText("GET_DIAGNOSTICS");
			} else if (token[0].equals("Doors unlocked")) {
				replyMessage.setText("Doors unlocked");
			} else if (token[0].equals("Doors locked")) {
				replyMessage.setText("Doors locked");
			} else {
				replyMessage.setText("Unrecognized request: " + token[0]);
			}

			delims = "[/]+";
			token = incoming_topic.split(delims);
			String outgoing_topic = "/" + token[3] + "/" + token[4] + "/"
					+ token[5] + "/" + token[6];

			System.out.println("incoming_topic: " + incoming_topic);
			System.out.println("incoming message: " + text);
			System.out.println("outgoing_topic: " + outgoing_topic);
			System.out.println("outgoing message: " + replyMessage.getText());

			Destination replyDestination = replySession
					.createTopic(outgoing_topic);
			MessageProducer replyProducer = replySession
					.createProducer(replyDestination);

			replyProducer.send(replyMessage);
			replyProducer.close();
			// } else {
			// System.out.println(message.getJMSType());
			// System.out.println();
			// throw new Exception("Unexpected message received.");
			// }

			// replyMessage.setStringProperty("BridgeReplyToTopic",
			// message.getStringProperty("BridgeReplyToTopic"));

			// send the reply

			// close off JMS items
			replySession.close();
			replyConnection.close();

		} catch (Throwable e) {
			System.out.println("Exception caught:  " + e.getLocalizedMessage());
			e.printStackTrace();
		}
	}

}
