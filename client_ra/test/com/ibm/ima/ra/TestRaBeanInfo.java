/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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

import junit.framework.TestCase;

import java.awt.Image;
import java.beans.BeanDescriptor;
import java.beans.BeanInfo;
import java.beans.EventSetDescriptor;
import java.beans.MethodDescriptor;
import java.beans.PropertyDescriptor;

public class TestRaBeanInfo extends TestCase {
    ImaResourceAdapterBeanInfo beanInfo = null;
    
    public void setUp() {
        beanInfo = new ImaResourceAdapterBeanInfo();
    }
    
    public TestRaBeanInfo() {
        super("IMA JMS Resource Adapter RaBeanInfo test");
    }
    
    public void testBeanInfo() throws Exception {
        BeanInfo[] info = beanInfo.getAdditionalBeanInfo();
        assertEquals(null, info);
        
        BeanDescriptor descriptor = beanInfo.getBeanDescriptor();
        assertEquals(ImaResourceAdapter.class,descriptor.getBeanClass());
        
        int eidx = beanInfo.getDefaultEventIndex();
        assertEquals(-1, eidx);
        
        int pidx = beanInfo.getDefaultPropertyIndex();
        assertEquals(-1, pidx);
        
        EventSetDescriptor[] edescriptors = beanInfo.getEventSetDescriptors();
        assertEquals(null, edescriptors);
        
        Image img = beanInfo.getIcon(BeanInfo.ICON_COLOR_16x16);
        assertEquals(null, img);
        
        img = beanInfo.getIcon(BeanInfo.ICON_COLOR_32x32);
        assertEquals(null, img);
        
        img = beanInfo.getIcon(BeanInfo.ICON_MONO_16x16);
        assertEquals(null, img);
        
        img = beanInfo.getIcon(BeanInfo.ICON_MONO_32x32);
        assertEquals(null, img);
        
        MethodDescriptor[] mdescriptors = beanInfo.getMethodDescriptors();
        assertEquals(null, mdescriptors);
        
        PropertyDescriptor[] pdescriptors = beanInfo.getPropertyDescriptors();
        assertEquals(3,pdescriptors.length);
        // TODO: Add checks for additional properties in the bean info class
        assertEquals("defaultTraceLevel",pdescriptors[0].getName());
        assertEquals(null, pdescriptors[0].getValue("defaultTraceLevel"));
        assertFalse(pdescriptors[0].isExpert());
        assertEquals("traceFile",pdescriptors[1].getName());
        assertEquals(null, pdescriptors[1].getValue("traceFile"));
        assertFalse(pdescriptors[1].isExpert());
        assertEquals("dynamicTraceEnabled",pdescriptors[2].getName());
        assertEquals(null, pdescriptors[2].getValue("dynamicTraceEnabled"));
        assertFalse(pdescriptors[2].isExpert());
        
        img = beanInfo.loadImage(null);
        assertEquals(null, img);
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
