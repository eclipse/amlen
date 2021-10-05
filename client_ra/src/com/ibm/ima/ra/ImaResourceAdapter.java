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

package com.ibm.ima.ra;

import java.lang.reflect.Constructor;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.HashMap;
import java.util.Timer;
import java.util.concurrent.atomic.AtomicInteger;

import javax.resource.ResourceException;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.BootstrapContext;
import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.ResourceAdapterInternalException;
import javax.resource.spi.UnavailableException;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.resource.spi.work.WorkManager;
import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
//import javax.validation.constraints.Max;
//import javax.validation.constraints.Min;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Pattern;
import javax.validation.constraints.Size;

import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.jms.impl.ImaTraceImpl;
import com.ibm.ima.ra.common.ImaResourceException;
import com.ibm.ima.ra.common.ImaWASTraceComponent;
import com.ibm.ima.ra.inbound.ImaActivationSpec;
import com.ibm.ima.ra.inbound.ImaConnectionPool;
import com.ibm.ima.ra.inbound.ImaMessageEndpoint;
import com.ibm.ima.ra.inbound.ImaMessageEndpointNonAsf;

public class ImaResourceAdapter implements ResourceAdapter {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    static {
        ImaTrace.init(true);
    }
    
    /**
     * reference to the BootstrapContext passed to us by the Application Server.
     */
    private BootstrapContext        bootstrapContext   = null;

    /**
     * reference to the WorkManager passed to us by the Application Server.
     */
    private WorkManager             workManager        = null;
    
    /**
     * timer for use in the ImaMessageEndpoint for retry on connection failure
     */
    private Timer                   timer         = null;

    /**
     * Map of endpoints owned by this RA. The key is a MessageEndpointProxyUID
     */
    private HashMap<MessageEndpointUID, ImaMessageEndpoint> endpoints = 
                                    new HashMap<MessageEndpointUID, ImaMessageEndpoint>(1024);

//    @NotNull (message="connectionPoolSize must be an integer value in the range of 1 to 1024")
//    @Min(value=1, message="connectionPoolSize must be an integer value in the range of 1 to 1024")
//    @Max(value=1024, message="connectionPoolSize must be an integer value in the range of 1 to 1024")
//    private String                  connectionPoolSize = "10";

    private ImaConnectionPool       conPool;

    @NotNull (message="{CWLNC2001}")
    @Size (min=1, message="{CWLNC2001}")
    private String                  traceFile = "stdout";
    private int                     defaultTraceLevel = 4;
    
//    @NotNull (message="logWriterEnabled must be True or False")
//    @Pattern (regexp="(true|false)", flags = Pattern.Flag.CASE_INSENSITIVE, 
//            message="logWriterEnabled must be True or False")
//    private String                  logWriterEnabled = "true";
    
    /**
     * Determine whether application server trace functionality that allows
     * for dynamic trace settings is permitted.
     */
    @NotNull (message="{CWLNC2002}")
    @Pattern (regexp="(true|false)", flags = Pattern.Flag.CASE_INSENSITIVE, 
            message="{CWLNC2002}")
    private String                  dynamicTraceEnabled = "false";

    private static final boolean    outsideWAS;
    
    private AtomicInteger           hashCode           = new AtomicInteger(0);
    
    /**
     * The class loader for a dynamic trace level class.
     */
    private static ClassLoader clLoader = null;

    static {
        PrivilegedAction<Boolean> pa = new PrivilegedAction<Boolean>() {
            public Boolean run()  {
                try {
                    String command = System.getProperty("sun.java.command");
                    if (command != null)
                        return new Boolean(!command.contains("com.ibm.ws.runtime.WsServer"));
                } catch (Exception ex) {
                    /* Defect 44505: Do nothing.  
                     * We only expect an exception here when updating the RAR in a cluster environment. 
                     * We use this try/catch block to code around a known WAS limitation.
                     */
                }
                return false;
            }
        };
        outsideWAS = AccessController.doPrivileged(pa).booleanValue();
    }

    public static boolean outsideWAS() {
        return outsideWAS;
    }

    /*
     * Start the resource adapter.
     * 
     * @see javax.resource.spi.ResourceAdapter#start(javax.resource.spi.BootstrapContext)
     */
    public void start(BootstrapContext ctx) throws ResourceAdapterInternalException {
        bootstrapContext = ctx;
        Exception wasEx = null;
        boolean suppressTimestamps = (((!outsideWAS() && (traceFile != null))
                && (traceFile.equals("stdout") || traceFile.equals("stderr"))));
        if (!outsideWAS() && dynamicTraceEnabled()) {
            try {
                createImaWASTraceComponent(traceFile, defaultTraceLevel, suppressTimestamps);
            } catch (Exception ex) {
                wasEx = ex;
            }
        } else {
            ImaTrace.init(traceFile, defaultTraceLevel, suppressTimestamps);
        }
        if (wasEx != null) {
            ImaTrace.traceException(1, wasEx);
            ImaTrace.trace(1, "Failed to load the WebSphere Application Server trace component.  Using default MessageSight trace level settings."); 
        }
        workManager = ctx.getWorkManager();
        try {
            timer = ctx.createTimer();
        } catch (UnavailableException ex) {
            /*
             * Instantiate its own timer.
             */
            timer = new Timer("imaRATimer", true);
        }
        conPool = new ImaConnectionPool();
        ImaTrace.trace(1, "ImaResourceAdapter started");
        ImaTrace.trace(1, this.getRAVersionInfo());
        ImaTrace.trace(1, this.getJmsClientVersionInfo());
    }

    /*
     * Stop the resource adapter.
     *  
     * @see javax.resource.spi.ResourceAdapter#stop()
     */
    public void stop() {
        synchronized (endpoints) {
            for (ImaMessageEndpoint mep : endpoints.values()) {
                mep.stop();
            }
            endpoints.clear();
        }
        ImaTrace.trace(1, "ImaResourceAdapter stopped");
    }

    /*
     * Activate an endpoint.
     * 
     * @see javax.resource.spi.ResourceAdapter#endpointActivation(javax.resource.spi.endpoint.MessageEndpointFactory,
     * javax.resource.spi.ActivationSpec)
     */
    public void endpointActivation(MessageEndpointFactory mef, ActivationSpec spec) throws ResourceException {
        if (spec instanceof ImaActivationSpec) {
            ImaActivationSpec ias = (ImaActivationSpec) spec;
            if (!ias.validated())
                ias.validate();
            MessageEndpointUID muid = new MessageEndpointUID(mef, ias);
            ImaMessageEndpoint mep = ias.isASF() ? null : new ImaMessageEndpointNonAsf(this, mef, ias, workManager, timer);
            synchronized (endpoints) {
                endpoints.put(muid, mep);
            }
        } else {
            ImaResourceException re = new ImaResourceException("CWLNC2101", "Endpoint activation failed because the ActivationSpec class {0} is not an IBM MessageSight activation specification.", spec.getClass().getName());
            ImaTrace.traceException(1, re);
            throw re;
        }
    }

    /*
     * Close an endpoint.
     * 
     * @see javax.resource.spi.ResourceAdapter#endpointDeactivation(javax.resource.spi.endpoint.MessageEndpointFactory,
     * javax.resource.spi.ActivationSpec)
     */
    public void endpointDeactivation(MessageEndpointFactory mef, ActivationSpec spec) {
        if (spec instanceof ImaActivationSpec) {
            ImaActivationSpec ias = (ImaActivationSpec) spec;
            MessageEndpointUID muid = new MessageEndpointUID(mef, ias);
            ImaMessageEndpoint mep;
            synchronized (endpoints) {
                mep = endpoints.remove(muid);
            }
            if (mep != null) {
                mep.stop();
                ImaTrace.trace(5, "Endpoint " + mep + " was deactivated.");
            } else {
                ImaTrace.trace(4, "Endpoint was not found for: " + mef + " " + spec);
            }
        } else {
            ImaRARuntimeException rte = new ImaRARuntimeException("CWLNC2901", "Endpoint deactivation failed because the ActivationSpec class {0} is not an IBM MessageSight activation specification. This might indicate a problem with the application server.", spec.getClass().getName());
            ImaTrace.traceException(1, rte);
            throw rte;
        }
    }

    /*
     * At recover time, get a list of all XA resources which might need to be rolled back or committed.
     * 
     * @see javax.resource.spi.ResourceAdapter#getXAResources(javax.resource.spi.ActivationSpec[])
     */
    public XAResource[] getXAResources(ActivationSpec[] specs) throws ResourceException {
        XAResource[] result = new XAResource[specs.length];
        for (int i = 0; i < result.length; i++) {
            if (specs[i] instanceof ImaActivationSpec) {
                try {
                    ((ImaActivationSpec)specs[i]).validate();
                    result[i] = new ImaRecoveryXAResource(this, (ImaActivationSpec) specs[i]);
                } catch (XAException e) {
                    ImaTrace.trace(1, "Failed to create recovery XAResource for activation specification: " + specs[i]);
                    ImaResourceException re = new ImaResourceException("CWLNC2103", e, "Creation of an XAResource for recovery failed due to an exception.");
                    ImaTrace.traceException(1, re);
                    throw re;
                }
            } else {
                ImaTrace.trace(1, "While attempting to create an XAResource, an activation specification of type " + specs[i].getClass().getName() + " was found when " + ImaActivationSpec.class.getName() + " was expected.");
                ImaResourceException re = new ImaResourceException("CWLNC2104", "Failed to create an XAResource for recovery because the ActivationSpec class {0} is not an IBM MessageSight activation specification.", specs[i].getClass().getName());
                ImaTrace.traceException(1, re);
                throw re;
            }
        }
        return result;
    }
    
//    /**
//     * Set the connection pool size.
//     * 
//     * @param connectionPoolSize The connection pool size to set.
//     */
//    public void setConnectionPoolSize(String connectionPoolSize) {
//        try {
//            Integer.parseInt(connectionPoolSize);
//            this.connectionPoolSize = connectionPoolSize;
//        } catch (NumberFormatException nfe) {
//            ImaTrace.trace(2, "\'"+ connectionPoolSize + "\' is not a valid value for connectionPoolSize");
//            this.connectionPoolSize = null;
//        }
//    }
//
//    /**
//     * Return the connection pool size
//     * 
//     * @return Returns the startupRetryCount.
//     */
//    public String getConnectionPoolSize() {
//        return connectionPoolSize;
//    }

    /*
     * Return the connection pool
     */
    public ImaConnectionPool getConnectionPool() {
        return conPool;
    }

    /**
     * Return the trace level
     * @return the trace level property
     */
    public String getDefaultTraceLevel() {
        return String.valueOf(defaultTraceLevel);
    }
    
    public int defaultTraceLevel() {
        return defaultTraceLevel;
    }

    /**
     * Set the trace level.
     *  
     * @param tracelvl the trace level to set.
     * The trace level is a number between 0 and 9.
     */
    public void setDefaultTraceLevel(String tracelvl) {
        if (tracelvl == null) {
            ImaTrace.trace(2, "traceLevel must be set.  Setting defaulTraceLevel to 0.");
            tracelvl = "0";
        }
        try {
            defaultTraceLevel = Integer.parseInt(tracelvl);
        } catch (NumberFormatException ne) {
            ImaTrace.trace(2, "\'"+ tracelvl + "\' is not a valid value for traceLevel. Valid values are 0 through 9. Setting traceLevel to 4.");
            defaultTraceLevel = 4;
        }
    }
    
    /**
     * Return the trace file property.
     * @return the trace file property.
     */
    public String getTraceFile() {
        return traceFile;
    }

    /**
     * @param tracefile the trace file to set.
     * The trace file can be a file name or "stdout" or "stderr".
     */
    public void setTraceFile(String tracefile) {
        if (tracefile == null) {
            ImaTrace.trace(2, "traceFile must be set.  Setting traceFile to stdout.");
            tracefile = "stdout";
        }
        this.traceFile = tracefile;
    }

//    /**
//     * @return the value of log writer enabled.
//     */
//    public String getLogWriterEnabled() {
//        return logWriterEnabled;
//    }
//
//    /**
//     * Set the value of the log writer enabled.
//     * @param logWriterEnabled  whether to enable the log writer
//     */
//    public void setLogWriterEnabled(String logWriterEnabled) {
//        if (logWriterEnabled != null) {
//            if ("true".equals(logWriterEnabled.toLowerCase())) {
//                this.logWriterEnabled = "true";
//                return;
//            }
//            if ("false".equals(logWriterEnabled.toLowerCase())) {
//                this.logWriterEnabled = "false";
//                return;
//            }
//            ImaTrace.trace(2,"\'" + logWriterEnabled + "\' is not a valid value for logWriterEnabled.");
//        }
//        this.logWriterEnabled = null;
//    }

    /**
     * @return the value of dynamicTraceEnabled.
     */
    public String getDynamicTraceEnabled() {
        return dynamicTraceEnabled;
    }
    
    /**
     * @return the boolean value for dynamicTraceEnabled.
     */
    public boolean dynamicTraceEnabled() {
        if (dynamicTraceEnabled != null)
          return Boolean.valueOf(dynamicTraceEnabled);
        return false;
    }

    /**
     * Set the value of the dynamicTraceEnabled.
     * @param dynamicTraceEnabled  whether to enable application server trace 
     *                             with dynamic trace level settings
     */
    public void setDynamicTraceEnabled(String dynamicTraceEnabled) {
        if (dynamicTraceEnabled != null) {
            if ("true".equals(dynamicTraceEnabled.toLowerCase())) {
                this.dynamicTraceEnabled = "true";
                return;
            }
            if ("false".equals(dynamicTraceEnabled.toLowerCase())) {
                this.dynamicTraceEnabled = "false";
                return;
            }
            ImaTrace.trace(2,"\'" + dynamicTraceEnabled + "\' is not a valid value for dynamicTraceEnabled. Valid values are true or false.");
        }
        this.dynamicTraceEnabled = null;
    }
    
    public Timer getTimer() {
        return timer;
    }
    
    public boolean equals(Object o) {
        if (this == o)
            return true;
        if (o instanceof ImaResourceAdapter) {
            ImaResourceAdapter ra = (ImaResourceAdapter) o;
            if (bootstrapContext != ra.bootstrapContext)
                return false;
            if (defaultTraceLevel != ra.defaultTraceLevel)
                return false;
            if (traceFile.equals(ra.traceFile))
                return true;
            return false;
        }
        return false;
    }
    
    public int hashCode() {
        int hc = hashCode.get();
        if(hc != 0)
            return hc;
        if (bootstrapContext != null)
            hc = 17 * bootstrapContext.hashCode();
        hc += 17 * defaultTraceLevel + traceFile.hashCode();
        hashCode.compareAndSet(0, hc);
        return hashCode.get();
    }

    /*
     * Class the describe the a message endpoint from its parts.
     */
    private static final class MessageEndpointUID {
        private final MessageEndpointFactory messageEndpointFactory;
        private final ImaActivationSpec      activationSpec;

        MessageEndpointUID(MessageEndpointFactory mef, ImaActivationSpec spec) {
            messageEndpointFactory = mef;
            activationSpec = spec;
        }

        public boolean equals(Object o) {
            if (o == null)
                return false;
            if (this != o) {
                if (o instanceof MessageEndpointUID) {
                    MessageEndpointUID muid = (MessageEndpointUID) o;
                    return activationSpec.equals(muid.activationSpec)
                            && messageEndpointFactory.equals(muid.messageEndpointFactory);
                }
                return false;
            }
            return true;
        }

        public int hashCode() {
            return activationSpec.hashCode() * messageEndpointFactory.hashCode();
        }
    }
    
    private String getRAVersionInfo() {
        ImaResourceAdapterMetaData md = new ImaResourceAdapterMetaData();
        return md.getAdapterName() + " " + md.getAdapterVersion() + " Build: BUILD_ID";
    }
    
    private String getJmsClientVersionInfo() {
        return "IBM MessageSight JMS Client " + ImaJmsObject.getClientVerstion();
    }
    
    /*
     * Load the ImaWASTraceComponent to get access to WebSphere Application 
     * Server trace levels.  This allows trace level settings to be changed
     * at runtime when running a resource adapter on WAS.
     */
    private static void createImaWASTraceComponent(String traceFile, int defaultTraceLevel, boolean suppressTimestamps) throws Exception {
        if (clLoader == null) {
            clLoader = Thread.currentThread().getContextClassLoader();
        }
        Class<?> cl = clLoader.loadClass(ImaWASTraceComponent.class.getName());
        @SuppressWarnings("unchecked")
        Constructor<ImaTraceImpl> ctor = (Constructor<ImaTraceImpl>) cl
                .getDeclaredConstructor(new Class<?>[] {String.class, int.class, boolean.class});
        ImaTraceImpl wasTraceImpl = ctor.newInstance((Object)traceFile, defaultTraceLevel, suppressTimestamps);
        ImaTrace.init(wasTraceImpl);
    }

}
