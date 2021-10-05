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
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;

import javax.jms.JMSException;
import javax.jms.Topic;
import javax.naming.Context;
import javax.naming.Name;
import javax.naming.NamingException;
import javax.naming.RefAddr;
import javax.naming.Reference;
import javax.naming.Referenceable;
import javax.naming.StringRefAddr;
import javax.naming.spi.ObjectFactory;

/**
 *
 */
public class ImaTopic extends ImaDestination implements Topic, Serializable, Referenceable, ObjectFactory { 
    /**
	 * 
	 */
	private static final long serialVersionUID = 7047734562172176037L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

	
	public ImaTopic(String name) {
		super(name);
		props.put("ObjectType", "topic");
	}
    
    /*
     * Constructor with no name.
     */
    public ImaTopic() {
        this(null);
    }

	/*
     * Get the name from which the topic was constructed
     */
    public String getTopicName() throws JMSException {
        Object ret = props.get("Name");
        if (ret == null) {
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0007", "An operation that requires the destination name to be set failed because the destination name is not set.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        return ret.toString();
    }
    
    /*
     * Create a reference to store in JNDI providers which do not accept serialized objects.
     * This is a simple form of serialization use by JNDI providers such as RefFSContext.
     * @see javax.naming.Referenceable#getReference()
     */
    public Reference getReference() throws NamingException {
        if (this instanceof ImaTemporaryTopic) {
        	ImaNamingException nex = new ImaNamingException("CWLNC0016", "An operation to store a TemporaryTopic or a TemoraryQueue in a JNDI repository failed because temporary destinations are runtime objects only.");
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        Reference ref = new Reference(ImaTopic.class.getName(), ImaTopic.class.getName(), null);
        Iterator <String> it = props.keySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            if (!("ObjectType".equals(key))) {
	            RefAddr refa = getRefAddr(key);
	            if (refa != null)
	                ref.add(refa);
            }
        }
        return ref;
    }
    

    /*
     * Create an object from the reference.
     * @see javax.naming.spi.ObjectFactory#getObjectInstance(java.lang.Object, javax.naming.Name, javax.naming.Context, java.util.Hashtable)
     */
    public Object getObjectInstance(Object obj, Name name, Context nameCtx, Hashtable<?, ?> environment) throws Exception {
        Reference ref = (Reference)obj;
        RefAddr rname = ref.get("Name");
        /* The name is required so return an error if it is not set */
        if (rname == null || !(rname instanceof StringRefAddr))
            return null;
        String dname = (String)rname.getContent();
        if (dname.length()>1 && dname.charAt(1)==':')
            dname = dname.substring(2);
        ImaTopic dest = new ImaTopic(dname);
        /* Set each of the properties */
        Enumeration <RefAddr> prp = ref.getAll();
        while (prp.hasMoreElements()) {
            RefAddr sri = (RefAddr)prp.nextElement();
            if (!("Name".equals(sri.getType()) && !("ObjectType".equals(sri.getType()))))     /* Do not put the Name property */
                dest.putRefAddr(sri);
        }
        return dest;
    } 
    
    
    /*
     * 
     * @see java.lang.Object#toString()
     */
    public String toString() {
        Object obj = props.get("Name");
        return obj==null ? "IMATopic@"+hashCode() : "ImaTopic:"+obj;
    }
    
    /*
     * (non-Javadoc)
     * @see com.ibm.ima.jms.impl.ImaPropertiesImpl#getClassName()
     */
    public String getClassName() {
    	return "ImaTopic";
    }
    
    /*
     * (non-Javadoc)
     * @see com.ibm.ima.jms.impl.ImaPropertiesImpl#toString(int)
     */
    public String toString(int opt) {
    	return toString(opt, this);
    }
}
