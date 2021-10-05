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
import java.util.Enumeration;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Queue;
import javax.jms.QueueBrowser;
import javax.jms.QueueSender;
import javax.jms.QueueSession;
import javax.jms.Session;
import javax.jms.TextMessage;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


public class TestBrowserMatchingEnums {
    /*
     * Main method 
     */
	@SuppressWarnings("unchecked")
    public static void main(String [] args) {
	    int msgcount = 255;
	    boolean debug = false;
    	long wait = 50;
        String queuename = "MY_QUEUE";
        Connection connect = null;
        
        /*
         * Create connection factory and connection
         */
        try {
            ConnectionFactory fact = ImaJmsFactory.createConnectionFactory();
            String server = System.getenv("ImaServer");
            String port   = System.getenv("ImaPort");
            if (server == null)
                server = "127.0.0.1";
            if (port == null)
                port = "16102";
            ((ImaProperties)fact).put("Server", server);
            ((ImaProperties)fact).put("Port", port);

            connect = fact.createConnection();
            // TODO: Fix trace output for acks when dups ok is set!
//            Session session = connect.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
            Session session = connect.createSession(false, Session.AUTO_ACKNOWLEDGE);
            Session rsession = connect.createSession(false, Session.AUTO_ACKNOWLEDGE);
            Queue queue = session.createQueue(queuename);
            QueueBrowser browser = rsession.createBrowser(queue);
            Enumeration<Message>benum = browser.getEnumeration();
            Enumeration<Message>benum2 = browser.getEnumeration();
            QueueSender sender = ((QueueSession)session).createSender(queue);
            Message msg = null;
            for (int i = 0; i < msgcount; i++) {
                msg = session.createTextMessage();
                ((TextMessage)msg).setText("test message "+i);
            	Message smsg = msg;
            	smsg.setStringProperty("X", "x");
            	sender.send(smsg);
            }

            int countmsg = 0;
            while(benum.hasMoreElements()) {
            	TextMessage brmsg = (TextMessage)benum.nextElement();
            	++countmsg;
            	if (debug)
            	    System.out.println("BROWSER found msg "+countmsg+": "+brmsg.getText());
            }
            System.out.println("BROWSER found "+countmsg+" messages");
            System.out.println("Queue is "+browser.getQueue().toString());
// Uncomment this to recreate defect 27569 -- server abort
//browser.close();

            countmsg = 0;
            while(benum2.hasMoreElements()) {
            	TextMessage brmsg = (TextMessage)benum2.nextElement();
            	++countmsg;
            	if (debug)
            	    System.out.println("BROWSER2 found msg "+countmsg+": "+brmsg.getText());
            }
            System.out.println("BROWSER2 found "+countmsg+" messages");
            
            MessageConsumer cons = rsession.createConsumer(queue);
            connect.start();
            System.out.println("Receiving messages after browsing");
            TextMessage rmsg = (TextMessage) cons.receive(10000);
            countmsg = 0;
            while(rmsg != null) {
                if (debug)
            	    System.out.println("CONSUMER Found msg: "+rmsg.getText());
            	rmsg = (TextMessage) cons.receive(10000);
            	++countmsg;
            }
            System.out.println("Finished receiving messages after browsing");
            System.out.println("CONSUMER found "+countmsg+" messages");

        } catch (Exception ex) {
        	ex.printStackTrace(System.err);
        } finally {
        	try {
        	    if(connect != null)
        	        connect.close();
        	} catch (JMSException jex) {
        		jex.printStackTrace();
        	}
        }
    }    
}
