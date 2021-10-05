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
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.ImaProperties;


public class DeliveryCountTest {
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
        Connection connect;
        Session    session;
    
        try {
            ConnectionFactory factory = ImaJmsFactory.createConnectionFactory();
            String server = System.getenv("IMAServer");
            String port   = System.getenv("IMAPort");
            if (server == null)
                server = "127.0.0.1";
            if (port == null)
                port = "16102";
            ((ImaProperties)factory).put("Server", server);
            ((ImaProperties)factory).put("Port", port);
            ((ImaProperties)factory).put("ClientMessageCache", 50);
            connect = factory.createConnection();
            System.out.println("connect_no_cid="+ImaJmsObject.toString(connect, "*"));
            session = connect.createSession(false, Session.CLIENT_ACKNOWLEDGE);

            Topic  topic = session.createTopic("Topic1");
            MessageProducer prod = session.createProducer(topic);
            MessageConsumer cons = session.createConsumer(topic);
            connect.start();
            
            Message msg = session.createTextMessage("Test message");
            System.out.println("before send  count=" + msg.getIntProperty("JMSXDeliveryCount"));
            prod.send(msg);
            System.out.println("after send   count=" + msg.getIntProperty("JMSXDeliveryCount"));
            
            for (int i=0; i<10000; i++) {
                Message msg2 = cons.receive(500);
                System.out.println("after recv"+(i+1)+"   count=" + msg2.getIntProperty("JMSXDeliveryCount"));
                session.recover();
            }
            msg.acknowledge();
            connect.close();
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
        }
    }
}
