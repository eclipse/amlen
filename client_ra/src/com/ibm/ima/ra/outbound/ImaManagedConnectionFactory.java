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

import java.beans.PropertyDescriptor;
import java.io.PrintWriter;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.Set;
import java.util.Timer;
import java.util.concurrent.atomic.AtomicInteger;

import javax.jms.Connection;
import javax.jms.JMSException;
import javax.resource.ResourceException;
import javax.resource.spi.ConnectionManager;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionFactory;
import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.ResourceAdapterAssociation;
import javax.resource.spi.TransactionSupport;
import javax.resource.spi.ValidatingManagedConnectionFactory;
import javax.resource.spi.security.PasswordCredential;
import javax.security.auth.Subject;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Pattern;

import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.ra.ImaResourceAdapter;
import com.ibm.ima.ra.common.ImaFactoryConfig;
import com.ibm.ima.ra.common.ImaResourceException;
import com.ibm.ima.ra.common.ImaUtils;

public class ImaManagedConnectionFactory extends ImaFactoryConfig implements ResourceAdapterAssociation,
        ValidatingManagedConnectionFactory, ManagedConnectionFactory, TransactionSupport {
    private static final long serialVersionUID        = 5109972631231820067L;
    private static final ImaManagedConnectionFactoryBeanInfo beanInfo = new ImaManagedConnectionFactoryBeanInfo();
    private PrintWriter       logWriter               = null;
    private ImaResourceAdapter resourceAdapter         = null;
    
    @NotNull (message="{CWLNC2026}")
    @Pattern (regexp="(auto|automatic|bytes|text)", flags = Pattern.Flag.CASE_INSENSITIVE, 
            message="{CWLNC2026}")
    String                    convertMessageType      = "auto";
    String                    temporaryQueue          = null;
    String                    temporaryTopic          = null;
    @NotNull(message= "{CWLNC2041}")
    @Pattern(regexp = "(NoTransaction|LocalTransaction|XATransaction)", flags = Pattern.Flag.CASE_INSENSITIVE,
             message= "{CWLNC2041}")
    String transactionSupportLevel = "XATransaction";
    
    protected int             traceLevel              = -1;
    protected AtomicInteger   defaultTraceLevel       = new AtomicInteger(-1);

    /**
	 * 
	 */
    public ImaManagedConnectionFactory() {
        // TODO Auto-generated constructor stub
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ResourceAdapterAssociation#getResourceAdapter()
     */
    public ResourceAdapter getResourceAdapter() {
        return resourceAdapter;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ResourceAdapterAssociation#setResourceAdapter(javax .resource.spi.ResourceAdapter)
     */
    public void setResourceAdapter(ResourceAdapter ra) throws ResourceException {
        if (ra instanceof ImaResourceAdapter) {
            resourceAdapter = (ImaResourceAdapter) ra;
            defaultTraceLevel.set(resourceAdapter.defaultTraceLevel());
            return;
        }
        ImaResourceException re = new ImaResourceException("CWLNC2172","The ResourceAdapter object {1} could not be associated with a managed connection because it is not an IBM MessageSight object. Managed connection: {0}. This might indicate a problem with the application server.", this, ra.getClass().getName());
        ImaTrace.traceException(1, re);
        throw re;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ValidatingManagedConnectionFactory#getInvalidConnections (java.util.Set)
     */
    @SuppressWarnings("rawtypes")
    public Set getInvalidConnections(Set set) throws ResourceException {
        // TODO Auto-generated method stub
        return null;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionFactory#createConnectionFactory(javax .resource.spi.ConnectionManager)
     */
    public Object createConnectionFactory(ConnectionManager cm) throws ResourceException {
        return new ImaConnectionFactoryProxy(this, cm);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionFactory#createConnectionFactory()
     */
    public Object createConnectionFactory() throws ResourceException {
        return new ImaConnectionFactoryProxy(this, null);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionFactory#createManagedConnection(javax .security.auth.Subject,
     * javax.resource.spi.ConnectionRequestInfo)
     */
    public ManagedConnection createManagedConnection(Subject subj, ConnectionRequestInfo cri) throws ResourceException {
        ImaConnection con = createImaConnection(subj, cri);
        return new ImaManagedConnection(this, subj, cri, logWriter, con);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionFactory#matchManagedConnections(java .util.Set,
     * javax.security.auth.Subject, javax.resource.spi.ConnectionRequestInfo)
     */
    @SuppressWarnings("rawtypes")
    public ManagedConnection matchManagedConnections(final Set conSet, final Subject subj,
            final ConnectionRequestInfo cri) throws ResourceException {
        if ((conSet != null) && !conSet.isEmpty()) {
            /* if a CRI is supplied, it takes priority */
            if (cri != null) {
                for (Object obj : conSet) {
                    if (obj instanceof ImaManagedConnection) {
                        ImaManagedConnection imc = (ImaManagedConnection) obj;
                        if (imc.inTransaction())
                            continue;
                        if (cri.equals(imc.getConnectionRequestInfo())) {
                            if (ImaTrace.isTraceable(5))
                                ImaTrace.trace(5, "Matched connection was found for cri=" + cri.toString() + " connection="
                                        + imc.toString());
                            return imc;
                        }
                    }
                }
                if (ImaTrace.isTraceable(5))
                    ImaTrace.trace(5, "Matched connection was not found for cri=" + cri.toString());
                return null;
            }
            /* no CRI, a supplied Subject now takes priority */
            if (subj != null) {
                /*
                 * equals check on the security Subject must be performed inside of a privileged action.
                 */
                ManagedConnection result = AccessController.doPrivileged(new PrivilegedAction<ManagedConnection>() {
                    @Override
                    public ManagedConnection run() {
                        for (Object obj : conSet) {
                            if (obj instanceof ImaManagedConnection) {
                                ImaManagedConnection imc = (ImaManagedConnection) obj;
                                if (imc.inTransaction())
                                    continue;
                                if (subj.equals(imc.getSubject())) {
                                    if (ImaTrace.isTraceable(5))
                                        ImaTrace.trace(5, "Matched connection was found for subj=" + subj.toString()
                                                + " connection=" + imc.toString());
                                    return imc;
                                }
                            }
                        }
                        if (ImaTrace.isTraceable(5))
                            ImaTrace.trace(5, "Matched connection was not found for subj=" + subj.toString());
                        return null;
                    }

                });
                return result;
            }
            /* no CRI and no Subject were provided */
            for (Object obj : conSet) {
                if (obj instanceof ImaManagedConnection) {
                    ImaManagedConnection imc = (ImaManagedConnection) obj;
                    if (imc.inTransaction())
                        continue;
                    if ((imc.getConnectionRequestInfo() == null) && (imc.getSubject() == null)) {
                        ImaTrace.trace(5, "Matched connection was found: connection=" + imc.toString());
                        return imc;
                    }
                }
            }
            if (ImaTrace.isTraceable(5))
                ImaTrace.trace(5, "No matched connection was not found for null subject and cri");
        }
        return null;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionFactory#setLogWriter(java.io.PrintWriter )
     */
    public void setLogWriter(PrintWriter writer) throws ResourceException {
        logWriter = writer;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnectionFactory#getLogWriter()
     */
    public PrintWriter getLogWriter() throws ResourceException {
        return logWriter;
    }

    public TransactionSupportLevel getTransactionSupport() {
        return TransactionSupportLevel.valueOf(transactionSupportLevel);
    }
    /**
     * @return the transactionSupportLevel
     */
    public String getTransactionSupportLevel() {
        return transactionSupportLevel;
    }

    /**
     * @param transactionSupportLevel the transactionSupportLevel to set
     */
    public void setTransactionSupportLevel(String transactionSupportLevel) {
        this.transactionSupportLevel = transactionSupportLevel;
    }

    ImaConnection createImaConnection(final Subject subj, ConnectionRequestInfo cri) throws ResourceException {
        // defined username and password
        String user = null;
        String pwd = null;
        // use ConnectionRequestInfo object in preference to anything else
        if (cri != null) {
            if (cri instanceof ImaConnectionRequestInfo) {
                ImaConnectionRequestInfo icri = (ImaConnectionRequestInfo) cri;
                user = icri.getUsername();
                pwd = icri.getPassword();
            } else {
                ImaResourceException re = new ImaResourceException("CWLNC2162", "Failed to establish a physical connection to MessageSight because the ConnectionRequestInfo object {1} is not an IBM MessageSight object.  This might indicate a problem with the application server. Managed connection: {0}.",cri.getClass().getName(), this);
                ImaTrace.traceException(1, re);
                throw re;
            }
        } else {
            if (subj != null) {
                // we need an anonymous inner class to get PasswordCredentials from the Subject
                PrivilegedAction<PasswordCredential> pa = new PrivilegedAction<PasswordCredential>() {
                    @Override
                    public PasswordCredential run() {
                        // Set of PasswordCredentials contained in the Subject
                        Set<PasswordCredential> pcSet = subj.getPrivateCredentials(PasswordCredential.class);
                        // iterate over the PasswordCredentials, looking
                        // for this MCF
                        for (PasswordCredential pc : pcSet) {
                            if (this.equals(pc.getManagedConnectionFactory()))
                                return pc;
                        }
                        return null;
                    }
                };
                PasswordCredential passwordCredentials = AccessController.doPrivileged(pa);
                // if we found something, update the username and password
                if (passwordCredentials != null) {
                    user = passwordCredentials.getUserName();
                    pwd = new String(passwordCredentials.getPassword());
                }
            }
        }
        if (user == null) {
            user = getUser();
            pwd = getPassword();
        }
        try {
            int connTraceLevel = Integer.valueOf(getTraceLevel());
            /* Neither the connection factory configuration nor the RA configuration
             * have supplied a value so use default of 4.  If they want 0, they
             * must set 0.
             */
            if (connTraceLevel == -1)
                connTraceLevel = 4;
            ImaTrace.trace(1, "Setting trace level for " + imaCF + " to "+connTraceLevel);
            Connection conn = imaCF.createConnection(user, pwd, (connTraceLevel + 10));
            ImaUtils.checkServerCompatibility(conn);
            return (ImaConnection) conn;
        } catch (JMSException e) {
            ImaResourceException re = new ImaResourceException("CWLNC2166", e, "Failed to establish a physical connection to MessageSight for a managed connection due to an exception. Connection configuration: {0}", imaCF);
            ImaTrace.traceException(1, re);
            throw re;
        }
    }
    
    /**
     * Return the message conversion type.
     * @return the message conversion type
     */
    public String getConvertMessageType() {
        return convertMessageType;
    }
    
    /**
     * Set the message conversion type.
     * @param cvttype
     */
    public void setConvertMessageType(String cvttype) {
        if (cvttype != null && cvttype.length()>0) {
            if ("auto".equalsIgnoreCase(cvttype) || "automatic".equalsIgnoreCase(cvttype))
                this.convertMessageType = "auto";
            else if ("text".equalsIgnoreCase(cvttype))
                this.convertMessageType = "text";
            else
                this.convertMessageType = "bytes";
        }
    }
    
    /**
     * Return the temporary queue prototype.
     * @return the temporary queue prototype
     */
    public String getTemporaryQueue() {
        return temporaryQueue;
    }
    
    /**
     * Set the temporary queue prototype.
     * @param tmpQProto
     */
    public void setTemporaryQueue(String tmpQProto) {
        temporaryQueue = tmpQProto;
    }
    
    /**
     * Return the temporary topic prototype.
     * @return the temporary topic prototype
     */
    public String getTemporaryTopic() {
        return temporaryTopic;
    }
    
    /**
     * Set the temporary topic prototype.
     * @param tmpTProto
     */
    public void setTemporaryTopic(String tmpTProto) {
        temporaryTopic = tmpTProto;
    }
    
    /**
     * @return the logLevel
     */
    public String getTraceLevel() {
        if (traceLevel > -1)
            return String.valueOf(traceLevel);
        if (resourceAdapter != null) {
            defaultTraceLevel.set(((ImaResourceAdapter)resourceAdapter).defaultTraceLevel());
            return String.valueOf(defaultTraceLevel.get());
        }
        return String.valueOf(traceLevel);
    }

    /**
     * @param logLevel the logLevel to set
     */
    public void setTraceLevel(String ll) {
        try {
            traceLevel = Integer.parseInt(ll);
        } catch (NumberFormatException ne) {
            ImaTrace.trace(2, "\'"+ ll + "\' is not a valid value for traceLevel. Valid values are -1 through 9. Setting traceLevel to default.");
            if (resourceAdapter != null)
                defaultTraceLevel.set(((ImaResourceAdapter)resourceAdapter).defaultTraceLevel());
        }
        if ((traceLevel < -1) || (traceLevel > 9)) {
            ImaTrace.trace(2, "The traceLevel setting must be in the range of 0 to 9 (or -1 for default setting).  Resetting " + traceLevel +" to default.");
            traceLevel = -1;
        }
    }

    Timer getTimer() {
        if (resourceAdapter != null)
            return resourceAdapter.getTimer();
        return new Timer("RAOutboundTimer", true);
    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.ima.ra.common.ImaFactoryConfig#getSetableProperites()
     */
    protected PropertyDescriptor[] getSetableProperites() {
        return beanInfo.getPropertyDescriptors();
    }
    
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
