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
package svt.jms.chat;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class JMSReceiveThread extends Thread {

    public Connection connection = null;
    Session session = null;
    MessageConsumer consumer = null;

    public void run() {
        connect();
        listen();
        close();
    }

    void connect() {
        try {
            ConnectionFactory connectionFactory = null;
            connectionFactory = ImaJmsFactory.createConnectionFactory();

            ((ImaProperties) connectionFactory).put("Server", "9.3.177.157");
            ((ImaProperties) connectionFactory).put("Port", "16102");
            ((ImaProperties) connectionFactory).put("Protocol", "tcp");

            ImaJmsException[] exceptions = ((ImaProperties) connectionFactory).validate(false);
            if (exceptions != null) {
                for (ImaJmsException e : exceptions)
                    System.out.println(e.getMessage());
                return;
            }

            connection = connectionFactory.createConnection();
            connection.setClientID("rkar");
        } catch (Throwable e) {
            e.printStackTrace();
            return;
        }

        try {
            session = connection.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);

            Destination dest = session.createTopic("/topic");
            if (dest == null) {
                System.err.println("ERROR:  Unable to create topic:  /topic");
                System.exit(1);
            }

            consumer = session.createDurableSubscriber((Topic) dest, "rkac");
            connection.start();

        } catch (JMSException e) {
            e.printStackTrace();
        }
    }

    void listen() {
        Message msg = null;
        try {
            msg = consumer.receive(100000);
            while (msg != null) {
                System.out.println(msg);
                msg = consumer.receive(10000);
            }
        } catch (JMSException e) {
            e.printStackTrace();
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    void close() {
        try {
            connection.close();
        } catch (JMSException e) {
            e.printStackTrace();
        }
        return;
    }

}
