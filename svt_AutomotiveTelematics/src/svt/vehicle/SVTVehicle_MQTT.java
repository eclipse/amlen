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
import java.util.Properties;
import java.util.Stack;

import svt.util.SVTLog;
import svt.util.SVTLog.TTYPE;
import svt.util.SVTUtil;

import com.ibm.micro.client.mqttv3.MqttCallback;
import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttClientPersistence;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttSecurityException;
import com.ibm.micro.client.mqttv3.MqttTopic;

public class SVTVehicle_MQTT {
    static TTYPE TYPE = TTYPE.MQTT;
    // static String ISM = "ISM2BRIDGE";
    // static String Vehicle = "/CAR/1/#";
    // static String User = "/USER/1";
    int qos = 0;

    String ismserver = null;
//    String ismserver2 = null;
    Integer ismport = null;
    int guid = (int) (Math.random() * 10000.0);
    String command = null;
    // static String dataStoreDir = System.getProperty("java.io.tmpdir");
    boolean cleanSession = true;
    boolean ssl = false;
    String password = null;
    String userName = null;
    boolean persistence = false;
    boolean done = false;
    long carId = 0;
    Byte[] pubLock = new Byte[1];

    public SVTVehicle_MQTT(String ismserver, Integer ismport, long carId, int qos, boolean ssl, String userName,
                    String password) {
        this.ismserver = ismserver;
//        this.ismserver2 = ismserver2;
        this.ismport = ismport;
        this.carId = carId;
        this.qos = qos;
        this.ssl = ssl;
        this.userName = userName;
        this.password = password;
    }

//    synchronized private boolean isDone(Boolean setDone) {
//        if (setDone != null) {
//            done = setDone;
//        }
//        return done;
//    }

    public MqttMessage getMessage(String ismserver, Integer ismport, String topicName, String cClientId)
                    throws MqttException {
        // MqttDefaultFilePersistence dataStore = new
        // MqttDefaultFilePersistence(
        // dataStoreDir + "/" + guid);
        MqttClientPersistence dataStore = null;
        String ism = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver + ":" + ismport;
		MqttClient client = null;

		
		SVTLog.log(TYPE, "ISM server " + ism);
		client = new MqttClient(ism, cClientId, dataStore);		

        MqttConnectOptions options = new MqttConnectOptions();
//		if (ismserver2 != null) {
//			String ism2 = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver2
//					+ ":" + ismport;
//			String[] list = new String[2];
//			list[0] = ism;
//			list[1] = ism2;
////			options.setServerURIs(list);
//			SVTLog.log(TYPE, "ISM servers " + ism + " " + ism2);
//		} else {
			SVTLog.log(TYPE, "ISM server " + ism);
//		}

        // set CleanSession true to automatically remove server data associated
        // with the client id at
        // disconnect. In this sample the clientid is based on a random number
        // and not typically
        // reused, therefore the release of client's server side data on
        // disconnect is appropriate.
        options.setCleanSession(cleanSession);
        if (userName != null) {
            options.setUserName(userName);
        }
        if (password != null) {
            options.setPassword(password.toCharArray());
        }
        if (ssl) {
            Properties sslClientProps = new Properties();
            //sslClientProps.setProperty("com.ibm.ssl.protocol", "SSLv3");
            sslClientProps.setProperty("com.ibm.ssl.protocol", "TLSv1.2");
            sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "TLS_RSA_WITH_AES_128_GCM_SHA256");

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

            options.setSSLProperties(sslClientProps);
        }
        options.setKeepAliveInterval(36000);
        options.setConnectionTimeout(36000);

        SVTVehicle_Callback callback = new SVTVehicle_Callback(client, options);
        client.setCallback(callback);

        int i = 0;
        boolean connected = false;
        SVTLog.logtry(TYPE, "connect");
        while (connected == false) {
            try {
                client.connect(options);
                connected = client.isConnected();
                if (connected == false) {
                    throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
                }
            } catch (MqttException e) {
                if (i++ < 100) {
                    try {
                        Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                    } catch (InterruptedException e2) {
                        SVTLog.logex(TYPE, "sleep", e);
                    }                } else {
                    SVTLog.logex(TYPE, "connect", e);
                    throw e;
                }
                SVTLog.logretry(TYPE, "connect", e, i);
            }
        }
        SVTLog.logdone(TYPE, "connect");

        // Subscribe to the topic. messageArrived() callback invoked when
        // message received.
        client.subscribe(topicName, qos);
        SVTLog.log(TYPE, "Subscribed to topic: '" + topicName + "'.");

        TopicMessage topicMessage = null;
        // wait for messages to arrive before disconnecting
        try {
            while (callback.stacksize() < 1) {
                Thread.sleep(500);
            }
            topicMessage = callback.pop();
            // isDone(false);
        } catch (InterruptedException e) {
            SVTLog.logex(TYPE, "sleep", e);
        }

        // Disconnect the client
        client.unsubscribe(topicName);
        client.disconnect();

        return topicMessage.message;
    }

    public MqttTopic getTopic(String pClientId, String topicName) throws Exception {
        MqttClient client = getClient(pClientId);
        return client.getTopic(topicName);
    }

    public MqttClient getClient(String pClientId) throws Exception {
    	SVTMqttClient client = getSVTClient(pClientId);
    	return client.client;
    }

    public SVTMqttClient getSVTClient(String pClientId) throws Exception {
        MqttClient client = null;
        SVTLog.logtry(TYPE, "client");
        int i = 0;
        String ism = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver + ":" + ismport; 

		while (client == null) {
			try {
				client = new MqttClient(ism, pClientId, null);
			} catch (MqttException e) {
                if (i++ >= 100) {
                    SVTLog.logex(TYPE, "client", e);
                    throw e;
                }
                SVTLog.logretry(TYPE, "client", e, i);
            }
            if (client == null) {
                try {
                    Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                } catch (InterruptedException e) {
                    SVTLog.logex(TYPE, "sleep", e);
                }
            }
        }
        SVTLog.logdone(TYPE, "client");

        MqttConnectOptions options = new MqttConnectOptions();

//		if (ismserver2 != null) {
//			String ism2 = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver2
//					+ ":" + ismport;
//			String[] list = new String[2];
//			list[0] = ism;
//			list[1] = ism2;
////			options.setServerURIs(list);
//		}

        options.setCleanSession(cleanSession);
        if (userName != null) {
            options.setUserName(userName);
        }
        if (password != null) {
            options.setPassword(password.toCharArray());
        }
        if (ssl) {
            Properties sslClientProps = new Properties();
            // sslClientProps.setProperty("com.ibm.ssl.protocol", "SSLv3");
            // sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "SSL_RSA_WITH_AES_128_CBC_SHA");
            sslClientProps.setProperty("com.ibm.ssl.protocol", "TLSv1.2");
            sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "TLS_RSA_WITH_AES_128_GCM_SHA256");
            
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

            options.setSSLProperties(sslClientProps);
        }
        options.setKeepAliveInterval(86400);
        options.setConnectionTimeout(86400);

        SVTVehicle_Callback callback = new SVTVehicle_Callback(client, options);
        client.setCallback(callback);

        i = 0;
        boolean connected = false;
        SVTLog.logtry(TYPE, "connect");
        while (connected == false) {
            try {
                client.connect(options);
                connected = client.isConnected();
                if (connected == false) {
                    throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
                }
            } catch (Exception e) {
            	// Catching all exceptions for now. May need to separate logic for MqttException
                if (i++ >= 100) {
                    SVTLog.logex(TYPE, "connect", e);
                    throw e;
                }

                try {
                    Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                } catch (InterruptedException e2) {
                    SVTLog.logex(TYPE, "sleep", e);
                }
                SVTLog.logretry(TYPE, "connect", e, i);
            }
        }

        SVTLog.logdone(TYPE, "connect");

        return new SVTMqttClient(client, options, callback);
    }

    public void sendMessage(SVTMqttClient svtclient, String payload, String topicName) throws MqttException {
        // long tid = Thread.currentThread().getId();
        // String store = dataStoreDir + "/svt_" + pClientId;
        // System.out.println("mqtt(" + tid + ") dataStore: " + store);
        // MqttDefaultFilePersistence dataStore = new
        // MqttDefaultFilePersistence(store);
        int i = 0;

//        MqttConnectOptions options = new MqttConnectOptions();
//
//        // set CleanSession true to automatically remove server data associated
//        // with the client id at disconnect. In this sample the clientid is
//        // based on a random number and not typically
//        // reused, therefore the release of client's server side data on
//        // disconnect is appropriate.
//        options.setCleanSession(cleanSession);
//        if (userName != null) {
//            options.setUserName(userName);
//        }
//        if (password != null) {
//            options.setPassword(password.toCharArray());
//        }
//        if (ssl) {
//            Properties sslClientProps = new Properties();
//            // sslClientProps.setProperty("com.ibm.ssl.protocol", "SSLv3");
//            // sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "SSL_RSA_WITH_AES_128_CBC_SHA");
//            sslClientProps.setProperty("com.ibm.ssl.protocol", "TLSv1.2");
//            sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "TLS_RSA_WITH_AES_128_GCM_SHA256");
//
//            String keyStore = System.getProperty("com.ibm.ssl.keyStore");
//            if (keyStore == null)
//               keyStore = System.getProperty("javax.net.ssl.keyStore");
//
//            String keyStorePassword = System.getProperty("com.ibm.ssl.keyStorePassword");
//            if (keyStorePassword == null)
//               keyStorePassword = System.getProperty("javax.net.ssl.keyStorePassword");
//
//            String trustStore = System.getProperty("com.ibm.ssl.trustStore");
//            if (trustStore == null)
//               trustStore = System.getProperty("javax.net.ssl.trustStore");
//
//            String trustStorePassword = System.getProperty("com.ibm.ssl.trustStorePassword");
//            if (trustStorePassword == null)
//               trustStorePassword = System.getProperty("javax.net.ssl.trustStorePassword");
//
//            if (keyStore != null)
//               sslClientProps.setProperty("com.ibm.ssl.keyStore", keyStore);
//            if (keyStorePassword != null)
//               sslClientProps.setProperty("com.ibm.ssl.keyStorePassword", keyStorePassword);
//            if (trustStore != null)
//               sslClientProps.setProperty("com.ibm.ssl.trustStore", trustStore);
//            if (trustStorePassword != null)
//               sslClientProps.setProperty("com.ibm.ssl.trustStorePassword", trustStorePassword);
//
//            options.setSSLProperties(sslClientProps);
//        }
        // options.setKeepAliveInterval(36000);

        i = 0;
        boolean connected = false;
        SVTLog.logtry(TYPE, "connect");
        while (connected == false) {
            try {
                svtclient.client.connect(svtclient.options);
                connected = svtclient.client.isConnected();
                if (connected == false) {
                    throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
                }
            } catch (MqttException e) {
                if (i++ >= 100) {
                    SVTLog.logex(TYPE, "connect", e);
                    throw e;
                }

                try {
                    Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                } catch (InterruptedException e2) {
                    SVTLog.logex(TYPE, "sleep", e);
                }
                SVTLog.logretry(TYPE, "connect", e, i);
            }
        }

        SVTLog.logdone(TYPE, "connect");

        // Get an instance of the topic
        MqttTopic topic = svtclient.client.getTopic(topicName);

        // Construct the message to publish
        MqttMessage message = new MqttMessage(payload.getBytes());
        message.setQos(qos);

        // The default retained setting is false.
        if (persistence)
            message.setRetained(true);

        // Publish the message
        // System.out.print(" publish(" + tid + ")");
        SVTLog.logtry(TYPE, "publish[" + payload + "]");
        MqttDeliveryToken token = null;
        i = 0;
        while (token == null) {
            try {
                token = topic.publish(message);
                if (token == null) {
                    throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
                }
            } catch (MqttException e) {
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
        // System.out.print(". ");

        // Wait until the message has been delivered to the server
        if (qos > 0) {
            // System.out.print(" wait(" + tid + ")");
            token.waitForCompletion();
            // System.out.print(". ");
        }

        // Disconnect the client
        // System.out.print(" disconnect(" + tid + ")");
        SVTLog.logtry(TYPE, "disconnect");
        svtclient.client.disconnect();
        SVTLog.logdone(TYPE, "disconnect");
        // System.out.println(". ");

    }

    public void sendMessage(MqttTopic topic, String payload) throws Throwable {

        // Construct the message to publish
        MqttMessage message = new MqttMessage(payload.getBytes());
        message.setQos(qos);

        // The default retained setting is false.
        if (persistence)
            message.setRetained(true);

        // Publish the message
        // System.out.print(" publish(" + tid + ")");
        SVTLog.logtry(TYPE, "publish[" + payload + "]");
        MqttDeliveryToken token = null;
        int i = 0;
        while (token == null) {
            try {
                synchronized (pubLock) {
                }
                token = topic.publish(message);
                if (token == null) {
                    throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
                }
                // Wait until the message has been delivered to the server
                if (qos > 0) {
                    // System.out.print(" wait(" + tid + ")");
                    token.waitForCompletion();
                    // System.out.print(". ");
                }
            } catch (Exception e) {
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
        // System.out.print(". ");

       
    }

    public void sendMessage(MqttTopic topic, MqttMessage message) throws Throwable {

        // Publish the message
        SVTLog.logtry(TYPE, "publish[...]");
        MqttDeliveryToken token = null;
        int i = 0;
        while (token == null) {
            try {
                synchronized (pubLock) {
                }
                token = topic.publish(message);
                if (token == null) {
                    throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
                }
            } catch (MqttException e) {
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

        // Wait until the message has been delivered to the server
        if (qos > 0) {
            // System.out.print(" wait(" + tid + ")");
            token.waitForCompletion();
            // System.out.print(". ");
        }
    }

    public void sendMessage(String topicName, String pClientId, String payload, long n) throws Throwable {
        try {
            // long tid = Thread.currentThread().getId();
            // String store = dataStoreDir + "/svt_" + pClientId;
            // System.out.println("mqtt(" + tid + ") dataStore: " + store);
            // MqttDefaultFilePersistence dataStore = new
            // MqttDefaultFilePersistence(store);
            SVTLog.logtry(TYPE, "client");
            int i = 0;
            MqttClient client = null;
			String ism = (this.ssl == false ? "tcp" : "ssl") + "://"
					+ ismserver + ":" + ismport;
			while (client == null) {
				try {
					client = new MqttClient(ism, pClientId, null);
                    // client = new MqttClient("tcp://" + ismserver + ":"
                    // + ismport, pClientId);
                } catch (Throwable e) {
                    if (i++ < 100) {
                        Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                        SVTLog.logretry(TYPE, "client", e, i);
                    } else {
                        SVTLog.logex(TYPE, "client", e);
                        throw e;
                    }
                }
            }
            SVTLog.logdone(TYPE, "client");

            MqttConnectOptions options = new MqttConnectOptions();

//    		if (ismserver2 != null) {
//    			String ism2 = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver2
//    					+ ":" + ismport;
//    			String[] list = new String[2];
//    			list[0] = ism;
//    			list[1] = ism2;
////    			options.setServerURIs(list);
//    		}

            // set CleanSession true to automatically remove server data
            // associated with the client id
            // at
            // disconnect. In this sample the clientid is based on a random
            // number and not typically
            // reused, therefore the release of client's server side data on
            // disconnect is appropriate.
            options.setCleanSession(cleanSession);
            if (userName != null) {
                options.setUserName(userName);
            }
            if (password != null) {
                options.setPassword(password.toCharArray());
            }
            if (ssl) {
                Properties sslClientProps = new Properties();
                //sslClientProps.setProperty("com.ibm.ssl.protocol", "SSLv3");
                // sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "SSL_RSA_WITH_AES_128_CBC_SHA");
                sslClientProps.setProperty("com.ibm.ssl.protocol", "TLSv1.2");
                sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "TLS_RSA_WITH_AES_128_GCM_SHA256");
                
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

                options.setSSLProperties(sslClientProps);
            }
            options.setKeepAliveInterval(36000);
            options.setConnectionTimeout(36000);

            SVTVehicle_Callback callback = new SVTVehicle_Callback(client, options);
            client.setCallback(callback);
            i = 0;
            boolean connected = false;
            SVTLog.logtry(TYPE, "connect");
            while (connected == false) {
                try {
                    client.connect(options);
                    connected = client.isConnected();
                    if (connected == false) {
                        throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
                    }
                } catch (MqttException e) {
                    if (i++ < 100) {
                        Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                        SVTLog.logretry(TYPE, "connect", e, i);
                    } else {
                        SVTLog.logex(TYPE, "connect", e);
                        throw e;
                    }
                }
            }
            SVTLog.logdone(TYPE, "connect");

            // Get an instance of the topic
            MqttTopic topic = client.getTopic(topicName);

            // Construct the message to publish
            MqttMessage message = new MqttMessage(payload.getBytes());
            message.setQos(qos);

            // The default retained setting is false.
            if (persistence)
                message.setRetained(true);

            MqttDeliveryToken token = null;
            i = 0;
            SVTLog.logtry(TYPE, "publish", n);
            while (token == null) {
                try {
                    synchronized (pubLock) {
                    }
                    token = topic.publish(message);
                    if (token == null) {
                        throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
                    }
                } catch (MqttException e) {
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

            // Wait until the message has been delivered to the server
            // if (qos > 0) {
            // System.out.print(" wait(" + tid + ")");
            // token.waitForCompletion();
            // System.out.print(". ");
            // }

            // Thread.sleep(1000L);

            i = 0;
            done = false;
            // Disconnect the client
            SVTLog.logtry(TYPE, "disconnect");
            while (done == false) {
                try {
                    client.disconnect(100L);
                    done = true;
                    SVTLog.logdone(TYPE, "disconnect");
                } catch (MqttException e) {
                    if (e.getReasonCode() == MqttException.REASON_CODE_CLIENT_ALREADY_DISCONNECTED) {
                        done = true;
                    }
                    if ((done == false) && (i++ >= 100)) {
                        SVTLog.logex(TYPE, "disconnect", e);
                        throw e;
                    } else {
                        SVTLog.logretry(TYPE, "disconnect", e, i);
                    }
                } catch (Throwable e) {
                    if (i++ < 100) {
                        SVTLog.logretry(TYPE, "disconnect", e, i);
                    } else {
                        SVTLog.logex(TYPE, "disconnect", e);
                        throw e;
                    }
                }
            }

        } catch (InterruptedException e1) {
            SVTLog.logex(TYPE, "sleep", e1);
            throw e1;
        }
    }

    public void actOnMessage(String topic, String messageText) throws Throwable {
        SVTLog.log(TYPE, "incoming topic " + topic);

        String[] token = topic.split("/");
        String outgoing_topic = "/" + token[3] + "/" + token[4] + "/" + token[5] + "/" + token[6] + "/" + token[1]
                        + "/" + token[2];
        SVTLog.log(TYPE, "outgoing topic " + outgoing_topic);

        if ("UNLOCK".equals(messageText)) {
            SVTLog.log(TYPE, "Doors unlocked");
            sendMessage(outgoing_topic, SVTUtil.MyClientId(TYPE), "Doors unlocked", 0);
        } else if ("LOCK".equals(messageText)) {
            SVTLog.log(TYPE, "Doors locked");
            sendMessage(outgoing_topic, SVTUtil.MyClientId(TYPE), "Doors locked", 0);
        } else if ("GET_DIAGNOSTICS".equals(messageText)) {
            SVTLog.log(TYPE, "Unsupported feature");
        }
    }

    public void actOnMessage(SVTMqttClient svtclient, TopicMessage alert) throws Throwable {
        SVTLog.log(TYPE, "incoming topic " + alert.getTopic());

        String[] token = alert.getTopic().split("/");
        String outgoing_topic = "/" + token[3] + "/" + token[4] + "/" + token[5] + "/" + token[6] + "/" + token[1]
                        + "/" + token[2];
        SVTLog.log(TYPE, "outgoing topic " + outgoing_topic);

        String messageText = alert.getMessageText();
        if ("UNLOCK".equals(messageText)) {
            SVTLog.log(TYPE, "Doors unlocked");
            sendMessage(svtclient, outgoing_topic, "Doors unlocked");
        } else if ("LOCK".equals(messageText)) {
            SVTLog.log(TYPE, "Doors locked");
            sendMessage(svtclient, outgoing_topic, "Doors locked");
        } else if ("GET_DIAGNOSTICS".equals(messageText)) {
            SVTLog.log(TYPE, "Unsupported feature");
        }
    }

    public class SVTMqttClient {
    	public MqttClient client;
    	public MqttConnectOptions options;
    	public MqttCallback callback;
    	
    	SVTMqttClient(MqttClient client, MqttConnectOptions options, MqttCallback callback){
        	this.client = client;
        	this.options = options;
        	this.callback = callback;
    	}
    };

    class SVTVehicle_Listener implements Runnable {
        Boolean run = new Boolean(true);
        MqttTopic topic = null;
        SVTMqttClient svtclient = null;

        public void run() {
            boolean r = true;
            String lClientId = SVTUtil.MyClientId(TYPE, "l");
            String topicName = "/CAR/" + carId + "/#";

            TopicMessage alert = null;
            try {
                SVTLog.log(TYPE, "SVTVehicle_ListenerThread");

                svtclient = getSVTClient(lClientId);

                while (r == true) {
                    alert = null;

                    if (svtclient == null) {
                        svtclient = getSVTClient(lClientId);
                        svtclient.client.subscribe(topicName);
                    }

                    alert = ((SVTVehicle_Callback)svtclient.callback).pop();

                    if (alert != null) {
                        SVTLog.log(TYPE, "Alert received:  " + new String(alert.getMessageText()));
                        actOnMessage(svtclient, alert);
                    }

                    synchronized (run) {
                        r = run;
                    }
                }
            } catch (Throwable e) {
                SVTLog.logex(TYPE, "getMessage|actOnMessage", e);
            }
            return;
        }

        public void stop() {
            synchronized (run) {
                run = false;
            }
        }
    }

    public SVTVehicle_Listener listener = null;
    Thread listenerThread = null;

    public void startListener() {
        synchronized (this) {
            listener = new SVTVehicle_Listener();
            listenerThread = new Thread(listener);
            listenerThread.start();
        }
    }

    public void stopListener() {
        synchronized (this) {
            listener.stop();

            while (listenerThread.isAlive()) {
                try {
                    listenerThread.join(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public void startInteractive() {
        String pClientId = SVTUtil.MyClientId(TYPE, "p");
        String cClientId = SVTUtil.MyClientId(TYPE, "c");
        String topicName = "/APP/1/CAR/" + carId + "/CAR/" + carId;
        MqttMessage message = null;
        String messageText = null;

        startListener();

        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));

        try {
            SVTLog.log(TYPE, "commands are exit, get_alerts, send_gps, send_errorcode, update_system:  ");
            String command = br.readLine();
            while (!("exit".equalsIgnoreCase(command))) {
                if ("get_alerts".equalsIgnoreCase(command)) {
                    sendMessage(topicName, pClientId, "QUERY_ALERT", 0);
                    message = getMessage(ismserver, ismport, topicName, cClientId);
                    SVTLog.log(TYPE, "Reply message:  " + message);
                    while (message != null) {
                        messageText = new String(message.getPayload());
                        actOnMessage(topicName, messageText);
                        message = getMessage(ismserver, ismport, topicName, cClientId);
                        SVTLog.log(TYPE, "Reply message:  " + messageText);
                    }
                } else if ("send_gps".equalsIgnoreCase(command)) {
                    int x = (int) (Math.random() * 10000.0);
                    int y = (int) (Math.random() * 10000.0);
                    sendMessage(topicName, pClientId, "GPS " + x + ", " + y + ")", 0);
                    SVTLog.log(TYPE, "sent message: gps(" + x + ", " + y + ")");
                    message = getMessage(ismserver, ismport, topicName, cClientId);
                    SVTLog.log(TYPE, "Reply message:  " + message);
                } else if ("send_errorcode".equalsIgnoreCase(command)) {
                    int x = (int) (Math.random() * 10000.0);
                    sendMessage(topicName, pClientId, "RECORD_ERRORCODE " + x + ")", 0);
                    SVTLog.log(TYPE, "sent message: errorcode(" + x + ")");
                    message = getMessage(ismserver, ismport, topicName, cClientId);
                    SVTLog.log(TYPE, "Reply message:  " + new String(message.getPayload()));
                } else if ("update_system".equalsIgnoreCase(command)) {
                    sendMessage(topicName, pClientId, "SYSTEM_UPDATE", 0);
                    SVTLog.log(TYPE, "sent message: update_system");
                    message = getMessage(ismserver, ismport, topicName, cClientId);
                    SVTLog.log(TYPE, "Reply message:  " + new String(message.getPayload()));
                } else {
                    SVTLog.log(TYPE, "Did not understand command:  " + command);
                }
                SVTLog.log(TYPE, "commands are exit, get_alerts, send_gps, send_errorcode, update_system:  ");
                command = br.readLine();
            }
        } catch (Throwable e) {
            SVTLog.logex(TYPE, "sendMessage|getMessage", e);
        }

        stopListener();

    }

    public static void main(String[] args) {
        String ismserver = null;
        Integer ismport = null;
        long carId = 0;
        int qos = 0;
        boolean ssl = false;
        String userName = null;
        String password = null;

        if (args.length == 6) {
            ismserver = args[0];
            ismport = Integer.parseInt(args[1]);
            carId = Long.parseLong(args[2]);
            qos = Integer.parseInt(args[3]);
            ssl = Boolean.parseBoolean(args[4]);
            userName = args[5];
            password = args[6];
        } else {
            System.out.println("Usage: <ismserver> <ismport> <carId> <qos> <ssl> <userName> <password> ");
        }

        SVTVehicle_MQTT vehicle = new SVTVehicle_MQTT(ismserver, ismport, carId, qos, ssl, userName, password);
        // start() method loops until user enters "exit"
        vehicle.startInteractive();

    }

//    class ClientCallback {
//        private MqttClient client;
//        private SVTVehicle_Callback callback;
//
//        public ClientCallback(MqttClient client) {
//            this.client = client;
//            this.callback = new SVTVehicle_Callback();
//        }
//
//        public MqttClient getClient() {
//            return this.client;
//        }
//
//        public SVTVehicle_Callback getCallback() {
//            return this.callback;
//        }
//
//        public String getClientId() {
//            return client.getClientId();
//        }
//
//        public int getStackSize() {
//            return callback.stacksize();
//        }
//
//        public TopicMessage pop() {
//            return callback.pop();
//        }
//    }

    class TopicMessage {
        private String topic;
        private MqttMessage message;

        public TopicMessage(String topic, MqttMessage message) {
            this.topic = topic;
            this.message = message;
        }

        public String getTopic() {
            return this.topic;
        }

        public MqttMessage getMessage() throws MqttException {
            return message;
        }

        // public String getTopicName() {
        // return topic;
        // }

        public String getMessageText() throws MqttException {
            return new String(message.getPayload());
        }
    }

    class SVTVehicle_Callback implements MqttCallback {
        Stack<TopicMessage> stack = new Stack<TopicMessage>();
        MqttConnectOptions options = null;
        MqttClient client = null;
        
        SVTVehicle_Callback(MqttClient client, MqttConnectOptions options){
        	this.client = client; 
        	this.options = options;
        }

        public void sleep(long s) {
            try {
            	SVTLog.log(TYPE, "sleep "+s);
                Thread.sleep(s);
            } catch (InterruptedException e2) {
                System.exit(1);
            }
        }
		public void connectionLost(Throwable arg0) {
			synchronized (pubLock) {
				while (client.isConnected() == false) {
					SVTLog.logtry(TYPE, "connect");
					try {
						client.connect(options);
						SVTLog.logdone(TYPE, "reconnected.");
					} catch (MqttSecurityException e) {
						SVTLog.logex(TYPE, "reconnect", e);
					} catch (Exception e) {
						SVTLog.log(TYPE, "reconnect failed "+ e.getMessage());
					}
					sleep(500);
				}
			}
		}

        public void deliveryComplete(MqttDeliveryToken arg0) {
            // TODO Auto-generated method stub
        }

        public int stacksize() {
            int size = 0;
            synchronized (stack) {
                size = stack.size();
            }
            return size;
        }

        public TopicMessage pop() {
            TopicMessage entry = null;
            synchronized (stack) {
                if (stack.size() > 0)
                    entry = stack.pop();
            }
            return entry;
        }

    
		@Override
		public void messageArrived(MqttTopic arg0, MqttMessage arg1)
				throws Exception {
			synchronized (stack) {
				stack.push(new TopicMessage(arg0.toString(), arg1));
			}
		}
	}

}
