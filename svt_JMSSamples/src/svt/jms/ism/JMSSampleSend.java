/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
package svt.jms.ism;

import javax.jms.BytesMessage;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;

/**
 * class that encapsulates the IMA message send logic.
 * 
 * This class contains the doPublish method for sending messages to a destination.
 * 
 */
public class JMSSampleSend {
    JMSSample config = null;
    long iCount = 0;
    Session session = null;
    Destination dest = null;
    MessageProducer producer = null;
    long sent_count = 0;
    Long startTime = null;

    public JMSSampleSend(JMSSample config) {
        this.config = config;
        setup();
    }

    public JMSSampleSend(String clientId, JMSSample config) {
        this.config = config;
        this.config.clientId = new String(clientId);
        setup();
    }
    
    private Boolean doneFlag = false;

    synchronized public boolean senderIsDone(Boolean done, String message) {
        if ((done != null) && (done == true)) {
            config.log.println(message);
            doneFlag = done;
        }
        return doneFlag;
    }

    private void setup() {

        try {
            session = config.connection.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
            if (session == null) {
                System.err.println("ERROR:  Unable to create JMS Session");
                System.exit(1);
            }

            // The JMS Destination administered objects are loaded from a JNDI
            // repository when specified in the command line arguments.
            // Otherwise the JMS Destination is created from the session object by specifying the destination name.

            if (config.jndi_context != null) {
                dest = (Destination) config.retrieveFromJndi(config.jndi_context, config.destName);

                if (dest == null) {
                    System.err.println("ERROR:  Unable to retrieve JNDI " + config.destType + ":  " + config.destName);
                    System.exit(1);
                }

            } else {
                if ("topic".equals(config.destType))
                    dest = session.createTopic(config.destName);
                else
                    dest = session.createQueue(config.destName);

                if (dest == null) {
                    System.err.println("ERROR:  Unable to create " + config.destType + ":  " + config.destName);
                    System.exit(1);
                }
            }

            producer = session.createProducer(dest);
            if (producer == null) {
                System.err.println("ERROR:  Unable to create JMS Producer");
                System.exit(1);
            }

            // Parameter checks in JMSSample ensure persistence is only set for topics
            if (config.persistence) {
                producer.setDeliveryMode(DeliveryMode.PERSISTENT);
            } else {
                producer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
            }

            if (config.timetoliveSeconds != 0) {
                producer.setTimeToLive(config.timetoliveSeconds*1000L);
            }
            
        } catch (JMSException e) {
            e.printStackTrace();
        }

    }

    public boolean sendHA(String message) throws JMSException {
        boolean sent = false;
        Message msg = null;
        try {
            msg = session.createTextMessage(message);
            if (msg != null)
                sent = send(msg);
        } catch (Exception ex) {
            //ex.printStackTrace(System.err);
        }
        int count = 0;
        while (!sent && config.isConnClosed() && (++count < 100)) {
            try {
                if (config.doConnect()) {
                    setup();
                    msg = session.createTextMessage(message);
                    if (msg != null)
                        sent = send(msg);
                }
            } catch (Exception e) {
                config.log.println("unable to reconnect ... attempt " + count);
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e1) {
                    // TODO Auto-generated catch block
                    // e1.printStackTrace();
                }
            }
        }
        return sent;
    }

    public boolean send(byte[] bytes) throws JMSException {
        Message msg = session.createBytesMessage();
        ((BytesMessage) msg).writeBytes(bytes);
        return send(msg);
    }

    public boolean send(Message msg) throws JMSException {
        boolean sent = false;

        if (startTime == null) {
            startTime = System.currentTimeMillis();
        }

        if (config.throttleWaitMSec > 0) {
            long currTime = System.currentTimeMillis();
            double elapsed = (double) (currTime - startTime);
            double projected = ((double) iCount / (double) config.throttleWaitMSec) * 1000.0;
            if (elapsed < projected) {
                double sleepInterval = projected - elapsed;
                try {
                    Thread.sleep((long) sleepInterval);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }

        if (sent = sendMessage(msg))
            iCount++;

        if (config.verbose) {
            if (msg instanceof BytesMessage) {
                ((BytesMessage) msg).reset();
                config.log.println(iCount + ": Sending " + ((BytesMessage) msg).getBodyLength() + " bytes to "
                                + config.destType + " " + config.destName);
            } else if (msg instanceof TextMessage) {
                String text = ((TextMessage) msg).getText();
                if (text.length() < 80)
                    config.log.println(iCount + ": Client '" + config.clientId + " sending \"" + text + "\" to "
                                    + config.destType + " " + config.destName);
                else
                    config.log.println(iCount + ": Sending " + text.substring(0, 29) + "... (" + text.length()
                                    + " chars total) to " + config.destType + " " + config.destName);
            }
        }

        return sent;
    }

    private boolean sendMessage(Message msg) {
        boolean sent = false;
        int count = 0;
        while (sent == false) {
            try {
                producer.send(msg);
                sent = true;
                sent_count++;
            } catch (JMSException e) {
                if (++count > 10)
                    break;
                else
                    try {
                        Thread.sleep(500);
                    } catch (InterruptedException e2) {
                        // e2.printStackTrace();
                    }
                System.out.println(e.getMessage() + "... retry " + count);
            }
        }
        return sent;
    }

    public void publisher_connect() {
    }

    public void publisher_disconnect() {
    }

    /**
     * Send message to the specified destination
     * 
     * @param config
     *            JMSSample instance containing configuration settings
     * 
     * @throws JMSException
     *             the JMS exception
     */
    public void topDoSendLoop() throws JMSException {

        config.log.println("Client '" + config.clientId + "' ready to send to " + config.destType + ": '"
                        + config.destName + ".");
        String text = null;
        boolean sent = false;

        int sentcount = 0;
        
        if (startTime == null) {
            startTime = System.currentTimeMillis();
        }
        
        while (!senderIsDone(null, null)){
            if (config.order == true) {
                text = "ORDER:" + config.clientId + ":" + config.qos + ":" + sentcount + ":" + (config.count - 1) + ":"
                                + new String(config.payload);
                sent = sendHA(text);
            } else if ("bytes".equals(config.payload_type)) {
                sent = send(config.payload);
            } else if ("text".equals(config.payload_type)) {
                sent = sendHA(new String(config.payload));
            }

            if (sent == false)
                break;
            sentcount++;
            
            if ((config.subscribeTimeSec > 0)
                            && ((System.currentTimeMillis() - startTime) >= (config.subscribeTimeSec * 1000l))) {
                config.log.println("Sent " + sentcount + " messages in "
                                + (System.currentTimeMillis() - startTime) / 1000l + " seconds");
                senderIsDone(true, "topDoSendLoop is setting isDone(true) because "
                                + (System.currentTimeMillis() - startTime) + " >= "
                                + (config.subscribeTimeSec * 1000l));
            } else if ((config.subscribeTimeSec == 0) && (sentcount == config.count)) {
                config.log.println("Sent " + sentcount + " messages in "
                                + (System.currentTimeMillis() - startTime) / 1000l + " seconds");
                senderIsDone(true, "topDoSendLoop is setting receiverIsDone(true) because " + config.count
                                + " messages were sent.");
            } else if ((config.count <= 0) && (config.subscribeTimeSec == 0)) {
                senderIsDone(true, "topDoSendLoop is setting receiverIsDone(true) because config.count is "
                                + config.count + " and config.subscribeTimeSec is " + config.subscribeTimeSec);
            }
            
        }

        config.log.println("Client " + config.clientId + " sent " + sent_count + " messages to " + config.destType
                        + " " + config.destName);

        config.connection.close();
    }

}
