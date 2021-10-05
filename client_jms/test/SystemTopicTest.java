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
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


/*
 * Test of $SYS topic restrictions
 */
public class SystemTopicTest {
    static ConnectionFactory fact;
    static Connection conn;
    
    public static void main(String [] args) {
    
        try {
            fact = ImaJmsFactory.createConnectionFactory();
            String server = System.getenv("IMAServer");
            String port   = System.getenv("IMAPort");
            if (server == null)
                server = "127.0.0.1";
            if (port == null)
                port = "16102";
            ((ImaProperties)fact).put("Server", server);
            ((ImaProperties)fact).put("Port", port);
            ((ImaProperties)fact).put("DisableMessageTimestamp", true);
            ((ImaProperties)fact).put("DisableMessageID", true);
            ((ImaProperties)fact).put("ClientID", "me");
            conn = fact.createConnection();
            Session session = conn.createSession(false, 0);
            Topic topic1 = session.createTopic("$SYS/me/Endpoint");
            Topic topic2 = session.createTopic("$SYSTEM");
            
            MessageProducer prod;
            MessageConsumer cons;
            
            try {
                prod = session.createProducer(topic1);
                System.out.println("No JMS Exception for: " + topic1);
            } catch (JMSException e) {
                System.out.println("Exception as expected:");
                e.printStackTrace(System.out);
            }
            try {
                prod = session.createProducer(topic2);
                System.out.println("No JMS Exception for: " + topic1);
            } catch (JMSException e) {
                System.out.println("Exception as expected:");
                e.printStackTrace(System.out);
            }
            prod = session.createProducer(null);
            
            TextMessage tmsg = session.createTextMessage("{}");
            
            try {
                prod.send(topic1, tmsg);
                System.out.println("No JMS Exception for: " + topic1);
            } catch (JMSException e) {
                System.out.println("Exception as expected:");
                e.printStackTrace(System.out);
            }
            
            cons = session.createConsumer(topic1);
            cons = session.createConsumer(topic2);
            try {
                cons = session.createDurableSubscriber(topic1,  "SYS");
                System.out.println("No JMS Exception for: " + topic1);
            } catch (JMSException e) {
                System.out.println("Exception as expected:");
                e.printStackTrace(System.out);
            }
            try {
                cons = session.createDurableSubscriber(topic2,  "SYS");
                System.out.println("No JMS Exception for: " + topic2);
            } catch (JMSException e) {
                System.out.println("Exception as expected:");
                e.printStackTrace(System.out);
            }
            conn.close();            
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
            return;
        }    
    }
}
