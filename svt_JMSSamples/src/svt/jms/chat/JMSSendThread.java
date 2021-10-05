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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Session;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class JMSSendThread extends Thread {
    Session session = null;
    MessageProducer producer = null;
    public Connection connection = null;

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
            connection.setClientID("rka");

            session = connection.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
            Destination dest = null;
            dest = session.createTopic("/topic");
            if (dest == null) {
                System.err.println("ERROR:  Unable to create topic:  /topic");
                System.exit(1);
            }

            producer = session.createProducer(dest);
            producer.setDeliveryMode(DeliveryMode.PERSISTENT);
        } catch (Throwable e) {
            e.printStackTrace();
            return;
        }
    }

    void listen() {
        BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
        String s = null;
        try {
            while ((s = in.readLine()) != null && s.length() != 0) {
                send(s);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (JMSException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
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

    void send(String s) throws JMSException, InterruptedException {
        Message msg = session.createTextMessage(s);

        boolean sent = false;
        int count = 0;
        while (sent == false) {
            try {
                producer.send(msg);
                sent = true;
            } catch (JMSException e) {
                if (++count > 10)
                    break;
                else
                    Thread.currentThread().sleep(100);
                System.out.println(e.getMessage() + "... retry " + count);
            }
        }
    }

}
