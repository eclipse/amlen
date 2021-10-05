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
package com.ibm.ima.plugin.impl;

import java.util.HashMap;
import java.util.Map;

import com.ibm.ima.plugin.ImaEndpoint;

/*
 * The endpoint is a readonly object to the plug-in. 
 */
public class ImaEndpointImpl implements ImaEndpoint {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    private   String   name;
    private   String   address;
    private   String   msgHub;
    private   int      maxMessageSize;
    private   int      port;
    private   int      flags;
    private   long     protomask;

    static final int F_Enabled  = 1;
    static final int F_Secure   = 2;
    static final int F_Reliable = 4;
    static final int F_UseCert  = 8;
    static final int F_UsePW    = 16;
    
    static HashMap<String,ImaEndpointImpl> endpoints = new HashMap<String,ImaEndpointImpl>();
    
    /*
     * Construct an endpoint from a map
     */
    ImaEndpointImpl(Map<String,Object> map) {
        synchronized (ImaEndpointImpl.class) {
            this.name = getStringProperty(map, "Name");
            if (this.name == null)
                throw new IllegalArgumentException("Endpoint does not include name");
            this.address = getStringProperty(map, "Interface");
            this.port = getIntProperty(map, "Port", 0);
            this.msgHub = getStringProperty(map, "MessageHub");
            this.maxMessageSize = getIntProperty(map, "MaxMessageSize", 0);
            this.protomask = getLongProperty(map, "ProtoMask", 0L);
            
            if (getIntProperty(map, "Enabled", 0) != 0)
                flags |= F_Enabled;
            if (getIntProperty(map, "Secure", 0) != 0)
                flags |= F_Secure;
            if (getIntProperty(map, "Reliable", 1) != 0)
                flags |= F_Reliable;
            if (getIntProperty(map, "UseClientCertificate", 0) != 0)
                flags |= F_UseCert;
            if (getIntProperty(map, "UsePassword", 0) != 0)
                flags |= F_UsePW;
           
            
            endpoints.put(name, this);
        }
        if (ImaPluginMain.trace.isTraceable(4)) {
            ImaPluginMain.trace.trace(""+this);
        }
    }
    
    /*
     * Private endpoint constructor for not-found case
     */
    private ImaEndpointImpl(String name) {
        endpoints.put(name, this);
    }

    /*
     * Get an endpoint by name
     */
    public static ImaEndpointImpl getEndpoint(String name, boolean create) {
        synchronized (ImaEndpointImpl.class) {
            ImaEndpointImpl ret = endpoints.get(name);
            if (ret == null && create) {
                ret = new ImaEndpointImpl(name);
            }
            return ret;
        }    
    }
    
    /*
     * Utility to return a string property
     */
    static String getStringProperty(Map<String,Object>map, String name) {
        Object obj = map.get(name);
        if (obj == null)
            return null;
        if (obj instanceof String)
            return (String)obj;
        return ""+obj;
    }
    
    /*
     * Utility to return an int property
     */
    static int getIntProperty(Map<String,Object>map, String name, int defval) {
        Object obj = map.get(name);
        if (obj == null)
            return defval;
        if (obj instanceof Number)
            return ((Number)obj).intValue();
        return defval;
    }
    
    /*
     * Utility to return a long property
     */
    static long getLongProperty(Map<String,Object>map, String name, long defval) {
        Object obj = map.get(name);
        if (obj == null)
            return defval;
        if (obj instanceof Number)
            return ((Number)obj).intValue();
        return defval;
    }
    
    /*
     * @see com.ibm.ima.ext.ImaEndpoint#getName()
     */
    public String getName() {
        return name;
    }

    /*
     * @see com.ibm.ima.ext.ImaEndpoint#getAddress()
     */
    public String getAddress() {
        return address;
    }

    /*
     * @see com.ibm.ima.ext.ImaEndpoint#getMaxMessgeSize()
     */
    public int getMaxMessgeSize() {
        return maxMessageSize;
    }

    /*
     * @see com.ibm.ima.ext.ImaEndpoint#getMessageHub()
     */
    public String getMessageHub() {
        return msgHub;
    }

    /*
     * @see com.ibm.ima.ext.ImaEndpoint#getPort()
     */
    public int getPort() {
        return port;
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaEndpoint#getProtocolMask()
     */
    public long getProtocolMask() {
        return protomask;
    }

    /*
     * @see com.ibm.ima.ext.ImaEndpoint#isSecure()
     */
    public boolean isSecure() {
        return (flags&F_Secure) != 0;
    }

    /*
     * @see com.ibm.ima.ext.ImaEndpoint#isReliable()
     */
    public boolean isReliable() {
        return (flags&F_Reliable) != 0;
    }

    /*
     * @see com.ibm.ima.ext.ImaEndpoint#useClientCert()
     */
    public boolean useClientCert() {
        return (flags&F_UseCert) != 0;
    }

    /*
     * @see com.ibm.ima.ext.ImaEndpoint#usePassword()
     */
    public boolean usePassword() {
        return (flags&F_UsePW) !=0;
    }

    /*
     * @see java.lang.Object#toString()
     * TODO: add more later
     */
    public String toString() {
        return "ImaEndpointImpl Name=\"" + name + "\" Interface=\"" + address + "\" Port=" + port;    
    }
}
