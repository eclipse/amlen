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

import java.beans.IntrospectionException;
import java.beans.PropertyDescriptor;
import java.io.Serializable;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;

import javax.jms.JMSException;
import javax.resource.spi.InvalidPropertyException;
import javax.validation.constraints.Max;
import javax.validation.constraints.Min;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Pattern;
import javax.validation.constraints.Size;

import com.ibm.ima.jms.impl.ImaConnectionFactory;
import com.ibm.ima.jms.impl.ImaTrace;

public abstract class ImaFactoryConfig implements Serializable {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static final long            serialVersionUID  = 224473341379108533L;
    
    static {
        ImaTrace.init(true);
    }

    protected final ImaConnectionFactory imaCF;
 
    protected String                     clientId          = null;
    protected String                     sFactory          = null;
    protected String                     sConfig           = null;
    protected String                     userName          = null;
    protected String                     password          = null;
    
    /* Validate these connection factory properties. */
    @NotNull (message="{CWLNC2011}")
    @Size(min=1, message="{CWLNC2011}")
    protected String                     server            = null;
    
    @NotNull (message="{CWLNC2012}")
    @Size(min=1, message="{CWLNC2012}")
    protected String                        port;
    
    @Min(value=1, message="{CWLNC2013}")
    @Max(value=65535, message="{CWLNC2013}")
    private int                              portValue;
    
    @NotNull (message="{CWLNC2014}")
    @Pattern(regexp="(tcp|tcps)", flags=Pattern.Flag.CASE_INSENSITIVE,  
            message="{CWLNC2014}")
    protected String                     protocol          = "tcp";

    private AtomicInteger                hashCode         = new AtomicInteger(0);
    
    /*
     * Create a default MessageSight Connection Factory
     */
    public ImaFactoryConfig() {
        imaCF = new ImaConnectionFactory();
    }

    /*
     * Return the user name.
     * The user name is not part of the MessageSight connection factory but is only kept in the proxy. 
     */
    public String getUser() {
        return userName;
    }

    /*
     * Set the user name.
     * The user name is not part of the MessageSight connection factory but is only kept in the proxy. 
     */
    public void setUser(String userName) {
        this.userName = userName;
    }
    
    /**
     * @return userName
     */
    public String getUserName() {
        return this.getUser();
    }

    /**
     * Set userName
     * 
     * @param name the userName to set
     */
    public void setUserName(String name) {
        this.setUser(name);
    }

    /*
     * Return the password.
     * TODO: obfuscate at rest
     */
    public String getPassword() {
        return password;
    }

    /*
     * Set the password.
     * TODO: obfuscate at rest
     */
    public void setPassword(String password) {
        this.password = password;
    }

    /**
     * Return the clientID 
     * @return the clientID
     */
    public String getClientId() {
        if (clientId == null)
            clientId = imaCF.getString("ClientID");
        return clientId;
    }

    /**
     * Set the clientID
     * @param clientID the clientID to set
     */
    public void setClientId(String clientID) {
        this.clientId = clientID;
        imaCF.put("ClientID", this.clientId);
    }

    /**
     * Return the server list.
     * The server list is a space and/or comma separated list of server names which can be either a numeric IP address 
     * or a resolvable name.  Both IPv4 and IPv6 are supported. 
     * @return the server
     */
    public String getServer() {
        if (server == null)
            server = imaCF.getString("Server");
        return server;
    }

    /**
     * Set the server list.
     * The server list is a space and/or comma separated list of server names which can be either a numeric IP address 
     * or a resolvable name.  Both IPv4 and IPv6 are supported. 
     * @param serverHost the serverHost to set
     */
    public void setServer(String serverHost) {
        server = serverHost;
        imaCF.put("Server", server);
    }
    
    /**
     * Return the port number.
     * The port is an integer from 1 to 65535 
     * @return the Port
     */
    public String getPort() {
        if (port == null) {
            String portStr = imaCF.getString("Port");
            if (portStr == null)
                portStr = "0";
            try {
                portValue = Integer.valueOf(portStr);
            } catch (NumberFormatException e) {
                ImaTrace.trace(2, "\'"+ portStr + "\' is not a valid value for port. The value must be a valid Java integer.");
                portValue = 0;
            }
            port = String.valueOf(portValue);
        }
        return port;
    }

    /**
     * Set the port number.
     * @param serverPort the port number to set
     */

    public void setPort(String serverPort) {
        if (serverPort==null)
            serverPort = "0";
        try {
            portValue = Integer.valueOf(serverPort);
            port = serverPort;
        } catch (NumberFormatException e) {
            ImaTrace.trace(2, "\'"+ serverPort + "\' is not a valid value for port. The value must be a valid Java integer.");
            portValue = 0;
            port = "0";
        }
        imaCF.put("Port", port);
    }

    /**
     * Return the connection protocol.
     * The protocols "tcp" and "tcps" are supported.
     * 
     * @return the connection protocol
     */
    public String getProtocol() {
        if (protocol == null)
            protocol = imaCF.getString("Protocol");       
        return protocol;
    }

    /**
     * Set the connection protocol.
     * The protocols "tcp" and "tcps" are supported.
     * 
     * @param protocol the connection protocol to set
     */
    public void setProtocol(String protocol) {
        this.protocol = protocol;
        imaCF.put("Protocol", this.protocol);
    }

    /*
     * Set the TLS socket factory. 
     */
    public void setSecuritySocketFactory(String factory) {
        if ((factory != null) && (factory.trim().length() != 0)) {
            this.sFactory = factory;
            imaCF.put("SecuritySocketFactory", this.sFactory);
        }
    }
    
    /*
     * Set the TLS socket factory configuration. 
     */
    public void setSecurityConfiguration(String config) {
        if ((config != null) && (config.trim().length() != 0)) {
            this.sConfig = config;
            imaCF.put("SecurityConfiguration", this.sConfig);
        }
    }

    /*
     * Return the TLS socket factory setting
     */
    public String getSecuritySocketFactory() {
        if (sFactory == null)
            sFactory = imaCF.getString("SecuritySocketFactory");
        return sFactory;
    }
    
    /*
     * Return the TLS socket factory configuration setting
     */
    public String getSecurityConfiguration() {
        if (sConfig == null)
            sConfig = imaCF.getString("SecurityConfiguration");
        return sConfig;
    }

    /*
     * Return the IBM MessageSight JMS Connection Factory
     */
    public ImaConnectionFactory getImaFactory() {
        return imaCF;
    }

    /*
     * Validate the properties
     * TODO: This does not actually work
     * 
     * @see javax.resource.spi.ActivationSpec#validate()
     */
    public void validate() throws InvalidPropertyException {
        HashSet<PropertyDescriptor> invalidPDs = null;
        Set<String> keys = imaCF.propertySet();
        for (String key : keys) {
            try {
                imaCF.validate(key, true);
            } catch (JMSException e) {
                ImaTrace.traceException(2, e);
                try {
                    PropertyDescriptor pd = new PropertyDescriptor(key, null, null);
                    if (invalidPDs == null)
                        invalidPDs = new HashSet<PropertyDescriptor>();
                    invalidPDs.add(pd);
                } catch (IntrospectionException e1) {
                    ImaTrace.trace(2, "Failure on attempt to add "+key+" to list of invalid property descriptors");
                    ImaTrace.traceException(2, e1);
                }
            }
        }
        if (invalidPDs != null) {
            InvalidPropertyException ipe = new ImaInvalidPropertyException("CWLNC2321", "Validation of an activation specification failed due to errors found in the property settings for connection configuration: {0}. This error will occur with error CWLNC2340.", imaCF);
            /* Only trace the base exception here.  The properties will be included in the full list generated for the activation specification. */
            ImaTrace.traceException(1, ipe);
            ipe.setInvalidPropertyDescriptors(invalidPDs.toArray(new PropertyDescriptor[invalidPDs.size()]));
            throw ipe;
        }
    }

    protected abstract PropertyDescriptor[] getSetableProperites();
    public boolean equals(Object o) {
        if (this == o)
            return true;
        if ((o != null) && (o.getClass() == this.getClass())) {
            try {
                PropertyDescriptor[] props = getSetableProperites();
                for (int i = 0; i < props.length; i++) {
                    Method mthd = props[i].getReadMethod();
                    String s1 = (String) mthd.invoke(this);
                    String s2 = (String) mthd.invoke(o);
                    if (s1 != null) {
                        if (s1.equals(s2))
                            continue;
                        return false;
                    }
                    /* At this point s1 is null */
                    if (s2 == null)
                        continue;
                    return false;
                }
                return true;
            } catch (Exception ex) {
                throw new RuntimeException("Could not compare two objects", ex);
            }
        }
        return false;
    }

    public int hashCode() {
        int hc = hashCode.get();
        if (hc != 0)
            return hc;
        try {
            PropertyDescriptor[] props = getSetableProperites();
            for (int i = 0; i < props.length; i++) {
                Method mthd = props[i].getReadMethod();
                String s = (String) mthd.invoke(this);
                if (s != null) {
                    hc = hc * 17 + s.hashCode();
                }
            }
        } catch (Exception ex) {
            throw new RuntimeException("Failed to calculate hashcode", ex);
        }
        hashCode.compareAndSet(0, hc);
        return hashCode.get();
    }
    
    public ImaFactoryConfig cloneConfiguration() {
        Class<? extends ImaFactoryConfig> myClass = this.getClass();
        try {
            Constructor<? extends ImaFactoryConfig> ctor = myClass.getDeclaredConstructor();
            ImaFactoryConfig result = ctor.newInstance();
            PropertyDescriptor[] props = getSetableProperites();
            for (int i = 0; i < props.length; i++) {
                Method rdMthd = props[i].getReadMethod();
                String s = (String) rdMthd.invoke(this);
                Method wMthd = props[i].getWriteMethod();
                if (s != null)
                    wMthd.invoke(result, s);
            }
            return result;
        } catch (Exception ex) {
            throw new RuntimeException("Failed to clone configuration: " + this.toString(), ex);
        }
    }

    public String toString() {
        StringBuffer sb = new StringBuffer(1024);
        sb.append(this.getClass().getSimpleName()).append('@').append(super.hashCode()).append('{');
        try {
            PropertyDescriptor[] props = getSetableProperites();
            for (int i = 0; i < props.length; i++) {
                Method mthd = props[i].getReadMethod();
                String s = (String) mthd.invoke(this);
                if (s != null) {
                    String name = props[i].getName();
                    if ("password".equals(name))
                        s = "******";
                    sb.append(name).append('=').append(s).append(' ');
                }
            }
            sb.append('}');
            return sb.toString();
        } catch (Exception ex) {
            return (this.getClass().getSimpleName() + '@' + super.hashCode());
        }
    }
}
