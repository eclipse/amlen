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

import java.io.Serializable;
import java.util.HashMap;
import java.util.Iterator;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.naming.Referenceable;

import com.ibm.ima.jms.ImaProperties;

/**
 * Implement the Destination, Topic, and Queue interface for the IBM MessageSight JMS client.
 * This is an administered object and must be serializable.
 * <p>
 * The name of a Destination is a String which describes the topic.  This is often 
 * different from the name under which it is stored in JNDI.   
 */
abstract public class ImaDestination extends ImaPropertiesImpl implements Destination, Referenceable, Serializable, ImaProperties {

    private static final long serialVersionUID = -2664726340677810792L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /*
     * Constructor with a name
     */
    public ImaDestination(String name) {
        super(ImaJms.PTYPE_Destination);
    
        if (name != null) 
            initDestination(name);
    }
    
    
    /*
     * Constructor with no name.
     */
    public ImaDestination() {
        this(null);
    }
    
    /*
     * Get the name 
     */
    public String getName() {
        Object ret = props.get("Name");
        if (ret == null)
            return null;
        return ""+ret;
    }
    
    /*
     * Set the name 
     */
    public void setName(String name) {
        props.put("Name", name);
    }
    
    /*
     * Get the disable ACK 
     */
 //   public boolean getDisableACK() {
 //       return getBoolean("DisableACK", false);
 //   }
    
    /*
     * Set the disable ACK
     */
 //   public void setDisableACK(boolean disableAck) {
 //       put("DisableACK", disableAck);
 //   }
    /*
     * Get the client message cache
     */
 //   public int getClientMessageCache() {
 //       return getInt("ClientMessageCache", -1);
 //   }
    
    /*
     * Set the client message cache 
     */
 //   public void setClientMessageCache(int clientCache) {
 //       putInternal("ClientMessageCache", Integer.valueOf(clientCache));
 //   }
    
    
    /*
     * Initialize the destination name.
     * This is common code so that the exception is raised in only one location.
     */
    void initDestination(String name) {
    	// NOTE: This method assumes a non-null value for name has been provided
        props.put("Name", name);
    }

    
    /*
     * Allow the property name to be in any case 
     */
    String fixName(String name) {
        String goodname = ImaJms.getCasemap(proptype).get(name.toLowerCase());
        return goodname != null ? goodname : name; 
    }
    
 
    /*
     * Get the current set of properties 
     */
    public ImaProperties getCurrentProperties() throws JMSException {
        ImaPropertiesImpl props = new ImaPropertiesImpl(ImaJms.PTYPE_Destination);
        props.putAll(this.props);
        
        return (ImaProperties)props;
    }
    

    /*
     * Check for equality of destinations.
     * The two objects must have the same properties, but the properties
     * could have changed type as long as the value is the same.
     * Only
     * @see java.lang.Object#equals(java.lang.Object)
     */
    public boolean equals(Object obj) {
        if (!(obj instanceof ImaDestination))
            return false;
        
        HashMap <String,Object> props2;
        if (props.size() != ((ImaProperties)obj).size())
            return false;
        
        props2 = ((ImaPropertiesImpl)obj).props;
        synchronized (props) {
            Iterator <String>it = props.keySet().iterator();
            while (it.hasNext()) {
                String key = it.next();
                Object obj1 = props.get(key);
                Object obj2 = props2.get(key);
                if (obj2 == null)
                    return false;
                if (obj1 instanceof String || obj1 instanceof Number) {
                    if (!(""+obj1).equals(""+obj2)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    
    /*
     * @see java.lang.Object#hashCode()
     */
    public int hashCode() {
        return 0;
    }
     
}
