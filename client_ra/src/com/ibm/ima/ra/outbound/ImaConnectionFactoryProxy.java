
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

import java.io.Serializable;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.QueueConnection;
import javax.jms.QueueConnectionFactory;
import javax.jms.TopicConnection;
import javax.jms.TopicConnectionFactory;
import javax.naming.NamingException;
import javax.naming.Reference;
import javax.resource.Referenceable;
import javax.resource.ResourceException;
import javax.resource.spi.ConnectionManager;

import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;
import com.ibm.ima.jms.impl.ImaNamingException;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.ra.common.ImaUtils;

final class ImaConnectionFactoryProxy implements ConnectionFactory, TopicConnectionFactory, QueueConnectionFactory,
        Serializable, Referenceable {
    private static final long                 serialVersionUID = -8350205817861362843L;
    private final ImaManagedConnectionFactory managedCF;
    private final ConnectionManager           conMgr;
    private Reference                         reference        = null;

    /**
	 * 
	 */
    ImaConnectionFactoryProxy(ImaManagedConnectionFactory mcf, ConnectionManager cm) {
        managedCF = mcf;
        conMgr = cm;
    }

    @Override
    public Connection createConnection() throws JMSException {
        return createConnection(null, null);
    }

    @Override
    public Connection createConnection(String username, String password) throws JMSException {
        if (conMgr == null)
            return createUnmanagedConnection(username, password);
        return createManagedConnection(username, password);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.QueueConnectionFactory#createQueueConnection()
     */
    public QueueConnection createQueueConnection() throws JMSException {
        return (QueueConnection) createConnection(null, null);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.QueueConnectionFactory#createQueueConnection(java.lang.String, java.lang.String)
     */
    public QueueConnection createQueueConnection(String user, String pwd) throws JMSException {
        return (QueueConnection) createConnection(user, pwd);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.TopicConnectionFactory#createTopicConnection()
     */
    public TopicConnection createTopicConnection() throws JMSException {
        return (TopicConnection) createConnection(null, null);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.TopicConnectionFactory#createTopicConnection(java.lang.String, java.lang.String)
     */
    public TopicConnection createTopicConnection(String user, String pwd) throws JMSException {
        return (TopicConnection) createConnection(user, pwd);
    }

    private Connection createUnmanagedConnection(String username, String password) throws JMSException {
        int connTraceLevel = Integer.valueOf(managedCF.getTraceLevel());
        /* Neither the connection factory configuration nor the RA configuration
         * have supplied a value so use default of 4.  If they want 0, they
         * must set 0.
         */
        if (connTraceLevel == -1)
            connTraceLevel = 4;
        Connection conn = managedCF.getImaFactory().createConnection(username, password, (connTraceLevel + 10));
        ImaUtils.checkServerCompatibility(conn);
        return conn;
    }

    private Connection createManagedConnection(String username, String password) throws JMSException {
        // connection request info object to send (will be null unless there is a username/password to send as part of
        // application-managed sign-on see 9.1.7)
        ImaConnectionRequestInfo cri = (username == null) ? null : new ImaConnectionRequestInfo(username, password);
        try {
            Connection con;
            con = (Connection) conMgr.allocateConnection(managedCF, cri);
            return con;
        } catch (ResourceException e) {
            ImaJmsExceptionImpl ex = new ImaJmsExceptionImpl("CWLNC2560", e, "Failed to create a managed connection. Connection allocation failed due to an exception.");
            ImaTrace.traceException(1, ex);
            throw ex;
        }
    }

    public Reference getReference() throws NamingException {
        // 17.5.3 requires us to throw an exception if the reference is null
        if (reference == null) {
            ImaNamingException ne = new ImaNamingException("CWLNC2660", "Failed to retrieve the connection factory because the connection factory reference was not set by the application server. This might indicate a problem with the application server.");
            ImaTrace.traceException(1, ne);
            throw ne;
        }
        // otherwise return the reference
        return reference;
    }

    /**
     * Set a reference. This is called by the server deployment code.
     */
    public void setReference(Reference ref) {
        reference = ref;
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


}
