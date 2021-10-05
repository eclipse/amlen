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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Topic;
import javax.jms.Session;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.ImaProperties;


public class FlushTopic {
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	long wait = 50;
        String topicname = "MY_TOPIC";
        String subname   = "commitAckMsgTopicTestSubscription";
        String clientid  = "durableClient";
        
        /*
         * Create connection factory and connection
         */
        try {
            ConnectionFactory fact = ImaJmsFactory.createConnectionFactory();
            String server = System.getenv("ISMServer");
            String port   = System.getenv("ISMPort");
            if (server != null)
                ((ImaProperties)fact).put("Server", server);
            if (port != null)
                ((ImaProperties)fact).put("Port", port);
            Connection connect = fact.createConnection();
            connect.setClientID(clientid);
            Session session = connect.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
            Topic topic = session.createTopic(topicname);
            MessageConsumer consumer = session.createDurableSubscriber(topic, subname);
            connect.start();
            for (;;) {
            	Message msg = consumer.receive(wait);
            	if (msg == null)
            		break;
            	System.out.println(ImaJmsObject.toString(msg, "*b"));
            }
            consumer.close();
            session.unsubscribe(subname);
            connect.close();
        } catch (Exception ex) {
        	ex.printStackTrace(System.err);
        }
    }    
}
