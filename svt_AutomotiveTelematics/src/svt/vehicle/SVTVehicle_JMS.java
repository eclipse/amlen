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
package svt.vehicle;

import java.io.BufferedReader;
import java.io.InputStreamReader;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import svt.util.SVTJMSMessage;
import svt.util.SVTLog;
import svt.util.SVTMessage;
import svt.util.SVTUtil;
import svt.util.SVTLog.TTYPE;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class SVTVehicle_JMS implements Runnable {
    ConnectionFactory connectionFactory = null;
    static TTYPE TYPE = TTYPE.ISMJMS;
    // static String Vehicle = "/CAR/1/#";

    String ismserver = null;
    Integer ismport = null;
    int guid = (int) (Math.random() * 10000.0);
    String message = null;
    static String command = null;
    long tid = Thread.currentThread().getId();

    long carId = 0;
    boolean ssl = false;
    String userName = null;
    String password = null;

    // public SVTVehicle_JMS(String ismserver, Integer ismport, long carId) {
    // this.ismserver = ismserver;
    // this.ismport = ismport;
    // this.carId = carId;
    // this.ssl = false;
    // this.userName = null;
    // this.password = null;
    // }

    public SVTVehicle_JMS(String ismserver, Integer ismport, long carId, boolean ssl, String userName, String password) {
        this.ismserver = ismserver;
        this.ismport = ismport;
        this.carId = carId;
        this.ssl = ssl;
        this.userName = userName;
        this.password = password;
    }

    public SVTMessage getMessage(String topicName, String cClientId) {
        return getMessage(topicName, cClientId, false, null, null);
    }

    public SVTMessage getMessage(String topicName, String cClientId, boolean ssl, String userName, String password) {
        Connection consumerConn = null;
        Session consumerSession = null;
        MessageConsumer theConsumer = null;
        SVTMessage message = null;
        ImaJmsException[] errstr = null;

        try {
            if (theConsumer == null) {
                connectionFactory = ImaJmsFactory.createConnectionFactory();
                ((ImaProperties) connectionFactory).put("Server", ismserver);
                ((ImaProperties) connectionFactory).put("Port", ismport);
                // ((ImaProperties) connectionFactory).put("KeepAlive", 0);
                if (ssl)
                    ((ImaProperties) connectionFactory).put("Protocol", "tcps");

                errstr = ((ImaProperties) connectionFactory).validate(ImaProperties.WARNINGS);
                if (errstr != null) {
                    for (int i = 0; i < errstr.length; i++)
                        SVTLog.log(TYPE, "" + errstr[i]);
                    System.exit(1);
                }

                if (password != null)
                    consumerConn = connectionFactory.createConnection(userName, password);
                else
                    consumerConn = connectionFactory.createConnection();

                consumerConn.setClientID(cClientId);
                consumerSession = consumerConn.createSession(false, Session.AUTO_ACKNOWLEDGE);
                Topic topic = consumerSession.createTopic(topicName);
                theConsumer = consumerSession.createConsumer(topic);

                consumerConn.start();
            }

            Message m = theConsumer.receive(5000);
            message = new SVTJMSMessage(m);

            consumerConn.close();
        } catch (JMSException e) {
            SVTLog.logex(TYPE, "Consumer Session-Connection: ", e);
        }
        return message;
    }

    public Session getSession(String pClientId) throws Exception {
        // public Session getSession(String ismserver, Integer ismport, String pClientId) throws Exception {

        Session producerSession = null;
        Connection producerConn = null;
        ImaJmsException[] errstr = null;

        if (connectionFactory == null) {
            // System.out.print(" factory(" + tid + ")");
            connectionFactory = ImaJmsFactory.createConnectionFactory();
            // System.out.print(". ");

            ((ImaProperties) connectionFactory).put("Server", ismserver);
            ((ImaProperties) connectionFactory).put("Port", ismport);
            // ((ImaProperties) connectionFactory).put("KeepAlive", 0);
            if (ssl)
                ((ImaProperties) connectionFactory).put("Protocol", "tcps");
            else
                ((ImaProperties) connectionFactory).put("Protocol", "tcp");

            errstr = ((ImaProperties) connectionFactory).validate(ImaProperties.WARNINGS);
            if (errstr != null) {
                for (int i = 0; i < errstr.length; i++)
                    SVTLog.log(TYPE, "" + errstr[i]);
                System.exit(1);
            }
        }

        int i = 0;
        producerConn = null;
        SVTLog.logtry(TYPE, "connection");
        while (producerConn == null) {
            try {
                if (password != null)
                    producerConn = connectionFactory.createConnection(userName, password);
                else
                    producerConn = connectionFactory.createConnection();
            } catch (JMSException e) {
                producerConn = null;
                if (i++ < 100) {
                    SVTLog.logretry(TYPE, "connection", i);
                    Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                } else
                    throw e;
            }
        }
        SVTLog.logdone(TYPE, "connection");
        if (producerConn == null) {
            throw new Exception("producerConn(" + tid + ") == null");
        }

        producerConn.setClientID(pClientId);
        producerSession = null;
        SVTLog.logtry(TYPE, "session");
        while (producerSession == null) {
            try {
                producerSession = producerConn.createSession(false, Session.AUTO_ACKNOWLEDGE);
            } catch (JMSException e) {
                producerSession = null;
                if (i++ < 100) {
                    SVTLog.logretry(TYPE, "session", i);
                    Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                } else
                    throw e;
            }
        }
        SVTLog.logdone(TYPE, "session");

        return producerSession;
    }

    public MessageProducer getProducer(Session producerSession, String topicName) throws Exception {
        MessageProducer theProducer = null;

        Topic topic = producerSession.createTopic(topicName);
        theProducer = producerSession.createProducer(topic);
        theProducer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);

        return theProducer;
    }

    public void sendMessage(MessageProducer theProducer, Session producerSession, String message, long n)
                    throws JMSException {

        TextMessage textmessage = producerSession.createTextMessage(message);

        SVTLog.logtry(TYPE, "publish[" + message + "]");
        int i = 0;
        boolean sent = false;
        while (sent == false) {
            try {
                theProducer.send(textmessage);
                sent = true;
            } catch (JMSException e) {
                if (i++ >= 100) {
                    SVTLog.logex(TYPE, "publish", e);
                    throw e;
                }
                try {
                    Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                } catch (InterruptedException e2) {
                    SVTLog.logex(TYPE, "sleep", e);
                }
                SVTLog.logretry(TYPE, "publish", e, i);
            }
        }
        SVTLog.logdone(TYPE, "publish", "");
    }

    public void close(MessageProducer theProducer, Session session) {

        try {
            theProducer.close();
        } catch (JMSException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        try {
            session.close();
        } catch (JMSException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    public void sendMessage(String topicName, String pClientId, String message, long n) throws Exception {
        // public void sendMessage(String ismserver, Integer ismport, String topicName, String pClientId, String
        // message,
        // long n) throws Exception {
        Session producerSession = null;
        Connection producerConn = null;
        MessageProducer theProducer = null;
        ImaJmsException[] errstr = null;

        try {

            if (connectionFactory == null) {
                // System.out.print(" factory(" + tid + ")");
                connectionFactory = ImaJmsFactory.createConnectionFactory();
                // System.out.print(". ");

                ((ImaProperties) connectionFactory).put("Server", ismserver);
                ((ImaProperties) connectionFactory).put("Port", ismport);
                // ((ImaProperties) connectionFactory).put("KeepAlive", 0);
                if (ssl)
                    ((ImaProperties) connectionFactory).put("Protocol", "tcps");
                else
                    ((ImaProperties) connectionFactory).put("Protocol", "tcp");

                errstr = ((ImaProperties) connectionFactory).validate(ImaProperties.WARNINGS);
                if (errstr != null) {
                    for (int i = 0; i < errstr.length; i++)
                        SVTLog.log(TYPE, "" + errstr[i]);
                    System.exit(1);
                }
            }

            int i = 0;
            producerConn = null;
            SVTLog.logtry(TYPE, "connection");
            while (producerConn == null) {
                try {
                    if (password != null)
                        producerConn = connectionFactory.createConnection(userName, password);
                    else
                        producerConn = connectionFactory.createConnection();
                } catch (JMSException e) {
                    producerConn = null;
                    if (i++ < 100) {
                        SVTLog.logretry(TYPE, "connection", i);
                        Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                    } else
                        throw e;
                }
            }
            SVTLog.logdone(TYPE, "connection");
            if (producerConn == null) {
                throw new Exception("producerConn(" + tid + ") == null");
            }

            producerConn.setClientID(pClientId);
            producerSession = null;
            SVTLog.logtry(TYPE, "session");
            while (producerSession == null) {
                try {
                    producerSession = producerConn.createSession(false, Session.AUTO_ACKNOWLEDGE);
                } catch (JMSException e) {
                    producerSession = null;
                    if (i++ < 100) {
                        SVTLog.logretry(TYPE, "session", i);
                        Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                    } else
                        throw e;
                }
            }
            SVTLog.logdone(TYPE, "session");

            Topic topic = producerSession.createTopic(topicName);
            theProducer = producerSession.createProducer(topic);
            theProducer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);

            TextMessage textmessage = producerSession.createTextMessage(message);
            // Topic replyTo = producerSession.createTopic(replyName);
            // textmessage.setJMSReplyTo(replyTo);

            SVTLog.logtry(TYPE, "publish[" + message + "]");
            i = 0;
            boolean sent = false;
            while (sent == false) {
                try {
                    theProducer.send(textmessage);
                    sent = true;
                } catch (JMSException e) {
                    if (i++ >= 100) {
                        SVTLog.logex(TYPE, "publish", e);
                        throw e;
                    }
                    try {
                        Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                    } catch (InterruptedException e2) {
                        SVTLog.logex(TYPE, "sleep", e);
                    }
                    SVTLog.logretry(TYPE, "publish", e, i);
                }
            }
            SVTLog.logdone(TYPE, "publish", "");            SVTLog.logdone(TYPE, "publish", n);

            theProducer.close();
            producerSession.close();
            SVTLog.logtry(TYPE, "disconnect");
            producerConn.close();
            SVTLog.logdone(TYPE, "disconnect");

        } catch (JMSException e) {
            SVTLog.log(TYPE, tid + " - Exception occurred with Consumer Session-Connection: " + e.toString());
            e.printStackTrace();
            throw e;
        }

    }

    public void actOnMessage(SVTMessage message) throws Throwable {
        String pClientId = SVTUtil.MyClientId(TYPE);

        String incoming_topic = message.getTopicName();
        System.out.println(incoming_topic);
        String[] token = message.getTopicTokens();
        String outgoing_topic = "/APP/1/" + token[3] + "/" + token[4] + "/" + token[1] + "/" + token[2];

        if ("UNLOCK".equals(message.getMessageText())) {
            SVTLog.log(TYPE, "Doors unlocked");
            sendMessage(outgoing_topic, pClientId, "Doors unlocked", 0L);
        } else if ("LOCK".equals(message.getMessageText())) {
            SVTLog.log(TYPE, "Doors locked");
            sendMessage(outgoing_topic, pClientId, "Doors locked", 0L);
        } else if ("GET_DIAGNOSTICS".equals(message)) {
            SVTLog.log(TYPE, "Unsupported feature");
        }
    }

    public void run() {

        String lClientId = SVTUtil.MyClientId(TYPE, "l");
        String topicName = "/CAR/" + carId + "/#";

        SVTMessage alert = null;
        try {
            SVTLog.log(TYPE, "SVTVehicle_ListenerThread");
            while (true) {
                alert = null;
                // alert = getMessage(ismserver, ismport, topicName, lClientId);
                alert = getMessage(topicName, lClientId);
                if (alert != null) {
                    SVTLog.log(TYPE, "Alert received:  " + alert.getMessageText());
                    actOnMessage(alert);
                }
            }
        } catch (Throwable e) {
            SVTLog.logex(TYPE, "getMessage|actOnMessage", e);
        }
        return;
    }

    public void start() {
        // create thread for Message Listener
        String topicName = "/APP/1/CAR/" + carId + "/CAR/" + carId;
        Thread listener = new Thread(new SVTVehicle_JMS(ismserver, ismport, carId, ssl, userName, password));
        listener.start();

        String pClientId = SVTUtil.MyClientId(TYPE, "p");
        String cClientId = SVTUtil.MyClientId(TYPE, "c");

        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));

        SVTMessage message = null;

        SVTLog.log(TYPE, "commands are exit, get_alerts, send_gps, send_errorcode, update_system:  ");

        try {
            command = br.readLine();
            while (!("exit".equalsIgnoreCase(command))) {
                if ("get_alerts".equalsIgnoreCase(command)) {
                    sendMessage(topicName, pClientId, "QUERY_ALERT", 0);
                    message = getMessage(topicName, cClientId);
                    SVTLog.log(TYPE, "Reply message:  " + message);
                    while (message != null) {
                        actOnMessage(message);
                        message = getMessage(topicName, cClientId);
                        SVTLog.log(TYPE, "Reply message:  " + message.getMessageText());
                    }
                } else if ("send_gps".equalsIgnoreCase(command)) {
                    int x = (int) (Math.random() * 10000.0);
                    int y = (int) (Math.random() * 10000.0);
                    sendMessage(topicName, pClientId, "GPS " + x + ", " + y + ")", 0);
                    SVTLog.log(TYPE, "sent message: gps(" + x + ", " + y + ")");
                    message = getMessage(topicName, cClientId);
                    SVTLog.log(TYPE, "Reply message:  " + message.getMessageText());
                } else if ("send_errorcode".equalsIgnoreCase(command)) {
                    int x = (int) (Math.random() * 10000.0);
                    sendMessage(topicName, pClientId, "RECORD_ERRORCODE " + x + ")", 0);
                    SVTLog.log(TYPE, "sent message: errorcode(" + x + ")");
                    message = getMessage(topicName, cClientId);
                    SVTLog.log(TYPE, "Reply message:  " + message.getMessageText());
                } else if ("update_system".equalsIgnoreCase(command)) {
                    sendMessage(topicName, pClientId, "SYSTEM_UPDATE", 0);
                    SVTLog.log(TYPE, "sent message: update_system");
                    message = getMessage(topicName, cClientId);
                    SVTLog.log(TYPE, "Reply message:  " + message.getMessageText());
                } else {
                    SVTLog.log(TYPE, "Did not understand command:  " + command);
                }
                SVTLog.log(TYPE, "commands are exit, get_alerts, send_gps, send_errorcode, update_system:  ");
                command = br.readLine();
            }
        } catch (Throwable e) {
            SVTLog.logex(TYPE, "sendMessage|getMessage", e);
        }
    }

    public static void main(String[] args) {
        String ismserver = null;
        Integer ismport = null;
        long carId = 0;
        boolean ssl = false;
        String userName = null;
        String password = null;

        if (args.length == 6) {
            ismserver = args[0];
            ismport = Integer.parseInt(args[1]);
            carId = Long.parseLong(args[2]);
            ssl = Boolean.parseBoolean(args[3]);
            userName = args[4];
            password = args[5];
        } else {
            System.out.println("Usage: <ismserver> <ismport> <carId> <ssl> <userName> <password> ");
        }
        SVTVehicle_JMS vehicle = new SVTVehicle_JMS(ismserver, ismport, carId, ssl, userName, password);
        vehicle.start();
    }

    public void startListener() {
        // TODO Auto-generated method stub

    }

    public void stopListener() {
        // TODO Auto-generated method stub

    }
}
