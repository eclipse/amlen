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
package svt.scale.user;

import java.io.FileOutputStream;
import java.io.PrintStream;
import java.sql.Timestamp;
import java.util.Random;
import java.util.concurrent.atomic.AtomicLong;

import org.eclipse.paho.client.mqttv3.MqttClient;

/**
 * The purpose of this sample is to demonstrate a basic example of how to use MQ's native MQTTv3 classes to publish and
 * subscribe to an ISM topic. This sample consists of three classes: MqttSample.java, MqttSamplePublish.java and
 * MqttSampleSubscribe.java
 * 
 * MqttSample contains methods for parsing and validating arguments, logging messages, and other utility type functions.
 * 
 * MqttSampleSubscribe contains methods specific to subscribing to messages published to an ISM topic.
 * 
 * MqttSamplePublish contains methods specific to publishing messages to an ISM topic.
 * 
 * When publish is specified on the command line as the action argument, a message is repeatedly published to the topic
 * a specified number of times.
 * 
 * Similarly when subscribe is specified on the command line as the action argument, the sample will subscribe to the
 * topic and wait for the specified number of messages to be received.
 * 
 * See logged messages for verification of sent and received messages.
 * 
 * NOTE: This class implements Runnable to support execution in a separate thread. However, a separate thread is not
 * required and the main function does not use one for execution.
 * 
 * Command line options:
 * 
 * -i clientID Optional parameter specifying the client id used for registering the client with the ISM server.
 * 
 * -s serverURI The URI address of the ISM server. This is in the format of tcp://<ip address>:<port> This is a required
 * parameter.
 * 
 * -a action Either the String: publish or subscribe. This is a required parameter.
 * 
 * -t topicName The name of the topic on which the messages are published. The default topic name is /MQTTv3Sample
 * 
 * -m message A String representing the message to be sent. The default message is JMS Sample Message
 * 
 * -n count The number of times the specified message is to be sent. The default number of message sent is 1.
 * 
 * -q qos The Quality of Service level 0, 1, or 2 The default QOS is 0.
 * 
 * -c cleansession true or false value indicating if server side session data should be removed when client disconnects.
 * 
 * -e unsubscribe at client disconnect.
 * 
 * -o Log filename The log defaults to stdout.
 * 
 * -r Enable persistence and specify datastore directory The default persistence is false.
 * 
 * -u client user id for secure communications. Optional parameter.
 * 
 * -p client password for secure communications. Optional parameter.
 * 
 * -w rate at which messages are sent in units of messages/second.
 * 
 * -ct Connection Time in MQTT Options object
 * 
 * -cr Connection Retries, # of attempts to establish a successful connection ( -1 Infinite, 0 No Attempts, >0 Count of
 * retry attempts)
 * 
 * -v Indicates verbose output
 * 
 * -h Output usage statement
 * 
 * Additionally, per MQ's MQTT documentation, valid SSL system properties are:
 * 
 * com.ibm.ssl.protocol One of: SSL, SSLv3, TLS, TLSv1, SSL_TLS. com.ibm.ssl.contextProvider Underlying JSSE provider.
 * For example "IBMJSSE2" or "SunJSSE" com.ibm.ssl.keyStore The name of the file that contains the KeyStore object that
 * you want the KeyManager to use. For example /mydir/etc/key.p12 com.ibm.ssl.keyStorePassword The password for the
 * KeyStore object that you want the KeyManager to use. The password can either be in plain-text, or may be obfuscated
 * using the static method: com.ibm.micro.security.Password.obfuscate(char[] password). This obfuscates the password
 * using a simple and insecure XOR and Base64 encoding mechanism. Note that this is only a simple scrambler to obfuscate
 * clear-text passwords. com.ibm.ssl.keyStoreType Type of key store, for example "PKCS12", "JKS", or "JCEKS".
 * com.ibm.ssl.keyStoreProvider Key store provider, for example "IBMJCE" or "IBMJCEFIPS". com.ibm.ssl.trustStore The
 * name of the file that contains the KeyStore object that you want the TrustManager to use.
 * com.ibm.ssl.trustStorePassword The password for the TrustStore object that you want the TrustManager to use. The
 * password can either be in plain-text, or may be obfuscated using the static method:
 * com.ibm.micro.security.Password.obfuscate(char[] password). This obfuscates the password using a simple and insecure
 * XOR and Base64 encoding mechanism. Note that this is only a simple scrambler to obfuscate clear-text passwords.
 * com.ibm.ssl.trustStoreType The type of KeyStore object that you want the default TrustManager to use. Same possible
 * values as "keyStoreType". com.ibm.ssl.trustStoreProvider Trust store provider, for example "IBMJCE" or "IBMJCEFIPS".
 * com.ibm.ssl.enabledCipherSuites A list of which ciphers are enabled. Values are dependent on the provider, for
 * example: SSL_RSA_WITH_AES_128_CBC_SHA;SSL_RSA_WITH_3DES_EDE_CBC_SHA. com.ibm.ssl.keyManager Sets the algorithm that
 * will be used to instantiate a KeyManagerFactory object instead of using the default algorithm available in the
 * platform. Example values: "IbmX509" or "IBMJ9X509". com.ibm.ssl.trustManager Sets the algorithm that will be used to
 * instantiate a TrustManagerFactory object instead of using the default algorithm available in the platform. Example
 * values: "PKIX" or "IBMJ9X509".
 * 
 */

public class ATUser implements Runnable {

	final static int AT_MOST_ONCE = 0;
	final static int AT_LEAST_ONCE = 1;
	final static int EXACTLY_ONCE = 2;

	/**
	 * The main method executes the client's run method.
	 * 
	 * @param args
	 *            Commandline arguments. See usage statement.
	 */
	public static void main(String[] args) {

		/*
		 * Instantiate and run the client.
		 */
		new ATUser(args).run();

	}

	//    public String action = null;
	// public String clientID = null;
	public String clientID = null;
	public String subscriberID = null;
	public String publisherID = null;
	public String userName = null;
	public String password = null;
	public long throttleWaitMSec = 0;
	public long count = 1;
	public boolean defaultcount = true;
	// public Long[] qosCount = {0L,0L,0L};
	public Long qos0Min = 0L;
	public Long qos0Max = 0L;
	public Long qos1Min = 0L;
	public Long qos2 = 0L;
	// public Integer shared = 1;
	public String dataStoreDir = ".";
	public String payload = "ISM WebSocket Sample Message";
	public boolean persistence = false;
	public boolean retained = false;
	public int qos = AT_MOST_ONCE;
	public boolean cleanSession = true;
	public boolean cleanSessionState = false; // Control whether to do the CleanSession State Swizzle
	public String serverURI = null;
	public String subscriberURI = null;
	public String publisherURI = null;
	public String oauthProviderURI = null;
	public String oauthToken = null;
//	public String topicName = "/MQTTv3Sample";
	public boolean ssl = false;
	public String sslProtocol = null;
	public boolean verbose = false;
	public boolean veryverbose = false;
	// public long seconds = 0;
	public boolean order = false;
	public boolean orderJSON = false;
	public boolean gatherer = false;
	public int sharedSubscribers = 0;
	public int publishers = 0;
	public log log = new log(System.out);
	public time time = new time();
	public int keepAliveInterval = 150;
	public int connectionTimeout = 150;
	public int connectionRetry = -1; // -1 Infinite Retry, 0 No Retry, >0 Count of Retries to get a successful
	// connection
	public String mqttVersion = null;
	boolean unsubscribe = false;
	boolean debug = false;
	String subscriberTopic = null;
	String publisherTopic = null;
	Long skip = null;
	Long duplicate = null;
	boolean discard = false;
	long bufferSize = 0;
	int initialWait = 0;
	int wait = 0;
	public int maxInflight = 0;
	public String user = null;
	public String app = null;
	public String car = null;
	boolean iotf = false;
	public String evt = null;

	/**
	 * Instantiates a new MQTT client.
	 * 
	 * 
	 */
	public ATUser(String[] args) {
		parseArgs(args);
	}

	public ATUser(ATUser config) {
		//        this.action = config.action;
		this.clientID = config.clientID;
		this.userName = config.userName;
		this.password = config.password;
		this.throttleWaitMSec = config.throttleWaitMSec;
		this.count = config.count;
		this.defaultcount = config.defaultcount;
		// public Long[] qosCount = {0L,0L,0L};
		this.qos0Min = config.qos0Min;
		this.qos0Max = config.qos0Max;
		this.qos1Min = config.qos1Min;
		this.qos2 = config.qos2;
		// this.shared = config.shared;
		this.dataStoreDir = config.dataStoreDir;
		this.payload = config.payload;
		this.persistence = config.persistence;
		this.retained = config.retained;
		this.qos = config.qos;
		this.cleanSession = config.cleanSession;
		this.cleanSessionState = config.cleanSessionState;
		this.serverURI = config.serverURI;
		this.oauthProviderURI = config.oauthProviderURI;
		this.oauthToken = config.oauthToken;
//		this.topicName = config.topicName;
		this.ssl = config.ssl;
		this.sslProtocol = config.sslProtocol;
		this.verbose = config.verbose;
		this.veryverbose = config.veryverbose;
		// this.seconds = config.seconds;
		this.order = config.order;
		this.orderJSON = config.orderJSON;
		this.subscriberURI = config.subscriberURI;
		this.subscriberID = config.subscriberID;
		this.publisherURI = config.publisherURI;
		this.publisherID = config.publisherID;
		this.subscriberTopic = config.subscriberTopic;
		this.publisherTopic = config.publisherTopic;
		this.gatherer = config.gatherer;
		this.sharedSubscribers = config.sharedSubscribers;
		this.publishers = config.publishers;
		this.log = config.log;
		this.keepAliveInterval = config.keepAliveInterval;
		this.connectionTimeout = config.connectionTimeout;
		this.connectionRetry = config.connectionRetry;
		this.mqttVersion = config.mqttVersion;
		this.unsubscribe = config.unsubscribe;
		this.debug = config.debug;
		this.skip = config.skip;
		this.duplicate = config.duplicate;
		this.discard = config.discard;
		this.bufferSize = config.bufferSize;
		this.time = config.time;
		this.initialWait = config.initialWait;
		this.wait = config.wait;
		this.maxInflight = config.maxInflight;
		this.user = config.user;
		this.app = config.app;
		this.car = config.car;
		this.iotf = config.iotf;
		this.evt = config.evt;
	}

	/**
	 * Parses the command line arguments passed into main().
	 * 
	 * @param args
	 *            the command line arguments. See usage statement.
	 */
	private void parseArgs(String[] args) {
		boolean showUsage = false;
		String comment = null;

		for (int i = 0; i < args.length; i++) {
			if ("-oa".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				oauthProviderURI = args[i];
			} else if ("-s".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				serverURI = args[i];
				if (serverURI.startsWith("tcp://")) {
					ssl = false;
				} else if (serverURI.startsWith("ssl://")) {
					ssl = true;
				} else {
					showUsage = true;
					comment = "Invalid Parameter:  " + args[i] + ".  serverURI must begin with either tcp:// or ssl://.";
					break;
				}
				//            } else if ("-a".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				//                i++;
				//                action = args[i];
			} else if ("-i".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				clientID = args[i];
			} else if ("-m".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				payload = args[i];
//			} else if ("-t".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
//				i++;
//				// if (monitor == false) {
//				topicName = args[i];
//				// }
			} else if ("-n".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				count = Long.parseLong(args[i]);
				defaultcount = false;
			} else if ("-nq".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				String[] words = args[i].split(":");
				if (words[0] != null) {
					qos0Min = Long.parseLong(words[0]);
				}
				if (words[1] != null) {
					qos0Max = Long.parseLong(words[1]);
				}
				if (words[2] != null) {
					qos1Min = Long.parseLong(words[2]);
				}
				if (words[3] != null) {
					qos2 = Long.parseLong(words[3]);
				}
				// } else if ("-shared".equals(args[i]) && (i + 1 < args.length)) {
				// i++;
				// shared = Integer.parseInt(args[i]);
			} else if ("-resendURI".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				publisherURI = args[i];
			} else if ("-resendTopic".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				publisherTopic = args[i];
			} else if ("-resendID".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				publisherID = args[i];
				if (publisherID.length() > 23) {
					showUsage = true;
					comment = "Invalid Parameter:  " + args[i] + ".  The resendID cannot exceed 23 characters.";
					break;
				}
			} else if ("-gather".equals(args[i])) {
				gatherer = true;
			} else if ("-sharedsubscribers".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				sharedSubscribers = Integer.parseInt(args[i]);
			} else if ("-publishers".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				publishers = Integer.parseInt(args[i]);
			} else if ("-ka".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				keepAliveInterval = Integer.parseInt(args[i]);
			} else if ("-ct".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				connectionTimeout = Integer.parseInt(args[i]);
			} else if ("-cr".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				connectionRetry = Integer.parseInt(args[i]);
			} else if ("-N".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				long seconds = Long.parseLong(args[i]) * 60;
				time.setRuntimeSeconds(seconds);
			} else if ("-Nm".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				long seconds = Long.parseLong(args[i]) * 60;
				time.setRuntimeSeconds(seconds);
			} else if ("-Ns".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				long seconds = Long.parseLong(args[i]);
				time.setRuntimeSeconds(seconds);
			} else if ("-o".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				log = new log(args[i]);
			} else if ("-O".equals(args[i])) {
				order = true;
			} else if ("-OJSON".equals(args[i])) {
				order = true;
				orderJSON = true;
			} else if ("-skip".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				skip = Long.parseLong(args[i]);
			} else if ("-duplicate".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				duplicate = Long.parseLong(args[i]);
			} else if ("-pm".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				dataStoreDir = args[i];
				persistence = true;
			} else if ("-rm".equalsIgnoreCase(args[i])) {
				retained = true;
			} else if ("-v".equalsIgnoreCase(args[i])) {
				verbose = true;
			} else if ("-discard".equalsIgnoreCase(args[i])) {
				discard = true;
			} else if ("-buffersize".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				bufferSize = Long.parseLong(args[i]);
			} else if ("-vv".equalsIgnoreCase(args[i])) {
				veryverbose = true;
				verbose = true;
			} else if ("-debug".equalsIgnoreCase(args[i])) {
				debug = true;
			} else if ("-h".equalsIgnoreCase(args[i])) {
				showUsage = true;
				break;
			} else if ("-q".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				if ("0".equals(args[i])) {
					qos = AT_MOST_ONCE;
				} else if ("1".equals(args[i])) {
					qos = AT_LEAST_ONCE;
				} else if ("2".equals(args[i])) {
					qos = EXACTLY_ONCE;
				} else {
					showUsage = true;
					comment = "Invalid Parameter:  " + args[i];
					break;
				}
			} else if ("-c".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				if ("true".equalsIgnoreCase(args[i])) {
					cleanSession = true;
					cleanSessionState = false;
				} else if ("false".equalsIgnoreCase(args[i])) {
					cleanSession = false;
					cleanSessionState = false;
				} else if ("static-true".equalsIgnoreCase(args[i])) { //DON'T DO THE TRUE-FALSE Swizzle for HA
					cleanSession = true;
					cleanSessionState = true;
				} else {
					showUsage = true;
					comment = "Invalid Parameter:  " + args[i];
					break;
				}
			} else if ("-e".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				if ("true".equalsIgnoreCase(args[i])) {
					unsubscribe = true;
				} else if ("false".equalsIgnoreCase(args[i])) {
					unsubscribe = false;
				} else {
					showUsage = true;
					comment = "Invalid Parameter:  " + args[i];
					break;
				}
			} else if ("-u".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				userName = args[i];
			} else if ("-p".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				password = args[i];
			} else if ("-w".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				throttleWaitMSec = Integer.parseInt(args[i]);
				log.println("throttleWaitMSec:  " + throttleWaitMSec);
				// } else if ("-M".equals(args[i])) {
				// monitor = true;
				// topicName = "/svt/#";
				// qos = 1;
			} else if ("-maxInflight".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				maxInflight = Integer.parseInt(args[i]);
			} else if ("-mqtt".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				mqttVersion = args[i];
			} else if ("-sslProtocol".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				sslProtocol = args[i];
			} else if ("-receiverTimeout".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				String[] words = args[i].split(":");
				if (words[0] != null) {
					initialWait = Integer.parseInt(words[0]);
				}
				if (words[1] != null) {
					wait = Integer.parseInt(words[1]);
				}
			} else if ("-user".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				user = args[i];
			} else if ("-app".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				app = args[i];
			} else if ("-car".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
				i++;
				car = args[i];
         } else if ("-evt".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            evt = args[i];
			} else if ("-iotf".equalsIgnoreCase(args[i])) {
				iotf = true;
			} else {
				showUsage = true;
				comment = "Invalid Parameter:  " + args[i];
				break;
			}
		}

		if (veryverbose == true) {
			log.print("java " + this.getClass().getName());
			for (String a : args) {
				log.print(" " + a);
			}
			log.print("\n");
		}

		if (showUsage == false) {
			if (dataStoreDir == null)
				dataStoreDir = System.getProperty("java.io.tmpdir");

			if (clientID == null) {
				clientID = MqttClient.generateClientId();
				if ((clientID == null) || ((clientID.length() > 23) && ("3.1".equals(mqttVersion)))) {
					// If the generated clientId is too long, then create one based on action and a
					// random number.
					Random rnd = new Random();
					rnd.setSeed(System.currentTimeMillis());
					clientID = String.format("%s_%.0f", "USER", (float) rnd.nextInt(99999));
				}
			}

			if ((clientID.length() > 23) && ("3.1".equals(mqttVersion))) {
				comment = "Invalid Client ID:  " + clientID + ".  The clientId cannot exceed 23 characters.";
				showUsage = true;
			}

			if ((qos == 0) && (qos0Max == 0) && (count != 0)) {
				qos0Max = count;
			} else if ((qos == 1) && (qos1Min == 0) && (count != 0)) {
				qos1Min = count;
			} else if ((qos == 2) && (qos2 == 0) && (count != 0)) {
				qos2 = count;
			}

			if (iotf) {
				if (car == null) {
					comment = "Missing required parameter: -car <car id>";
					showUsage = true;
				} else {
					subscriberID = clientID;
					subscriberURI = serverURI;
					subscriberTopic = "iot-2/cmd/+/fmt/+";
					publisherID = clientID;
					publisherURI = serverURI;
				   publisherTopic = "iot-2/evt/"+evt+":car:" + car + "/fmt/json";
				}
			} else {
				if (app == null) {
					comment = "Missing required parameter: -app <app id>";
					showUsage = true;
				} else if (user == null) {
					comment = "Missing required parameter: -user <user id>";
					showUsage = true;
				} else if (car == null) {
					comment = "Missing required parameter: -car <car id>";
					showUsage = true;
				} else {
					subscriberID = clientID;
					subscriberURI = serverURI;
					subscriberTopic = "/USER/" + user + "/APP/" + app + "/#";
					publisherID = clientID;
					publisherURI = serverURI;
					publisherTopic = "/APP/" + app + "/USER/" + user + "/CAR/" + car;
					payload = "" + System.currentTimeMillis();
				}
			}

			if (serverURI == null) {
				comment = "Missing required parameter: -s <serverURI>";
				showUsage = true;
				//			} else if (action == null) {
				//				comment = "Missing required parameter: -a publish|subscribe";
				//				showUsage = true;
				//			} else if (!("publish".equals(action)) && !("subscribe".equals(action))) {
				//				comment = "Invalid parameter: -a " + action;
				showUsage = true;
			} else if ((password != null) && (userName == null)) {
				comment = "Missing parameter: -u <userId>";
				showUsage = true;
			} else if ((publisherURI != null) && (publisherID == null)) {
				comment = "Missing parameter: -resendID <userId>";
				showUsage = true;
			} else if ((publisherID != null) && (publisherURI == null)) {
				comment = "Missing parameter: -resendURI <resendURI>";
				showUsage = true;
			} else if ((userName != null) && (password == null)) {
				comment = "Missing parameter: -p <password>";
				showUsage = true;
			} else if ((keepAliveInterval < 0) || (keepAliveInterval > (6480))) {
				comment = "Maximum keepalive is 18 hours: -ka 6480";
				showUsage = true;
			} else if ((mqttVersion != null) && !("3.1.1".equals(mqttVersion)) && !("3.1".equals(mqttVersion))) {
				comment = "Invalid parameter:  -mqtt " + mqttVersion;
				showUsage = true;
			} else if ((discard) && (bufferSize <= 0)) {
				comment = "Missing parameter: -buffersize <buffersize>";
				showUsage = true;
			} else if ((!discard) && (bufferSize > 0)) {
				comment = "Missing parameter: -discard";
				showUsage = true;
			}

			if (!showUsage) {
				//				if (action == "subscribe") {
				//					subscriberID = clientID;
				//					subscriberURI = serverURI;
				//					subscriberTopic = topicName;
				//				} else if (action == "publish") {
				publisherID = clientID;
				publisherURI = serverURI;
//				publisherTopic = topicName;
				//				}
			}

		}

		if (showUsage) {
			usage(args, comment);
			System.exit(1);
		}
	}

	/**
	 * Primary MQTT client method that either publishes or subscribes messages on a topic
	 */
	public void run() {

		//        if (oauthProviderURI != null) {
		//            this.oauthToken = new GetOAuthAccessToken().getOAuthToken(clientID, this);
		//        }

		try {
			//            if ("publish".equalsIgnoreCase(action)) {
			new ATUserEngine(this).run();
			//            } else { // If action is not publish then it must be subscribe
			//                new MqttSampleSubscribe().run(clientID, this);
			//            }
		} catch (Throwable e) {
			log.println("MqttException caught in MQTT sample:  " + e.getMessage());
			e.printStackTrace();
		}

		/*
		 * close program handles
		 */
		log.close();

		return;
	}

	/**
	 * Change logging to a file
	 * 
	 * @param filename
	 *            filename for log file
	 */

	/**
	 * Output the usage statement to standard out.
	 * 
	 * @param args
	 *            The command line arguments passed into main().
	 * @param comment
	 *            An optional comment to be output before the usage statement
	 */
	private void usage(String[] args, String comment) {

		if (comment != null) {
			System.err.println(comment);
			System.err.println();
		}

		System.err
		         .println("usage:  "
		                  + "-s <Server URI> -a publish|subscribe [-i <clientId>][-t <topic name>][-m <payload>][-n <num messages>][-q 0|1|2][-o <logfile>][-c true|false][-r <temp datastore dir>][-u <userId>][-p <password>][-w <rate>][-v][-h]");
		System.err.println();
		System.err
		         .println(" -i clientId  The client identity used for connection to the ISM server.  The maximum length is 23 characters.  This is an optional parameter.");
		System.err
		         .println(" -s serverURI The URI address of the ISM server. This is in the format of tcp://<ipaddress>:<port>. This is a required parameter.");
		System.err
		         .println(" -resendURI serverURI The URI address of the ISM server to resend messages. This is in the format of tcp://<ipaddress>:<port>.");
		System.err
		         .println(" -resendID clientId  The client identity used for the resend connection to the ISM server.  The maximum length is 23 characters.");
		System.err.println(" -a action Either the String: publish or subscribe. This is a required parameter.");
		System.err
		         .println(" -t topicName The name of the topic on which the messages are published. The default topic name is /MQTTv3Sample.");
		System.err.println(" -m message A String representing the message to be sent. Default value is \"ISM WebSocket Sample Message\"");
		System.err.println(" -n count The number of times the specified message is to be sent. The default number of message sent is 1.");
		System.err.println(" -N hours The number of hours the publisher will send the specified message.");
		System.err.println(" -Nm minutes The number of minutes the publisher will send the specified message.");
		System.err.println(" -q qos The Quality of Service level 0, 1, or 2 The default QOS is 0.");
		System.err
		         .println(" -c cleanSession  true or false value indicating if session data is to be removed on client disconnect. Defaults value is true");
		System.err
		         .println(" -e unsubscribe  true or false value indicating if client should unsubscribe at disconnect. Defaults value is true");
		System.err.println(" -o Log filename The log defaults to stdout.");
		System.err.println(" -pm enable persistence and specify datastore directory. The default persistence is false.");
		System.err.println(" -rm enable retained messages. The default retained is false.");
		System.err.println(" -u userid for secure communications. This is an optional parameter.");
		System.err.println(" -p password for secure communications.  This is an optional parameter.");
		System.err.println(" -w rate at which messages are sent in units of messages/second.");
		System.err.println(" -O ordercheck of shared subscription");
		System.err.println(" -qn optional parameter for order checking. Specify : separated list qos0min:qos0max:qos1min:qos2");
		System.err.println(" -OJSON ordercheck of shared subscription passing IoT-Foundation JSON Syntax");
		System.err.println(" -sharedSubscribers start # of shared subscribers");
		System.err.println(" -gather ordercheck of shared subscription");
		System.err.println(" -cr Connection Retries ");
		System.err.println(" -debug Indicates debug output");
		System.err.println(" -v Indicates verbose output");
		System.err.println(" -M Monitor Automotive Telematics statistics");
		System.err.println(" -w throttle number of messages per second.");
		System.err.println(" -ka keepAliveInterval.");
		System.err.println(" -ct connectionTimeout.");
		System.err.println(" -cr Connection Retries ");
		System.err.println(" -oa <URI> OAuth provider. This is in the format of <ipaddress>:<port>");
		System.err.println(" -mqtt <version> MQTT Specification version used for API calls.  Must be either 3.1 or 3.1.1");
		System.err
		         .println(" -receiverTimeout optional parameter in minutes for timing out the receiver. Specify initialWait:wait. Default is 0:0 (disabled).");
		System.err.println(" -sslProtocol <protocol> the protocol used for ssl connections such as TLS, TLSv1, TLSv1.1, or TLSv1.2");
		System.err.println(" -roundtrip <topic> sends timestamp JSON message and waits for response message.");
		System.err.println(" -h output usage statement.");

	}

	class time {
		private long seconds = 0;
		private AtomicLong startTime = new AtomicLong(System.currentTimeMillis());

		void setStartTime() {
			startTime.set(System.currentTimeMillis());
		}

		void setRuntimeSeconds(long seconds) {
			this.seconds = seconds;
		}

		long actualRunTimeMillis() {
			return System.currentTimeMillis() - startTime.get();
		}

		long configuredRunTimeSeconds() {
			return seconds;
		}

		long configuredRunTimeMillis() {
			return seconds * 1000L;
		}

		long remainingRunTimeMillis() {
			long remainingtime = Long.MAX_VALUE;
			if (configuredRunTimeSeconds() > 0) {
				remainingtime = configuredRunTimeMillis() - actualRunTimeMillis();
			}
			return remainingtime;
		}

		boolean runTimeRemains() {
			return (remainingRunTimeMillis() > 0);
		}

		boolean runTimeExpired() {
			return (remainingRunTimeMillis() <= 0);
		}

	}

	class log {
		public PrintStream stream = System.out;
		Byte[] lock = new Byte[1];

		log(PrintStream ps) {
			stream = ps;
		}

		log(String filename) {
			try {
				FileOutputStream fos = new FileOutputStream(filename);
				stream = new PrintStream(fos);
				if (verbose)
					log.println("Log sent to " + filename);

			} catch (Throwable e) {
				log.println("Log entries sent to System.out instead of " + filename);
			}
		}

		void println(String string) {
			synchronized (lock) {
				String now = new Timestamp(new java.util.Date().getTime()).toString();
				String[] list = now.split("[.]");
				stream.printf("%s.%s  %s\n", list[0], String.format("%-3s", list[1]).replace(' ', '0'), string);
				stream.flush();
			}
		}

		void printErr(String string) {
			synchronized (lock) {
				String now = new Timestamp(new java.util.Date().getTime()).toString();
				String[] list = now.split("[.]");
				stream.printf("%s.%s  %s\n", list[0], String.format("%-3s", list[1]).replace(' ', '0'), string);
				stream.flush();
				System.err.printf("%s.%s  %s\n", list[0], String.format("%-3s", list[1]).replace(' ', '0'), string);
				System.err.flush();
			}
		}

		void printTm(String string) {
			synchronized (lock) {
				String now = new Timestamp(new java.util.Date().getTime()).toString();
				String[] list = now.split("[.]");
				stream.printf("%s.%s  %s", list[0], String.format("%-3s", list[1]).replace(' ', '0'), string);
			}
		}

		void println() {
			synchronized (lock) {
				stream.println();
			}
		}

		void print(String string) {
			synchronized (lock) {
				stream.print(string);
			}
		}

		void print(StackTraceElement[] ste) {
			synchronized (lock) {
				for (StackTraceElement e : ste)
					stream.println(e);
			}
		}

		void close() {
			if (stream != System.out)
				synchronized (lock) {
					stream.close();
				}
		}
	}

}
