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

import javax.resource.cci.ResourceAdapterMetaData;

import junit.framework.TestCase;

public class TestRaMetaData extends TestCase {
    ResourceAdapterMetaData metaData = null;
    public void setUp() {
        metaData = new ImaResourceAdapterMetaData();
    }
    
    public TestRaMetaData() {
        super("IMA JMS Resource Adapter RaMetaData test");
    }
    
    public void testMetaData() {
        //TODO: Get the version number from paths.properties
    	//if (!metaData.getAdapterVersion().equals("VERSION_ID"))
        //    assertEquals("5.0.0.2", metaData.getAdapterVersion());
        assertEquals("Eclipse Foundation", metaData.getAdapterVendorName());
        assertEquals("Eclipse Amlen JMS Resource Adapter", metaData.getAdapterName());
        assertEquals("Eclipse Amlen JMS Resource Adapter", metaData.getAdapterShortDescription());
        assertEquals("1.6", metaData.getSpecVersion());
        assertEquals(0, metaData.getInteractionSpecsSupported().length);
        assertFalse(metaData.supportsExecuteWithInputAndOutputRecord());
        assertFalse(metaData.supportsExecuteWithInputRecordOnly());
        assertFalse(metaData.supportsLocalTransactionDemarcation());
    }
}
