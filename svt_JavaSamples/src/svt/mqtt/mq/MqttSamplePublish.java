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

import java.util.Hashtable;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.atomic.AtomicLong;

import org.eclipse.paho.client.mqttv3.IMqttAsyncClient;
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
 * Class that encapsulates the ISM message publish logic.
 * 
 * This class contains the doPublish method for publishing messages to an ISM topic. It also defines the necessary MQTT
 * callbacks for asynchronous publishing.
 * 
 */
public class MqttSamplePublish implements MqttCallback {

    MqttSample config = null;
    IMqttAsyncClient client = null;
    MqttConnectOptions options = null;
    MqttDefaultFilePersistence dataStore = null;
    Byte[] pubLock = new Byte[1];
    IMqttToken conToken = null;
    IMqttDeliveryToken token = null;
    boolean sent = false;
    Hashtable<IMqttDeliveryToken, MqttMessage> pendingMessages = new Hashtable<IMqttDeliveryToken, MqttMessage>(60000);
    long iCount = 0;
    long startTimeStamp = 0L;
    long prevTimeStamp = 0L;
    private String clientID;
    Thread batchThread = new Thread(new batchThread());
    Long batch = 100000L;
    Byte[] delivery = new Byte[1];
    long length = 0;
    long size = 20000;
    boolean size_reset = false;
    AtomicLong dCount = new AtomicLong(0L);

    private Boolean reconnect = true;

    synchronized public boolean shouldReconnect(Boolean done, String message) {
        if (done != null) {
            if (config.debug)
                config.log.println(message);
            reconnect = done;
        }
        return reconnect;
    }

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
                    publisher_connect();
                }
            } else if (remainingTime > 500) {
                // if less than 1000ms runtime remain (and more than 500ms) then sleep 200ms
                sleep(200);
                // test if reconnect flag is still true after sleeping
                if (shouldReconnect(null, null)) {
                    publisher_connect();
                }
            }
        }
    }

    /**
     * Callback invoked when message delivery complete
     * 
     * @param token
     *            same token that was returned when the messages was sent with the method MqttTopic.publish(MqttMessage)
     */

    public void deliveryComplete(IMqttDeliveryToken arg0) {
        try {
            if (arg0 != null) {
                if (arg0.getException() != null) {
                    config.log.println(clientID + " deliveryComplete invoked with exception " + arg0.getException().getMessage());
                } else if (arg0.isComplete()) {
                	dCount.incrementAndGet();
                    // MqttReceivedMessage message = pendingMessages.remove(arg0);
						synchronized (delivery) {
							delivery.notify();
						}

                    if (config.verbose) {
                        synchronized (batch) {
                            batch.notify();
                        }
                        if (config.veryverbose) {
                            MqttMessage message = (MqttMessage)arg0.getUserContext();
                            if (message != null) {
                                config.log.println(clientID + " deliveryComplete invoked for "
                                                + new String(message.getPayload()) + " published to topic "
                                                + arg0.getTopics()[0] + " [msgid=" + arg0.getMessageId() + "]");
                                config.log.println("IMqttDeliveryToken.isComplete() for "
                                                + new String(message.getPayload()) + " is set  " + arg0.isComplete());
                                if (arg0.getException() != null)
                                    config.log.println("IMqttDeliveryToken.getException().getMessage() for "
                                                    + new String(message.getPayload()) + " is  "
                                                    + arg0.getException().getMessage());
                                else
                                    config.log.println("IMqttDeliveryToken.getException() for "
                                                    + new String(message.getPayload()) + " is null");
                        } else {
                            config.log.println(clientID + " deliveryComplete invoked for " + "<message is null>"
                                            + " to topic " + arg0.getTopics()[0]);
                        }
                    }
                    }
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            System.exit(1);
        }

    }

    public MqttSamplePublish(MqttSample config) {
        this.config = new MqttSample(config);

        if (config.publisherID != null)
            this.config.clientID = config.publisherID;
        if (config.publisherURI != null)
            this.config.serverURI = config.publisherURI;
        if (config.publisherTopic != null)
            this.config.topicName = config.publisherTopic;
        clientID = this.config.clientID;
    }

    /**
     * Publish message to the specified topic
     * 
     * @param config
     *            MqttSample instance containing configuration settings
     * 
     * @throws MqttException
     */
	public void run() throws MqttException {
		if (config.qos > 0)
			size = 8;

		if (config.verbose) {
			Thread batchThread = new Thread(new batchThread());
			batchThread.start();
		}
		doConnect();
		if (config.roundtrip != null) {
			client.subscribe(config.roundtrip, 1);
		}
		if (this.shouldReconnect(null, null)) {
			sendMessages();
		}
		if (config.roundtrip != null) {
			client.unsubscribe(config.roundtrip);
		}
		BatchThreadStop(true);

	}

    public void doConnect() throws MqttException {

        config.log.println("Client ID:  " + clientID);

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

            client = new MqttAsyncClient(list[0], clientID, dataStore);
            options.setServerURIs(list);
        } else {
            client = new MqttAsyncClient(config.serverURI, clientID, dataStore);
        }

        if ("3.1.1".equals(config.mqttVersion)) {
            options.setMqttVersion(org.eclipse.paho.client.mqttv3.MqttConnectOptions.MQTT_VERSION_3_1_1);
        } else if ("3.1".equals(config.mqttVersion)) {
            options.setMqttVersion(org.eclipse.paho.client.mqttv3.MqttConnectOptions.MQTT_VERSION_3_1);
        }

        client.setCallback(this);

        // set CleanSession true to automatically remove server data associated with the client id at
        // disconnect. In this sample the clientid is based on a random number and not typically
        // reused, therefore the release of client's server side data on disconnect is appropriate.
        options.setCleanSession(config.cleanSession);

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
        
        //  CleanSessionState indicates if you want to skip Rick's Swizzle on connections for HA Failover reconnects.
		if (!config.cleanSessionState) {
			//Rick's Swizzle for HA Connections
			if (config.cleanSession) {
				options.setCleanSession(true);
				publisher_connect();
				publisher_disconnect();
			}

			// options.setCleanSession(config.cleanSession);
			options.setCleanSession(false);
			publisher_connect();
		} else {
			options.setCleanSession(config.cleanSession);
			publisher_connect();
		}
    }

    public void sendMessages() throws MqttPersistenceException, MqttException {
        // MqttReceivedMessage message = null;
        MqttMessage message = null;

        if (config.order == false) {
            // message = new MqttReceivedMessage();
            message = new MqttMessage();
            message.setPayload(config.payload.getBytes());
            message.setQos(config.qos);
            if (config.retained)
                message.setRetained(true);
            // else message.setRetained(false);
        }

        // startTime was set above, we reset it here to better correspond to first message sent.
        // startTime = System.currentTimeMillis();
        iCount = 0;
        String text = null;
        if ((config.defaultcount == true) && (config.time.configuredRunTimeMillis() != 0)) {
            // NOTE: if config.hours is set to -1 then while loop will run forever.
            while (config.time.remainingRunTimeMillis() > 0) {
                if (config.order == true) {
                	if ( config.orderJSON ){
                		Long theTimeStamp = System.currentTimeMillis();
                		if ( iCount == 0 ){
                			startTimeStamp = theTimeStamp;
                			prevTimeStamp = theTimeStamp;
                		}
                		while ( theTimeStamp == prevTimeStamp ){
                			theTimeStamp = System.currentTimeMillis();
                		}
                		prevTimeStamp = theTimeStamp;
                		text="{\"ts\":"+ theTimeStamp +",\"d\":{\"cTime\":"+ theTimeStamp +", \"sTime\":"+ startTimeStamp +", \"Pub\":\""+ clientID +"\" , \"Topic\":\""+ config.topicName +"\" , \"QoS\":"+ config.qos +" , \"Retain\":"+ config.retained +" , \"Index\":"+ iCount +" , \"payload\":"+ config.payload +"  }}";
                	} else {
                		/*
                		 * Below is the header of body and body itself used to verify order. Both
                		 * subscriber and publisher must agree to format.
                		 */
                    	text = "ORDER:" + clientID + ":" + config.qos + ":" + iCount + ":" + iCount + ":" + config.payload;
                	}
                
                    // message = new MqttReceivedMessage();
                    message = new MqttMessage();
                    message.setPayload(text.getBytes());
                    message.setQos(config.qos);
                    if (config.retained)
                        message.setRetained(true);
                    // else message.setRetained(false);
                }

                send(message, config.topicName, config.time);
            }
        } else {
            // NOTE: if config.count is set to -1 then while loop will run forever.
            while ((config.count == -1) || (iCount < config.count)) {
                if (config.time.runTimeExpired()) {
                    break;
                }
                if (config.order == true) {
                	if ( config.orderJSON ){
                		Long theTimeStamp = System.currentTimeMillis();
                		if ( iCount == 0 ){
                			startTimeStamp = theTimeStamp;
                			prevTimeStamp = theTimeStamp;
                		}
                		while ( theTimeStamp == prevTimeStamp ){
                			theTimeStamp = System.currentTimeMillis();
                		}
                		prevTimeStamp = theTimeStamp;
                		text="{\"ts\":"+ theTimeStamp +",\"d\":{\"cTime\":"+ theTimeStamp +", \"sTime\":"+ startTimeStamp +", \"Pub\":\""+ clientID +"\" , \"Topic\":\""+ config.topicName +"\" , \"QoS\":"+ config.qos +" , \"Retain\":"+ config.retained +" , \"Index\":"+ iCount +" , \"payload\":"+ config.payload +"  }}";
                	} else {
                	
                    text = "ORDER:" + clientID + ":" + config.qos + ":" + iCount + ":" + (config.count - 1) + ":"
                                    + config.payload;
                	}
                    // message = new MqttReceivedMessage();
                    message = new MqttMessage();
                    message.setPayload(text.getBytes());
                    message.setQos(config.qos);
                    if (config.retained)
                        message.setRetained(true);
                    // else message.setRetained(false);
                }

                send(message, config.topicName, config.time);
                if ( iCount == 1){
                	System.out.println("First Msg:"+ text);
                }
            }
        }

        // If the client disconnecst too quickly then the finally message count is not correct.
      while (client.getPendingDeliveryTokens().length > 0) {
         config.log.println("waiting for delivery of remaining " + client.getPendingDeliveryTokens().length + " tokens");
         if (client.isConnected() == false) {
            config.log.println("client is *not* connected... reconnecting");
            publisher_connect();
         }

         sleep(50);

         if (config.time.runTimeExpired()) {
            break;
         }

      }
        publisher_disconnect();
        
        if (!config.cleanSessionState) {
			if (config.cleanSession) {
				options.setCleanSession(true);
				publisher_connect();
				publisher_disconnect();
			}
		}
        
        if (client.isConnected() == false) {
            client.close();
        } else {
            config.log.println("unable to close client.");
        }

        config.log.println("Published " + dCount.get() + " messages to topic " + config.topicName);
        return;
    }

    public void sleep(long s) {
        try {
            Thread.sleep(s);
        } catch (InterruptedException e2) {
            System.exit(1);
        }
    }

	public void waitfordelivery() {
        int loop = 0;
		// length = client.getPendingDeliveryTokens().length;
		// length = pendingMessages.size();
		length = iCount - dCount.get();
		while (length > size ) {
			synchronized (delivery) {
				try {
					delivery.wait(1);
				} catch (InterruptedException e) {
				}
			}
			if (((++loop)%10==0)&&(config.time.runTimeExpired())) {
				break;
			}
			// sleep(length);
			length = iCount - dCount.get();
		}
	}

    public void publisher_connect() {
        config.log.printTm("publisher_connect called" + "\n");
        int currentConnectionRetry = 0;
        synchronized (pubLock) {
            while (client.isConnected() == false) {
                config.log.printTm("client.isConnected " + client.isConnected() + "\n");
                config.log.printTm("config.time.configuredRuntime " + (config.time.configuredRunTimeMillis()) + "\n");
                config.log.printTm("config.time.remainingRuntime " + (config.time.remainingRunTimeMillis()) + "\n");
                if (config.connectionRetry != -1) {
                    currentConnectionRetry++;
                }
                try {
                    conToken = null;
                    shouldReconnect(true, "publisher_connect called");
                    config.log.printTm("Publisher " + this.client.getClientId() + " connecting ...");
                    conToken = client.connect(options);
                    config.log.print("waiting...");
                    while ((conToken != null) && (conToken.isComplete() == false)) {
                        conToken.waitForCompletion();
                    }
                    if ((conToken != null) && (conToken.isComplete() == true)) {
                        config.log.println("\nEstablished connection to " + config.serverURI);
                    }
				} catch (MqttException ex) {
					config.log.print("connect failed.  " + ex.getMessage()
							+ "\n");
					if (config.veryverbose) {
						if (ex.getCause() != null) {
							config.log.print("cause:" + ex.getCause().getMessage());
							if (ex.getCause().getCause() != null) {
								config.log
										.print("cause.cause:"
												+ ex.getCause().getCause()
														.getMessage());
							}
						}
						config.log.print(ex.getStackTrace());
					}
					if (currentConnectionRetry >= config.connectionRetry
							&& config.connectionRetry != -1) {
						shouldReconnect(false,
								" - Aborting connection attempts after "
										+ currentConnectionRetry
										+ " failures. ");
						break;
					}
					sleep(500);

            } catch (Exception ex) {
                    String exMsg = ex.getMessage();
                    config.log.print("connect failed.  " + exMsg + "\n");
                    if (currentConnectionRetry >= config.connectionRetry && config.connectionRetry != -1) {
                        shouldReconnect(false, exMsg + " - Aborting connection attempts after "
                                        + currentConnectionRetry + " failures. ");
                        break;
                    }
                    sleep(500);
                }
                if (config.time.runTimeExpired()) {
                    break;
                }
            }
        }

    }

    public void publisher_disconnect() {
        config.log.printTm("publisher_disconnect called" + "\n");
        // synchronized (pubLock) {
        while (client.isConnected() == true) {
            try {
                conToken = null;
                shouldReconnect(false, "publisher_disconnect called");
                config.log.printTm("Publisher disconnecting...");
                conToken = client.disconnect();
                config.log.print("waiting...");
                while ((conToken != null) && (conToken.isComplete() == false)) {
                    // if ((config.seconds > 0) && (System.currentTimeMillis() - startTime()) >= (config.seconds *
                    // 1000l)) {
                    // break;
                    // } else {
                    MqttException e = conToken.getException();
                    if (e != null) {
                        config.log.printTm("conToken.getException() return " + e.getMessage() + ":" + e.getReasonCode()
                                        + "\n");
                    }
                    conToken.waitForCompletion();
                    // }
                }
                if ((conToken != null) && (conToken.isComplete() == true)) {
                    config.log.println("\nclient disconnected to " + config.serverURI);
                }
            } catch (Exception ex) {
                config.log.print("disconnect failed. " + ex.getMessage() + "\n");
                sleep(500);
            }
            if (config.time.runTimeExpired()) {
                break;
            }
        }
        // }
    }

    long projected = 0;
    long elapsed = 0;
    long sleepInterval = 0;

    // public void send(MqttSample config, MqttReceivedMessage message, String topicName, Long startTime)
    public void send(MqttMessage message, String topicName, MqttSample.time time) throws MqttPersistenceException,
            MqttException {
      // if (config.verbose)
      // config.log.println(iCount + ": Publishing \"" + config.payload +
      // "\" to topic " + config.topicName
      // + " on server " + config.serverURI);

      if (iCount == 0) {
         time.setStartTime();
      } else {
         waitfordelivery();

         if (config.throttleWaitMSec > 0) {
            projected = (dCount.get() / config.throttleWaitMSec) * 1000L;
            elapsed = time.actualRunTimeMillis();
            if (elapsed < projected) {
               sleepInterval = projected - elapsed;
               sleep(sleepInterval);
            }
         }
      }

      if ((config.skip != null) && (iCount == config.skip))
         sent = true;
      else
         sent = false;

      if (config.timestamp)
         topicName = topicName + "/" + System.currentTimeMillis();

      while ((sent == false) && (time.remainingRunTimeMillis() > 0)) {

         try {
            if (config.veryverbose) {
               config.log.println(clientID + " publishing " + new String(message.getPayload()) + " to topic "
                        + topicName + ", isRetained set to " + message.isRetained());
            }
            token = null;
            synchronized (pubLock) {
            }
            token = client.publish(topicName, message);

            if (token == null) {
               System.err.println("Publish returned null token");
               System.exit(1);
            } else if (config.veryverbose) {
            	token.setUserContext(message);
            }
//            pendingMessages.put(token, message);
            sent = true;
         } catch (MqttException ex) {
            config.log.println(clientID + " Failed to publish " + new String(message.getPayload())
                     + " due to MqttException" + ex.getMessage() + ", reason code " + ex.getReasonCode());
            if (32202 == ex.getReasonCode() /* too many publishers */) {
//               int pendingsize = pendingMessages.size();
//               if ((pendingsize < size)&&(pendingsize > 8)&&(size_reset==false)) {
//                  size = pendingMessages.size() - 1;
//                  size_reset = true;
//               }
               waitfordelivery();
            } else {
               sleep(500);
            }
         } catch (Exception ex) {
            config.log.println(clientID + " Failed to publish " + new String(message.getPayload())
                     + " due to Exception" + ex.getMessage());
            sleep(500);
         }
      }
      if ((config.duplicate != null) && (iCount == config.duplicate)) {
         config.duplicate = null;
      } else {
         iCount++;
      }
   }

    @Override
    public void messageArrived(String topic, MqttMessage message) throws Exception {
    	if (config.roundtrip != null) {
            long currentTimeMillis = System.currentTimeMillis();
    		long startTimeMillis = Long.parseLong(new String(message.getPayload()));
    		long deltaTimeMillis = currentTimeMillis-startTimeMillis;
    		config.log.println("startTime: "+startTimeMillis+" currentTime: "+currentTimeMillis+ " deltaTime; "+deltaTimeMillis);
    	}
    	
    }

    static MqttSamplePublish resendPublisher = null;

   public static boolean sendMessage(String clientID, String serverURI, MqttSample config, List<String> payloadList) {
      boolean sent = false;
      MqttSample localconfig = new MqttSample(config);
      MqttMessage localmessage = new MqttMessage();
      // MqttSamplePublish resendPublisher = new MqttSamplePublish(localconfig);
      synchronized (config) {
         if (resendPublisher == null) {
            resendPublisher = new MqttSamplePublish(localconfig);
            try {
               resendPublisher.doConnect();
               // long startTime = System.currentTimeMillis();
            } catch (MqttException e1) {
               e1.printStackTrace();
            }
         }
      }

      for (String payload : payloadList) {
         localmessage.setPayload(payload.getBytes());
         localmessage.setQos(config.qos);
         localmessage.setRetained(config.retained);

         try {
            resendPublisher.send(localmessage, config.topicName, config.time);
            sent = true;
         } catch (MqttPersistenceException e) {
            e.printStackTrace();
         } catch (MqttException e) {
            e.printStackTrace();
         }
      }

      resendPublisher.publisher_disconnect();
      return sent;
   }

    public static boolean sendMessage(MqttSample config, String payload) {
        boolean sent = false;
        MqttSample localconfig = new MqttSample(config);
        MqttSamplePublish resendPublisher = new MqttSamplePublish(localconfig);
        try {
            MqttMessage localmessage = new MqttMessage();
            resendPublisher.doConnect();
            // long startTime = System.currentTimeMillis();

            localmessage.setPayload(payload.getBytes());
            localmessage.setQos(config.qos);
            localmessage.setRetained(config.retained);

            try {
                resendPublisher.send(localmessage, config.topicName, config.time);
                sent = true;
            } catch (MqttPersistenceException e) {
                e.printStackTrace();
            } catch (MqttException e) {
                e.printStackTrace();
            }
        } catch (MqttException e1) {
            e1.printStackTrace();
        }

        resendPublisher.publisher_disconnect();
        return sent;
    }

    boolean privBatchThreadStop = false;

    public synchronized boolean BatchThreadStop(Boolean s) {
        boolean prev = privBatchThreadStop;
        if (s != null) {
            privBatchThreadStop = s;
            synchronized (batch) {
                batch.notify();
            }
        }
        return prev;
    }

    class batchThread implements Runnable {
        long currentTimeMillis = System.currentTimeMillis();
        long deltaTimeSec = 0;
        long ldCount = 0;
        long holdTime = currentTimeMillis;
        long holdCount = 0;

        public void run() {
            if (config.verbose) {
                while (true) {
                    synchronized (batch) {
                        try {
                            batch.wait(20000);
                        } catch (InterruptedException e) {
                        }
                    }
                    if (BatchThreadStop(null) == true)
                        break;
                    ldCount = dCount.get();
                    currentTimeMillis = System.currentTimeMillis();
                    deltaTimeSec = (currentTimeMillis - holdTime) / 1000L;
                    if ((((ldCount % 5000L) == 0) && (deltaTimeSec > 0)) || (deltaTimeSec > 20L)) {
                        config.log.println((ldCount - holdCount) + " messages were sent at "
                                        + ((ldCount - holdCount) / deltaTimeSec) + " msgs/sec");
                        holdCount = ldCount;
                        holdTime = currentTimeMillis;
                    }
                }
            }
        }
    }

}
