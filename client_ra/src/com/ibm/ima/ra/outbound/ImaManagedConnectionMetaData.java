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

import javax.jms.ConnectionMetaData;
import javax.jms.JMSException;
import javax.resource.spi.ManagedConnectionMetaData;

public class ImaManagedConnectionMetaData implements ManagedConnectionMetaData {
    /** Product Name */
	private String productName    = "";
    /** Version */
    private String productVersion = "";
    /** the username used to create the connection */
    private final String              userName;
    /**
     * The maximum number of connections supported. For now this is will always
     * return 0, implying we don't have an (RA-defined) limit.
     */
    private static final int    maxConnections = 0;

    /**
	 * 
	 */
    public ImaManagedConnectionMetaData(String usr, ConnectionMetaData metaData) {
        userName = usr;
        if (metaData != null) {
        	try {
                productVersion = metaData.getProviderVersion();
                /* Note: The product name is never sent from the server.  Only the 
                 * the version is sent.  We are using the product name from the 
                 * JMS client.
                 */
                productName = metaData.getJMSProviderName();
        	} catch (JMSException ex) {
        		/* Ignore.
        		 * While JMSException is declared for these methods, the JMS client does not
        		 * throw any exceptions for either of them.
        		 */
        	}
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionMetaData#getEISProductName()
     */
    public String getEISProductName()
            throws javax.resource.ResourceException {
    	/* If an empty string is returned, it means there is not a connection to the server yet. */
        return productName;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionMetaData#getEISProductVersion()
     */
    public String getEISProductVersion()
            throws javax.resource.ResourceException {
    	/* If an empty string is returned, it means there is not a connection to the server yet. */
        return productVersion;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionMetaData#getMaxConnections()
     */
    public int getMaxConnections() throws javax.resource.ResourceException {
        return maxConnections;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionMetaData#getUserName()
     */
    public String getUserName()
            throws javax.resource.ResourceException {
        return userName;
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


}
