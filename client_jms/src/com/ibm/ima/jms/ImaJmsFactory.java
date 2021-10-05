/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.jms;

import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.Queue;
import javax.jms.QueueConnectionFactory;
import javax.jms.Topic;
import javax.jms.TopicConnectionFactory;

import com.ibm.ima.jms.impl.ImaConnectionFactory;
import com.ibm.ima.jms.impl.ImaQueue;
import com.ibm.ima.jms.impl.ImaTopic;

/**
 * Constructs IBM MessageSight JMS client administered objects.
 * 
 * <h3>Table of contents</h3>
 * <ul>
 * <li><a href="#method_summary">Method summary</a></li>
 * <li><a href="#ConnectionFactory">ConnectionFactory properties</a></li>
 * <li><a href="#DestinationProperties" >Queue and Topic properties</a></li>
 * <li><a href="#OtherProperties" >Read only properties for other objects</a></li>
 * </ul> 
 * See <a href="http://download.oracle.com/javaee/6/api/javax/jms/package-summary.html" >
 * The JMS API documentation</a> for information about the JMS classes and methods.
 * <p>
 * <h2>Introduction</h2>
 * Normally when using JMS, the ConnectionFactory and Destination objects are looked up in a JNDI repository.
 * This class allows these objects to be constructed so that they can be stored in a JNDI repository.
 * <p>
 * There are four steps used to create an IBM MessageSight JMS client administered object:
 * <ul>
 * <li>Create the administered object using create methods in the ImaJmsFactory class.</li>
 * <li>Set the properties in the administered object by casting it to ImaProperties and using
 * put methods in the ImaProperties interface.</li>
 * <li>Validate the properties by using validate methods in the ImaProperties interface.</li>
 * <li>Store the object in the JNDI repository using the bind or rebind methods.</li>
 * </ul>
 * <p>
 * The following sample shows some simplified logic to create a ConnectionFactory
 * and bind it into a JNDI context.  
 * <pre>
 *   <code>
 *     // Create a connection factory
 *     ConnectionFactory cf = ImaJmsFactory.createConnectionFactory();
 *     // Set the desired properties
 *     ImaProperties props = (ImaProperties)cf;
 *     props.put("Port", "1883");
 *     props.put("Server", "server1.company.com");
 *     // Validate the properties.  A null return means that the properties are valid.
 *     ImaJmsException [] errstr = props.validate(ImaProperties.WARNINGS);
 *     if (errstr == null) {
 *         // Open the initial context
 *         InitialContext ctx;
 *         Hashtable env = new Hashtable();
 *         env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");
 *         env.put(Context.PROVIDER_URL, "ldap://server2.company.com/o=jndiTest");
 *         try {
 *             ctx = new InitialContext(env);
 *             // Bind the connection factory to a name
 *             ctx.rebind("ConnName", cf);
 *         } catch (Exception e) {
 *             System.out.println("Unable to open an initial context: " + e);
 *         }
 *     } else {
 *         // Display the validation errors
 *         for (int i=0; i&lt;errstr.length; i++)
 *             System.out.println(""+errstr[i]);
 *     }
 *  </code>
 * </pre>
 * <p>
 * Similar logic can be used to create IBM MessageSight JMS client objects without storing the
 * object in JNDI.
 * <p>
 * In the IBM MessageSight JMS client there are three administered objects:
 * <dl>
 * <dt>ConnectionFactory</dt><dd>The ConnectionFactory implementation in the IBM MessageSight JMS client implements the
 * three connection factory interfaces: ConnectionFactory, TopicConnectionFactory, and QueueConnectionFactory.</dd>
 * <dt>Queue</dt><dd>The Queue implementation in the IBM MessageSight JMS client implements two destination 
 * interfaces: Destination and Queue.</dd>
 * <dt>Topic</dt><dd>The Topic implementation in the IBM MessageSight JMS client implements two destination 
 * interfaces: Destination and Topic.</dd>
 * </dl>
 * <p>
 * <a name="ConnectionFactory" />
 * <h2>ConnectionFactory properties</h2>
 * The properties for ConnectionFactory are listed in the following table. 
 * These properties are used to create the IBM MessageSight JMS client Connection object.
 * <p>
 * When a Connection is created, these properties are copied to the Connection object and are
 * available as read only properties in the Connection.  When a Session is created from the Connection,
 * the properties of the Connection are copied into the Session and are available as read only properties.
 * The properties within the Connection and Session represent the current state of those objects.
 * <p>
 * For consumer specific property values, the UserData property is defined on the ConnectionFactory interface.
 * Because it is intended for use by API consumers, this property value can be set at any time on the
 * ConnectionFactory and on Connection and Session objects derived from it.
 * Additional user properties can be defined for the ConnectionFactory using the ImaProperties.addValidProperty() method.  
 * In order for a custom property to be writable on Connection and Session objects, the property 
 * name must contain the string "user".  Otherwise, the property will be read only for the these objects. 
 * <p>
 * The properties which almost always need to be set are the Server and Port which define
 * the location of the server.  To use secure communications you will need to set the Protocol to "tcps".  
 * 
 * <a name="ConnectionFactory_common" />
 * <h3>Properties for a connection factory</h3>
 * <table border=1 cellspacing=0 cellpadding=3>
 * <thead bgcolor=#b0d0FF>
 * <tr><th>Name              </th><th>Type    </th><th>Description </th> </tr>
 * </thead>
 * <tr><td>AsyncTransactionSend</td><td>Boolean</td><td>All persistent messages are sent synchronously to the server and non-persistent 
 *                     messages are normally sent asynchronously.  This means that for non-persistent messages you will not get the exception for 
 *                     any send problem on the send but it will be raised later.  If AsyncTransactionSend has the default value of false,
 *                     non-persistent messages within a transaction are sent synchronously so that you can see if there is a problem before
 *                     doing a commit.  If AsyncTransactionSend is set to true, non-persistent messages within a transaction are sent
 *                     asynchronously, and if such a send fails, a commit will fail.
 * <tr><td>ClientID          </td><td>Name    </td><td> The clientID for this connection.  Although in JMS the client ID is optional,
 *                     IBM MessageSight requires that every connection have a unique clientID.  
 *                     This can be set in the ConnectionFactory properties (in JNDI). 
 *                     It can also be set using the ClientID environment variable, or by the application
 *                     after the Connection is created and before the first Session is created.  
 *                     If the clientID is not set (either by setting the ClientID property on the ConnectionFactory object or by calling setClientID()
 *                     on the connection object) before the first session is created or before the start() method is called on the connection object,
 *                     the IBM MessageSight JMS client will check to see whether the ClientID environment variable is set and will use that value if
 *                     it is found.  
 *                     <p>If the clientID is not otherwise set, the IBM MessageSight JMS client will automatically 
 *                     create a clientID.  An automatically generated clientID cannot be used to create non-shared 
 *                     durable subscriptions, and shared subscriptions are global.  
 *                     <p>The IBM MessageSight JMS client allows any valid Unicode string to be used as a clientID, 
 *                     but we recommend that only displayable characters be used.
 *                     System generated client IDs start with an underscore (_) and users should avoid
 *                     using this as an initial character to avoid conflicts. ClientIDs starting with two underscores are
 *                     reserved for system use.</td></tr>
 * <tr><td>ClientMessageCache</td><td>UInt</td><td>The default maximum number of messages which should be cached at the client
 *                     for each consumer created within connections created by this connection factory.
 *                     When not set, the default value is 1000 for unshared topics.  This value is always 0 for queues and for shared
 *                     subscriptions.  Setting a larger value increases performance.  This value is used by the messaging
 *                     server as a hint but might not be honored exactly.  When set, this value only applies to non-shared topics.
 *                     The default value of 0 is enforced for queues and for shared subscriptions.  In order to set a larger value for queues and
 *                     for shared subscriptions, you can set the ClientMessageCache property on the destination.</td></tr>                  
 * <tr><td>ConvertMessageType</td><td>Enum automatic, bytes, text</td><td> The action to take when converting 
 *                     a message to JMS from a source which does not indicate the message type.  If the message comes from JMS this
 *                     is not used.  
 *                     The values can be: automatic, bytes, or text.  If this property is not set, the default value is automatic.
 *                     <p>
 *                     If <code>automatic</code> is specified, the JMS client will create the message type by looking at the content
 *                     of the message.  When using automatic conversion, you should use <code>instanceof</code> to check the class of the message. 
 *                     Messages coming from MQTT will be converted to either BytesMessage or TextMessage.
 *                     Any messages containing control characters or invalid character sequences are converted to BytesMessages.
 *                     <p>   
 *                     If <code>bytes</code> is specified, the message body is converted into a BytesMessage.  If the message 
 *                     contains only a text string, you must get the entire content as a byte array and convert it
 *                     to a String using the String constructor with the UTF-8 character set.
 *                     <p>
 *                     If <code>text</code> is specified, the message body is converted into a TextMessage.  If any invalid 
 *                     character sequences are found the text cannot be received.</td></tr> 
 * <tr><td>DefaultTimeToLive </td><td>UInt    </td><td> The default JMS time to live in seconds for producers created from this
 *                     connection.  The default value of zero indicates that messages do not expire.</td> </tr>
 * <tr><td>DisableACK</td><td>Boolean </td><td>If true all ACK processing is disabled for this connection unless it is overridden
 *                     by the DisableACK setting of the destination.  Disable of ACK is the fastest method of transmitting 
 *                     messages and provides a quality of service below NON_PERSISTENT.  If this property is not set, the default value is false.</td> </tr>
 * <tr><td>DisableMessageID  </td><td>Boolean </td><td>Set the default for whether JMS computes a message ID and puts it in the
 *                     message.  The default is false but can be overridden when the JMS MessageProducer
 *                     is created.  Creating the message ID takes time and increases the size of a message. 
 *                     </td> </tr>
 * <tr><td>DisableMessageTimestamp</td><td>Boolean</td><td>Set the default for whether JMS puts a timestamp in the message.  
 *                    The default is false but can be overridden when the JMS MessageProducer is created.  Putting
 *                    the timestamp in the message takes time and increases the size of the message.
 *                    If the timestamp is not stored in the message, messages will not ever expire.</td> </tr>
 * <tr><td>DisableTimeout</td><td>Boolean</td><td>Disable the detection of a silent disconnect.  Normally when one side of a 
 *                    connection disconnects, the other side is notified and closes its resources.  In some cases this notification
 *                    does not happen.  In order to detect this condition, the JMS client periodically contacts the server to make
 *                    sure it is still active and closes if it is not.  The default setting of false allows this timeout.
 *                    If set to true, the timeout is disabled in both directions.  This should be used with care as it can lead
 *                    to resources not being freed for long periods of time.  You may need to use this option if you are debugging
 *                    a client application using breakpoints, or sending or receiving very large messages over slow networks.</td></tr>
 *                     
 * <tr><td>Port</td><td>Int 1:65535</td><td>The port number of the server to communicate with.  If there are multiple servers
 *                    in the list, they all use the same port.</td></tr>
 * <tr><td>Protocol</td><td>Enum tcp, tcps</td><td>The transmit protocol.  This specifies the transmit protocol to use.
 *                    This can be <code>tcp</code> for a non-secure connection, or <code>tcps</code> for a secure
 *                    connection.  If secure connection is used, you must set up the key store for the JVM on which the 
 *                    IBM MessageSight JMS client is running.</td></tr>
 * <tr><td>RecvSockBuffer</td><td>Unit</td><td>The TCP receive socket buffer size.  Setting this larger allows for faster message
 *                    receive by using more memory.  The default is based on the platform TCP buffer size.  The platform also
 *                    has a maximum TCP buffer size, and setting it above this size does not work.  The value can have
 *                    a K or M suffix to indicate the units (kilobytes, or megabytes).</td></tr>
 * <tr><td>Server</td><td>String</td><td>A space or comma delimited list with names or IP addresses of servers to communicate with.  
 *                    The client will attempt to connect to each server until a connection is successfully established.  The name or
 *                    IP address for the successful connection is stored as the Server property in the Connection object.</td></tr>
 * <tr><td>TemporaryQueue</td><td>String</td><td>Specify the prototype name for a temporary queue name.
 *                    A unique identifier is appended to this name to form the actual temporary queue name.  Using this facility
 *                    allows policies to be assigned to the temporary queue by placing it into a known place in
 *                    the queue name space.  If the prototype contains the character sequence $CLIENTID it will be replaced by the
 *                    clientID of the connection.  The default value is "_TQ/$CLIENTID/".</td></tr>
 * <tr><td>TemporaryTopic</td><td>String</td><td>Specify the prototype name for a temporary topic name.
 *                    A unique identifier is appended to this name to form the actual temporary topic name.  Using this facility
 *                    allows policies to be assigned to the temporary topic by placing it into a known place in
 *                    the topic tree.  If the prototype contains the character sequence $CLIENTID it will be replaced by the
 *                    client ID of the connection.  The default value is "_TT/$CLIENTID/".</td></tr>
 * <tr><td>UserData   </td><td>Object</td><td>User data associated with the Connection.
 *                    This property is not used by the IBM MessageSight JMS client and can be set on the Connection.
 *                    This allows data to be kept with the object.</td></tr>
 * </table>
 * <p>
 * <a name="DestinationProperties" />
 * <h2>Queue and Topic properties</h2>
 * The properties for Queue and Topic are listed in the following table.  These destination properties are used when creating an
 * IBM MessageSight JMS client MessageConsumer or MessageProducer.  
 * <p>
 * The only property which is normally set for a Destination is the Name.  Because there are so few properties 
 * associated with a destination in the IBM MessageSight JMS client, it is common to just use the Session.createTopic() method or the
 * Session.createQueue() method to create the destination by name.  Some properties such as DisableACK and ClientMessageCache can be 
 * set on a Destination to override the value set on the ConnectionFactory.
 * <p>
 * When a MessageProducer or MessageConsumer is created, its effective properties are created from
 * the Destination properties and are available as read only properties.
 * <p>
 * For consumer specific property values, the UserData property is defined on the Destination interface.
 * Because it is intended for use by API consumers, this property value can be set at any time on the
 * Destination and on MessageConsumer and MessageProducer objects derived from it.
 * Additional user properties can be defined for the Destination using the ImaProperties.addValidProperty() method.  
 * In order for a custom property to be writable on MessageConsumer and MessageProducer objects, 
 * the property name must contain the string "user".  Otherwise, the property will be read only for these objects. 
 * <p>
 * <table border=1 cellspacing=0 cellpadding=3>
 * <thead bgcolor=#b0d0FF >
 * <tr><th>Name              </th><th>Type    </th><th>Description </th> </tr>
 * </thead>
 * <tr><td>Name      </td><td>String  </td><td>The JMS name of the Queue or Topic.  This is the
 *                   queueName or topicName known to JMS.  This property is required but is normally set when creating the Topic or Queue.  
 *                   </td></tr>
 * <tr><td>ClientMessageCache</td><td>UInt</td><td>The maximum number of messages which should be cached at the client
 *                   for consumers created for this destination.    
 *                   The value is used by the messaging server as a hint of the performance tradeoff but might not be honored 
 *                   exactly.  Using a larger value increases performance but decreases fairness when there are 
 *                   multiple consumers of a queue or a shared subscription.
 *                   <p>This value overrides the ClientMessageCache property set on the ConnectionFactory when using non-shared topics.
 *                   When using a queue or shared topic, the default value is zero.</td></tr>  
 * <tr><td>DisableACK</td><td>Boolean </td><td>If true ACK processing is disabled for this destination.  This setting overrides
 *                   the DisableACK setting of the connection.  Disable of ACK is the fastest method of transmitting 
 *                   messages and provides a quality of service below NON_PERSISTENT.  
 *                   If this property is not set, the default value is false.</td> </tr>
 * <tr><td>UserData      </td><td>Object</td><td>User data associated with the Destination, MessageConsumer, or MessageProducer.
 *                   This property is not used by the IBM MessageSight JMS client and can be set on the MessageConsumer or MessageProducer.</td></tr>
 * </table>
 * 
 * <a name="OtherProperties" />
 * <h2>Read only properties for other objects</h2>
 *  
 * The Connection, Session, MesssgeConsumer, and MessageProducer objects within the IBM MessageSight JMS client contain read only properties
 * that are derived from the objects used to create them.  
 * 
 * <a name="ConnectionProps" />
 * <h3>Connection properties</h3>
 * In addition to read only properties derived from the <a href="#ConnectionFactory">ConnectionFactory properties</a>, the following read only properties 
 * may exist for Connection objects.
 * <p>
 * <table border=1 cellspacing=0 cellpadding=3>
 * <thead bgcolor=#b0d0FF>
 * <tr><th>Name              </th><th>Type    </th><th>Description </th> </tr>
 * </thead>
 * <tr><td>isClosed       </td><td>Boolean    </td><td> Whether the connection to the server is closed.</td></tr>
 * <tr><td>isStopped      </td><td>Boolean    </td><td> Whether the connection to the server is stopped.</td></tr>
 * <tr><td>ObjectType     </td><td>Enum common, queue, topic    </td><td> Indicates the type of messaging supported by the Connection object; topic, queue or common 
 *                        (where common indicates the object can be used for either topic or queue messaging).</td></tr>
 * <tr><td>userid         </td><td>String    </td><td> The user name specified when the connection was created.</td></tr>
 * 
 * </table>
 * 
 * <a name="SessionProps" />
 * <h3>Session properties</h3>
 * In addition to read only properties derived from the <a href="#ConnectionProps">Connection properties</a>, the following read only properties 
 * may exist for Session objects.
 * <p>
 * <table border=1 cellspacing=0 cellpadding=3>
 * <thead bgcolor=#b0d0FF >
 * <tr><th>Name              </th><th>Type    </th><th>Description </th> </tr>
 * </thead>
 * <tr><td>Connection     </td><td>Object    </td><td> The Connection object used to create the Session.</td></tr>
 * <tr><td>isClosed       </td><td>Boolean    </td><td> Whether the Session is closed.</td></tr>
 * <tr><td>ObjectType     </td><td>Enum common, queue, topic    </td><td> Indicates the type of messaging supported by the Session object; topic, queue or common 
 *                        (where common indicates the object can be used for either topic or queue messaging).</td></tr>
 * </table>
 * 
 * <a name="MsgConsProps" />
 * <h3>Message consumer properties</h3>
 * In addition to read only properties derived from the <a href="#DestinationProperties">Destination properties</a>, the following read only properties 
 * may exist for MessageConsumer objects.  These properties represent the actual values and might differ from the properties of the source objects. 
 * <p>
 * <table border=1 cellspacing=0 cellpadding=3>
 * <thead bgcolor=#b0d0FF >
 * <tr><th>Name              </th><th>Type    </th><th>Description </th> </tr>
 * </thead>
 * <tr><td>Connection        </td><td>Object    </td><td> The Connection object used to create the Session that created the consumer.</td></tr>
 * <tr><td>subscriptionName  </td><td>String    </td><td> The subscription name if the consumer is over a durable or shared subscription.</td></tr>
 * <tr><td>isClosed          </td><td>Boolean   </td><td> Whether the MessageConsumer is closed.</td></tr>
 * <tr><td>isDurable         </td><td>Boolean   </td><td> Whether the MessageConsumer is a durable subscriber.</td></tr>
 * <tr><td>noLocal           </td><td>Boolean   </td><td> Whether a topic subscriber is prohibited from receiving messages published on 
 *                           the same connection.</td></tr>
 *                        
 * <tr><td>ObjectType        </td><td>Enum queue, topic    </td><td> Indicates the type of messaging supported by the MessageConsumer object; topic or queue.</td></tr>
 * <tr><td>Session           </td><td>Object    </td><td> The Session object used to create the consumer.</td></tr>
 * <tr><td>SubscriptionShared </td><td>Enum NonShared, Shared, NonDurable</td><td>Specifies the type of 
 *                        shared subscription.</td></tr>
 * </table>
 * 
 * <a name="MsgProdProps" />
 * <h3>Message producer properties</h3>
 * In addition to read only properties derived from the <a href="#DestinationProperties">Destination properties</a>, the following read only properties 
 * may exist for MessageProducer objects.
 * <p>
 * <table border=1 cellspacing=0 cellpadding=3>
 * <thead bgcolor=#b0d0FF >
 * <tr><th>Name              </th><th>Type    </th><th>Description </th> </tr>
 * </thead>
 * <tr><td>Connection     </td><td>Object    </td><td> The Connection object used to create the Session that created the producer.</td></tr>
 * <tr><td>isClosed       </td><td>Boolean   </td><td> Whether the MessageProducer is closed.</td></tr>
 * <tr><td>ObjectType     </td><td>Enum queue, topic    </td><td> Indicates the type of messaging supported by the MessageProducer object; topic or queue.</td></tr>
 * <tr><td>Session       </td><td>Object    </td><td> The Session object used to create the producer.</td></tr>
 * </table>
 */
public final class ImaJmsFactory {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /*
     * This class is never instantiated.
     */
    private ImaJmsFactory() {}

    /**
     * Creates an IBM MessageSight JMS client connection factory.
     * <p>
     * The ConnectionFactory object within the IBM MessageSight JMS client allows you to create
     * a Connection.  In order to do so you must first set properties in the
     * Connection factory object.  This can be done by casting the ConnectionFactory
     * to the ImaProperties interface and setting the properties.
     * @return A ConnectionFactory which can also be cast to ImaProperties.
     * @throws JMSException If the connection factory cannot be created
     */
    public static ConnectionFactory createConnectionFactory() throws JMSException {
        ConnectionFactory ret = (ConnectionFactory)new ImaConnectionFactory();
        ((ImaProperties)ret).put("ObjectType", "common");
        return ret;
    }
    /**
     * Creates an IBM MessageSight JMS client connection factory.
     * <p>
     * The ConnectionFactory object within the IBM MessageSight JMS client allows you to create
     * a Connection.  This method allows you to set the most common connection factory properties.
     * <p>
     * If one of the values is null (or zero for port) the default is used.
     * @param protocol - The protocol - tcp for non-secure or tcps for secure.
     * @param server - A space and/or semicolon separated list of server addresses or resolvable names.
     * @param port - The port to connect to
     * @param clientID - The client identifier for this connection
     * @return A ConnectionFactory which can also be cast to ImaProperties or used to create a Connection.
     * @throws JMSException if the connection factory cannot be created
     */
    public static ConnectionFactory createConnectionFactory(String protocol, String server, int port, String clientID) 
        throws JMSException {
        ConnectionFactory ret = (ConnectionFactory)new ImaConnectionFactory();
        ((ImaProperties)ret).put("ObjectType", "common");
        if (protocol != null)
            ((ImaProperties)ret).put("Protocol", protocol);
        if (server != null)
            ((ImaProperties)ret).put("Server", server);
        if (port != 0)
            ((ImaProperties)ret).put("Port", port);
        if (clientID != null)
            ((ImaProperties)ret).put("ClientID", clientID);
        return ret;
    }

    
    /**
     * Creates an IBM MessageSight JMS client topic connection factory.
     * <p>
     * @return A TopicConnectionFactory which can also be cast to ImaProperties.
     * @throws JMSException If the topic connection factory cannot be created
     */
    public static TopicConnectionFactory createTopicConnectionFactory() throws JMSException {
        TopicConnectionFactory ret = (TopicConnectionFactory)new ImaConnectionFactory();
        ((ImaProperties)ret).put("ObjectType", "topic");
        return ret;
    }

    
    /**
     * Creates an IBM MessageSight JMS client queue connection factory.
     * <p>
     * @return A QueueConnectionFactory which can also be cast to ImaProperties.
     * @throws JMSException If the queue connection factory cannot be created
     */
    public static QueueConnectionFactory createQueueConnectionFactory() throws JMSException {
        QueueConnectionFactory ret = (QueueConnectionFactory)new ImaConnectionFactory();
        ((ImaProperties)ret).put("ObjectType", "queue");
        return ret;
    }


    /**
     * Creates a topic given a name.
     * The name of a topic is a String which describes the topic.  This is often
     * different from the name under which it is stored in JNDI. 
     * <p>
     * It is possible to call this method with a null value for the name, but the
     * topic must have a name before it can be used.  The name can be set later
     * using the Name property.
     * <p>
     * @param name The name of the topic
     * @return  A Topic which can also be cast to ImaProperties.
     * @throws JMSException If the topic cannot be created
     * 
     */
    public static Topic createTopic(String name) throws JMSException {
        return (Topic)new ImaTopic(name);
    }


    /**
     * Creates a queue given a name.
     * The name of a queue is a String which describes the queue.  This is often
     * different from the name under which it is stored in JNDI. 
     * <p>
     * It is possible to call this method with a null value for the name, but the
     * queue must have a name before it can be used.  The name can be set later
     * using the Name property.
     * <p>
     * @param name The name of the queue
     * @return  A Queue which can also be cast to ImaProperties.
     * @throws JMSException If the destination cannot be created
     * 
     */
   public static Queue createQueue(String name) throws JMSException {
       return (Queue) new ImaQueue(name);
   }
   
}
