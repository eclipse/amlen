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

import java.util.Collections;
import java.util.Hashtable;
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;

import javax.jms.BytesMessage;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaSubscription;

/**
 * Class to encapsulate the JMS message consumer logic.
 * 
 * This class contains the doSubscribe method for subscribing to JMS messages.
 * 
 */

public class JMSSampleReceive {

    private List<String> receivedMessageList = Collections.synchronizedList(new LinkedList<String>());
    // Stack<String> ostack = new Stack<String>();
    // public Lock receiverMutex = new ReentrantLock();
    JMSSample config;

    /**
     * Synchronized method for indicating all messages received.
     * 
     * @param setDone
     *            optional Boolean parameter indicating all messages received.
     * 
     * @return Boolean flag
     */
    private Boolean doneFlag = false;

    public void sleep(long i) {
        try {
            Thread.sleep(i);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    synchronized public boolean receiverIsDone(Boolean done, String message) {
        if ((done != null) && (done == true)) {
            config.log.println(message);
            doneFlag = done;
        }
        return doneFlag;
    }

    private Boolean resendDoneFlag = false;

    synchronized public boolean isResendDone(Boolean done, String message) {
        if ((done != null) && (done == true)) {
            config.log.println(message);
            resendDoneFlag = done;
        }
        return resendDoneFlag;
    }

    public boolean doHAReceive(JMSSample config) throws JMSException {

        boolean done = false;

        while (!done) {
            done = doReceive(config);
            if (!done) {
                config.log.println("Client not done... reconnecting");
                while (config.isConnClosed()) {
                    if (!config.doConnect())
                        sleep(1000);
                }
                if ((config.subscribeTimeSec > 0)
                                && ((System.currentTimeMillis() - config.startTime) >= (config.subscribeTimeSec * 1000l))) {
                    break;
                }
            }
        }
        return done;

    }

    /**
     * receive messages sent to the destination
     * 
     * @param config
     *            JMSSample instance containing configuration settings
     * 
     * @throws JMSException
     *             the JMS exception
     */
    public boolean doReceive(JMSSample config) throws JMSException {
        this.config = config;

        // config.connect();

        Session session = config.connection.createSession(false, Session.AUTO_ACKNOWLEDGE);

        // The JMS Destination administered objects are loaded from a JNDI
        // repository when specified in the command line arguments.
        // Otherwise the JMS Destination is created from the session object by specifying the destination name.

        Destination dest = null;
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

        MessageConsumer consumer = null;

        // Parameter checks in JMSSample ensure isDurable is only set for topics
        if ((config.isDurable) && (config.shared == null)) {
            consumer = session.createDurableSubscriber((Topic) dest, config.clientId);
        } else if ((config.isDurable) && (config.shared != null)) {
            consumer = ((ImaSubscription) session).createSharedDurableConsumer((Topic) dest, config.shared);
        } else if ((!config.isDurable) && (config.shared != null)) {
            consumer = ((ImaSubscription) session).createSharedConsumer((Topic) dest, config.shared);
        } else {
            consumer = session.createConsumer(dest);
        }

        if ((config.count > 0) || (config.subscribeTimeSec != 0)) {
            config.connection.start();
            config.log.println("Client " + config.clientId + " ready to consume messages from " + config.destType
                            + ": '" + config.destName + "'.");

            Thread orderMessage = null;
            if (config.order == true) {
                orderMessage = new Thread(new orderMessage());
                orderMessage.start();
            }

            Thread resendMessage = null;
            if ((config.resendServer != null) && (config.resendPort != null)) {
                resendMessage = new Thread(new resendMessage(config.resendID, config));
                resendMessage.start();
            }

            do {
                // consumer.receive will block for 1 seconds waiting for a message
                // to arrive
                Message msg = null;
                try {
                    msg = consumer.receive((config.timeout == 0) ? 1000 : config.timeout * 1000);
                } catch (JMSException e) {
                    config.log.println("Exception caught in JMS sample:  " + e.getMessage());
                    if (config.isConnClosed()) {
                        return false;
                    } else {
                        throw e;
                    }
                }
                if (msg != null) {
                    config.recvcount++;
                    if (config.recvcount == 1)
                        config.log.println("Client " + config.clientId + " has received first message on "
                                        + config.destType + " '" + getDestName(msg) + "'");

                    if (config.verbose) {
                        String id = (config.clientId == null) ? config.userName : config.clientId;
                        // Only expect to receive text messages, but check before cast.
                        if (msg instanceof TextMessage) {
                            config.log.println("Client " + id + " has received TextMessage " + config.recvcount + " on "
                                            + config.destType + " '" + getDestName(msg) + "': "
                                            + ((TextMessage) msg).getText());
                        } else if (msg instanceof BytesMessage) {
                            config.log.println("Client " + id + " Message " + config.recvcount + " received on topic '"
                                            + getDestName(msg) + "': " + ((BytesMessage) msg).getBodyLength()
                                            + " bytes");
                        } else {
                            config.log.println("Client " + id + " has received Message " + config.recvcount + " on "
                                            + config.destType + " '" + getDestName(msg));
                        }
                    }

                    if (((config.order == true) || (config.resendServer != null)) && (msg instanceof TextMessage)) {
                        // receiverMutex.lock();
                        String text = ((TextMessage) msg).getText();
                        receivedMessageList.add(text);
                        // receiverMutex.unlock();
                    }

                    if (config.throttleWaitMSec > 0) {
                        long currTime = System.currentTimeMillis();
                        double elapsed = (double) (currTime - config.startTime);
                        double projected = ((double) config.recvcount / (double) config.throttleWaitMSec) * 1000.0;
                        if (elapsed < projected) {
                            double sleepInterval = projected - elapsed;
                            try {
                                Thread.sleep((long) sleepInterval);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    }

                }
                // else if (config.timeout > 0) {
                // break;
                // }

                if ((config.subscribeTimeSec > 0)
                                && ((System.currentTimeMillis() - config.startTime) >= (config.subscribeTimeSec * 1000l))) {
                    config.log.println("Received " + config.recvcount + " messages in "
                                    + (System.currentTimeMillis() - config.startTime) / 1000l + " seconds");
                    receiverIsDone(true, "doReceive is setting isDone(true) because "
                                    + (System.currentTimeMillis() - config.startTime) + " >= "
                                    + (config.subscribeTimeSec * 1000l));
                } else if ((config.subscribeTimeSec == 0) && (config.recvcount == config.count)) {
                    config.log.println("Received " + config.recvcount + " messages in "
                                    + (System.currentTimeMillis() - config.startTime) / 1000l + " seconds");
                    receiverIsDone(true, "doReceive is setting receiverIsDone(true) because " + config.count
                                    + " messages were received.");
                } else if ((config.count <= 0) && (config.subscribeTimeSec == 0)) {
                    receiverIsDone(true, "doReceive is setting receiverIsDone(true) because config.count is "
                                    + config.count + " and config.subscribeTimeSec is " + config.subscribeTimeSec);
                }

            } while (!receiverIsDone(null, null));

            try {
                Thread.sleep(100);
            } catch (InterruptedException e1) {
                e1.printStackTrace();
            }

            if (config.order) {
                try {
                    orderMessage.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            config.log.println("Client " + config.clientId + " received " + config.recvcount + " messages.");

            config.connection.stop();
            if (config.resendServer != null) {
                try {
                    resendMessage.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }

        consumer.close();

        if (config.deleteDurableSubscription) {
            session.unsubscribe(config.clientId);
            config.log.println("Client " + config.clientId + " removed durable subscription.");
        }

        session.close();
        if (config.verbose) {
            config.log.println("Subscriber disconnected.");
            config.log.println("doSubscribe return.");
        }
        return true;
    }

    /**
     * Gets the topic name from a message.
     * 
     * @param msg
     *            The message from which the topic name is extracted.
     * 
     * @throws JMSException
     *             the jMS exception
     */
    public static String getDestName(Message msg) throws JMSException {
        String destName = null;
        Destination dest = msg.getJMSDestination();
        if ((dest != null) && (dest instanceof Queue)) {
            destName = ((Queue) dest).getQueueName();
        } else if ((dest != null) && (dest instanceof Topic)) {
            destName = ((Topic) dest).getTopicName();
        }
        return destName;
    }

    class orderMessage implements Runnable {
        Hashtable<String, Long> table = new Hashtable<String, Long>();
        Long previous = 0L;
        Long incoming = 0L;
        private List<String> mystack = Collections.synchronizedList(new LinkedList<String>());
        private List<String> failures = Collections.synchronizedList(new LinkedList<String>());
        String failure = null;
        String entrytext = null;
        String[] words = null;
        boolean sleep = false;
        boolean pass = true;
        boolean done = false;
        int incomingqos = 0;
        boolean empty = false;
        int qos = 0;
        String pubId = null;
        long verifyCount = 0;

        public void run() {
            if (config.verbose) {
                config.log.println("orderMessage.run() entry");
            }
            verifyCount = 0;
            // receiverMutex.lock();
            done = receiverIsDone(null, null);
            empty = receivedMessageList.isEmpty();
            // receiverMutex.unlock();
            while (!done || !empty) {
                // receiverMutex.lock();
                sleep = true;
                if (receivedMessageList.size() > 0) {
                    sleep = false;
                    if (config.debug) {
                        config.log.println("orderMessage verifying " + receivedMessageList.size()
                                        + " additional messages.  (" + (receivedMessageList.size() + verifyCount)
                                        + " total)");
                    }
                    while (receivedMessageList.isEmpty() == false) {
                        mystack.add(receivedMessageList.remove(0));
                        verifyCount++;
                    }
                }
                // receiverMutex.unlock();

                while (mystack.isEmpty() == false) {
                    entrytext = mystack.remove(0);
                    words = entrytext.split(":");
                    if ("ORDER".equals(words[0])) {
                        pubId = words[1];
                        previous = table.get(pubId);
                        incomingqos = Integer.parseInt(words[2]);
                        qos = Math.min(config.qos, incomingqos);
                        incoming = Long.parseLong(words[3]);
                        if ((qos == 0) && ((previous == null) || (incoming > previous))) {
                            table.put(pubId, incoming);
                        } else if ((qos == 1) && ((previous == null) && (incoming == 0))) {
                            table.put(pubId, incoming);
                        } else if ((qos == 1)
                                        && ((previous != null) && ((incoming == previous) || (incoming == (previous + 1))))) {
                            table.put(pubId, incoming);
                        } else if ((qos == 2) && ((previous == null) && (incoming == 0))) {
                            table.put(pubId, incoming);
                        } else if ((qos == 2) && ((previous != null) && (incoming == (previous + 1)))) {
                            table.put(pubId, incoming);
                        } else {
                            table.put(pubId, incoming);
                            config.log.printErr("Order Error from " + pubId + " Qos (sent:" + incomingqos + " recv:"
                                            + config.qos + " comb:" + qos + "), " + " index " + words[3]
                                            + " is incorrect.  Previous index was " + previous);
                            pass = false;
                            // System.exit(1);
                        }
                    }
                }
                if (sleep == true) {
                    try {
                        Thread.sleep(500);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                // receiverMutex.lock();
                done = receiverIsDone(null, null);
                empty = receivedMessageList.isEmpty();
                // receiverMutex.unlock();
                if (empty && done) {
                    int sleepCount = 0;
                    while ((empty) && (sleepCount < 20)) {
                        try {
                            Thread.sleep(500);
                            sleepCount++;
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        empty = receivedMessageList.isEmpty();
                    }
                }
            }

            if ((table.size() > 0) && (pass == true)) {
                config.log.println("Message Order Pass");
            } else {
                config.log.println("\n");
                config.log.println("Failure Report");
                config.log.println("--------------");
                config.log.println("-  Pass: " + pass);
                config.log.println("\n");
                config.log.println("-  Number of senders: " + table.size());
                if (table.size() > 0) {
                    config.log.println("-  List of senders:");
                    for (Entry<String, Long> entry : table.entrySet()) {
                        config.log.println("-  " + entry.getKey());
                    }
                }
                config.log.println("\n");
                config.log.println("-  Number of failures:" + failures.size());
                if (failures.size() > 0) {
                    config.log.println("-  List of failures:");
                    while (failures.isEmpty() == false) {
                        config.log.println("-  " + failures.remove(0));
                    }
                }
                config.log.println("--------------\n");
            }

            if (config.verbose) {
                config.log.println("orderMessage.run() exit");
            }
            return;
        }
    }

    class resendMessage implements Runnable {
        private List<String> resendStack = Collections.synchronizedList(new LinkedList<String>());

        String entrytext = null;
        String entry = null;
        String[] words = null;
        boolean sleep = false;
        JMSSample config = null;
        JMSSample resendConnect = null;
        JMSSampleSend resendPublisher = null;
        long startTime = 0;
        long resentCount = 0;

        resendMessage(String resendClientId, JMSSample config) {

            this.config = new JMSSample(config);
            this.config.action = "publish";
            this.config.clientId = resendClientId;
            this.config.destName = config.resendTopic;
            this.config.destType = "topic";
            this.config.isDurable = true;
            this.config.order = false;
            this.config.payload_type = "text";
            this.config.server = config.resendServer;
            this.config.port = config.resendPort;
            this.config.resendServer = null;
            this.config.resendPort = null;

        }

        public void run() {

            try {
                // if (config.debug) {
                config.log.println(config.clientId + " creating producer for " + config.server + ":" + config.port
                                + " on topic " + config.destName + "\n");
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                // }
                resendConnect = new JMSSample(config);

                resendConnect.connect();
                resendPublisher = new JMSSampleSend(resendConnect);

                // receiverMutex.lock();
                boolean empty = receivedMessageList.isEmpty();
                boolean subscriberDone = receiverIsDone(null, null);
                String payload = null;
                // receiverMutex.unlock();
                String id = (config.clientId == null) ? config.userName : config.clientId;

                while ((subscriberDone == false) || (empty == false)) {
                    if (config.debug) {
                        config.log.println("subscriberDone:  " + subscriberDone);
                        config.log.println("receivedMessageList.isEmpty:  " + empty);
                    }
                    // receiverMutex.lock();
                    // while (receivedMessageList.isEmpty() == false) {
                    // payload = receivedMessageList.remove(0);
                    // if (config.debug) {
                    // config.log.println("Moving payload:  " + payload + " to resend list.");
                    // }
                    // resendStack.add(payload);
                    // }
                    // receiverMutex.unlock();

                    sleep = receivedMessageList.isEmpty();
                    while (receivedMessageList.isEmpty() == false) {
                        entry = receivedMessageList.remove(0);
                        entrytext = id + ":" + entry;

                        resendPublisher.sendHA(entrytext);
                        resentCount++;

                    }
                    if (sleep == true) {
                        try {
                            Thread.sleep(500);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }

                    // receiverMutex.lock();
                    empty = receivedMessageList.isEmpty();
                    subscriberDone = receiverIsDone(null, null);
                    if (empty && subscriberDone) {
                        Thread.sleep(500);
                        empty = receivedMessageList.isEmpty();
                        subscriberDone = receiverIsDone(null, null);
                    }
                    // receiverMutex.unlock();

                }
            } catch (JMSException e1) {
                e1.printStackTrace();
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

            while (receiverIsDone(null, null) == false) {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            isResendDone(
                            true,
                            "ResendMessage.run() is setting isResendDone(true) because receiverQueue is empy, isDone is true, and resendList contains "
                                            + receivedMessageList.size() + " messages.");

            if (config.verbose) {
                config.log.println("resendMessage.run() resent " + resentCount + " messages.");
                config.log.println("resendMessage.run() calling resendPublisher.publisher_disconnect()");
            }
            resendPublisher.publisher_disconnect();
            resendConnect.disconnect();

            if (config.verbose) {
                config.log.println("resendMessage.run() calling return.");
            }
            return;
        }

    }

}
