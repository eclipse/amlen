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

package svt.mqtt.mq;


import java.util.Collections;
import java.util.Hashtable;
import java.util.LinkedList;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttAsyncClient;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttPersistenceException;
import org.eclipse.paho.client.mqttv3.persist.MqttDefaultFilePersistence;

/**
 * Class to encapsulate the ISM topic subscribe logic.
 * 
 * This class contains the doSubscribe method for subscribing to ISM topic messages. It also defines the necessary MQTT
 * callbacks for asynchronous subscriptions.
 * 
 */

public class MqttSampleSubscribe implements MqttCallback {
//    Byte[] lock = new Byte[1];
//    Byte[] cLock = new Byte[1];

    MqttSample config = null;
    // String clientId = null;
    public List<String> receivedMessageList = Collections.synchronizedList(new LinkedList<String>());
    AtomicLong receivedCount = new AtomicLong(0L);
    // private Lock cmutex = new ReentrantLock();
    // public Lock subscriberMutex = new ReentrantLock();
    MqttAsyncClient client = null;
    MqttConnectOptions options = null;
    Long batch = 100000L;
//    long startTime = 0;
    MqttDefaultFilePersistence dataStore = null;
    IMqttToken conToken = null;
    IMqttToken subToken = null;
    Long qosCount[] = { 0L, 0L, 0L };



    /**
     * Callback invoked when the MQTT connection is lost
     * 
     * @param e
     *            exception returned from broken connection
     * 
     */
    
    public void connectionLost(Throwable e) {
        config.log.println("Lost connection to " + config.serverURI + ".  " + e.getMessage());
        if (shouldReconnect(null, null)) {

            long remainingTime = config.time.remainingRunTimeMillis();
            // if more than 1000ms runtime remain then sleep 500ms to prevent overwhelming the server
            if (remainingTime > 1000) {
                sleep(500);
                // test if reconnect flag is still true after sleeping
                if (shouldReconnect(null, null)) {
                    subscriber_connect(true);
                }
            } else if (remainingTime > 500) {
                // if less than 1000ms runtime remain (and more than 500ms) then sleep 200ms
                sleep(200);
                // test if reconnect flag is still true after sleeping
                if (shouldReconnect(null, null)) {
                    subscriber_connect(true);
                }
            }
        }

    }
    

    public void sleep(long s) {
        try {
            Thread.sleep(s);
        } catch (InterruptedException e2) {
            System.exit(1);
        }
    }

    public synchronized void subscriber_connect(boolean verbose) {
        if (config.debug)
            config.log.println("entering subscriber_connect");

        int currentConnectionRetry = 0;
        while ((client.isConnected() == false) && shouldReconnect(null, null) && !subscriberIsDone(null, null) && config.time.runTimeRemains()) {
            try {
            	if ( config.connectionRetry != -1 ){
            		currentConnectionRetry++;
            	}
                conToken = null;
                if (verbose)
                    config.log.printTm("Subscriber " + this.client.getClientId() + " connecting ...");
                conToken = client.connect(options);
                if (verbose)
                    config.log.print("waiting...");
                while ((conToken != null) && (conToken.isComplete() == false)) {
                    conToken.waitForCompletion();
                }
                if (verbose)
                    config.log.println("\n Established connection to " + config.serverURI);
            } catch (MqttException ex) {
               String exMsg = ex.getMessage();
                String m = "failed connection to " + this.client.getServerURI() + ":  " + exMsg;
                if (ex.getCause() != null) {
                    m = m + ", caused by " + ex.getCause().getMessage();
                }
                m = m + "\n";
                config.log.print(m);
                
                if (ex.getReasonCode()==MqttException.REASON_CODE_NOT_AUTHORIZED) {
                   shouldReconnect(false, exMsg + " - Aborting connection attempts because " + exMsg +". ");
                }
                else if ( currentConnectionRetry >= config.connectionRetry && config.connectionRetry != -1 ){
                    shouldReconnect(false, exMsg + " - Aborting connection attempts after " + currentConnectionRetry + " failures. ");
                    //config.log.println(exMsg);
                }
                sleep(500);
            } catch (Exception ex) {
            	String exMsg = ex.getMessage();
                String m = "failed connection to " + this.client.getServerURI() + ":  " + exMsg;
                if (ex.getCause() != null) {
                    m = m + ", caused by " + ex.getCause().getMessage();
                }
                m = m + "\n";
                config.log.print(m);
                
                if ( currentConnectionRetry >= config.connectionRetry && config.connectionRetry != -1 ){
                    shouldReconnect(false, exMsg + " - Aborting connection attempts after " + currentConnectionRetry + " failures. ");
                    //config.log.println(exMsg);
                }
                sleep(500);
            }

        }
        if (config.debug)
            config.log.println("exiting subscriber_connect");
    }

    public void subscriber_disconnect(boolean verbose) {
        while (client.isConnected() == true) {
            try {
                conToken = null;
                if (verbose)
                    config.log.printTm("Subscriber disconnecting ...");
                conToken = client.disconnect();
                if (verbose)
                    config.log.print("waiting...");
                while ((conToken != null) && (conToken.isComplete() == false)) {
                    conToken.waitForCompletion();
                }
                if (verbose)
                    config.log.println("\n Disconnected from " + config.serverURI);

            } catch (Exception ex) {
                config.log.println("failed.  " + ex.getMessage() + "\n");
                sleep(500);
            }
        }
    }

    /**
     * Synchronized method for indicating all messages received.
     * 
     * @param setDone
     *            optional Boolean parameter indicating all messages received.
     * 
     * @return Boolean flag
     */
    private Boolean doneFlag = false;

    synchronized public boolean subscriberIsDone(Boolean done, String message) {
        if ((done != null) && (done == true)) {
            if (config.debug)
                config.log.println(message);
            doneFlag = done;
        }
        return doneFlag;
    }

    private Boolean reconnect = true;

    synchronized public boolean shouldReconnect(Boolean done, String message) {
        if (done != null) {
            if (config.debug)
                config.log.println(message);
            reconnect = done;
        }
        return reconnect;
    }

    private Boolean resendDoneFlag = false;

    synchronized public boolean isResendDone(Boolean done, String message) {
        if ((done != null) && (done == true)) {
            if (config.debug)
                config.log.println(message);
            resendDoneFlag = done;
        }
        return resendDoneFlag;
    }
   
    synchronized private long qosCount(int i, Integer n) {
        if (n != null) {
            qosCount[i] += n;
        }
        return this.qosCount[i];
    }

    boolean useWait = false;
    AtomicLong startWaitTime = new AtomicLong(0L);
    AtomicInteger maxWait = new AtomicInteger(3);
//    Byte[] waitMutex = new Byte[1];
    
    /**
     * subscribe to messages published on the topic name
     * 
     * @param config
     *            MqttSample instance containing configuration settings
     * 
     * @throws MqttException
     */
    public void run(String clientId, MqttSample config) throws MqttException {

        this.config = config;
        maxWait.set(config.initialWait);
        if((config.initialWait > 0) || (config.wait > 0)) {
        	useWait = true;
        }

        // this.clientId = new String(clientId);

        if (config.persistence)
            dataStore = new MqttDefaultFilePersistence(config.dataStoreDir);

        options = new MqttConnectOptions();
        if ((config.serverURI.indexOf(' ') != -1) || (config.serverURI.indexOf(',') != -1)
                        || (config.serverURI.indexOf('|') != -1) || (config.serverURI.indexOf('!') != -1)
                        || (config.serverURI.indexOf('+') != -1)) {
            String[] list = config.serverURI.split("[ ,|!+]");

            for (int l = 0; l < list.length; l++) {
                config.log.println("list[" + l + "] \"" + list[l] + "\"");
            }

            client = new MqttAsyncClient(list[0], clientId, dataStore);
            options.setServerURIs(list);
        } else {
            client = new MqttAsyncClient(config.serverURI, clientId, dataStore);
        }

        if ("3.1.1".equals(config.mqttVersion)) {
            options.setMqttVersion(org.eclipse.paho.client.mqttv3.MqttConnectOptions.MQTT_VERSION_3_1_1);
        } else if ("3.1".equals(config.mqttVersion)) {
            options.setMqttVersion(org.eclipse.paho.client.mqttv3.MqttConnectOptions.MQTT_VERSION_3_1);
        }
        client.setCallback(this);

        // set CleanSession true to automatically remove server data associated
        // with the client id at disconnect. In this sample the clientid is based on a random number
        // and not typically reused, therefore the release of client's server side data on
        // disconnect is appropriate.
        if (config.oauthToken != null) {
            options.setUserName("IMA_OAUTH_ACCESS_TOKEN");
            options.setPassword(config.oauthToken.toCharArray());
        } else if (config.password != null) {
            options.setUserName(config.userName);
            options.setPassword(config.password.toCharArray());
        }

        if (config.ssl == true) {
            Properties sslClientProps = new Properties();

            String keyStore = System.getProperty("com.ibm.ssl.keyStore");
            if (keyStore == null)
                keyStore = System.getProperty("javax.net.ssl.keyStore");

            String keyStorePassword = System.getProperty("com.ibm.ssl.keyStorePassword");
            if (keyStorePassword == null)
                keyStorePassword = System.getProperty("javax.net.ssl.keyStorePassword");

            String trustStore = System.getProperty("com.ibm.ssl.trustStore");
            if (trustStore == null)
                trustStore = System.getProperty("javax.net.ssl.trustStore");

            String trustStorePassword = System.getProperty("com.ibm.ssl.trustStorePassword");
            if (trustStorePassword == null)
                trustStorePassword = System.getProperty("javax.net.ssl.trustStorePassword");

            if (keyStore != null)
                sslClientProps.setProperty("com.ibm.ssl.keyStore", keyStore);
            if (keyStorePassword != null)
                sslClientProps.setProperty("com.ibm.ssl.keyStorePassword", keyStorePassword);
            if (trustStore != null)
                sslClientProps.setProperty("com.ibm.ssl.trustStore", trustStore);
            if (trustStorePassword != null)
                sslClientProps.setProperty("com.ibm.ssl.trustStorePassword", trustStorePassword);

            if (config.sslProtocol != null)
               sslClientProps.setProperty("com.ibm.ssl.protocol", config.sslProtocol);
            
            options.setSSLProperties(sslClientProps);
        }

        // KeepAliveInterval is set to 0 to keep the connection from timing out
        options.setKeepAliveInterval(config.keepAliveInterval);
        options.setConnectionTimeout(config.connectionTimeout);
        if (config.maxInflight > 0) 
        	options.setMaxInflight(config.maxInflight);
        
		// CleanSessionState indicates if you want to skip Rick's Swizzle on
		// connections for HA Failover reconnects.
		if (!config.cleanSessionState) {
			// Rick's Swizzle for HA Connections

			if (config.cleanSession) {
				options.setCleanSession(true);
				subscriber_connect(true);
				subscriber_disconnect(true);
			}

			options.setCleanSession(false);
			subscriber_connect(true);
		} else {
			options.setCleanSession(config.cleanSession);
			subscriber_connect(true);			
		}

        if ( client.isConnected() ) {
        // Subscribe to the topic. messageArrived() callback invoked when
        // message received.
//        startTime = System.currentTimeMillis();
        // if (config.monitor == false)
        	////????????????????????????????????????????????????????  ------------ RKA
        subToken = client.subscribe(config.topicName, config.qos);
        while ((subToken != null) && (subToken.isComplete() == false)) {
            subToken.waitForCompletion();
        }

        config.log.println("Client '" + client.getClientId() + "' subscribed to topic: '" + config.topicName + "' on "
                        + client.getServerURI() + " with QOS " + config.qos + ".");

        if ((config.count == 0) && (config.time.configuredRunTimeMillis() == 0)) {
            subscriberIsDone(true,
                            "doSubscribe is setting isDone(true) because config.count == 0 and config.minutes==0");
            if (config.unsubscribe) {
                // Disconnect the client
                subToken = client.unsubscribe(config.topicName);
                while ((subToken != null) && (subToken.isComplete() == false)) {
                    subToken.waitForCompletion();
                }
            }

            subscriber_disconnect(true);

        } else {
            Thread batchMessage = new Thread(new batchMessage());
            batchMessage.start();

            Thread orderMessage = null;
            Thread resendMessage = null;
            if (config.order == true) {
                if (config.gatherer) {
                    orderMessage = new Thread(new globalOrderMessage(this));
                } else {
                    orderMessage = new Thread(new orderMessage());
                }
                orderMessage.start();
            } else if (config.publisherURI != null) {
                resendMessage = new Thread(new resendMessage(config));
                resendMessage.start();
            }
            long lReceivedCount = 0L;
            while (!subscriberIsDone(null, null)) {
                lReceivedCount = receivedCount.get();
                if (receivedMessageList.size() > 500000) {
                    shouldReconnect(false, "suspending reconnect because receivedMessageList.size is "
                                    + receivedMessageList.size());
                    subscriber_disconnect(false);
                    while (receivedMessageList.size() > 10000) {
                        sleep(50);
                    }
                    shouldReconnect(true, "Enabling reconnect because receivedMessageList.size is "
                                    + receivedMessageList.size());
                    subscriber_connect(false);
                }

                if (config.time.runTimeExpired()) {
                    config.log.println("Received " + receivedCount.get() + " messages in " + config.time.actualRunTimeMillis()
                                    + " ms");
                    subscriberIsDone(true, "doSubscribe is setting isDone(true) because runtime has expired");
                    synchronized (batch) {
                        batch.notify();
                    }
                }else if (((config.defaultcount==false)&&(lReceivedCount == config.count))||((config.defaultcount==true)&&(config.time.configuredRunTimeSeconds()==0))) {
                      config.log.println("Received " + receivedCount.get() + " messages.");
                      synchronized (batch) {
                          batch.notify();
                      }
                      subscriberIsDone(true,
                                      "messageArrived is setting isDone(true) because config.count has been reached");
                } else if  (((config.order == true) || (config.publisherURI != null))&&(maxWait.get() > 0) && (receivedMessageList.size() == 0)) {
                        if (startWaitTime.get() == 0)
                            startWaitTime.set(System.currentTimeMillis());
                        else if ((System.currentTimeMillis() - startWaitTime.get()) >= (maxWait.get() * 60000l)) {
                            subscriberIsDone(true, "subscriberIsDone set true since no messages received in " + maxWait.get()
                                            + " minutes");
                            synchronized (batch) {
                                batch.notify();
                            }
                    }
                }

                if ((config.throttleWaitMSec > 0)&&(!subscriberIsDone(null, null))) {
                    double elapsed = (double) config.time.actualRunTimeMillis();
                    double projected = ((double) lReceivedCount / (double) config.throttleWaitMSec) * 1000.0d;
                    double sleepInterval = projected - elapsed;

                    if (sleepInterval > 400) {
                        // going too fast
                         config.log.println("need to slow down ... Received " + receivedCount.get() + " messages in "
                         + elapsed + " milliseconds, sleepInterval is " + (long) (sleepInterval - 200));
                        shouldReconnect(false, "suspending reconnect because throttle");
                        subscriber_disconnect(false);
                        sleep((long) sleepInterval - 100);
                    } else if (sleepInterval < -400) {
                        // going too slow
                         config.log.println("need more messages... Received " + receivedCount.get() + " messages in "
                         + elapsed + " milliseconds, sleepInterval is " + (long) sleepInterval);
                        shouldReconnect(true, "Enabling reconnect because throttle");
                        subscriber_connect(false);
                        sleep(config.throttleWaitMSec/20);
                        if (sleepInterval > -1000) {
                            shouldReconnect(false, "suspending reconnect because throttle");
                            subscriber_disconnect(false);
                        }
                    }
                }
            }

            // if (config.resendURI != null) {
            // while (isResendDone(null, null)) {
            // sleep(500);
            // }
            // }

            if (config.unsubscribe) {
                // Disconnect the client
                subToken = client.unsubscribe(config.topicName);
                while ((subToken != null) && (subToken.isComplete() == false)) {
                    subToken.waitForCompletion();
                }
            }
        
            subscriber_disconnect(true);

            if (config.order) {
                try {
                    orderMessage.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            if ((config.order == false) && (config.publisherURI != null)) {
                try {
                    resendMessage.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
        }
		
        if (!config.cleanSessionState) {
			if (config.cleanSession) {
				options.setCleanSession(true);
				subscriber_connect(true);
				subscriber_disconnect(true);
			}
		}
		
        client.close();

        if (config.debug) {
            config.log.println("Subscriber disconnected.");
            config.log.println("doSubscribe return.");
        }
        return;
    }

    /**
     * Callback invoked when a message arrives.
     * 
     * @param topic
     *            the topic on which the message was received.
     * @param receivedMessage
     *            the received message.
     * @throws Exception
     *             the exception
     */
    // public void messageArrived(MqttTopic topic, MqttMessage receivedMessage) throws Exception {
    @Override
    public void messageArrived(String topic, MqttMessage receivedMessage) throws Exception {
        // increment count of received messages
      if ((config.exact == false) || (receivedCount.get() < config.count)) {
         long lreceivedcount = receivedCount.incrementAndGet();
         if (useWait) {
            startWaitTime.set(0);
         }
         if (config.order == true) {
            qosCount(receivedMessage.getQos(), 1);
         }
		
        if (config.veryverbose) {
            config.log.println("Received message " + lreceivedcount + " on topic '" + topic + "' and Qos "
                            + receivedMessage.getQos() + " " + new String(receivedMessage.getPayload())
                            + ". Dup flag is "
                            // +
                            // receivedMessage.isDuplicate()+" [messageid="+((MqttReceivedMessage)receivedMessage).getMessageId()+"]");
                            + receivedMessage.isDuplicate() + ", isRetained is " + receivedMessage.isRetained());
        }

//        if (config.time.runTimeExpired()) {
//            config.log.println("Received " + lreceivedcount + " messages.");
//            synchronized (batch) {
//                batch.notify();
//            }
//            subscriberIsDone(true,
//                            "messageArrived is setting isDone(true) because  runtime has expired");
//        } else 
//        	if (((config.defaultcount==false)&&(lreceivedcount == config.count))||((config.defaultcount==true)&&(config.time.getRunTimeSeconds()==0))) {
//            config.log.println("Received " + lreceivedcount + " messages.");
//            synchronized (batch) {
//                batch.notify();
//            }
//            subscriberIsDone(true,
//                            "messageArrived is setting isDone(true) because config.count has been reached");
//        } else 
		if (lreceivedcount == 1) {
			config.log.println("Received first message on topic '" + topic
					+ "'");
			byte[] msgBytes = receivedMessage.getPayload();
			config.log.println("MSG:" + new String(msgBytes));
			if (useWait) {
				maxWait.set(config.wait);
			}
		}

        if ((config.order == true) || (config.publisherURI != null)) {
            // subscriberMutex.lock();
            // synchronized (lock) {
            String resendText = new String(receivedMessage.getPayload());
            receivedMessageList.add(resendText+":"+receivedMessage.isDuplicate());
            // }
            // if ((config.gatherer > 0) && (receivedMessageList.size() > 2000)) {
            // while (receivedMessageList.size() > 500) {
            // sleep(100);
            // }
            // }

            // subscriberMutex.unlock();
            if (config.veryverbose)
                config.log.println(resendText + " added to end of receivedMessageList");
        }

        if ((config.verbose == true) && (lreceivedcount % batch == 0)) {
            synchronized (batch) {
                batch.notify();
            }
        }
      }
    }

    class batchMessage implements Runnable {
        long nowTime = 0;
        long prevTime = 0;
        long nowCount = 0;
        long prevCount = 0;
        long rate = 0;

        public void run() {
            while (subscriberIsDone(null, null) == false) {
                synchronized (batch) {
                    try {
                        batch.wait(20000);
                    } catch (InterruptedException e) {
                    }
                }
                nowTime = System.currentTimeMillis();
                nowCount = receivedCount.get();

                if ((prevTime > 0) && (subscriberIsDone(null, null) == false) && ((nowTime - prevTime) > 0)) {
                    rate = ((nowCount - prevCount) * 1000L) / (nowTime - prevTime);
                    config.log.println(" Received last " + (nowCount - prevCount) + " msgs at the rate " + rate
                                    + " msg/sec");
                }
                prevTime = nowTime;
                prevCount = nowCount;
            }
        }
    }

    class orderMessage implements Runnable {
        Hashtable<String, Long> publishersIndexTable = new Hashtable<String, Long>();
        Long previous = 0L;
        Long incoming = 0L;
        LinkedList<String> orderMessageList = new LinkedList<String>();
        LinkedList<String> failureList = new LinkedList<String>();;
        String failure = null;
        String entrytext = null;
        String[] words = null;
        boolean sleep = false;
        boolean pass = true;
        boolean receiverDone = false;
        int incomingqos = 0;
        boolean dup = false;
        boolean empty = false;
        int qos = 0;
        String pubId = null;

        public void run() {
            if (config.verbose) {
                config.log.println("orderMessage.run() entry");
            }

            // subscriberMutex.lock();
            receiverDone = subscriberIsDone(null, null);
            empty = receivedMessageList.isEmpty();
            // subscriberMutex.unlock();

            while (!receiverDone || !empty) {
                // subscriberMutex.lock();
                sleep = true;
                if (receivedMessageList.size() > 0) {
                    sleep = false;
                    if (config.veryverbose) {
                        config.log.println("orderMessage verifying " + receivedMessageList.size() + " messages.");
                    }
                    while (receivedMessageList.isEmpty() == false) {
                        orderMessageList.add(receivedMessageList.remove(0));
                    }
                }
                // subscriberMutex.unlock();

                while (orderMessageList.isEmpty() == false) {
                    entrytext = orderMessageList.removeFirst();
                    words = entrytext.split(":");
                    if ("ORDER".equals(words[0])) {
                        pubId = words[1];
                        previous = publishersIndexTable.get(pubId);
                        incomingqos = Integer.parseInt(words[2]);
                        qos = Math.min(config.qos, incomingqos);
                        incoming = Long.parseLong(words[3]);
                        dup = Boolean.parseBoolean(words[4]);
                        if ((qos == 0) && ((previous == null) || (incoming > previous))) {
                            publishersIndexTable.put(pubId, incoming);
                        } else if ((qos == 1) && ((previous == null) && (incoming == 0))) {
                            publishersIndexTable.put(pubId, incoming);
                        } else if ((qos == 1)
                                        && ((previous != null) && ((incoming == previous) || (incoming == (previous + 1))))) {
                            publishersIndexTable.put(pubId, incoming);
                        } else if ((qos == 1)
                              && ((previous != null) && ((incoming < previous)))) {
                           publishersIndexTable.put(pubId, incoming);
                           failure = "Order WARNING from " + pubId + " Qos (sent:" + incomingqos + " recv:" + config.qos
                                           + " comb:" + qos + "), index " + incoming + " (" + words[3] 
                                           + ") is incorrect.  Dup is " + dup + ".  Previous index was " + previous + ".";
                           failureList.addLast(failure);
                           config.log.printErr(failure);
                        } else if ((qos == 2) && ((previous == null) && (incoming == 0))) {
                            publishersIndexTable.put(pubId, incoming);
                        } else if ((qos == 2) && ((previous != null) && (incoming == (previous + 1)))) {
                            publishersIndexTable.put(pubId, incoming);
                        } else {
                            publishersIndexTable.put(pubId, incoming);
                            failure = "Order Error from " + pubId + " Qos (sent:" + incomingqos + " recv:" + config.qos
                                  + " comb:" + qos + "), index " + incoming + " (" + words[3] 
                                  + ") is incorrect.  Dup is " + dup + ".  Previous index was " + previous + ".";
                            failureList.addLast(failure);
                            config.log.printErr(failure);
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
                // subscriberMutex.lock();
                receiverDone = subscriberIsDone(null, null);
                empty = receivedMessageList.isEmpty();
                // subscriberMutex.unlock();
            }

            if ((pass == true) && (config.qos0Min <= qosCount(0, null)) && (config.qos0Max >= qosCount(0, null))
                            && (config.qos1Min <= qosCount(1, null)) && (config.qos2 == qosCount(2, null))) {
                config.log.println("Message Order Pass");
            } else {
                config.log.println("\n");
                config.log.println("Configured Qos0 Min: " + config.qos0Min);
                config.log.println("Configured Qos0 Max: " + config.qos0Max);
                config.log.println("Configured Qos1 Min: " + config.qos1Min);
                config.log.println("Configured qos2: " + config.qos2);
                config.log.println("Qos0 messages received: " + qosCount(0, null));
                config.log.println("Qos1 messages received: " + qosCount(1, null));
                config.log.println("Qos2 messages received: " + qosCount(2, null));
                config.log.println("\n");
                config.log.println("Failure Report");
                config.log.println("--------------");
                config.log.println("-  Pass: " + pass);
                config.log.println("\n");
                config.log.println("-  Number of senders: " + publishersIndexTable.size());
                if (publishersIndexTable.size() > 0) {
                    config.log.println("-  List of senders:");
                    for (String sender : publishersIndexTable.keySet()) {
                        config.log.println("-  " + sender);
                    }
                }
                config.log.println("\n");
                config.log.println("-  Number of failures:" + failureList.size());
                if (failureList.size() > 0) {
                    config.log.println("-  List of failures:");
                    while (failureList.isEmpty() == false) {
                        config.log.println("-  " + failureList.removeFirst());
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
        LinkedList<String> resendList = new LinkedList<String>();
        String entrytext = null;
        String entry = null;
        String[] words = null;
        boolean sleep = false;
        MqttSample config = null;

        // String clientId = null;

        resendMessage(MqttSample config) {
            this.config = new MqttSample(config);
            if (config.subscriberID != null)
                this.config.clientID = config.subscriberID;
            if (config.subscriberURI != null)
                this.config.serverURI = config.subscriberURI;
            if (config.subscriberTopic != null)
                this.config.topicName = config.subscriberTopic;

        }

        public void run() {
            String payload = null;
            MqttSamplePublish resendPublisher = new MqttSamplePublish(config);
            try {
                resendPublisher.doConnect();
//                long startTime = System.currentTimeMillis();
                MqttMessage resendMessage = null;

                // subscriberMutex.lock();
                boolean empty = receivedMessageList.isEmpty();
                boolean subscriberDone = subscriberIsDone(null, null);
                // subscriberMutex.unlock();

                while ((subscriberDone == false) || (empty == false)) {
                    // if (config.debug) {
                    // config.log.println("subscriberDone:  " + subscriberDone);
                    // config.log.println("receivedMessageList.isEmpty:  " + empty);
                    // }
                    // subscriberMutex.lock();
                    while (receivedMessageList.isEmpty() == false) {
//                        synchronized (lock) {
                            payload = receivedMessageList.remove(0);
//                        }
                        if (config.veryverbose) {
                            config.log.println("Moving payload:  " + payload + " to resend list.");
                        }
                        resendList.addLast(payload);
                    }
                    // subscriberMutex.unlock();

                    sleep = resendList.isEmpty();
                    while (resendList.isEmpty() == false) {
                        entry = resendList.removeFirst();
                        entrytext = config.clientID + ":" + entry;

                        resendMessage = new MqttMessage();
                        resendMessage.setPayload(entrytext.getBytes());
                        resendMessage.setQos(2);
                        resendMessage.setRetained(false);

                        try {
                            resendPublisher.send(resendMessage, config.publisherTopic, config.time);
                        } catch (MqttPersistenceException e) {
                            e.printStackTrace();
                        } catch (MqttException e) {
                            e.printStackTrace();
                        }

                    }
                    if (sleep == true) {
                        try {
                            Thread.sleep(2000);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }

                    // subscriberMutex.lock();
                    empty = receivedMessageList.isEmpty();
                    subscriberDone = subscriberIsDone(null, null);
                    if (empty && subscriberDone) {
                        int sleepCount = 0;
                        while ((empty) && (sleepCount < 20)) {
                            sleepCount++;
                            sleep(2000);
                            empty = receivedMessageList.isEmpty();
                        }
                    }
                    // subscriberMutex.unlock();

                }
            } catch (MqttException e1) {
                e1.printStackTrace();
            }

            while (subscriberIsDone(null, null) == false) {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            isResendDone(
                            true,
                            "ResendMessage.run() is setting isResendDone(true) because receiverQueue is empy, isDone is true, and resendList is empty (???)");

            if (config.verbose) {
                config.log.println("resendMessage.run() calling resendPublisher.publisher_disconnect()");
            }
            resendPublisher.publisher_disconnect();

            if (config.verbose) {
                config.log.println("resendMessage.run() calling return.");
            }
            return;
        }
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken arg0) {
    }

}
