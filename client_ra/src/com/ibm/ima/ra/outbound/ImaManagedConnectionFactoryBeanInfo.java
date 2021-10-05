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

package com.ibm.ima.ra.outbound;

import java.beans.BeanDescriptor;
import java.beans.PropertyDescriptor;

import com.ibm.ima.ra.common.ImaFactoryConfigBeanInfo;

/**
 */
public class ImaManagedConnectionFactoryBeanInfo extends ImaFactoryConfigBeanInfo {

    /** the Bean class associated with this BeanInfo */
    private static final Class<ImaManagedConnectionFactory> beanClass = ImaManagedConnectionFactory.class;

    private static final ConfigParameter[]        configParams = {
        /*                              name                 preferred  expert   description */ 
        new ConfigParameter(beanClass, "convertMessageType",     true,  true,  "The action to take when converting a message to JMS from a source " +
                                                                               "which does not indicate the message type (auto/bytes/text)"),
        new ConfigParameter(beanClass, "temporaryQueue",         true,  true,  "Specifies the prototype name for a temporary queue name"),
        new ConfigParameter(beanClass, "temporaryTopic",         true,  true,  "Specifies the prototype name for a temporary topic name"),
        new ConfigParameter(beanClass, "transactionSupportLevel",true,  true,  "Specifies the types of transactions to support for this connection\n" +
                                                                               "(XATransaction - supports global and local transcations / LocalTransaction - " +
                                                                               "supports resource manager local transactions / NoTransaction - does not support " +
                                                                               "local or global transactions)"),
        new ConfigParameter(beanClass, "traceLevel",              true, false, "The level of detail provided in trace output for this connection (-1, 0-9).  The default " +
                                                                               "value of -1 indicates that the default trace level setting in the resource adapter configuration " +
                                                                               " is used.")
    };
    
    
    /**
     * Implementation of getBeanDescriptor()
     * 
     * @return BeanDescriptor
     */
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
