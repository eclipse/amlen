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
package com.ibm.ima.test.stat;

import java.io.FileInputStream;
import java.io.IOException;
import java.math.BigInteger;
import java.security.KeyStore;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;

/**
 * Config represents producer and consumer configurations. Config also
 * contains static methods to assist with the creation of Config instances.
 *
 */
public class Config {


	private static boolean verboseOutput = false;

	// TOPIC or QUEUE
	private DEST_TYPE destinationType = null;

	private CLIENT_TYPE clientType = null;

	// username and password
	private String username = null;
	private String password = null;

	// for client authentication
	private String keyStore = null;
	private String keyStorePassword = null;

	private static String cipher = null;

	// for ssl transport
	private String trustStore = null;
	private String trustStorePassword = null;

	/*
	 * The thread count applies only to an overall configuration. That is
	 * a configuration defined in the properties file. Instances of Producer
	 * and Consumer will ignore threadCount
	 */
	private int threadCount = 1;

	/*
	 * The clientIdPrefix count applies only to an overall configuration. That is
	 * a configuration defined in the properties file. Instances of Producer
	 * and Consumer will ignore clientIdPrefix
	 */
	private String clientIdPrefix = null;
	
    /* 
     * The anonymous designation indicates whether a client ID is supplied
     * for the JMS connection object or not.  This is used only
     * for consumers.
     */
    private boolean anonymous = false;

	// ima port
	private String port = null;

	// ima server - hostname or ip
	private String server = null;

	// queue or topic name
	private String destinationName = null;

	private int totalSeconds = 60;
	private boolean ignoreErrors = false;

	// qos for MQTT publisher and subscriber
	// for JMS publisher when DisableACK is set to true in ConnectionFactory
	private int qos = 1;
	
	// destDisableACK for JMS only
	// used for JMS publisher only when DisableACK is set true (which resets this true)
	private boolean destDisableACK = false;

	// clean store will be used for both JMS and MQTT
	private boolean cleanStore = false;

	// ############## Producer only ##############

	private long msgsToSend = 0;
	private long sendInterval = 10;
	private String payload = "A simple text message";
	private int padLen = 0;

	// ############## Consumer only ##############

	private long consumeSleepInterval = 10;
	private long consumeWaitForMsgs = 100;
	private long msgsToConsume = 0;
	private String subscriptionName = null;
    private boolean shared = false;
    private boolean forceUnsubscribe = false;

	// ############# Security specific #############
	private static boolean ssl = false;

	// which protocol to use.. 
	private static SSL_PROTOCOL sslProtocol = null;

	private static String keyStorePath = null;
	private static String keyStorePasswordString = null;

	// for connection storms only - how many to do
	private int connStorm = 0;

	public enum DEST_TYPE {

		TOPIC("TOPIC"),
		QUEUE("QUEUE");

		private String value;


		DEST_TYPE(String value) {
			this.value = value;
		}


		public String getText() {
			return this.value;
		}


		public static DEST_TYPE fromString(String value) {
			if (value != null) {
				for (DEST_TYPE b : DEST_TYPE.values()) {
					if (value.equalsIgnoreCase(b.value)) {
						return b;
					}
				}
			}
			return null;
		}
	}

	public enum CLIENT_TYPE {

		JMS("JMS"),
		MQTT("MQTT"),
		MQTT_CONN_STORM("MQTT_CONN_STORM");

		private String value;


		CLIENT_TYPE(String value) {
			this.value = value;
		}


		public String getText() {
			return this.value;
		}


		public static CLIENT_TYPE fromString(String value) {
			if (value != null) {
				for (CLIENT_TYPE b : CLIENT_TYPE.values()) {
					if (value.equalsIgnoreCase(b.value)) {
						return b;
					}
				}
			}
			return null;
		}
	}

	public enum SSL_PROTOCOL {

		SSLv3("SSLv3"),
		TLSv1("TLSv1"),
		TLSv1_1("TLSv1.1"),
		TLSv1_2("TLSv1.2");

		private String value;


		SSL_PROTOCOL(String value) {
			this.value = value;
		}


		public String getText() {
			return this.value;
		}


		public static SSL_PROTOCOL fromString(String value) {
			if (value != null) {
				for (SSL_PROTOCOL b : SSL_PROTOCOL.values()) {
					if (value.equalsIgnoreCase(b.value)) {
						return b;
					}
				}
			}
			return null;
		}
	}

	/**
	 * Config default constructor
	 */
	public Config() {

	}

	/**
	 * Initialize security settings from information provided in the properties file
	 * 
	 * @param configProps
	 * @param keyFileRoot
	 * @throws Exception
	 */
	public static void initSecurity(Properties configProps, String keyFileRoot) throws Exception {

		String sslString = configProps.getProperty("useSSL"); 
		String sslProtocolString = configProps.getProperty("sslProtocol");

		keyStorePath = configProps.getProperty("keyStore");
		if (keyFileRoot != null) {
			keyStorePath = keyFileRoot + keyStorePath;
		}
		keyStorePasswordString = configProps.getProperty("keyStorePassword"); 

		if (sslString != null) {
			if (sslString.equalsIgnoreCase("true")) {
				ssl = true;
			}
		}

		if (ssl == false) return;

		if (sslProtocolString != null) {
			sslProtocol = SSL_PROTOCOL.fromString(sslProtocolString);
			if (sslProtocol == null) {
				throw new Exception("Invalid SSL protocol specified...");
			}
		} 

		cipher = configProps.getProperty("cipher"); 
		if (cipher != null) {
			cipher = cipher.trim();
			if (cipher.equals("")) {
				cipher = null;
			}
		}

		if (keyStorePath != null) {
			System.setProperty("javax.net.ssl.keyStore", keyStorePath);
			System.setProperty("javax.net.ssl.keyStorePassword", keyStorePasswordString);
			System.setProperty("javax.net.ssl.trustStore", keyStorePath);
			System.setProperty("javax.net.ssl.trustStorePassword", keyStorePasswordString);
		}


	}

	/**
	 * setSocketFactory may be called to set the default context for an SSL connection.
	 * This method will not set ciphers. The default list of ciphers will be enabled..
	 * 
	 * @throws Exception
	 */
	private  void setSocketFactory() throws Exception {

		if (ssl == false) {
			return;
		}

		try {

			KeyStore ks = KeyStore.getInstance("JKS");
			ks.load(new FileInputStream(keyStorePath), keyStorePasswordString.toCharArray());
			KeyManagerFactory kmf = KeyManagerFactory.getInstance("IbmX509","IBMJSSE2");
			kmf.init(ks, keyStorePasswordString.toCharArray());

			TrustManager[] tm;
			TrustManagerFactory tmf = TrustManagerFactory.getInstance("PKIX","IBMJSSE2");
			tmf.init(ks);
			tm = tmf.getTrustManagers();

			SSLContext sslContext = SSLContext.getInstance(sslProtocol.getText(), "IBMJSSE2");
			sslContext.init(kmf.getKeyManagers(), tm, null);
			SSLContext.setDefault(sslContext);

		} catch (Exception e) {
			System.out.println("Failed to read keystore file <" + keyStorePath + ">");
			throw e;
		}


	}

	/**
	 * getMQTTSSLProps can be called by the MQTT client to set
	 * specific security properties for a specific connection. This
	 * method will enforce a specific protocol (SSLv3, TLSv1, etc...)
	 * 
	 * @return   A properties object that can be used with the MQTT connection
	 */
	public Properties getMQTTSSLProps() {

		if (ssl == false) {
			return null;
		}

		Properties mqttSSLProps = new Properties();

		// For now we just have IBM Java security settings...

		// protocol may be SSLv3, TLSv1, TLSv1.1, TLSv1.2
		mqttSSLProps.setProperty("com.ibm.ssl.protocol",sslProtocol.getText());
		/*	mqttSSLProps.setProperty("com.ibm.ssl.contextProvider","IBMJSSE2");
		mqttSSLProps.setProperty("com.ibm.ssl.keyStore",keyStorePath);
		mqttSSLProps.setProperty("com.ibm.ssl.keyStorePassword",keyStorePasswordString);
		mqttSSLProps.setProperty("com.ibm.ssl.keyStoreType","JKS");
		mqttSSLProps.setProperty("com.ibm.ssl.keyStoreProvider","IBMJSSE2");
		mqttSSLProps.setProperty("com.ibm.ssl.trustStore",keyStorePath);
		mqttSSLProps.setProperty("com.ibm.ssl.trustStorePassword",keyStorePasswordString);
		mqttSSLProps.setProperty("com.ibm.ssl.trustStoreType","JKS");
		mqttSSLProps.setProperty("com.ibm.ssl.trustStoreProvider","IBMJSSE2");
		mqttSSLProps.setProperty("com.ibm.ssl.keyManager","IbmX509");
		mqttSSLProps.setProperty("com.ibm.ssl.trustManager","PKIX");*/

		/*
		 * If a specific cipher is specified use - if not do not set it the
		 * default enabled list for the JVM will be used....
		 */
		if (cipher != null) {
			mqttSSLProps.setProperty("com.ibm.ssl.enabledCipherSuites",cipher);
		}

		return mqttSSLProps;

	}

	/**
	 * Obtain a list of producer specific configuration instances. 
	 * 
	 * @param configProps
	 * @param hostname
	 * @return
	 * @throws IOException
	 */
	public static List<Config> getProducerConfigList(Properties configProps, String hostname) throws IOException {
		List<Config> configList = new ArrayList<Config>();

		try {

			int producerCount = 0;

			String producerCountString = configProps.getProperty("producerConfigCount");
			if (producerCountString != null) {
				producerCount = Integer.parseInt(producerCountString);
			}

			for (int i=1; i<=producerCount; i++) {

				Config configObj = new Config();

				String configDestTypeString = configProps.getProperty("producer.destinationType." + i);
				DEST_TYPE configDestType = DEST_TYPE.fromString(configDestTypeString);

				if (configDestType == null) {
					throw new Exception("Destination type (producer.destinationType) must be either TOPIC or QUEUE");
				}

				configObj.setDestinationType(configDestType);

				String configClientTypeString = configProps.getProperty("producer.clientType." + i);
				CLIENT_TYPE configClientType = CLIENT_TYPE.fromString(configClientTypeString);

				if (configClientType == null) {
					throw new Exception("Client type (clientType) must be either JMS or MQTT");
				}

				configObj.setClientType(configClientType);

				String clientCountString = configProps.getProperty("producer.clientCount." + i);
				configObj.setThreadCount(Integer.parseInt(clientCountString));

				String msgsToSendString = configProps.getProperty("producer.msgsToSend."+ i);
				if (msgsToSendString != null) {
					configObj.setMsgsToSend(Long.parseLong(msgsToSendString));
				}

				String qosString = configProps.getProperty("producer.qos."+ i);
				if (qosString != null) {
					int qosVal = Integer.parseInt(qosString);
					if (qosVal < 0 || qosVal > 2) {
						throw new Exception("QoS must be 0, 1 or 2");
					}
					configObj.setQos(qosVal);
				}
				
				String destDisableACKString = configProps.getProperty("producer.destDisableACK." + i);
				if (destDisableACKString != null) {
					if (destDisableACKString.equalsIgnoreCase("true"))
						configObj.setDestDisableACK(true);
				}
				
				String clientPrefix = configProps.getProperty("producer.clientPrefix."+ i);
				configObj.setClientIdPrefix(clientPrefix);

				String runTimeString = configProps.getProperty("producer.runTimeSec."+ i);
				configObj.setTotalSeconds(Integer.parseInt(runTimeString));

				String intervalString = configProps.getProperty("producer.sendIntervalMs."+ i);
				configObj.setSendInterval(Long.parseLong(intervalString));

				String payload = configProps.getProperty("producer.payload."+ i);
				configObj.setPayload(payload);

				String port = configProps.getProperty("producer.port."+ i);
				configObj.setPort(port);

				if (hostname == null) {
					String serverName = configProps.getProperty("producer.server."+ i);
					configObj.setServer(serverName);
				} else {
					configObj.setServer(hostname);
				}

				String destination = configProps.getProperty("producer.destinationName."+ i);
				configObj.setDestinationName(destination);


				String ignoreErrorsString = configProps.getProperty("producer.ignoreErrors." +i);
				configObj.setIgnoreErrors(Boolean.parseBoolean(ignoreErrorsString));

				String cleanStoreString = configProps.getProperty("producer.cleanStore." +i);
				if (cleanStoreString != null) {
					configObj.setCleanStore(Boolean.parseBoolean(cleanStoreString));
				}

				if (configObj.getDestinationType().equals(DEST_TYPE.QUEUE) && configObj.getClientType().equals(CLIENT_TYPE.MQTT)) {
					throw new Exception("MQTT client not supported for Queues!");
				}

				String padPayloadString = configProps.getProperty("producer.padPayload." +i);
				if (padPayloadString != null) {
					int padLenVal = Integer.parseInt(padPayloadString);
					if (padLenVal > 0) {
						configObj.setPadLen(padLenVal);
					}
				}

				String connStormString = configProps.getProperty("producer.connStorm." + i);
				if (connStormString != null) {
					int connStormVal = Integer.parseInt(connStormString);
					configObj.setConnStorm(connStormVal);
				}

				String usernameString = configProps.getProperty("producer.username." + i);
				configObj.setUsername(usernameString);

				String passwordString = configProps.getProperty("producer.password." + i);
				configObj.setPassword(passwordString);

				configList.add(configObj);

			}


		} catch (Throwable t) {
			throw new IOException(t);
		}


		return configList;

	}


	public static List<Config> getConsumerConfigList(Properties configProps, String hostname) throws IOException {

		List<Config> configList = new ArrayList<Config>();

		try {
			int producerCount = 0;

			String producerCountString = configProps.getProperty("consumerConfigCount");
			if (producerCountString != null) {
				producerCount = Integer.parseInt(producerCountString);
			}

			for (int i=1; i<=producerCount; i++) {

				Config configObj = new Config();

				String configDestTypeString = configProps.getProperty("consumer.destinationType." + i);
				DEST_TYPE configDestType = DEST_TYPE.fromString(configDestTypeString);

				if (configDestType == null) {
					throw new Exception("Destination type must be either TOPIC or QUEUE");
				}

				configObj.setDestinationType(configDestType);

				String configClientTypeString = configProps.getProperty("consumer.clientType." + i);
				CLIENT_TYPE configClientType = CLIENT_TYPE.fromString(configClientTypeString);

				if (configClientType == null) {
					throw new Exception("Client type must be either JMS or MQTT");
				}

				configObj.setClientType(configClientType);


				String clientCountString = configProps.getProperty("consumer.clientCount." + i);
				configObj.setThreadCount(Integer.parseInt(clientCountString));

				String clientPrefix = configProps.getProperty("consumer.clientPrefix."+ i);
				configObj.setClientIdPrefix(clientPrefix);

				String runTimeString = configProps.getProperty("consumer.runTimeSec."+ i);
				configObj.setTotalSeconds(Integer.parseInt(runTimeString));

				String sleepIntervalString = configProps.getProperty("consumer.sleepIntervalMs."+ i);
				configObj.setConsumeSleepInterval(Long.parseLong(sleepIntervalString));

				String msgsToConsumeString = configProps.getProperty("consumer.msgsToConsume."+ i);
				if (msgsToConsumeString != null) {
					configObj.setMsgsToConsume(Long.parseLong(msgsToConsumeString));
				}

				String qosString = configProps.getProperty("consumer.qos."+ i);
System.out.println("Consumer "+ i +" QoS string is "+qosString);
				if (qosString != null) {
					int qosVal = Integer.parseInt(qosString);
					if (qosVal < 0 || qosVal > 2) {
						throw new Exception("QoS must be 0, 1 or 2");
					}
					configObj.setQos(qosVal);
				}
System.out.println("Consumer "+ i +" QoS value is "+configObj.getQos());

				String waitForMsgsString = configProps.getProperty("consumer.waitForMsgsMs."+ i);
				configObj.setConsumeWaitForMsgs(Long.parseLong(waitForMsgsString));


				String port = configProps.getProperty("consumer.port."+ i);
				configObj.setPort(port);

				if (hostname == null) {
					String serverName = configProps.getProperty("consumer.server."+ i);
					configObj.setServer(serverName);
				} else {
					configObj.setServer(hostname);
				}
				
				String anonString = configProps.getProperty("consumer.anonymous." +i); 

				if (anonString != null) {
					if (anonString.equalsIgnoreCase("true")) {
						configObj.setAnonymous(true);
					}
				}
				
				String sharedString = configProps.getProperty("consumer.shared." +i); 

				if (sharedString != null) {
					if (sharedString.equalsIgnoreCase("true")) {
						configObj.setShared(true);
					}
				}

				String destination = configProps.getProperty("consumer.destinationName."+ i);
				configObj.setDestinationName(destination);


				String ignoreErrorsString = configProps.getProperty("consumer.ignoreErrors." +i);
				configObj.setIgnoreErrors(Boolean.parseBoolean(ignoreErrorsString));

				String cleanStoreString = configProps.getProperty("consumer.cleanStore." +i);
				if (cleanStoreString != null) {
					configObj.setCleanStore(Boolean.parseBoolean(cleanStoreString));
				}

				if (configObj.getDestinationType().equals(DEST_TYPE.QUEUE) && configObj.getClientType().equals(CLIENT_TYPE.MQTT)) {
					throw new Exception("MQTT client not supported for Queues!");
				}

				String subscriptionNameString = configProps.getProperty("consumer.subscriptionName." +i);
				if (subscriptionNameString != null && subscriptionNameString.length() > 0) {
					configObj.setSubscriptionName(subscriptionNameString);
					String unsubString = configProps.getProperty("consumer.forceUnsubscribe."+i);
					if (unsubString != null) {
						if (unsubString.equalsIgnoreCase("true"))
							configObj.setForceUnsubscribe(true);
					}
				}

				String usernameString = configProps.getProperty("consumer.username." + i);
				configObj.setUsername(usernameString);

				String passwordString = configProps.getProperty("consumer.password." + i);
				configObj.setPassword(passwordString);

				configList.add(configObj);

			}



		} catch (Throwable t) {
			throw new IOException(t);
		}

		return configList;
	}

	public static String getRandomPayload(int payloadLen) {

		String retVal = null;
		StringBuffer buf = new StringBuffer();

		while (buf.length() < payloadLen ) {

			SecureRandom random = new SecureRandom();
			String randomString = new BigInteger(130, random).toString(32);
			buf.append(randomString);

		}

		retVal = buf.toString().substring(0, payloadLen);
		return retVal;

	}

	public static String getSVTUser(String userRange)  {

	
		if (userRange == null) {
			return null;
		}
		
		boolean goodRange = userRange.matches("[uUaAcC]([0-9]{7})-[uUaAcC]([0-9]{7})");

		
		if (!goodRange) {
			return null;
		}
		
		String firstChar = userRange.substring(0,1);
		String firstChar2 = userRange.substring(9,10);
		userRange = userRange.replaceAll("[uUaAcC]", "");
		String[] rangeValues = userRange.split("-");

		if (firstChar.equalsIgnoreCase((firstChar2)) != true ) {
			return null;
		}
		
		int lowVal = Integer.parseInt(rangeValues[0]);
		int highVal = Integer.parseInt(rangeValues[1]);

		if (lowVal > highVal) {
			int tmpVal = lowVal;
			lowVal = highVal;
			highVal = tmpVal;
		}

		int val =  lowVal + (int)(Math.random() * ((highVal - lowVal) + 1));

		String valString = "" + val;
		String userId= firstChar + "0000000";
		userId = userId.substring(0, ((userId.length()) - (valString.length()) ));
		userId = userId + valString;
		
		return userId;

	}

	public String getDestinationName() {
		return destinationName;
	}

	public void setDestinationName(String destinationName) {
		this.destinationName = destinationName;
	}

	public String getPort() {
		return port;
	}

	public void setPort(String port) {
		this.port = port;
	}

	public String getServer() {
		return server;
	}

	public void setServer(String server) {
		this.server = server;
	}

	public int getThreadCount() {
		return threadCount;
	}

	public void setThreadCount(int threadCount) {
		this.threadCount = threadCount;
	}

	public String getPayload() {
		if (padLen == 0) {
			return payload;
		} else {
			return getRandomPayload(padLen);
		}
	}

	public void setPayload(String payload) {
		this.payload = payload;
	}

	public int getTotalSeconds() {
		return totalSeconds;
	}

	public void setTotalSeconds(int totalSeconds) {
		this.totalSeconds = totalSeconds;
	}

	public long getSendInterval() {
		return sendInterval;
	}

	public void setSendInterval(long sendInterval) {
		this.sendInterval = sendInterval;
	}

	public String getClientIdPrefix() {
		return clientIdPrefix;
	}

	public void setClientIdPrefix(String clientIdPrefix) {
		this.clientIdPrefix = clientIdPrefix;
	}
	
	public boolean isAnonymous() {
		return anonymous;
	}
	
	public boolean setAnonymous(boolean anonymous) {
		return this.anonymous = anonymous;
	}

	public boolean isIgnoreErrors() {
		return ignoreErrors;
	}

	public void setIgnoreErrors(boolean ignoreErrors) {
		this.ignoreErrors = ignoreErrors;
	}

	public long getConsumeSleepInterval() {
		return consumeSleepInterval;
	}

	public void setConsumeSleepInterval(long consumeSleepInterval) {
		this.consumeSleepInterval = consumeSleepInterval;
	}

	public long getConsumeWaitForMsgs() {
		return consumeWaitForMsgs;
	}

	public long getMsgsToSend() {
		return msgsToSend;
	}

	public void setMsgsToSend(long msgsToSend) {
		this.msgsToSend = msgsToSend;
	}

	public void setConsumeWaitForMsgs(long consumeWaitForMsgs) {
		this.consumeWaitForMsgs = consumeWaitForMsgs;
	}

	public static boolean isVerboseOutput() {
		return verboseOutput;
	}

	public static void setVerboseOutput(boolean verboseOutput) {
		Config.verboseOutput = verboseOutput;
	}

	public long getMsgsToConsume() {
		return msgsToConsume;
	}

	public void setMsgsToConsume(long msgsToConsume) {
		this.msgsToConsume = msgsToConsume;
	}

	public DEST_TYPE getDestinationType() {
		return destinationType;
	}

	public void setDestinationType(DEST_TYPE destinationType) {
		this.destinationType = destinationType;
	}

	public CLIENT_TYPE getClientType() {
		return clientType;
	}

	public void setClientType(CLIENT_TYPE clientType) {
		this.clientType = clientType;
	}

	public int getQos() {
		return qos;
	}

	public void setQos(int qos) {
		this.qos = qos;
	}
	
	public boolean getDestDisableACK() {
		return destDisableACK;
	}

	public void setDestDisableACK(boolean destDisableACK) {
		this.destDisableACK = destDisableACK;
	}

	public boolean isCleanStore() {
		return cleanStore;
	}

	public void setCleanStore(boolean cleanStore) {
		this.cleanStore = cleanStore;
	}

	public static boolean isSsl() {
		return ssl;
	}

	public static void setSsl(boolean ssl) {
		Config.ssl = ssl;
	}

	public String getUsername() {
		String checkUsername = getSVTUser(username);
		if (checkUsername != null) {
			return checkUsername;
		} else {
			return username;
		}
	}

	public void setUsername(String username) {
		this.username = username;
	}

	public String getPassword() {
		return password;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public String getKeyStore() {
		return keyStore;
	}

	public void setKeyStore(String keyStore) {
		this.keyStore = keyStore;
	}

	public String getKeyStorePassword() {
		return keyStorePassword;
	}

	public void setKeyStorePassword(String keyStorePassword) {
		this.keyStorePassword = keyStorePassword;
	}

	public String getTrustStore() {
		return trustStore;
	}

	public void setTrustStore(String trustStore) {
		this.trustStore = trustStore;
	}

	public String getTrustStorePassword() {
		return trustStorePassword;
	}

	public void setTrustStorePassword(String trustStorePassword) {
		this.trustStorePassword = trustStorePassword;
	}

	public String getSubscriptionName() {
		return subscriptionName;
	}

	public void setSubscriptionName(String subscriptionName) {
		this.subscriptionName = subscriptionName;
	}
	
	public boolean doForceUnsubscribe() {
		return forceUnsubscribe;
	}
	
	public void setForceUnsubscribe(boolean forceUnsubscribe) {
		this.forceUnsubscribe = forceUnsubscribe;
	}
	
	public boolean isShared() {
		return shared;
	}
	
	public void setShared(boolean shared) {
		this.shared = shared;
	}

	public static SSL_PROTOCOL getSslProtocol() {
		return sslProtocol;
	}

	public static String getCipher() {
		return cipher;
	}

	public int getPadLen() {
		return padLen;
	}

	public void setPadLen(int padLen) {
		this.padLen = padLen;
	}

	public int getConnStorm() {
		return connStorm;
	}

	public void setConnStorm(int connStorm) {
		this.connStorm = connStorm;
	}

}
