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
package com.ibm.ima.ra.inbound;

import java.awt.Image;
import java.beans.BeanDescriptor;
import java.beans.BeanInfo;
import java.beans.EventSetDescriptor;
import java.beans.MethodDescriptor;
import java.beans.PropertyDescriptor;
import java.util.ArrayList;

import com.ibm.ima.ra.inbound.ImaActivationSpecBeanInfo;

import junit.framework.TestCase;

public class TestASBeanInfo extends TestCase {
    ImaActivationSpecBeanInfo beanInfo;
    
    static ArrayList<String> propNames = new ArrayList<String>();
    
    static {
        propNames.add("acknowledgeMode");
        propNames.add("convertMessageType");
        propNames.add("destination");
        propNames.add("destinationType");
        propNames.add("destinationLookup");
        propNames.add("subscriptionDurability");
        propNames.add("subscriptionName");
        propNames.add("subscriptionShared");
        propNames.add("maxDeliveryFailures");
        propNames.add("messageSelector");
        // TODO: Intentionally omitting nonASFRollbackEnabled - delete this line when removed from product source
        propNames.add("concurrentConsumers");
        propNames.add("server");
        propNames.add("port");
        propNames.add("protocol");
        // TODO: Check to see whether we are going to keep this property
        propNames.add("securitySocketFactory");
        propNames.add("securityConfiguration");
        // TODO: Note that test to check for this property will fail until the product code changes
        // userName to user per the design document
        propNames.add("user");
        propNames.add("password");
//        propNames.add("traceLevel");
        // TODO: Note that test to check for this property will fail until the product code changes
        // clientID to clientId per the design document
        propNames.add("clientId");
        propNames.add("enableRollback");
        propNames.add("ignoreFailuresOnStart");
        propNames.add("clientMessageCache");
        propNames.add("traceLevel");
    }

    public void setUp() {
        beanInfo = new ImaActivationSpecBeanInfo();
    }
    
    public void testBeanInfo() throws Exception {
        BeanInfo[] addlinfo = beanInfo.getAdditionalBeanInfo();
        assertEquals(null, addlinfo);
        
        BeanDescriptor bdescriptor = beanInfo.getBeanDescriptor();
        assertEquals(ImaActivationSpec.class, bdescriptor.getBeanClass());
        
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
        // TODO: This count is expected to change.  
        // assertEquals(16, pdescriptors.length);
        ArrayList<String> pdescNames = new ArrayList<String>();
        

        
        for(int i = 0; i < pdescriptors.length; i++) {
            String name = pdescriptors[i].getName();
            Object value = pdescriptors[i].getValue(name);
// Next two lines are for test dev debugging
//          System.out.println("Name: "+name+" Value: "+value);
//          if(!(name.equals("nonASFRollbackEnabled") || name.equals("userName") || name.equals("clientID")))
            assertTrue("Checking propNames contains " + name, propNames.contains(name));
            
            if (name.equals("acknowledgeMode")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            else if (name.equals("convertMessageType")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertTrue(pdescriptors[i].isExpert());
            }
            
            else if (name.equals("destination")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("destinationType")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("destinationLookup")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("subscriptionDurability")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("subscriptionName")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("subscriptionShared")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("messageSelector")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("concurrentConsumers")) {
                assertEquals(null, value);
                assertFalse(pdescriptors[i].isPreferred());
                assertTrue(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("server")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("port")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("protocol")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("securitySocketFactory")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("securityConfiguration")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("user")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("password")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("traceLevel")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("clientId")) {
                assertEquals(null, value);
                assertTrue(pdescriptors[i].isPreferred());
                assertFalse(pdescriptors[i].isExpert());
            }
            
            else if(name.equals("enableRollback")) {
                assertEquals(null, value);
                assertFalse(pdescriptors[i].isPreferred());
                assertTrue(pdescriptors[i].isExpert());
            }
            
            else
                System.out.println("Prperty descriptor "+ name + " not found");
        }

        for(int i = 0; i < pdescriptors.length; i++) {
            pdescNames.add(pdescriptors[i].getName());
        }
        
        for(int i = 0; i < propNames.size(); i++) {
            String name = propNames.get(i);
// Next two lines are for test dev debugging
//            System.out.println("PropName: " + name);
//            if(!(name.equals("user") || name.equals("clientId")))
            assertTrue("Checking property descriptors contain "+ name, pdescNames.contains(name));
        }
    }
}
