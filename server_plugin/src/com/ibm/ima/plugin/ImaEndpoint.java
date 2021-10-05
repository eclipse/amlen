/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.plugin;

/**
 * The ImaEndpoint object represents a read only object which contains the settings of the 
 * endpoint within IBM MessageSight.
 */
public interface ImaEndpoint {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    /**
     * Returns the name of the endpoint.
     * @return The name of the endpoint.
     */
    public String   getName();
    
    /**
     * Returns the IP address of the interface where the endpoint accepts connections.
     * @return the IP address of the endpoint or null if the endpoint accepts connections from all interfaces.
     */
    public String   getAddress();
    
    /**
     * Returns the maximum message size for the endpoint.
     * @return The maximum message size for this endpoint in bytes
     */
    public int      getMaxMessgeSize();
    
    /**
     * Returns the name of the message hub to which the endpoint belongs 
     * @return the name of the message hub 
     */
    public String   getMessageHub();
    
    /**
     * Returns the port number of the endpoint.
     * @return The port number on which the endpoint listens for connections.
     */
    public int      getPort();
    
    /**
     * Returns the protocol mask for the endpoint.
     * @return The mask of protocol bits
     */
    public long getProtocolMask();
    
    /**
     * Returns whether the connection is secure.
     * @return true if the connection is secure and false otherwise
     */
    public boolean  isSecure();
    
    /**
     * Returns whether the connection is reliable.
     * @return true if the endpoint is reliable (such as TCP), false if the endpoint is unreliable (UDP)
     */
    public boolean  isReliable();
    
    /**
     * Returns whether client certificates are used.
     * @return true if the endpoint checks client certificates, and false if the endpoint does not check client certificates
     */
    public boolean  useClientCert();
    
    /**
     * Returns whether the authorization requires passwords.
     * @return true if password authorization is required, and false if the password authorization is not required.
     */
    public boolean  usePassword();
}
