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

package com.ibm.ima.samples.jms;

import java.io.Console;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.sql.Timestamp;
import java.util.Hashtable;
import java.util.Random;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Queue;
import javax.jms.Topic;
import javax.naming.CompositeName;
import javax.naming.Context;
import javax.naming.InitialContext;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/**
 * The purpose of this sample is to demonstrate a basic example of how to use IBM MessageSight JMS classes to publish and subscribe
 * to an IBM MessageSight topic. This sample consists of three classes: JMSSample.java, JMSSampleSend.java and
 * JMSSampleReceive.java
 * 
 * JMSSample contains methods for parsing and validating arguments, logging messages, and other utility type functions.
 * 
 * JMSSampleReceive contains methods specific to subscribing to messages published to an IBM MessageSight topic.
 * 
 * JMSSampleSend contains methods specific to publishing messages to an IBM MessageSight topic.
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
 * -i clientID The client id used for registering the client with the IBM MessageSight server.
 * 
 * -s serverURI The URI address of the IBM MessageSight server. This is in the format of tcp://<ip address>:<port> This is a required
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
 * -jx factory The JNDI context factory (e. g., com.sun.jndi.ldap.LdapCtxFactory or com.sun.jndi.fscontext.RefFSContextFactory).
 * 
 * -jcf factory The JNDI connection factory (required if -s <Server URI> is not specified).
 * 
 * -jc URL The JNDI provider URL (required if -jx <JNDI context factory> is specified).
 * 
 * -ju userid The JNDI provider userid
 * 
 * -jp password The JNDI provider password
 * 
 * -jn prefix Name prefix for creating and looking up JNDI objects
 * 
 * -a action Either the String: publish or subscribe. This is a required parameter.
 * 
 * -t topicName The name of the topic on which the messages are sent or received.
 * 
 * -q queueName The name of the queue on which the messages are sent or received.
 * 
 * -m message A String representing the message to be sent. The default message is "Sample Message"
 * 
 * -n count The number of times the specified message is to be sent or the number of messages to receive. The default number of messages sent or
 * received is 1.
 * 
 * -o logfilename The log defaults to stdout.
 * 
 * -r Enable PERSISTENT delivery mode. The default delivery mode is NON_PERSISTENT.
 * 
 * -b Enable durable subscriptions. By default, topic subscriptions are non-durable.
 * 
 * -e Delete durable subscription at client disconnect.  By default, durable subscriptions are preserved after disconnect.
 * 
 * -d Disable packet acknowledgments by setting the DisableACK connection factory property to true. The default is false.
 * 
 * -u client user id for authentication with MessageSight.
 * 
 * -p client password for authentication with MessageSight. If -u is specified without -p then the user will be prompted for the
 * password.
 * 
 * -w rate The rate at which messages are sent in units of messages/second.
 * 
 * -x timeout Specify the timeout in seconds for each asynchronous call to receive a message.
 * 
 * -v Indicates verbose output
 * 
 * -h Output usage statement
 * 
 */
public class JMSSample implements Runnable {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


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
    public int count = 1;
    public String payload = "Sample Message";
    public boolean persistence = false;
    public boolean isDurable = false;
    public boolean deleteDurableSubscription = false;
    public String scheme = null;
    public String port = null;
    public String server = null;
    public String destName = null;
    public String destType = null;
    public boolean verbose = false;
    public boolean disableACKS = false;
    public String jndi_contextFactory;
    public String jndi_userName = null;
    public String jndi_password = null;
    public String jndi_connectionFactory = null;
    public String jndi_provider = null;
    public String jndi_namePrefix = null;
    public Context jndi_context = null;
    public boolean jndi_createObjects = false;
    public long timeout = 0;
    public String keystore = null;
    public char[] keystore_password = null;
    public boolean clear_keystore_password = false;
    public String truststore = null;
    public char[] truststore_password = null;
    public boolean clear_truststore_password = false;

    /**
     * Instantiates a new JMS client.
     */
    public JMSSample(String[] args) {
        parseArgs(args);
    }

    /**
     * Parses the command line arguments passed into main().
     * 
     * @param args
     *            the command line arguments. See usage statement.
     */
    private void parseArgs(String[] args) {
        String serverURI = null;
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
            } else if ("-i".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                clientId = args[i];
            } else if ("-m".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                payload = args[i];
            } else if ("-t".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                destName = args[i];
                destType = "topic";
            } else if ("-q".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                destName = args[i];
                destType = "queue";
            } else if ("-n".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                count = Integer.parseInt(args[i]);
            } else if ("-o".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
                i++;
                setfile(args[i]);
            } else if ("-r".equals(args[i])) {
                persistence = true;
            } else if ("-b".equalsIgnoreCase(args[i])) {
                isDurable = true;
            } else if ("-e".equalsIgnoreCase(args[i])) {
                deleteDurableSubscription = true;
            } else if ("-v".equalsIgnoreCase(args[i])) {
                verbose = true;
            } else if ("-h".equalsIgnoreCase(args[i])) {
                showUsage = true;
                break;
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
            } else {
                showUsage = true;
                comment = "Invalid Parameter:  " + args[i];
                break;
            }
        }

        if (showUsage == false) {
            if ((serverURI == null) && (jndi_connectionFactory == null)) {
                comment = "Missing required parameter: Either -s <serverURI> or -jcf <connection factory> must be specified.";
                showUsage = true;
            } else if ((serverURI == null) && (jndi_connectionFactory != null) 
            		&& ((jndi_contextFactory == null) || (jndi_provider == null))) {
                comment = "Missing parameter: Either -jx <JNDI context factory> or -jc <provider URL> is missing. (Both are required when -jcf <connection factory> is specified.)";
                showUsage = true;
            } else if ((jndi_contextFactory != null) && (jndi_provider == null)) {
            	comment = "Missing parameter: -jc <provider URL> (required when -jx <JNDI context factory> is specified)";
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
            } else if (destType == null) {
                comment = "Must specify either -t <topicName> or -q <queueName>";
                showUsage = true;
            } else if ((isDurable) && ("queue".equals(destType))) {
                comment = "Invalid parameter: -b   Durable subscription is applies only to topics.";
                showUsage = true;
            } else if ((isDurable) && ("publish".equals(action))) {
                comment = "Invalid parameter: -b   Durable subscription is invalid for publisher.";
                showUsage = true;
            } else if ((isDurable) && (clientId == null)) {
                comment = "Missing parameter: -i <clientId>   ClientID must be specified for durable subscriptions";
                showUsage = true;
            } else if ((deleteDurableSubscription) && (isDurable == false)) {
                comment = "Missing parameter: -b   deleteDurableSubscription only applies to durable subscriptions";
                showUsage = true;
            } else if ((password != null) && (userName == null)) {
                comment = "Missing parameter: -u <userId>";
                showUsage = true;
            } else if (serverURI != null) {
                scheme = serverURI.substring(0, serverURI.indexOf(':'));
                if (scheme == null) {
                    comment = "Invalid formatted parameter. Unable to determine scheme: -s " + serverURI;
                    showUsage = true;
                } else {
                    port = serverURI.substring(serverURI.lastIndexOf(':') + 1);
                    if (port == null) {
                        comment = "Invalid formatted parameter. Unable to determine port number: -s " + serverURI;
                        showUsage = true;
                    } else {
                        server = serverURI.substring(serverURI.indexOf("://") + 3, serverURI.lastIndexOf(':'));
                        if (server == null) {
                            comment = "Invalid formatted parameter. Unable to determine server address: -s "
                                            + serverURI;
                            showUsage = true;
                        }
                    }
                }
            }
            if ((userName != null) && (password == null)) {
                password = readPassword("User Password:");
                if (password == null) {
                    comment = "Unable to read password from System.console nor System.in.";
                    showUsage = true;
                }
            }
            if ((keystore != null) && (keystore_password == null)) {
                keystore_password = readPassword("Keystore Password:");
                if (keystore_password == null) {
                    comment = "Unable to read keystore password from System.console nor System.in.";
                    showUsage = true;
                }
            }
            if ((truststore != null) && (truststore_password == null)) {
                truststore_password = readPassword("Truststore Password:");
                if (truststore_password == null) {
                    comment = "Unable to read truststore password from System.console nor System.in.";
                    showUsage = true;
                }
            }

            if (clientId == null) {
                Random generator = new Random(System.currentTimeMillis());
                clientId = String.format("%s_%d", action, generator.nextInt(99999));
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
                            + "the sample outside an IDE, or specifying the password as command line option.");
        }
        return password;
    }

    /**
     * Primary method for running the this sample. Creates the connection factory and connection. Then either sends or
     * receives messages, and finally closes the connection.
     * 
     */
    public void run() {
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
            // specific properties using the IBM MessageSight provider specific ImaJmsFactory class.
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

            ImaJmsException[] exceptions = ((ImaProperties) connectionFactory).validate(false);
            if (exceptions != null) {
                for (ImaJmsException e : exceptions)
                    System.out.println(e.getMessage());
                return;
            }

            if (password != null) {
                connection = connectionFactory.createConnection(userName, new String(password));

                // clear passwords after they are no longer needed.
                java.util.Arrays.fill(password, '\0');
                if (clear_keystore_password)
                    System.clearProperty("javax.net.ssl.keyStorePassword");
                if (clear_truststore_password)
                    System.clearProperty("javax.net.ssl.trustStorePassword");
            } else
                connection = connectionFactory.createConnection();

            connection.setClientID(clientId);
        } catch (Throwable e) {
            e.printStackTrace(stream);
            return;
        }

        /*
         * Depending on action executes the send or receive logic
         */
        try {
            if ("publish".equals(action)) {
                JMSSampleSend.doSend(this);
            } else { // If action is not publish then it must be subscribe
                JMSSampleReceive.doReceive(this);
            }
        } catch (Throwable e) {
            println("Exception caught in JMS sample:  " + e.getMessage());
            e.printStackTrace(stream);
        }

        /*
         * close program handles
         */
        try {
            connection.close();
            close();
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

        System.err.println("usage:  -s <Server URI>|-jcf <connection factory> -a publish|subscribe -t <topic name>|-q <queue name>"
                                        + "[-ks <keystore>][-kspw <keystore password>][-ts <truststore>][-tspw <truststore password>]"
                                        + "[-jx <context factory>][-jc <provider URL>][-ju <provider userid>][-jp <provider password>][-jn <Name prefix>]"
                                        + "[-i <clientId>][-m <payload>][-n <num messages>][-o <logfile>][-b][-r][-d][-u <userId>][-p <password>]"
                                        + "[-w <rate>][-v][-x <seconds>][-h]");
        System.err.println();

        System.err.println(" -s serverURI The URI address of the IBM MessageSight server. This is in the format of <protocol>://<ipaddress>:<port> where <protocol> is tcp or tcps. Either serverURI or JNDI connection factory must be specified.");
        System.err.println(" -ks keystore The keystore repository of security certificates used for SSL encryption.");
        System.err.println(" -kspw keystore_pw The keystore password.");
        System.err.println(" -ts truststore The truststore repository used to establish trust relationships for SSL connections.");
        System.err.println(" -tspw truststore_pw The truststore password.");
        
        System.err.println(" -jcf jndi_connectionFactory The JNDI connection factory. Either serverURI or JNDI connection factory must be specified (Examples: ldap://127.0.0.1/o=jndiTest, file:///c:/test/MyJndi.");
        System.err.println(" -jx jndi_contextFactory The JNDI context factory (Examples: com.sun.jndi.ldap.LdapCtxFactory, com.sun.jndi.fscontext.RefFSContextFactory).");
        System.err.println(" -jc jndi_providerURL The JNDI provider URL.");
        System.err.println(" -ju jndi_user The JNDI provider userid.");
        System.err.println(" -jp jndi_pw The JNDI provider password.");
        System.err.println(" -jn jndi_prefix Name prefix for creating and looking up JNDI objects.");

        System.err.println(" -a action Either the String: publish or subscribe. This is a required parameter.");
        System.err.println(" -t topicName The name of the topic on which the messages are published or subscribed. Either topic or queue must be specified. (When JNDI context factory is specified, this is the JNDI name for the topic object.)");
        System.err.println(" -q queueName The name of the queue on which the messages are published or subscribed. Either topic or queue must be specified. (When JNDI context factory is specified, this is the JNDI name for the queue object.)");
        
        System.err.println(" -i clientId  The client identity used for connection to the IBM MessageSight server.");
        System.err.println(" -m message A String representing the message to be sent. The default message is \"Sample Message\".");
        System.err.println(" -n count The number of times the specified message is to be sent or the number of messages to receive. The default number of messages sent or received is 1.");

        System.err.println(" -o logfilename The log defaults to stdout.");
        System.err.println(" -d Set DisableAck in the connection factory to true. This property is set to false by default.");
        System.err.println(" -b Enable durable subscriptions on subscriber. Topic subscriptions are non-durable by default.");
        System.err.println(" -e Delete durable subscription at client disconnect. By default, durable subscriptions are preserved after disconnect.");
        System.err.println(" -r Enable PERSISTENT delivery mode. The default delivery mode is NON_PERSISTENT.");
        System.err.println(" -u userid The user name for authentication with MessageSight.");
        System.err.println(" -p password The user password for authentication with MessageSight.  If -u is specified without -p then the user will be prompted for the password.");
        System.err.println(" -w rate The rate at which messages are sent in units of messages/second.");
        System.err.println(" -v Provide verbose output");
        System.err.println(" -x timeout The timeout in seconds for each synchronous call to receive a message.");
        System.err.println(" -h Output the usage statement.");

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
                println("Log sent to " + filename);

        } catch (Throwable e) {
            println("Log entries sent to System.out instead of " + filename);
        }
    }

    /**
     * Prints the string to the output stream for this program. * @param string The string to print to the output
     * stream.
     */
    void println(String string) {
        synchronized (this) {
            stream.println(new Timestamp(new java.util.Date().getTime()) + " " + string);
        }
    }

    /**
     * Closes the stream used for printing messages for this program.
     */
    void close() {
            if (stream != System.out)
                stream.close();
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
            if (ex instanceof RuntimeException)
            	throw (RuntimeException)ex;
            throw new RuntimeException(ex);
        }

        return jndiCtxt;
    }

    public Object retrieveFromJndi(Context jndiCtxt, String objectName) {

        Object jndiObj = null;

        try {
            jndiObj = jndiCtxt.lookup(new CompositeName().add(objectName));
        } catch (Exception ex) {
            System.err.println("*** FAILED TO FIND " + objectName + " ***");
            ex.printStackTrace();
        }

        return jndiObj;
    }
    
    /**
     * Gets the topic or queue name from a destination object.
     * 
     * @param dest
     *            The destination from which the topic or queue name is extracted.
     * 
     * @throws JMSException
     *             the jMS exception
     */
    public static String getDestName(Destination dest) throws JMSException {
        String destName = null;
        if ((dest != null) && (dest instanceof Queue)) {
            destName = ((Queue) dest).getQueueName();
        } else if ((dest != null) && (dest instanceof Topic)) {
            destName = ((Topic) dest).getTopicName();
        }
        return destName;
    }
}
