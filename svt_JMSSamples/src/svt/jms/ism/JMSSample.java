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

import java.io.Console;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.sql.Timestamp;
import java.util.Hashtable;
import java.util.Random;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.naming.Context;
import javax.naming.InitialContext;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/**
 * The purpose of this sample is to demonstrate a basic example of how to use IMA JMS classes to publish and subscribe
 * to an IMA topic. This sample consists of three classes: JMSSample.java, JMSSamplePublish.java and
 * JMSSampleSubscribe.java
 * 
 * JMSSample contains methods for parsing and validating arguments, logging messages, and other utility type functions.
 * 
 * JMSSampleSubscribe contains methods specific to subscribing to messages published to an IMA topic.
 * 
 * JMSSamplePublish contains methods specific to publishing messages to an IMA topic.
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
 * -i clientID Optional parameter specifying the client id used for registering the client with the IMA server.
 * 
 * -s serverURI The URI address of the IMA server. This is in the format of tcp://<ip address>:<port> This is a required
 * parameter.
 * 
 * -ks keystore The keystore repository of security certificates used for SSL encryption.
 * 
 * -kspw password The keystore password.
 * 
 * -ts truststore The truststore repository used to establish trust relationships for SSL connections.
 * 
 * -tspw password The truststore password.
 * 
 * -jx factory The JNDI context factory.
 * 
 * -jcf factory The JNDI connection factory.
 * 
 * -jc URL The JNDI provider URL.
 * 
 * -ju userid The JNDI provider userid
 * 
 * -jp password The JNDI provider password
 * 
 * -jn prefix Name prefix for creating and looking up JNDI objects
 * 
 * -a action Either the String: publish or subscribe. This is a required parameter.
 * 
 * -t topicName The name of the topic on which the messages are sent or received. The default topic name is /JMSSample
 * 
 * -q queueName The name of the queue on which the messages are sent or received.
 * 
 * -m message A String representing the message to be sent. The default message is Sample Message
 * 
 * -n count The number of times the specified message is to be sent or received. The default number of messages sent or
 * received is 1.
 * 
 * -o Log filename The log defaults to stdout.
 * 
 * -r Enable persistence. The default persistence is false.
 * 
 * -b Enable durable subscriptions. The default is false.
 * 
 * -e Delete durable subscription at client disconnect.
 * 
 * -d Disable packet acknowledgments. The default is false.
 * 
 * -u client user id for secure communications. Optional parameter.
 * 
 * -p client password for secure communications. If -u is specified without -p then the user will be prompted for the
 * password.
 * 
 * -w rate at which messages are sent in units of messages/second.
 * 
 * -x Specify subscriber timeout in seconds.
 * 
 * -v Indicates verbose output
 * 
 * -h Output usage statement
 * 
 */
public class JMSSample implements Runnable {

    /**
     * The main method executes the client's run method.
     * 
     * @param args
     *            Command line arguments. See usage statement.
     */
    public static void main(String[] args) {

        /*
         * Instantiate and run the client.
         */
        new JMSSample(args).run();

    }

    public String action = null;
    public Connection connection = null;
    public String clientId = null;
    public String userName = null;
    public char[] password = null;
    public int throttleWaitMSec = 0;
    public int count = 0;
    public byte[] payload = "Sample Message".getBytes();
    public boolean persistence = false;
    public boolean isDurable = false;
    public boolean deleteDurableSubscription = false;
    public String scheme = null;
    public String port = null;
    public String server = null;
    public String defaultTopicName = "/JMSSample";
    public String destName = null;
    public String destType = null;
    public boolean verbose = false;
    public boolean disableACKS = false;
    public boolean order = false;
    public String jndi_contextFactory;
    public String jndi_userName = null;
    public String jndi_password = null;
    public String jndi_connectionFactory = null;
    public String jndi_provider = null;
    public String jndi_namePrefix = null;
    public Context jndi_context = null;
    public boolean jndi_createObjects = false;
    public int size = 0;
    public String payload_type = "text";
    public long timeout = 0;
    public String keystore = null;
    public char[] keystore_password = null;
    public boolean clear_keystore_password = false;
    public String truststore = null;
    public char[] truststore_password = null;
    public boolean clear_truststore_password = false;
    public int qos = 2;
    public log log = new log(System.out);
    public String resendServer = null;
    public String resendPort = null;
    public String resendID = null;
    public String resendTopic = null;
    public String resendScheme = null;
    public boolean debug = false;
    public long subscribeTimeSec = 0;
    public String shared = null;
    // public String serverList = null;
    public long connectTimeout = 120000;
    public long timetoliveSeconds = 0;
    public int recvcount = 0;
    long startTime = System.currentTimeMillis();
    public Integer clientMessageCache = null;
    // public int shared=1;

    /**
     * Instantiates a new JMS client.
     */
    public JMSSample(String[] args) {
        parseArgs(args);
    }

    public JMSSample(JMSSample config) {
        this.action = config.action;
        this.connection = config.connection;
        this.clientId = config.clientId;
        this.userName = config.userName;
        this.password = new String(config.password).toCharArray();
        this.throttleWaitMSec = config.throttleWaitMSec;
        this.count = config.count;
        // this.shared = config.shared;
        this.payload = config.payload;
        this.persistence = config.persistence;
        this.isDurable = config.isDurable;
        this.deleteDurableSubscription = config.deleteDurableSubscription;
        this.scheme = config.scheme;
        this.port = config.port;
        this.server = config.server;
        this.defaultTopicName = config.defaultTopicName;
        this.destName = config.destName;
        this.destType = config.destType;
        this.verbose = config.verbose;
        this.disableACKS = config.disableACKS;
        this.order = config.order;
        this.jndi_contextFactory = config.jndi_contextFactory;
        this.jndi_userName = config.jndi_userName;
        this.jndi_password = config.jndi_password;
        this.jndi_connectionFactory = config.jndi_connectionFactory;
        this.jndi_provider = config.jndi_provider;
        this.jndi_namePrefix = config.jndi_namePrefix;
        this.jndi_context = config.jndi_context;
        this.jndi_createObjects = config.jndi_createObjects;
        this.size = config.size;
        this.payload_type = config.payload_type;
        this.timeout = config.timeout;
        this.keystore = config.keystore;
        this.keystore_password = config.keystore_password;
        this.clear_keystore_password = config.clear_keystore_password;
        this.truststore = config.truststore;
        this.truststore_password = config.truststore_password;
        this.clear_truststore_password = config.clear_truststore_password;
        this.qos = config.qos;
        this.subscribeTimeSec = config.subscribeTimeSec;
        this.resendServer = config.resendServer;
        this.resendID = config.resendID;
        this.resendPort = config.resendPort;
        this.resendScheme = config.resendScheme;
        this.log = config.log;
        this.debug = config.debug;
        this.shared = config.shared;
        this.timetoliveSeconds = config.timetoliveSeconds;
        this.recvcount = config.recvcount;
        this.startTime = config.startTime;
        this.clientMessageCache = config.clientMessageCache;
    }

    /**
     * Parses the command line arguments passed into main().
     * 
     * @param args
     *            the command line arguments. See usage statement.
     */
    private void parseArgs(String[] args) {
        String serverURI = null;
        String resendURI = null;
        boolean showUsage = false;
        String comment = null;

        for (int i = 0; i < args.length; i++) {
            if ("-s".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                serverURI = args[i];
            } else if ("-ks".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                keystore = args[i];
            } else if ("-kspw".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                keystore_password = args[i].toCharArray();
            } else if ("-ts".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                truststore = args[i];
            } else if ("-tspw".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                truststore_password = args[i].toCharArray();
            } else if ("-jx".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                jndi_contextFactory = args[i];
            } else if ("-jc".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                jndi_provider = args[i];
            } else if ("-jcf".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                jndi_connectionFactory = args[i];
            } else if ("-jn".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                jndi_namePrefix = args[i];
            } else if ("-ju".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                jndi_userName = args[i];
            } else if ("-jp".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                jndi_password = args[i];
            } else if ("-a".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                action = args[i];
            } else if ("-resendID".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                resendID = args[i];
                if (resendID.length() > 23) {
                    showUsage = true;
                    comment = "Invalid Parameter:  " + args[i] + ".  The resendID cannot exceed 23 characters.";
                    break;
                }
            } else if ("-i".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                clientId = args[i];
            } else if ("-m".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                payload_type = "text";
                i++;
                payload = args[i].getBytes();
            } else if ("-t".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                destName = args[i];
                destType = "topic";
            } else if ("-q".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                destName = args[i];
                destType = "queue";
            } else if ("-n".equals(args[i]) && (i + 1 < args.length)) {
                i++;
                count = Integer.parseInt(args[i]);
            } else if ("-N".equals(args[i]) && (i + 1 < args.length)) {
                i++;
                subscribeTimeSec = Long.parseLong(args[i]) * 60L;
            } else if ("-Ns".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                subscribeTimeSec = Long.parseLong(args[i]);
            } else if ("-Nm".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                subscribeTimeSec = Long.parseLong(args[i]) * 60L;
            } else if ("-o".equals(args[i]) && (i + 1 < args.length)) {
                i++;
                setfile(args[i]);
            } else if ("-O".equals(args[i])) {
                order = true;
            } else if ("-r".equals(args[i])) {
                persistence = true;
            } else if ("-b".equalsIgnoreCase(args[i])) {
                isDurable = true;
            } else if ("-shared".equals(args[i]) && (i + 1 < args.length)) {
                i++;
                shared = args[i];
            } else if ("-e".equalsIgnoreCase(args[i])) {
                deleteDurableSubscription = true;
            } else if ("-debug".equalsIgnoreCase(args[i])) {
                debug = true;
            } else if (("-z".equalsIgnoreCase(args[i]) && (i + 1 < args.length))) {
                payload_type = "bytes";
                i++;
                size = Integer.parseInt(args[i]);
                payload = new byte[size];
                for (int j = 0; j < size; j++) {
                    payload[j] = (byte) j;
                }
            } else if ("-v".equalsIgnoreCase(args[i])) {
                verbose = true;
            } else if ("-h".equalsIgnoreCase(args[i])) {
                showUsage = true;
                break;
            } else if ("-resendURI".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                resendURI = args[i];
            } else if ("-resendTopic".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                resendTopic = args[i];
            } else if ("-u".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                userName = args[i];
            } else if ("-p".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                password = args[i].toCharArray();
            } else if ("-w".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                throttleWaitMSec = Integer.parseInt(args[i]);
            } else if ("-d".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                disableACKS = Boolean.parseBoolean(args[i]);
            } else if ("-x".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                timeout = Long.parseLong(args[i]);
            } else if ("-ttl".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                timetoliveSeconds = Long.parseLong(args[i]);
            } else if ("-clientMessageCache".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                clientMessageCache = Integer.parseInt(args[i]);
            } else {
                showUsage = true;
                comment = "Invalid Parameter:  " + args[i];
                break;
            }
        }

        if (showUsage == false) {
            if ((serverURI == null) && (jndi_contextFactory == null)) {
                comment = "Missing required parameter: -s <serverURI>";
                showUsage = true;
            } else if (action == null) {
                comment = "Missing required parameter: -a subscribe|publish";
                showUsage = true;
            } else if (!("publish".equalsIgnoreCase(action)) && !("subscribe".equalsIgnoreCase(action))) {
                comment = "Invalid parameter: -a " + action;
                showUsage = true;
            } else if ((persistence) && ("subscribe".equals(action))) {
                comment = "Invalid parameter: -r   Persistence is invalid for subscriber.";
                showUsage = true;
            } else if ((timetoliveSeconds != 0) && ("subscribe".equals(action))) {
                comment = "Invalid parameter: -ttl " + timetoliveSeconds + "   timetolive is invalid for subscriber.";
                showUsage = true;
            } else if (destType == null) {
                comment = "Must specify either -t <topicName> or -q <queueName> but not both";
                showUsage = true;
            } else if ((isDurable) && ("queue".equals(destType))) {
                comment = "Invalid parameter: -b   Durable subscription is applies only to topics.";
                showUsage = true;
            } else if ((isDurable) && ("publish".equals(action))) {
                comment = "Invalid parameter: -b   Durable subscription is invalid for publisher.";
                showUsage = true;
            } else if ((isDurable) && (shared != null) && (clientId != null)) {
                comment = "Missing parameter: -i <clientId>   ClientID must NOT be specified for shared durable subscriptions";
                showUsage = true;
            } else if ((isDurable) && (shared == null) && (clientId == null)) {
                comment = "Missing parameter: -i <clientId>   ClientID must be specified for nonshared durable subscriptions";
                showUsage = true;
            } else if ((deleteDurableSubscription) && (isDurable == false)) {
                comment = "Missing parameter: -b   deleteDurableSubscription only applies to durable subscriptions";
                showUsage = true;
            } else if ((password != null) && (userName == null)) {
                comment = "Missing parameter: -u <userId>";
                showUsage = true;
            } else if ((userName != null) && (password == null)) {
                password = readPassword("User Password:");
                if (password == null) {
                    comment = "Unable to read password from System.console nor System.in.";
                    showUsage = true;
                }
            } else if ((keystore != null) && (keystore_password == null)) {
                keystore_password = readPassword("Keystore Password:");
                if (keystore_password == null) {
                    comment = "Unable to read keystore password from System.console nor System.in.";
                    showUsage = true;
                }
            } else if ((truststore != null) && (truststore_password == null)) {
                truststore_password = readPassword("Truststore Password:");
                if (truststore_password == null) {
                    comment = "Unable to read truststore password from System.console nor System.in.";
                    showUsage = true;
                }
            } else if (serverURI != null) {
                serverURI = serverURI.replace('+', ',');
                serverURI = serverURI.replace('|', ',');
                serverURI = serverURI.replace('@', ',');
                String delimiters = ",";
                String[] list = serverURI.split(delimiters);

                scheme = list[0].substring(0, list[0].indexOf(':'));
                if (scheme == null) {
                    comment = "Invalid formatted parameter. Unable to determine scheme: -s " + serverURI;
                    showUsage = true;
                } else {
                    port = list[0].substring(list[0].lastIndexOf(':') + 1);
                    if (port == null) {
                        comment = "Invalid formatted parameter. Unable to determine port number: -s " + serverURI;
                        showUsage = true;
                    } else {
                        server = list[0].substring(list[0].indexOf("://") + 3, list[0].lastIndexOf(':'));
                        if (server == null) {
                            comment = "Invalid formatted parameter. Unable to determine server address: -s "
                                            + serverURI;
                            showUsage = true;
                        } else {
                            try {
                                for (int i = 1; i < list.length; i++)
                                    server = server
                                                    + ","
                                                    + list[i].substring(list[i].indexOf("://") + 3, list[i]
                                                                    .lastIndexOf(':'));
                            } catch (Exception e) {
                                comment = "Invalid formatted parameter. Unable to determine server list: -s "
                                                + serverURI;
                                showUsage = true;
                            }
                        }
                    }
                }
            }
            if (showUsage == false) {
                if (resendURI != null) {
                    resendURI = resendURI.replace('+', ',');
                    resendURI = resendURI.replace('|', ',');
                    resendURI = resendURI.replace('@', ',');
                    String delimiters = ",";
                    String[] list = resendURI.split(delimiters);

                    resendScheme = list[0].substring(0, list[0].indexOf(':'));
                    if (scheme == null) {
                        comment = "Invalid formatted parameter. Unable to determine scheme: -resendURI " + resendURI;
                        showUsage = true;
                    } else {
                        resendPort = list[0].substring(list[0].lastIndexOf(':') + 1);
                        if (resendPort == null) {
                            comment = "Invalid formatted parameter. Unable to determine port number: -resendURI "
                                            + resendURI;
                            showUsage = true;
                        } else {
                            resendServer = list[0].substring(list[0].indexOf("://") + 3, list[0].lastIndexOf(':'));
                            if (resendServer == null) {
                                comment = "Invalid formatted parameter. Unable to determine server address: -resendURI "
                                                + resendURI;
                                showUsage = true;
                            } else {
                                try {
                                    for (int i = 1; i < list.length; i++)
                                        resendServer = resendServer
                                                        + ","
                                                        + list[i].substring(list[i].indexOf("://") + 3, list[i]
                                                                        .lastIndexOf(':'));
                                } catch (Exception e) {
                                    comment = "Invalid formatted parameter. Unable to determine server list: -resendURI "
                                                    + resendURI;
                                    showUsage = true;
                                }
                            }
                        }
                    }
                } else if ((resendID != null) && (resendURI == null)) {
                    comment = "Missing parameter: -resendURI <resendURI>";
                    showUsage = true;
                }
                if ((shared == null) && (clientId == null)) {
                    Random generator = new Random(System.currentTimeMillis());
                    clientId = String.format("%s_%d", action, generator.nextInt(99999));
                }
            }
        }

        if (showUsage) {
            usage(args, comment);
            System.exit(1);
        }
    }

    char[] readPassword(String prompt) {
        char[] password = null;
        Console console = System.console();
        if (console != null) {
            password = console.readPassword("[%s]", prompt);
        } else {
            System.out.println("Unable to access System.console necessary to prompt for the required password.  "
                            + "Try running with a version of Java that supports console access, running "
                            + "the sample outside an IDE, or specifying the password as commandline option.");
        }
        return password;
    }

    /**
     * Primary method for running the this sample. Creates the connection factory and connection. Then either sends or
     * receives messages, and finally closes the connection.
     * 
     */
    public void run() {
        boolean connected = doConnect();
        /*
         * Depending on action executes the send or receive logic
         */
        if (connected) {
            try {
                if ("publish".equals(action)) {
                    new JMSSampleSend(this).topDoSendLoop();
                } else { // If action is not publish then it must be subscribe
                    new JMSSampleReceive().doHAReceive(this);
                }
            } catch (Throwable e) {
                log.println("Exception caught in JMS sample:  " + e.getMessage());
                e.printStackTrace(stream);
            }

            disconnect();
        }
    }

    public boolean connect() {
        try {

            if (keystore != null)
                System.setProperty("javax.net.ssl.keyStore", keystore);
            if (keystore_password != null) {
                System.setProperty("javax.net.ssl.keyStorePassword", new String(keystore_password));
                java.util.Arrays.fill(keystore_password, '\0');
                clear_keystore_password = true;
            }
            if (truststore != null)
                System.setProperty("javax.net.ssl.trustStore", truststore);
            if (truststore_password != null) {
                System.setProperty("javax.net.ssl.trustStorePassword", new String(truststore_password));
                java.util.Arrays.fill(truststore_password, '\0');
                clear_truststore_password = true;
            }
            // The following section of code loads the connection factory from a JNDI repository if specified
            // in the command line arguments. Otherwise the code creates a connection factory and sets
            // specific properties using the IMA provider specific ImaJmsFactory class.
            // Standard JMS code should instead load the connection factory from a JNDI repository.

            if (jndi_contextFactory != null) {
                jndi_context = getJndiContext(jndi_provider, jndi_userName, jndi_password);
            }

            ConnectionFactory connectionFactory = null;
            if ((jndi_context != null) && (jndi_connectionFactory != null)) {
                connectionFactory = (ConnectionFactory) retrieveFromJndi(jndi_context, jndi_connectionFactory);
            } else {
                connectionFactory = ImaJmsFactory.createConnectionFactory();
            }

            if (server != null)
                ((ImaProperties) connectionFactory).put("Server", server);
            if (port != null)
                ((ImaProperties) connectionFactory).put("Port", port);

            if ("tcps".equals(scheme))
                ((ImaProperties) connectionFactory).put("Protocol", "tcps");

            ((ImaProperties) connectionFactory).put("DisableACK", (disableACKS == false) ? "false" : "true");

            if (clientMessageCache != null)
                ((ImaProperties) connectionFactory).put("ClientMessageCache", clientMessageCache);
            
            ImaJmsException[] exceptions = ((ImaProperties) connectionFactory).validate(false);
            if (exceptions != null) {
                for (ImaJmsException e : exceptions)
                    System.out.println(e.getMessage());
                return false;
            }

            if (password != null) {
                try {
                    connection = connectionFactory.createConnection(userName, new String(password));
                } catch (Exception e) {
                    if (debug) {
                        log.printErr(e.getMessage());
                        if (e.getCause() != null)
                            log.printErr(e.getCause().getMessage());
                        e.printStackTrace();
                    }
                }
                // clear passwords after they are no longer needed.
                // java.util.Arrays.fill(password, '\0');
                if (clear_keystore_password)
                    System.setProperty("javax.net.ssl.keyStorePassword", null);
                if (clear_truststore_password)
                    System.setProperty("javax.net.ssl.trustStorePassword", null);
            } else
                try {
                    connection = connectionFactory.createConnection();
                } catch (Exception e) {
                    if (debug) {
                        log.printErr(e.getMessage());
                        if (e.getCause() != null)
                            log.printErr(e.getCause().getMessage());
                        e.printStackTrace();
                    }
                }

            if ((clientId != null) && (connection != null))
                try {
                    connection.setClientID(clientId);
                } catch (Exception e) {
                    if (debug) {
                        log.printErr(e.getMessage());
                        if (e.getCause() != null)
                            log.printErr(e.getCause().getMessage());
                        e.printStackTrace();
                    }
                }
        } catch (Throwable e) {
            e.printStackTrace(stream);
            return false;
        }
        return !isConnClosed();
    }

    public void disconnect() {
        /*
         * close program handles
         */
        try {
            if (connection != null)
                connection.close();
            log.close();
        } catch (Throwable e) {
            e.printStackTrace();
        }

    }

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
                        .println("usage:  -s <Server URI> -a publish|subscribe [-i <clientId>][-jx <context factory>][-jcf <connection factory>]"
                                        + "[-ks <keystore>][-kspw <keystore password>][-ts <truststore>][-tspw <truststore password>]"
                                        + "[-jc <provider URL>][-ju <provider userid>][-jp <provider password>][-jn <Name prefix>][-t <topic name>]"
                                        + "[-q <queue name>][-m <payload>][-n <num messages>][-o <logfile>][-b][-r][-d][-u <userId>][-p <password>]"
                                        + "[-w <rate>][-v][-x <seconds>][-h]");
        System.err.println();
        System.err
                        .println(" -i clientId  The client identity used for connection to the IMA server.  This is an optional parameter.");
        System.err
                        .println(" -s serverURI The URI address of the IMA server. This is in the format of tcp://<ipaddress>:<port>. This is a required parameter.");
        System.err
                        .println(" -resendURI serverURI The URI address of the ISM server to resend messages. This is in the format of tcp://<ipaddress>:<port>.");
        System.err
                        .println(" -resendID clientId  The client identity used for the resend connection to the ISM server.  The maximum length is 23 characters.");
        System.err.println(" -ks keystore repository of security certificates used for SSL encryption.");
        System.err.println(" -kspw The keystore password.");
        System.err.println(" -ts truststore repository used to establish trust relationships for SSL connections.");
        System.err.println(" -tspw The truststore password.");
        System.err.println(" -jx The JNDI context factory.");
        System.err.println(" -jcf The JNDI connection factory.");
        System.err.println(" -jc The JNDI provider URL.");
        System.err.println(" -ju The JNDI provider userid");
        System.err.println(" -jp The JNDI provider password");
        System.err.println(" -jn Name prefix for creating and looking up JNDI objects");

        System.err.println(" -a action Either the String: publish or subscribe. This is a required parameter.");

        System.err
                        .println(" -t topicName The name of the topic on which the messages are published or subscribed. The default topic name is /JMSSample");
        System.err.println(" -q queueName The name of the queue on which the messages are published or subscribed. ");

        System.err
                        .println(" -m message A String representing the message to be sent. The default message is Sample Message");

        System.err
                        .println(" -n count The number of times the specified message is to be sent or received. The default number of messages sent or received is 1.");

        System.err.println(" -o Log filename The log defaults to stdout.");
        System.err.println(" -d Set DisableAck in connection factory.");
        System.err.println(" -b enable durbable subscriptions on subscriber. The default is false.");
        System.err.println(" -e Delete durable subscription at client disconnect.");
        System.err.println(" -r enable persistence and specify datastore directory. The default persistence is false.");
        System.err.println(" -u userid for secure communications. This is an optional parameter.");
        System.err
                        .println(" -p password for secure communications.  If -u is specified without -p then the user will be prompted for the password.");
        System.err.println(" -w rate at which messages are sent in units of messages/second.");
        System.err.println(" -gather ordercheck of shared subscription");
        System.err.println(" -debug Indicates debug output");
        System.err.println(" -ttl TimeToLive for published messages in seconds");
        System.err.println(" -v Indicates verbose output");
        System.err.println(" -x timeout for subscriber in seconds");
        System.err.println(" -z size of BytesMessage body in bytes for publisher");
        System.err.println(" -clientMessageCache number of messages in jms client message cache");
        
        System.err.println(" -h output usage statement.");

    }

    /*
     * Stream for printing messages from this sample.
     */
    private PrintStream stream = System.out;

    /**
     * Sets the file name used for the stream that prints messages from this sample.
     * 
     * @param filename
     *            The name of the file to receive output from this stream.
     */
    void setfile(String filename) {
        try {
            FileOutputStream fos = new FileOutputStream(filename);
            stream = new PrintStream(fos);
            if (verbose)
                log.println("Log sent to " + filename);

        } catch (Throwable e) {
            log.println("Log entries sent to System.out instead of " + filename);
        }
    }

    public class log {
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

        void close() {
            if (stream != System.out)
                synchronized (lock) {
                    stream.close();
                }
        }
    }

    public Context getJndiContext(String provider, String userName, String password) {
        Context jndiCtxt = null;

        Hashtable<String, Object> env = new Hashtable<String, Object>();
        env.put(Context.INITIAL_CONTEXT_FACTORY, jndi_contextFactory);
        env.put(Context.PROVIDER_URL, provider);
        if (userName != null)
            env.put(Context.SECURITY_PRINCIPAL, userName);
        if (password != null)
            env.put(Context.SECURITY_CREDENTIALS, password);

        try {
            jndiCtxt = new InitialContext(env);
        } catch (Exception ex) {
            System.err.println(ex.getMessage());
            if (ex.getCause() != null) {
                System.err.println(ex.getCause().getMessage());
                if (ex.getCause().getCause() != null)
                    System.err.println(ex.getCause().getCause().getMessage());
            }
            ex.printStackTrace();
        }

        return jndiCtxt;
    }

    public Object retrieveFromJndi(Context jndiCtxt, String objectName) {

        Object jndiObj = null;

        try {
            javax.naming.CompositeName cn = new javax.naming.CompositeName();
            jndiObj = jndiCtxt.lookup(cn.add(objectName));
        } catch (Exception ex) {
            System.err.println("*** FAILED TO FIND " + objectName + " ***");
            ex.printStackTrace();
        }

        return jndiObj;
    }

    public boolean isConnClosed() {
        if (connection != null) {
            /*
             * Check the IBM MessageSight JMS isClosed connection property to determine whether the connection state is
             * closed. This mechanism for checking whether the connection is closed is a provider-specific feature of
             * the IBM MessageSight JMS client. So check first that the IBM MessageSight JMS client is being used.
             */
            if (connection instanceof ImaProperties) {
                return ((ImaProperties) connection).getBoolean("isClosed", false);
            } else {
                /*
                 * We cannot determine whether the connection is closed so return false
                 */
                return false;
            }
        } else {
            return true;
        }
    }

    public boolean doConnect() {
        int connattempts = 1;
        boolean connected = false;
        long starttime = System.currentTimeMillis();
        long currenttime = starttime;

        /*
         * Try for up to connectTimeout milliseconds to connect.
         */
        // System.out.println("Attempting to connect to server (attempt " + connattempts + ").");
        connected = connect();
        connattempts++;
        currenttime = System.currentTimeMillis();

        while (!connected && ((currenttime - starttime) < connectTimeout)) {
            try {
                Thread.sleep(5000);
            } catch (InterruptedException iex) {
            }
            System.out.println("Attempting to connect to server (attempt " + connattempts + ").");
            connected = connect();
            connattempts++;
            currenttime = System.currentTimeMillis();
        }
        return connected;
    }
}
