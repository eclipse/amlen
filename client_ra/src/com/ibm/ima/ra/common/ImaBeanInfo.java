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

package com.ibm.ima.ra.common;

import java.beans.IntrospectionException;
import java.beans.PropertyDescriptor;
import java.beans.SimpleBeanInfo;

import com.ibm.ima.jms.impl.ImaTrace;

public class ImaBeanInfo extends SimpleBeanInfo {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    static {
        ImaTrace.init(true);
    }
    
    /**
     * Implementation of getPropertyDescriptiors() that returns all configurable properties on the Bean.
     * 
     * @return PropertyDescriptor
     */
    public PropertyDescriptor[] getPropertyDescriptors(ConfigParameter[] params) {
        try {
            PropertyDescriptor[] result = new PropertyDescriptor[params.length];
            for (int i = 0; i < result.length; i++) {
                PropertyDescriptor pd = new PropertyDescriptor(params[i].name, params[i].beanClass);
                pd.setShortDescription(params[i].description);
                pd.setExpert(params[i].expert);
                pd.setPreferred(params[i].prefered);
                result[i] = pd;
            }
            return result;
        } catch (IntrospectionException e) {
            ImaTrace.traceException(1, e);
            return null;
        }
    }

    /*
     * Combine bean config within subclasses
     */
    protected ConfigParameter[] combineConfigParams(ConfigParameter[] params1, ConfigParameter[] params2) {
        ConfigParameter[] result = new ConfigParameter[params1.length + params2.length];
        System.arraycopy(params1, 0, result, 0, params1.length);
        System.arraycopy(params2, 0, result, params1.length, params2.length);
        return result;
    }

    /*
     * Inner class used to define bean config parameters
     */
    protected static final class ConfigParameter {
        private final String   name;
        private final boolean  expert;
        private final boolean  prefered;
        private final String   description;
        private final Class<?> beanClass;

        /*
         * Constructor
         */
        public ConfigParameter(Class<?> beanClass, String name, boolean prefered, boolean expert, String description) {
            this.name = name;
            this.expert = expert;
            this.prefered = prefered;
            this.description = description;
            this.beanClass = beanClass;
        }
    }
}
