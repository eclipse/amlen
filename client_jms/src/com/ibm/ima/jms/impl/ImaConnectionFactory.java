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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.QueueConnection;
import javax.jms.QueueConnectionFactory;
import javax.jms.TopicConnection;
import javax.jms.TopicConnectionFactory;
import javax.jms.XAConnection;
import javax.jms.XAConnectionFactory;
import javax.naming.Context;
import javax.naming.Name;
import javax.naming.NamingException;
import javax.naming.RefAddr;
import javax.naming.Reference;
import javax.naming.Referenceable;
import javax.naming.spi.ObjectFactory;

import com.ibm.ima.jms.ImaProperties;

/**
 * Implement the ConnectionFactory and TopicConnectionFactory interfaces for the IBM MessageSight JMS client.
 *
 * This object 
 */
public class ImaConnectionFactory extends ImaPropertiesImpl 
 implements ConnectionFactory, TopicConnectionFactory,
        QueueConnectionFactory, XAConnectionFactory,
        ImaProperties, Serializable, Referenceable, ObjectFactory {
    
    private static final long serialVersionUID = -8052665599811013060L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    public ImaConnectionFactory() {
        super(ImaJms.PTYPE_Connection);
    }
    
    
    /* 
     * Create a connection.
     * 
     * @see javax.jms.ConnectionFactory#createConnection()
     */
    public Connection createConnection() throws JMSException {
        ImaConnection ret = new ImaConnection(this, null, null, ImaJms.Common, -1);
        ret.props.put("ObjectType", "common");            
        return (Connection)ret;
    }

    /* 
     * Create a connection with a userid and password.
     * 
     * @see javax.jms.ConnectionFactory#createConnection(java.lang.String, java.lang.String)
     */
    public Connection createConnection(String userid, String password) throws JMSException {
        ImaConnection ret = new ImaConnection(this, userid, password, ImaJms.Common, -1);
        ret.props.put("ObjectType", "common");
        return (Connection)ret;
    }

    /* 
     * @see javax.jms.TopicConnectionFactory#createTopicConnection()
     */
    public TopicConnection createTopicConnection() throws JMSException {
        ImaConnection ret = new ImaConnection(this, null, null, ImaJms.Topic, -1);
        ret.props.put("ObjectType", "topic");
        return (TopicConnection)ret;
    }

    /* 
     * @see javax.jms.TopicConnectionFactory#createTopicConnection(java.lang.String, java.lang.String)
     */
    public TopicConnection createTopicConnection(String userid, String password) throws JMSException {
        ImaConnection ret = new ImaConnection(this, userid, password, ImaJms.Topic, -1);
        ret.props.put("ObjectType", "topic");
        return (TopicConnection)ret;
    }

    /*
     * 
     * @see javax.jms.QueueConnectionFactory#createQueueConnection()
     */
    public QueueConnection createQueueConnection() throws JMSException {
        ImaConnection ret = new ImaConnection(this, null, null, ImaJms.Queue, -1);
        ret.props.put("ObjectType", "queue");
        return (QueueConnection)ret;
    }

    /*
     * 
     * @see javax.jms.QueueConnectionFactory#createQueueConnection(java.lang.String, java.lang.String)
     */
    public QueueConnection createQueueConnection(String userid, String password) throws JMSException {
        ImaConnection ret = new ImaConnection(this, userid, password, ImaJms.Queue, -1);
        ret.props.put("ObjectType", "queue");
        return (QueueConnection)ret;
    }
    
    /* 
     * Create a connection with a userid and password.
     * 
     * @see javax.jms.ConnectionFactory#createConnection(java.lang.String, java.lang.String)
     */
    public Connection createConnection(String userid, String password, int trclevel) throws JMSException {
        ImaConnection ret = new ImaConnection(this, userid, password, ImaJms.Common, trclevel);
        ret.props.put("ObjectType", "common");
        return (Connection)ret;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XAConnectionFactory#createXAConnection()
     */
    public XAConnection createXAConnection() throws JMSException {
        return createXAConnection(null, null);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XAConnectionFactory#createXAConnection(java.lang.String, java.lang.String)
     */
    public XAConnection createXAConnection(String userid, String password) throws JMSException {
        return (XAConnection) createConnection(userid, password);
    }
    
    /* 
     * MessageSight contructor that allows a connection specific trace level to
     * be set.
     */
    public XAConnection createXAConnection(String userid, String password, int trclevel) throws JMSException {
        return (XAConnection) createConnection(userid, password, trclevel);
    }

    /*
     * Create a reference to store in JNDI providers which do not accept serialized objects.
     * This is a simple form of serialization use by JNDI providers such as RefFSContext.
     * @see javax.naming.Referenceable#getReference()
     */
    public Reference getReference() throws NamingException {
        Reference ref = new Reference(ImaConnectionFactory.class.getName(), ImaConnectionFactory.class.getName(), null);
        Iterator <String> it = props.keySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            RefAddr refa = getRefAddr(key);
            if (refa != null)
                ref.add(refa);
        }
        return ref;
    }

    /*
     * Create an object from a reference.
     * @see javax.naming.spi.ObjectFactory#getObjectInstance(java.lang.Object, javax.naming.Name, javax.naming.Context, java.util.Hashtable)
     */
    public Object getObjectInstance(Object obj, Name name, Context nameCtx, Hashtable<?, ?> environment) throws Exception {
        Reference ref = (Reference)obj;
        ImaConnectionFactory cf = new ImaConnectionFactory();
        Enumeration <RefAddr> prp = ref.getAll();
        while (prp.hasMoreElements()) {
            cf.putRefAddr(prp.nextElement());
        }
        return cf;
    }
    
    /*
     * Stub for close allowed check
     */
    static public void checkAllowed(ImaConnection connect) throws JMSException {
    }

    public String getClassName() {
    	return "ImaConnectionFactory";
    }
    

}
