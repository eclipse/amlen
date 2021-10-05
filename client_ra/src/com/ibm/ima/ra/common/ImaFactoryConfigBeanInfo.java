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

package com.ibm.ima.ra.common;

import java.beans.BeanDescriptor;
import java.beans.PropertyDescriptor;

public class ImaFactoryConfigBeanInfo extends ImaBeanInfo {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /** The Bean class associated with this BeanInfo */
    private static final Class<ImaFactoryConfig> beanClass    = ImaFactoryConfig.class;

    /*
     * Define the config parameters
     */
    private static final ConfigParameter[]       configParams = {
        /*                             name           preferred  expert  description     */
        new ConfigParameter(beanClass, "server",                true, false, "A space or comma delimited list with MessageSight host names or IP addresses"),
        new ConfigParameter(beanClass, "port",                  true, false, "The MessageSight TCP port to use for this connection (1-65535)"),
        new ConfigParameter(beanClass, "protocol",              true, false, "The transmit protocol for communication with MessageSight (tcp/tcps)"),
        new ConfigParameter(beanClass, "securitySocketFactory", true, false, "The name of the security socket factory to use for establishing secure connections"),
        new ConfigParameter(beanClass, "securityConfiguration", true, false, "Configuration for security socket factory"),
        new ConfigParameter(beanClass, "user",                  true, false, "The user name for user authentication with MessageSight"),
        new ConfigParameter(beanClass, "password",              true, false, "The password for user authentication with MessageSight"),
        new ConfigParameter(beanClass, "clientId",              true, false, "The client ID for this connection")
    };

    /*
     * Return the bean descriptor
     * @see java.beans.SimpleBeanInfo#getBeanDescriptor()
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
     return getPropertyDescriptors(configParams);
 }

 protected ConfigParameter[] getConfigParams() {
     return configParams;
 }
}
