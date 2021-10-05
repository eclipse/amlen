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

import javax.jms.JMSException;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaTopic;


/*
 * Shared subscription test
 */
public class SharedTest {
    int  action;
    static int  count = 10;
    static String topicname = "RequestT";

    static ConnectionFactory   factory;
    static Connection          connect;
    static Connection          connect_no_cid;
    static Session             session;
    static Session             session_no_cid;

    static boolean recvdone;
    
    static int  recvcount = 0;
    
    static MessageConsumer [] consumer = new MessageConsumer[6];
    static MessageConsumer    cons2;

      /*
     * Main method 
     */
    public static void main(String [] args) {
        
        /*
         * Create connection factory and connection
         */
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
            connect_no_cid = factory.createConnection();
            System.out.println("connect_no_cid="+ImaJmsObject.toString(connect_no_cid, "*"));
            
            session_no_cid  = connect_no_cid.createSession(false, 0);
            ((ImaProperties)factory).put("ClientID", "SharedTest");
            connect = factory.createConnection();
            System.out.println("connect="+ImaJmsObject.toString(connect, "*"));
            session = connect.createSession(false, 0);
            
            Topic  topic = session.createTopic(topicname);
            // ((ImaTopic)topic).put("ClientMessageCache", 50); 
            ((ImaTopic)topic).put("SubscriptionShared", "True"); /* will be ignored */
            consumer[0] = session.createConsumer(topic);
            System.out.println("0="+ImaJmsObject.toString(consumer[0], "*"));
            
            ((ImaTopic)topic).remove("SubscriptionShared");   
            consumer[1] = session.createDurableSubscriber(topic, "Durable");
            System.out.println("1="+ImaJmsObject.toString(consumer[1], "*"));
    //      session.createDurableSubscriber(topic, "Durable", "false", false);
            
            ((ImaTopic)topic).put("SubscriptionShared", "NonDurable");
            consumer[2] =  session.createDurableSubscriber(topic, "Subscription");
            System.out.println("2="+ImaJmsObject.toString(consumer[2], "*"));
            cons2 =  session.createDurableSubscriber(topic, "Subscription");
            System.out.println("cons2="+ImaJmsObject.toString(cons2, "*"));
            consumer[2].close();

            
            ((ImaTopic)topic).put("SubscriptionShared", "True");
            consumer[3] =  session.createDurableSubscriber(topic, "Subscription");
       //   consumer[3] =  session.createDurableSubscriber(topic, "Durable");
            System.out.println("3="+ImaJmsObject.toString(consumer[3], "*"));
            cons2 =  session.createDurableSubscriber(topic, "Subscription");
            System.out.println("cons2="+ImaJmsObject.toString(cons2, "*"));
            cons2.close();
            
            ((ImaTopic)topic).put("SubscriptionShared", "NonDurable");
            consumer[4] =  session_no_cid.createDurableSubscriber(topic, "Subscription");
            System.out.println("4="+ImaJmsObject.toString(consumer[4], "*"));
            cons2 =  session_no_cid.createDurableSubscriber(topic, "Subscription");
            System.out.println("cons2="+ImaJmsObject.toString(cons2, "*"));
            cons2.close();
            
            ((ImaTopic)topic).put("SubscriptionShared", 1);
            consumer[5] =  session_no_cid.createDurableSubscriber(topic, "Subscription");
            System.out.println("5="+ImaJmsObject.toString(consumer[5], "*"));
            cons2 =  session_no_cid.createDurableSubscriber(topic, "Subscription");
            System.out.println("cons2="+ImaJmsObject.toString(cons2, "*"));
            cons2.close();
            
            /* TODO: use them */
            
            
            consumer[1].close();
            consumer[3].close();
            consumer[5].close();
            session.unsubscribe("Durable");
            session.unsubscribe("Subscription");
            session_no_cid.unsubscribe("Subscription");
            
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
        } finally {
            try {
                if (connect != null) {
                    connect.close();
                    System.out.println("connect closed");
                }    
                if (connect_no_cid != null) {
                    connect_no_cid.close();
                    System.out.println("connect_no_cid closed");
                }    
            } catch (JMSException jmse2) {
                jmse2.printStackTrace(System.out);
            }
        }
        
    }
}

