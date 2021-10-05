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

package com.ibm.ima.jms.impl;

import java.util.Enumeration;
import java.util.Vector;

import javax.jms.ConnectionMetaData;
import javax.jms.JMSException;

/**
 * Implement the ConnectionMetaData for the IBM MessageSight JMS client.
 * 
 * Currently, the JMS metadata is the same for all IMA connections.  The provider metatdata can differ.
 */
public class ImaMetaData implements ConnectionMetaData {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    private static Vector<String> jmsx = new Vector<String>();
    private static int majorVersion = 0;              /* Default value to show IMAServerVersion was never set */
    private static int minorVersion = 0;              /* Default value */
    private static String IMAServerVersion = "";      /* Default value to show IMAServerVersion was never set */
    private static String IMAServerBuild = "";        /* Default value to show IMAServerBuild was never set */
    
    /* 
     * @see javax.jms.ConnectionMetaData#getJMSMajorVersion()
     */
    public int getJMSMajorVersion() throws JMSException {
        return 1;
    }

    /* 
     * @see javax.jms.ConnectionMetaData#getJMSMinorVersion()
     */
    public int getJMSMinorVersion() throws JMSException {
        return 1;
    }

    /* 
     * @see javax.jms.ConnectionMetaData#getJMSProviderName()
     */
    public String getJMSProviderName() throws JMSException {
        return "Eclipse Amlen"; 
    }

    /* 
     * @see javax.jms.ConnectionMetaData#getJMSVersion()
     */
    public String getJMSVersion() throws JMSException {
        return "1.1";
    }

    /* 
     * @see javax.jms.ConnectionMetaData#getJMSXPropertyNames()
     */
    @SuppressWarnings("unchecked")
    public Enumeration getJMSXPropertyNames() throws JMSException {
        jmsx.add("JMSXGroupID");
        jmsx.add("JMSXGroupSeq");
        jmsx.add("JMSXDeliveryCount");
        return jmsx.elements();
    }

    /* 
     * @see javax.jms.ConnectionMetaData#getProviderMajorVersion()
     */
    public int getProviderMajorVersion() throws JMSException {
        return majorVersion;
    }

    /* 
     * @see javax.jms.ConnectionMetaData#getProviderMinorVersion()
     */
    public int getProviderMinorVersion() throws JMSException {
        return minorVersion;
    }

    /* 
     * @see javax.jms.ConnectionMetaData#getProviderVersion()
     */
    public String getProviderVersion() throws JMSException {
        return IMAServerVersion + IMAServerBuild;
    }
    
    /* 
     * Package scope IMA helper methods for setting the provider version string and for setting
     * provider major and minor version int values.
     */
     
    /*
     * Sets the provider version based in data received from the server.
     * It uses this information to set provider major version and minor version.
     * The IMAServerVersion is first part of the string returned when ConnectionMetaData.getProviderVersion() is invoked.
     * The majorVersion and minorVersion values are returned when ConnecitonMedtaData.getMajorVersion() and
     * ConnectionMetaData.getMinorVersion() are invoked.
     * @param pVersion The provider version 
     */
    void setProviderVersion(int pVersion) {
        IMAServerVersion = ImaClient.showVersion(pVersion);
        majorVersion = pVersion/1000000;
        minorVersion = (pVersion/10000)%100;
    }
    
    float getImaServerVersion() {
    	if (IMAServerVersion != null) {
    		String[] version = IMAServerVersion.split("[.]");
    		try {
    			return Float.valueOf(version[0]+"."+version[1]);
    		} catch (NumberFormatException ex) {
    			return 0;
    		}
    	}
    	return 0;
    }
    
    /*
     * Sets the provider build ID based in data received from the server.
     * The IMAServerBuild is the second part of the string returned when ConnectionMetaData.getProviderVersion() is invoked. 
     * @param pBuild The provider build ID as a string
     */
    void setProviderBuild(String pBuild) {
    	if (pBuild != null)
            IMAServerBuild = pBuild;
    }

}
