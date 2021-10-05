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
package com.ibm.ima.ra.inbound;

import java.beans.BeanDescriptor;
import java.beans.PropertyDescriptor;

import com.ibm.ima.ra.common.ImaFactoryConfigBeanInfo;

public class ImaActivationSpecBeanInfo extends ImaFactoryConfigBeanInfo {
    /** the Bean class associated with this BeanInfo */
    private static final Class<ImaActivationSpec> beanClass    = ImaActivationSpec.class;
    
    private static final ConfigParameter[]        configParams = {
        /*                              name                 preferred  expert   description */
        new ConfigParameter(beanClass, "destination",            true,  false, "The name of the topic or queue from which messages will be received. " +
                                                                               "Either destination or destinationLookup must be specified."),
        new ConfigParameter(beanClass, "destinationType",        true,  false, "The type of destination for a specified destination value (javax.jms.Topic/javax.jms.Queue)"),
        new ConfigParameter(beanClass, "destinationLookup",      true,  false, "The name of a JNDI object representing the IBM MessageSight topic or " +
                                                                               "queue from which messages will be received. Either destination or destinationLookup must be specified."),
        new ConfigParameter(beanClass, "subscriptionDurability", true,  false, "Specifies whether a topic subscription is durable (NonDurable/Durable)"),
        new ConfigParameter(beanClass, "subscriptionName",       true,  false, "The name for a durable or shared subscription"),
        new ConfigParameter(beanClass, "subscriptionShared",     true,  false, "Specifies whether a subscription is shared between consumers (NonShared/Shared)"),
        new ConfigParameter(beanClass, "acknowledgeMode",        true,  false, "The mode for acknowledging received messages (Auto-acknowledge/Dups-ok-acknowledge)"),
        new ConfigParameter(beanClass, "messageSelector",        true,  false, "A JMS message selector specifying which messages are selected"),
        new ConfigParameter(beanClass, "convertMessageType",     true,  true,  "Convert type for non-JMS messages"),
        new ConfigParameter(beanClass, "concurrentConsumers",    false, true,  "The maximum number of consumers in this connection (1-100)"), 
        new ConfigParameter(beanClass, "clientMessageCache",     false, true,  "The maximum number of messages which should be cached for each consumer. " +
                                                                               "The default value of -1 sets cache size to 1000 for unshared topic subscriptions " + 
                                                                               "and to 0 for all other cases."), 
        new ConfigParameter(beanClass, "enableRollback",         false, true,  "Specifies the message handling when failures occur and the MDB is configured with transaction-type Bean " +
                                                                               "(false - do not rollback on failure, true - rollback on failure)"),
        new ConfigParameter(beanClass, "ignoreFailuresOnStart",  false, true,  "Specifies whether to start a MDB when the connection attempt at startup time " +
                                                                               "fails (false - allow the MDB to stop if initial connection attempt fails, true - start the MDB " +
                                                                               "despite the failure and periodically attempt to connect)"),
        new ConfigParameter(beanClass, "maxDeliveryFailures",    false, true,  "Specifies the maximum number of message delivery failures permitted for the MDB. The " +
        		                                                               "endpoint is paused after the specified failure count is reached. The default value of " +
                                                                               "-1 indicates the failure count is ignored so the endpoint is never paused."),
        new ConfigParameter(beanClass, "traceLevel",             true, false,  "The level of detail provided in trace output for this connection (-1, 0-9). The default " +
                                                                               "value of -1 indicates that the default trace level setting in the resource adapter configuration " +
                                                                               "is used.")
    };

    public BeanDescriptor getBeanDescriptor() {
        return new BeanDescriptor(beanClass);
    }
    
    /**
     * Implementation of getPropertyDescriptiors() that returns all configurable properties on the Bean.
     * 
     * @return PropertyDescriptor
     */
    public PropertyDescriptor[] getPropertyDescriptors() {
        return getPropertyDescriptors(getConfigParams());
    }
    
    protected ConfigParameter[] getConfigParams() {
        return combineConfigParams(configParams, super.getConfigParams());
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
