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

import javax.jms.InvalidDestinationException;
import javax.jms.InvalidSelectorException;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.Topic;

/**
 * Allow access to shared subscriptions from JMS 1.1.
 * <p>
 * JMS 2.0 adds the concept of shared subscriptions and the IBM MessageSight JMS client allows these 
 * methods to be accessed with a JMS 1.1 client using the ImaSubscription interface. 
 * The Session object returned by the IBM MssageSight JMS client implements the ImaSubscription interface
 * which adds new methods for shared subscriptions.  These methods match the design for the shared
 * subscription support added in JMS 2.0.
 * <p>
 * To use these methods you must cast the returned Session object to ImaSubscription:
 * <pre>
 * Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
 * MessageConsumer consumer = <b>((ImaSubscription)session)</b>.createSharedConsumer(topic, "Subscription");
 * </pre>  
 * 
 */
public interface ImaSubscription {
    /**
     * Creates a shared non-durable subscription and returns a message consumer over it.
     * <p> 
     * Creates a shared non-durable subscription with the specified name on the specified topic 
     * (if one does not already exist) and creates a consumer on that subscription.
     * This method creates the non-durable subscription without a message selector.
     * <p>
     * If a shared non-durable subscription already exists with the same name and client identifier (if set), 
     * and the same topic and message selector has been specified, then this method creates a MessageConsumer 
     * on the existing subscription.
     * <p>
     * A non-durable shared subscription is used by a client which needs to be able to share the work of 
     * receiving messages from a topic subscription amongst multiple consumers. A non-durable shared 
     * subscription may therefore have more than one consumer. Each message from the subscription will be 
     * delivered to only one of the consumers on that subscription. 
     * Such a subscription is not persisted and will be deleted (together with any undelivered messages 
     * associated with it) when there are no consumers on it. The term "consumer" here means a MessageConsumer 
     * object in any client. 
     * <p>
     * A shared non-durable subscription is identified by a name specified by the client and by 
     * the client identifier (which may be unset). An application which subsequently wishes to create 
     * a consumer on that shared non-durable subscription must use the same client identifier. 
     * <p>
     * If a shared non-durable subscription already exists with the same name and client identifier 
     * (if set) but a different topic or message selector has been specified, and there is a consumer 
     * already active (i.e. not closed) on the subscription, then a JMSException will be thrown. 
     * <p>
     * There is no restriction on durable subscriptions and shared non-durable subscriptions having 
     * the same name and clientId (which may be unset). Such subscriptions would be completely separate.
     * 
     * @param topic - The topic to subscribe to
     * @param name - The subscription name
     * @return A message consumer
     * @throws JMSException - 
     * <ul>
     * <li>if the name already exists but the topic or selector are different and there are already consumers,
     * <li>if a nonshared non-durable subscription already exists with the same name 
     * <li>if the subscription or consumer cannot be created due to an internal error
     * </ul>
     * @throws InvalidDestinationException - if an invalid topic is specified
     * @since IBM MessageSight V1.1
     */
    public MessageConsumer createSharedConsumer(Topic topic, String name) throws JMSException;

    /**
     * Creates a shared non-durable subscription and returns a message consumer over it.
     * <p> 
     * Creates a shared non-durable subscription with the specified name on the specified topic 
     * (if one does not already exist) specifying a message selector, and creates a consumer on that subscription.
     * <p>
     * If a shared non-durable subscription already exists with the same name and client identifier (if set), 
     * and the same topic and message selector has been specified, then this method creates a MessageConsumer 
     * on the existing subscription.
     * <p>
     * A non-durable shared subscription is used by a client which needs to be able to share the work of 
     * receiving messages from a topic subscription amongst multiple consumers. A non-durable shared 
     * subscription may therefore have more than one consumer. Each message from the subscription will be 
     * delivered to only one of the consumers on that subscription. 
     * Such a subscription is not persisted and will be deleted (together with any undelivered messages 
     * associated with it) when there are no consumers on it. The term "consumer" here means a MessageConsumer 
     * object in any client. 
     * <p>
     * A shared non-durable subscription is identified by a name specified by the client and by 
     * the client identifier (which may be unset). An application which subsequently wishes to create 
     * a consumer on that shared non-durable subscription must use the same client identifier. 
     * <p>
     * If a shared non-durable subscription already exists with the same name and client identifier 
     * (if set) but a different topic or message selector has been specified, and there is a consumer 
     * already active (i.e. not closed) on the subscription, then a JMSException will be thrown. 
     * <p>
     * There is no restriction on durable subscriptions and shared non-durable subscriptions having 
     * the same name and clientId (which may be unset). Such subscriptions would be completely separate.
     * 
     * @param topic - The topic to subscribe to
     * @param name - The subscription name
     * @param selector - The message selector
     * @return A message consumer
     * @throws JMSException - 
     * <ul>
     * <li>if the name already exists but the topic or selector are different and there are already consumers,
     * <li>if a nonshared non-durable subscription already exists with the same name 
     * <li>if the subscription or consumer cannot be created due to an internal error
     * </ul>
     * @throws InvalidDestinationException - if an invalid topic is specified
     * @throws InvalidSelectorException - if the message selector is not valid
     * @since IBM MessageSight V1.1
     */
    public MessageConsumer createSharedConsumer(Topic topic, String name, String selector) throws JMSException;


    /**
     * Creates a shared durable subscription and returns a message consumer over it.
     * <p> 
     * Creates a shared durable subscription on the specified topic (if one does not already exist), 
     * and creates a consumer on that durable subscription. 
     * This method creates the durable subscription without a message selector.
     * <p>
     * A durable subscription is used by an application which needs to receive all the messages 
     * published on a topic, including the ones published when there is no active consumer associated with it. 
     * The JMS provider retains a record of this durable subscription and ensures that all messages from the 
     * topic's publishers are retained until they are delivered to, and acknowledged by, a consumer on this 
     * durable subscription or until they have expired.
     * <p> 
     * A durable subscription will continue to accumulate messages until it is deleted using the unsubscribe method. 
     * <p>
     * This method may only be used with shared durable subscriptions. Any durable subscription created using 
     * this method will be shared. This means that multiple active (i.e. not closed) consumers on the subscription 
     * may exist at the same time. The term "consumer" here means a MessageConsumer object in any client.
     * <p>
     * A shared durable subscription is identified by a name specified by the client and by the client identifier 
     * (which may be unset). An application which subsequently wishes to create a consumer on that shared durable 
     * subscription must use the same client identifier. 
     * <p>
     * If a shared durable subscription already exists with the same name and client identifier (if set), 
     * and the same topic and message selector has been specified, then this method creates a MessageConsumer 
     * on the existing shared durable subscription.
     * <p>
     * If a shared durable subscription already exists with the same name and client identifier (if set) 
     * but a different topic or message selector has been specified, and there is no consumer already active (
     * i.e. any existing consumers are closed) on the durable subscription then this is equivalent to unsubscribing (deleting) the old 
     * one and creating a new one. 
     * <p>
     * If a shared durable subscription already exists with the same name and client identifier (if set) 
     * but a different topic or message selector has been specified, and there is a consumer already active (
     * i.e. not closed) on the durable subscription, then a JMSException will be thrown.
     * <p>
     * A shared durable subscription and an unshared durable subscription may not have the same name and 
     * client identifier (if set). If an unshared durable subscription already exists with the same name 
     * and client identifier (if set) then a JMSException is thrown.
     * <p>
     * There is no restriction on durable subscriptions and shared non-durable subscriptions having the 
     * same name and clientId (which may be unset). Such subscriptions would be completely separate.
     * 
     * @param topic - The topic to subscribe to
     * @param name - The subscription name
     * @return A message consumer
     * @throws JMSException - 
     * <ul>
     * <li>if the name already exists but the topic or selector are different and there are already consumers,
     * <li>if a nonshared durable subscription already exists with the same name 
     * <li>if the subscription or consumer cannot be created due to an internal error
     * </ul>
     * @throws InvalidDestinationException - if an invalid topic is specified
     * @since IBM MessageSight V1.1
     */
    public MessageConsumer createSharedDurableConsumer(Topic topic, String name) throws JMSException;

    /**
     * Creates a shared durable subscription and returns a message consumer over it.
     * <p> 
     * Creates a shared durable subscription on the specified topic (if one does not already exist), 
     * specifying a message selector, and creates a consumer on that durable subscription. 
     * This method creates the durable subscription with a message selector.
     * <p>
     * A durable subscription is used by an application which needs to receive all the messages 
     * published on a topic, including the ones published when there is no active consumer associated with it. 
     * The JMS provider retains a record of this durable subscription and ensures that all messages from the 
     * topic's publishers are retained until they are delivered to, and acknowledged by, a consumer on this 
     * durable subscription or until they have expired.
     * <p> 
     * A durable subscription will continue to accumulate messages until it is deleted using the unsubscribe method. 
     * <p>
     * This method may only be used with shared durable subscriptions. Any durable subscription created using 
     * this method will be shared. This means that multiple active (i.e. not closed) consumers on the subscription 
     * may exist at the same time. The term "consumer" here means a MessageConsumer object in any client.
     * <p>
     * A shared durable subscription is identified by a name specified by the client and by the client identifier 
     * (which may be unset). An application which subsequently wishes to create a consumer on that shared durable 
     * subscription must use the same client identifier. 
     * <p>
     * If a shared durable subscription already exists with the same name and client identifier (if set), 
     * and the same topic and message selector has been specified, then this method creates a MessageConsumer 
     * on the existing shared durable subscription.
     * <p>
     * If a shared durable subscription already exists with the same name and client identifier (if set) 
     * but a different topic or message selector has been specified, and there is no consumer already active (
     * i.e. any existing consumers are closed) on the durable subscription then this is equivalent to unsubscribing (deleting) the old 
     * one and creating a new one. 
     * <p>
     * If a shared durable subscription already exists with the same name and client identifier (if set) 
     * but a different topic or message selector has been specified, and there is a consumer already active (
     * i.e. not closed) on the durable subscription, then a JMSException will be thrown.
     * <p>
     * A shared durable subscription and an unshared durable subscription may not have the same name and 
     * client identifier (if set). If an unshared durable subscription already exists with the same name 
     * and client identifier (if set) then a JMSException is thrown.
     * <p>
     * There is no restriction on durable subscriptions and shared non-durable subscriptions having the 
     * same name and clientId (which may be unset). Such subscriptions would be completely separate.
     * 
     * @param topic - The topic to subscribe to
     * @param name - The subscription name
     * @param selector - The message selector
     * @return A message consumer
     * @throws JMSException - 
     * <ul>
     * <li>if the name already exists but the topic or selector are different and there are already consumers,
     * <li>if a nonshared durable subscription already exists with the same name 
     * <li>if the subscription or consumer cannot be created due to an internal error
     * </ul>
     * @throws InvalidDestinationException - if an invalid topic is specified
     * @throws InvalidSelectorException - if the message selector is not valid
     * @since IBM MessageSight V1.1
     */
    public MessageConsumer createSharedDurableConsumer(Topic topic, String name, String selector) throws JMSException;
    
    /**
     * Creates a durable subscription and returns a message consumer over it.
     * <p>
     * Creates an unshared durable subscription on the specified topic (if one does not already exist) 
     * and creates a consumer on that durable subscription. This method creates the durable subscription 
     * without a message selector and with a noLocal value of false. 
     * <p>
     * A durable subscription is used by an application which needs to receive all the messages 
     * published on a topic, including the ones published when there is no active consumer associated with it. 
     * The JMS provider retains a record of this durable subscription and ensures that all messages from the 
     * topic's publishers are retained until they are delivered to, and acknowledged by, a consumer on this 
     * durable subscription or until they have expired. 
     * <p>
     * A durable subscription will continue to accumulate messages until it is deleted using the unsubscribe method. 
     * <p>
     * This method may only be used with unshared durable subscriptions. Any durable subscription created 
     * using this method will be unshared. This means that only one active (i.e. not closed) consumer on the 
     * subscription may exist at a time. The term "consumer" here means a TopicSubscriber or MessageConsumer 
     * object in any client. 
     * <p>
     * An unshared durable subscription is identified by a name specified by the client and by the client 
     * identifier, which must be set. An application which subsequently wishes to create a consumer on that 
     * unshared durable subscription must use the same client identifier. 
     * <p>
     * If an unshared durable subscription already exists with the same name and client identifier, 
     * and the same topic, message selector and noLocal value has been specified, and there is no consumer 
     * already active (i.e. any previously existing consumers are closed) on the durable subscription then this 
     * method creates a MessageConsumer on the existing durable subscription. 
     * <p>
     * If an unshared durable subscription already exists with the same name and client identifier, 
     * and there is a consumer already active (i.e. not closed) on the durable subscription, then a 
     * JMSException will be thrown. 
     * <p>
     * If an unshared durable subscription already exists with the same name and client identifier 
     * but a different topic, message selector or noLocal value has been specified, and there is no consumer 
     * already active (i.e. any previously existing consumers are closed) on the durable subscription then this
     * is equivalent to unsubscribing (deleting) the old one and creating a new one. 
     * <p>
     * A shared durable subscription and an unshared durable subscription may not have the same name and 
     * client identifier. If a shared durable subscription already exists with the same name and client identifier 
     * then a JMSException is thrown.
     * <p>
     * There is no restriction on durable subscriptions and shared non-durable subscriptions having the 
     * same name and clientId. Such subscriptions would be completely separate.
     * <p>
     * This method is identical to the corresponding createDurableSubscriber method except that it returns 
     * a MessageConsumer rather than a TopicSubscriber to represent the consumer.
     * 
     * @param topic - the topic to subscribe to
     * @param name - the subscription name
     * @return A message consumer object
     * @throws JMSException -
     * <ul>
     * <li>if an unshared durable subscription already exists with the same name and client identifier, and 
     * there is a consumer already active.
     * <li>if a shared durable subscription already exists with the same name and client identifier 
     * <li>if the the subscription or MessageConsumer cannot be created due to some internal error
     * </ul>
     * @throws InvalidDestinationException - if an invalid topic is specified
     * @throws IllegalStateException - if the client identifier is not set
     * @since IBM MessageSight v1.1
     */ 
    public MessageConsumer createDurableConsumer(Topic topic, String name) throws JMSException;

    /**
     * Creates a durable subscription and returns a message consumer over it.
     * <p>
     * Creates an unshared durable subscription on the specified topic (if one does not already exist) 
     * specifying a message selector and the noLocal parameter, and creates a consumer on that durable subscription. 
     * <p>
     * A durable subscription is used by an application which needs to receive all the messages 
     * published on a topic, including the ones published when there is no active consumer associated with it. 
     * The JMS provider retains a record of this durable subscription and ensures that all messages from the 
     * topic's publishers are retained until they are delivered to, and acknowledged by, a consumer on this 
     * durable subscription or until they have expired. 
     * <p>
     * A durable subscription will continue to accumulate messages until it is deleted using the unsubscribe method. 
     * <p>
     * This method may only be used with unshared durable subscriptions. Any durable subscription created 
     * using this method will be unshared. This means that only one active (i.e. not closed) consumer on the 
     * subscription may exist at a time. The term "consumer" here means a TopicSubscriber or MessageConsumer 
     * object in any client. 
     * <p>
     * An unshared durable subscription is identified by a name specified by the client and by the client 
     * identifier, which must be set. An application which subsequently wishes to create a consumer on that 
     * unshared durable subscription must use the same client identifier. 
     * <p>
     * If an unshared durable subscription already exists with the same name and client identifier, 
     * and the same topic, message selector and noLocal value has been specified, and there is no consumer 
     * already active (i.e. any previously existing consumers are closed) on the durable subscription then this 
     * method creates a MessageConsumer on the existing durable subscription. 
     * <p>
     * If an unshared durable subscription already exists with the same name and client identifier, 
     * and there is a consumer already active (i.e. not closed) on the durable subscription, then a 
     * JMSException will be thrown. 
     * <p>
     * If an unshared durable subscription already exists with the same name and client identifier 
     * but a different topic, message selector or noLocal value has been specified, and there is no consumer 
     * already active (i.e. any previously existing consumers are closed) on the durable subscription then this
     * is equivalent to unsubscribing (deleting) the old one and creating a new one. 
     * <p>
     * A shared durable subscription and an unshared durable subscription may not have the same name and 
     * client identifier. If a shared durable subscription already exists with the same name and client identifier 
     * then a JMSException is thrown.
     * <p>
     * There is no restriction on durable subscriptions and shared non-durable subscriptions having the 
     * same name and clientId. Such subscriptions would be completely separate.
     * <p>
     * This method is identical to the corresponding createDurableSubscriber method except that it returns 
     * a MessageConsumer rather than a TopicSubscriber to represent the consumer.
     * 
     * @param topic - the topic to subscribe to
     * @param name - the subscription name
     * @param selector - the message selector
     * @param noLocal - if true then any messages published to the topic from a connection with the 
     * same client identifier will not be added to the durable subscription. 
     * @return A message consumer object
     * @throws JMSException -
     * <ul>
     * <li>if an unshared durable subscription already exists with the same name and client identifier, and 
     * there is a consumer already active.
     * <li>if a shared durable subscription already exists with the same name and client identifier 
     * <li>if the the subscription or MessageConsumer cannot be created due to some internal error
     * </ul>
     * @throws InvalidDestinationException - if an invalid topic is specified
     * @throws IllegalStateException - if the client identifier is not set
     * @throws InvalidSelectorException - if the message selector is not valid
     * @since IBM MessageSight v1.1
     */ 
    public MessageConsumer createDurableConsumer(Topic topic, String name, String selector, boolean noLocal) throws JMSException;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}

