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

package com.ibm.ima.ra;

import java.beans.BeanDescriptor;
import java.beans.PropertyDescriptor;

import com.ibm.ima.ra.common.ImaBeanInfo;

public class ImaResourceAdapterBeanInfo extends ImaBeanInfo {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /** the Bean class associated with this BeanInfo */
    private static final Class<ImaResourceAdapter> beanClass = ImaResourceAdapter.class;
    
    private static final ConfigParameter[]        configParams = {
        /*                              name                 preferred  expert   description */
        new ConfigParameter(beanClass, "defaultTraceLevel",     true,  false, "The level of detail provided in resource adapter trace output (0-9)"),
        new ConfigParameter(beanClass, "traceFile",             true,  false, "File name for resource adapter trace output (stdout, stderr, or path to file)"),
        new ConfigParameter(beanClass, "dynamicTraceEnabled",   true,  false, "Enable WebSphere Application Server dynamic trace level settings (true/false)") /*,
        new ConfigParameter(beanClass, "connectionPoolSize",    true,  false, "The maximum number of connections to MessageSight (1-1024)"),
        new ConfigParameter(beanClass, "logWriterEnabled",      true,  false, "Enable the log writer (true/false)") */
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
        return getPropertyDescriptors(configParams);
        
    }
}
