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

import java.util.Map;
import java.util.Set;

import javax.jms.JMSException;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaProperties;

/*
 * Add a thin wrapper over ImaPropertiesImpl to support a set of read only properties.
 * Read only properties are used for the objects which are not placed in JNDI including
 * Connection, Session, MessageConsumer, and MessageProducer.
 */
public class ImaReadonlyProperties implements ImaProperties {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaProperties props;
    
    /*
     * Constructor with a properties object.  
     * The properties object should never be null.
     */
    public ImaReadonlyProperties(ImaProperties props) {
        this.props = props;
    }

    /*
     * We do not allow a valid property to be added to one of the read only property objects.
     * @see com.ibm.ima.jms.ImaProperties#addValidProperty(java.lang.String, java.lang.String)
     */
    public void addValidProperty(String name, String validator) {
    	IllegalStateException iex = new ImaRuntimeIllegalStateException("CWLNC0318", "A method call to set, add, or remove ImaProperties values failed because the properties are read only.");
        ImaTrace.traceException(2, iex);
        throw iex;
    }

    /*
     * We do not allow the properties of a read only property object to be cleared.
     * @see com.ibm.ima.jms.ImaProperties#clear()
     */
    public void clear() throws JMSException {
    	IllegalStateException iex = new ImaRuntimeIllegalStateException("CWLNC0318", "A method call to set, add, or remove ImaProperties values failed because the properties are read only.");
        ImaTrace.traceException(2, iex);
        throw iex;
    }

    /*
     * Property exists.
     * @see com.ibm.ima.jms.ImaProperties#exists(java.lang.String)
     */
    public boolean exists(String name) {
        return props.exists(name);
    }

    /*     * 
     * Get a property.
     * @see com.ibm.ima.jms.ImaProperties#get(java.lang.String)
     */
    public Object get(String name) {
        return props.get(name);
    }

    /*
     * Get an integer form of a property.
     * This also handles various enumeration forms.
     * @see com.ibm.ima.jms.ImaProperties#getInt(java.lang.String, int)
     */
    public int getInt(String name, int defaultValue) {
        return props.getInt(name, defaultValue);
    }
    
    public boolean getBoolean(String name, boolean defaultValue) {
    	return props.getBoolean(name, defaultValue);
    }

    /*
     * Get the string value of a property.
     * All properties can be reduced to a string, but this is not very meaningful for complex objects.
     * @see com.ibm.ima.jms.ImaProperties#getString(java.lang.String)
     */
    public String getString(String name) {
        return getString(name,null);
    }

    public String getString(String name, String defaultString) {
        String result = props.getString(name);
        if (result != null)
        	return result;
        return defaultString;
    }

    /*     * 
     * Return the set of properties.
     * @see com.ibm.ima.jms.ImaProperties#propertySet()
     */
    public Set<String> propertySet() {
        return props.propertySet();
    }

    /*     * 
     * Set a property.
     * This normally does not work (hence read only) but you can modify user properties.
     * User properties are not constrained to serializable as we never convert a read only
     * property object to a reference. 
     * @see com.ibm.ima.jms.ImaProperties#put(java.lang.String, java.lang.Object)
     */
    public Object put(String name, Object value) throws JMSException {
        if (name != null) {
            String lowername = name.toLowerCase();
            if (lowername.indexOf("user") >= 0 && !lowername.equals("userid")) {
                ImaPropertiesImpl xprop = (ImaPropertiesImpl)this.props;
                return xprop.props.put(xprop.fixName(name), value);       /* Allow a non-serializable object */
            }  
        }
        IllegalStateException iex = new ImaRuntimeIllegalStateException("CWLNC0318", "A method call to set, add, or remove ImaProperties values failed because the properties are read only.");
        ImaTrace.traceException(2, iex);
        throw iex;
    }
    
    /*
     * Package private method to allow any value to be modified.
     * This overcomes both the readonly check and the serializable check.
     * This is used to set the status values which are not serializable.
     */
    void putInternal(String name, Object value) {
        ((ImaPropertiesImpl)this.props).props.put(name, value);
    }

    /*
     * We do not allow putAll on a read only property object.
     * @see com.ibm.ima.jms.ImaProperties#putAll(java.util.Map)
     */
    public void putAll(Map<String, Object> map) {
    	IllegalStateException iex = new ImaRuntimeIllegalStateException("CWLNC0318", "A method call to set, add, or remove ImaProperties values failed because the properties are read only.");
        ImaTrace.traceException(2, iex);
        throw iex;
    }

    /*
     * Remove a property.
     * This is only allowed for user properties.
     * @see com.ibm.ima.jms.ImaProperties#remove(java.lang.String)
     */
    public Object remove(String name) {
        if (name.toLowerCase().indexOf("user") >= 0)
            return props.remove(name);
        IllegalStateException iex = new ImaRuntimeIllegalStateException("CWLNC0318", "A method call to set, add, or remove ImaProperties values failed because the properties are read only.");
        ImaTrace.traceException(2, iex);
        throw iex;
    }

    /*
     * Return the number of items in the property set.
     * @see com.ibm.ima.jms.ImaProperties#size()
     */
    public int size() {
        return props.size();
    }

    /*
     * Validate all properties.
     * Force the validate to not return warnings as setting a system property generates a warning.
     * @see com.ibm.ima.jms.ImaProperties#validate(boolean)
     */
    public ImaJmsException [] validate(boolean nowarn) {
        return props.validate(true);
    }

    /*     * 
     * Validate a property.
     * FForce the validate to not return warnings as setting a system property generates a warning.
     * @see com.ibm.ima.jms.ImaProperties#validate(java.lang.String, boolean)
     */
    public void validate(String name, boolean nowarn) throws JMSException {
        props.validate(name, true);
    }

    /*
     * Validate a potential property.
     * Force the validate to not return warnings as setting a system property generates a warning.
     * @see com.ibm.ima.jms.ImaProperties#validate(java.lang.String, java.lang.Object, boolean)
     */
    public void validate(String name, Object value, boolean nowarn) throws JMSException {
        props.validate(name, value, true);
    }
    
    /*
     * Default classname.  This should not be used.
     */
    public String getClassName() {
    	return "ImaReadonlyProperties";
    }
    
    /*
     * Default into
     */
    public String getInfo() {
    	return null;
    }
    
    /*
     * Default details to null 
     */
    public String getDetails() {
    	return null;
    }
    
    
    /*
     * Default object to this
     */
    public String toString(int opt) {
    	return toString(opt, this);
    }
    
    /*
     * Put out the detailed tostring for the subclassed object
     */
    public String toString(int opt, ImaReadonlyProperties props) {
    	StringBuffer sb = new StringBuffer();
    	boolean started = false;
    	
    	if (props == null) {
    		return "";	
    	}
    	
    	if ((opt & ImaConstants.TS_Class) != 0) {
    		sb.append(props.getClassName());
    		started = true;
    	}
    	if ((opt & ImaConstants.TS_HashCode) != 0) {
    		sb.append('@').append(Integer.toHexString(hashCode()));
    		started = true;
    	}
    	
    	if ((opt & (ImaConstants.TS_Info|ImaConstants.TS_Properties) ) != 0) {
    		if (started)
    			sb.append(' ');
    		started = true;
    		String info = props.getInfo();
    		if (info != null) {
    			sb.append(info).append(' ');
    		}
    		sb.append("properties=").append(props.props);
    	}

    	if ((opt & ImaConstants.TS_Details) != 0) {
    		String details = props.getDetails();
    		if (details != null) {
	    		if (started)
	    			sb.append("\n  ");
	    		started = true;
	    		sb.append(details);
    		}
    	}

    	return sb.toString();
    }
}
