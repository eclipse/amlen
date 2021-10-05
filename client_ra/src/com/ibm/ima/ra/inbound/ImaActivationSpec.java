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

package com.ibm.ima.ra.inbound;

import java.beans.IntrospectionException;
import java.beans.PropertyDescriptor;
import java.util.Arrays;
import java.util.HashSet;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Topic;
import javax.naming.InitialContext;
import javax.naming.NamingException;
import javax.resource.ResourceException;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.InvalidPropertyException;
import javax.resource.spi.ResourceAdapter;
import javax.validation.constraints.Max;
import javax.validation.constraints.Min;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Pattern;
import javax.validation.constraints.Size;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaDestination;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.ra.ImaResourceAdapter;
import com.ibm.ima.ra.common.ImaFactoryConfig;
import com.ibm.ima.ra.common.ImaInvalidPropertyException;
import com.ibm.ima.ra.common.ImaResourceException;

public class ImaActivationSpec extends ImaFactoryConfig implements ActivationSpec {
    private static final long           serialVersionUID       = -5098106677752875753L;
    private static final ImaActivationSpecBeanInfo beanInfo               = new ImaActivationSpecBeanInfo();
    /** destinationType of Queue */
    static final String                 QUEUE                  = "javax.jms.Queue";
    /** destinationType of Topic */
    static final String                 TOPIC                  = "javax.jms.Topic";
    /** subscriptionDurability of Durable */
    static final String                 DURABLE                = "Durable";
    /** subscriptionDurability of NonDurable */
    static final String                 NON_DURABLE            = "NonDurable";
    /** subscriptionShared of Shared */
    static final String                 SHARED                 = "Shared";
    /** subscriptionShared of NonShared */
    static final String                 NON_SHARED             = "NonShared";
    /** subscriptionDurability of Durable */
    static final String                 AUTO_ACK               = "Auto-acknowledge";
    /** subscriptionDurability of NonDurable */
    static final String                 DUPSOK_ACK             = "Dups_ok_acknowledge";

    final static InitialContext ctx;
    
    @Min(value = 1, message= "{CWLNC2030}")
    @Max(value = 100, message= "{CWLNC2030}")
    int                         concurrentConsumers       = 1;

    @Min(value = -1, message= "{CWLNC2031}")
    @Max(value = 10000, message= "{CWLNC2031}")
    int                         clientMessageCache        = -1;

    @Min(value = -1, message= "{CWLNC2032}")
    long                        maxDeliveryFailures          = -1;

    /** Subscription durability */
    @NotNull (message="{CWLNC2021}")
    @Pattern (regexp="(durable|nondurable)", flags=Pattern.Flag.CASE_INSENSITIVE, 
            message="{CWLNC2021}")
    String                      subscriptionDurability = NON_DURABLE;

    /** Subscription name */
    String                      subscriptionName       = null;
    
    /** Shared subscription */
    @NotNull (message="{CWLNC2022}")
    @Pattern (regexp="(Shared|NonShared)", flags = Pattern.Flag.CASE_INSENSITIVE, 
            message="{CWLNC2022}")
    String                      subscriptionShared     = "NonShared";

    /**
     * Flag to indicate that messages should be rolled back on failure.
     */
     @NotNull(message= "{CWLNC2023}")
     @Pattern(regexp = "(true|false)", flags = Pattern.Flag.CASE_INSENSITIVE, message=
     "{CWLNC2023}")
    String                      enableRollback         = "false";
     
     /**
      * Flag to indicate that messages should be rolled back on failure.
      */
      @NotNull(message= "{CWLNC2024}")
      @Pattern(regexp = "(true|false)", flags = Pattern.Flag.CASE_INSENSITIVE, message=
      "{CWLNC2024}")
      String                    ignoreFailuresOnStart  = "false";
      
    
    String                      messageSelector        = null;
    
    @NotNull (message="{CWLNC2025}")
    @Pattern (regexp="(Auto-acknowledge|Dups-ok-acknowledge)", flags = Pattern.Flag.CASE_INSENSITIVE, 
            message="{CWLNC2025}")
    String                      acknowledgeMode        = AUTO_ACK;
    
    @NotNull (message="{CWLNC2026}")
    @Pattern (regexp="(auto|automatic|bytes|text)", flags = Pattern.Flag.CASE_INSENSITIVE, 
            message="{CWLNC2026}")
    String                      convertMessageType     = "auto";
    
    protected int               traceLevel        = -1;
    protected AtomicInteger     defaultTraceLevel = new AtomicInteger(-1);
    
    /* For unit testing */
    ImaConnection               loopbackConn           = null;

    /*
     * Static initializer to get the initial context 
     */
    static {
        InitialContext c = null;
        try {
            c = new InitialContext();
        } catch (NamingException e) {
            ImaTrace.traceException(1, e);
        }
        ctx = c;
    }
    
    /** The messaging domain.  This is required if the destination does not come from JNDI. */
    @Pattern (regexp="(javax\\.jms\\.Topic|javax\\.jms\\.Queue)", flags=Pattern.Flag.CASE_INSENSITIVE, 
            message="{CWLNC2027}")
    private String                      destinationType        = null;

    /**
     * The name of the destination that the MDB is listening on. This can be the name of a topic or queue.
     */
    @Size(min=1, message="{CWLNC2028}")
    private String                      destination            = null;

    /**
     * The JNDI name of the destination that the MDB is listening on. This can be the JNDI name of a topic or queue.
     */
    @Size(min=1, message="{CWLNC2029}")
    private String                      destinationLookup      = null;
    
    private AtomicBoolean               validated              = new AtomicBoolean(false);

    private ResourceAdapter             adapter;

    /**
	 * Default constructor for activation spec
	 */
    public ImaActivationSpec() {
        super();
    }

    /*
     * Get the resource adapter for this activation.
     * 
     * @see javax.resource.spi.ResourceAdapterAssociation#getResourceAdapter()
     */
    public ResourceAdapter getResourceAdapter() {
        return adapter;
    }

    /*
     * Set the resource adapter for this activation.
     * 
     * @see javax.resource.spi.ResourceAdapterAssociation#setResourceAdapter(javax .resource.spi.ResourceAdapter)
     */
    public void setResourceAdapter(ResourceAdapter ra) throws ResourceException {
        validated.set(false);
        if (adapter == null)
            adapter = ra;
        if (adapter != null)
            defaultTraceLevel.set(((ImaResourceAdapter)adapter).defaultTraceLevel());
    }

    /*
     * We do not implement ASF
     */
    public boolean isASF() {
        return false;
    }

    /*
     * Return the destination type.
     * @return the destinationType
     */
    public String getDestinationType() {
        return destinationType;
    }

    /*
     * Set the destination type.
     * This can be either "javax.jms.Queue" or "javax.jms.Topic"
     * @param destinationType the destinationType to set
     */
    public void setDestinationType(String destinationType) {
        validated.set(false);
        if (destinationType.equalsIgnoreCase(TOPIC))
            this.destinationType = TOPIC;
        if (destinationType.equalsIgnoreCase(QUEUE))
            this.destinationType = QUEUE;
    }

    /**
     * Return the destination name.
     * If destinationLookup is not null, the destination name is interpreted as the name of a topic or queue.
     * Otherwise the destination name is is not used. 
     * @return the destination
     */
    public String getDestination() {
        if (destinationLookup != null)
            return destinationLookup;
        return destination;
    }

    /**
     * Set the destination name.
     * @param destination the destination to set
     */
    public void setDestination(String destination) {
        validated.set(false);
        this.destination = destination;
    }
    
    /**
     * Return the destination name.
     * @return the destination
     */
    public String getDestinationLookup() {
        return destinationLookup;
    }

    /**
     * Set the destination name.
     * @param destination the destination to set
     */
    public void setDestinationLookup(String destinationLookup) {
        validated.set(false);
        this.destinationLookup = destinationLookup;
    }

    /**
     * @return the subscriptionName
     */
    public String getSubscriptionName() {
        return subscriptionName;
    }

    /**
     * @param subscriptionName the subscriptionName to set
     */
    public void setSubscriptionName(String subscriptionName) {
        validated.set(false);
        this.subscriptionName = subscriptionName;
    }

    
    /**
     * @return the subscriptionDurability
     */
    public String getSubscriptionDurability() {
        return subscriptionDurability;
    }


    /**
     * Set the subscription durability.
     * Subscription durability can be "Durable" or "NonDurable".
     * @param subscriptionDurability the subscriptionDurability to set
     */
    public void setSubscriptionDurability(String durable) {
        validated.set(false);
        this.subscriptionDurability = durable;
    }
    
    /**
     * Return the subscription share property
     * @return the subscription share property
     */
    public String getSubscriptionShared() {
        return subscriptionShared;
    }

    /**
     * @param shared the sharing option to set
     */
    public void setSubscriptionShared(String shared) {
        validated.set(false);
        this.subscriptionShared = shared;
    }    

    /**
     * Return the acknowledge mode.
     * @return the acknowledge mode
     */
    public String getAcknowledgeMode() {
        return acknowledgeMode;
    }

    /**
     * Set the acknowledge mode.
     * @param ackmode  The acknowledge mode to set
     */
    public void setAcknowledgeMode(String ackmode) {
        validated.set(false);
        this.acknowledgeMode = ackmode;
    }

    /**
     * Return the message conversion type.
     * @return the message conversion type
     */
    public String getConvertMessageType() {
    	if (this.convertMessageType == null) 
    		convertMessageType = imaCF.getString("ConvertMessageType");
        return convertMessageType;
    }

    /**
     * Set the message conversion type.
     * @param cvttype
     */
    public void setConvertMessageType(String cvttype) {
        validated.set(false);
        this.convertMessageType = cvttype;
        imaCF.put("ConvertMessageType", this.convertMessageType);
    }
    
    /**
     * Return the message selector.
     * @return the selector
     */
    public String getMessageSelector() {
        return messageSelector;
    }

    /**
     * Set the message selector.
     * @param selector the selector to set
     */
    public void setMessageSelector(String selector) {
        validated.set(false);
        this.messageSelector = selector;
    }

    /**
     * @return the number of concurrent consumers
     */
    public String getConcurrentConsumers() {
        return String.valueOf(concurrentConsumers);
    }


    /**
     * Set the number of concurrent consumers
     * 
     * @param cq the concurrentConsumers to set
     */
    public void setConcurrentConsumers(String cq) {
        validated.set(false);
        if (cq == null)
            cq = "0";
        try {
            concurrentConsumers = Integer.parseInt(cq);
        } catch (NumberFormatException e) {
            ImaTrace.trace(2, "\'"+ cq + "\' is not a valid value for concurrentConsumers.  The value must be a valid Java integer.");
            concurrentConsumers = 0;
        }
    }

    /**
     * Return the number of concurrent consumers as an integer.
     */
    int concurrentConsumers() {
        return concurrentConsumers;
    }

    /**
     * @return the client cache sizes
     */
    public String getClientMessageCache() {
        return String.valueOf(clientMessageCache);
    }

    /**
     * Set the client cache size
     * 
     * @param cq the client cache size to set
     */
    public void setClientMessageCache(String ccs) {
        validated.set(false);
        if (ccs == null) {
            clientMessageCache = -1;
            return;
        }
        try {
            clientMessageCache = Integer.parseInt(ccs);
        } catch (NumberFormatException e) {
            ImaTrace.trace(2, "\'" + ccs + "\' is not a valid value for clientCacheSize. The value must be a valid Java integer.");
            clientMessageCache = -1;
        }
    }
    
    /**
     * @return the max retry delivery count
     */
    public String getMaxDeliveryFailures() {
        return String.valueOf(maxDeliveryFailures);
    }

    /**
     * Set the max retry delivery count
     * 
     * @param rd the retry delivery
     */
    public void setMaxDeliveryFailures(String rd) {
        validated.set(false);
        if (rd == null) {
            maxDeliveryFailures = -1;
            return;
        }
        try {
            maxDeliveryFailures = Long.parseLong(rd);
        } catch (NumberFormatException e) {
            ImaTrace.trace(2, "\'" + rd + "\' is not a valid value for maxDeliveryFailures. The value must be a valid Java long integer.  Using default setting (-1).");
            maxDeliveryFailures = -1;
        }
    }


    /**
     * @return the logLevel
     */
    public String getTraceLevel() {
        if (traceLevel > -1)
            return String.valueOf(traceLevel);
        if (adapter != null) {
            defaultTraceLevel.set(((ImaResourceAdapter)adapter).defaultTraceLevel());
            return String.valueOf(defaultTraceLevel.get());
        }
        return String.valueOf(traceLevel);
    }

    /**
     * @param logLevel the logLevel to set
     */
    public void setTraceLevel(String ll) {
        validated.set(false);
        try {
            traceLevel = Integer.parseInt(ll);
        } catch (NumberFormatException ne) {
            ImaTrace.trace(2, "\'"+ ll + "\' is not a valid value for traceLevel.  Valid values are -1 through 9.  Setting traceLevel to default.");
            if (adapter != null)
                defaultTraceLevel.set(((ImaResourceAdapter)adapter).defaultTraceLevel());
        }
        if ((traceLevel < -1) || (traceLevel > 9)) {
            ImaTrace.trace(2, "The traceLevel setting must be in the range of 0 to 9 (or -1 for default setting).  Resetting " + traceLevel +" to default.");
            traceLevel = -1;
        }
    }

    /**
     * @return enableRollback
     */
    public String getEnableRollback() {
        return enableRollback;
    }

    /**
     * Set enableRollback
     * 
     * @param er the enableRollback to set
     */
    public void setEnableRollback(String er) {
        validated.set(false);
        enableRollback = er;
    }

    /**
     * Return enableRollback as a boolean.
     */
    boolean enableRollback() {
        return Boolean.parseBoolean(enableRollback);
    }
    
    /**
     * @return ignoreFailuresOnStart
     */
    public String getIgnoreFailuresOnStart() {
        return ignoreFailuresOnStart;
    }

    /**
     * Set ignoreFailuresOnStart
     * 
     * @param rs the ignoreFailuresOnStart to set
     */
    public void setIgnoreFailuresOnStart(String rs) {
        validated.set(false);
        ignoreFailuresOnStart = rs;
    }

    /**
     * Return ignoreFailuresOnStart as a boolean.
     */
    boolean ignoreFailuresOnStart() {
        return Boolean.parseBoolean(ignoreFailuresOnStart);
    }
    
    /*
     * Resolve the destination object.
     */
    ImaDestination getDestinationObject() throws ResourceException {
        Object destObj = null;
        ImaDestination dest = null;
        ResourceException rx;
        if (destinationLookup != null) {
            if (ctx == null) {
                rx =  new ImaResourceException("CWLNC2140", "Failed to retrieve destinationLookup {0} from JNDI because the JNDI context is not defined.", destinationLookup);
                ImaTrace.traceException(1, rx);
                throw rx;
            }    
            try {
                destObj = ctx.lookup(destinationLookup);
            } catch (NamingException e) {
                rx = new ImaResourceException("CWLNC2141", e, "Failed to retrieve destinationLookup {0} from JNDI because the specified name does not exist in the JNDI repository.", destinationLookup);
                rx.initCause(e);
                ImaTrace.traceException(1, rx);
                throw rx;
            }
            /* We need to separate the casting of the lookup object from the lookup action.
             * Otherwise, if the J2C adimin object has been created without setting the 
             * name property, the user sees a ClassCastExeception that is incomprehensible.
             * Split the actions to provide a more understandable error message.
             */
            if (destObj instanceof ImaDestination) {
                dest = (ImaDestination) destObj;

                if (dest instanceof Topic)
                    destinationType = TOPIC;
                else
                    destinationType = QUEUE;
            } else {
                ImaResourceException re = new ImaResourceException("CWLNC2147", "Failed to retrieve destinationLookup object {0} because the class {1} is not an IBM MessageSight destination class.  This can happen if the name property on the destination administered object {0} was not set.", destinationLookup, destObj.getClass().getName());
                ImaTrace.traceException(1, re);
                throw re;
            }
        } else {
            try {
                if (QUEUE.equals(destinationType)) {
                    dest = (ImaDestination) ImaJmsFactory.createQueue(destination);
                } else {
                    dest = (ImaDestination) ImaJmsFactory.createTopic(destination);
                }
            } catch (JMSException e) {
                rx = new ImaResourceException("CWLNC2142", e, "Failed to create JMS destination {0} due to an exception.", destination);
                rx.initCause(e);
                ImaTrace.traceException(1, rx);
                throw rx;
            }
        }
        if (dest instanceof ImaDestination) { 
            ImaDestination iDest = (ImaDestination) dest;
            if (subscriptionShared.equalsIgnoreCase(SHARED)) {
                if (subscriptionDurability.equalsIgnoreCase(NON_DURABLE))
                    iDest.put("SubscriptionShared", NON_DURABLE);
                else
                    iDest.put("SubscriptionShared", "True");
            } else {
                iDest.put("SubscriptionShared", "False");
            }
            if(clientMessageCache > -1) {
                iDest.put("ClientMessageCache", String.valueOf(clientMessageCache));
            }
            return dest;
        }
        ImaResourceException re = new ImaResourceException("CWLNC2143", "Failed to retrieve or create destination {0} because the class {1} is not an IBM MessageSight destination class.", (((destinationLookup != null)?destinationLookup:destination)), dest.getClass().getName());
        ImaTrace.traceException(1, re);
        throw re;
    }

    public void validate() throws InvalidPropertyException {
        HashSet<PropertyDescriptor> invalidPDs = null;
        Destination dest = null;
        try {
            super.validate();
        } catch (InvalidPropertyException ex) {
            invalidPDs = new HashSet<PropertyDescriptor>(Arrays.asList(ex.getInvalidPropertyDescriptors()));
        }
        
        /* Validate destination setting */
        if ((destination == null) && (destinationLookup == null)) {
            try {
                PropertyDescriptor pd = new PropertyDescriptor("destination", this.getClass());
                if (invalidPDs == null)
                    invalidPDs = new HashSet<PropertyDescriptor>();
                invalidPDs.add(pd);
                pd = new PropertyDescriptor("destinationLookup", this.getClass());
                invalidPDs.add(pd);
            } catch (IntrospectionException e1) {
                ImaTrace.trace(2, "Failure on attempt to add destination/destinationLookup to list of invalid property descriptors");
                ImaTrace.traceException(2, e1);
            }
            ImaTrace.trace(1, "Constraint Violation: both destination and destinationLookup are null; one of the values must be set");
        } else if (destinationLookup != null) {
            try {
                dest = getDestinationObject();
            } catch (ResourceException rex) {
                try {
                    PropertyDescriptor pd = new PropertyDescriptor("destinationLookup", this.getClass());
                    if (invalidPDs == null)
                        invalidPDs = new HashSet<PropertyDescriptor>();
                    invalidPDs.add(pd);
                } catch (IntrospectionException e1) {
                    ImaTrace.trace(2, "Failure on attempt to add destinationLookup to list of invalid property descriptors");
                    ImaTrace.traceException(2, e1);
                }
                ImaTrace.trace(1, "Constraint Violation: failure on attempt to find destinationLookup: " + destinationLookup + " due to "+rex);
            }
        } else if ((destinationLookup == null) && (destinationType == null)) {
            try {
                PropertyDescriptor pd = new PropertyDescriptor("destinationType", this.getClass());
                if (invalidPDs == null)
                    invalidPDs = new HashSet<PropertyDescriptor>();
                invalidPDs.add(pd);
            } catch (IntrospectionException e1) {
                ImaTrace.trace(2, "Failure on attempt to add destinationType to list of invalid property descriptors");
                ImaTrace.traceException(2, e1);
            }
            ImaTrace.trace(1, "Constraint Violation: both destination and destinationType must be set if destinationLookup is not specified");
        }
        
        if (((dest != null) && (dest instanceof Topic))
                || ((destination != null) && (destinationType != null) && (destinationType.equals(TOPIC)))) {
            
            /* Validate subscriptionName setting */
            if ((subscriptionName == null) && ((subscriptionShared.equals(SHARED) || subscriptionDurability.equals(DURABLE)))) {           
                try {
                    PropertyDescriptor pd = new PropertyDescriptor("subscriptionName", this.getClass());
                    if (invalidPDs == null)
                        invalidPDs = new HashSet<PropertyDescriptor>();
                    invalidPDs.add(pd);
                } catch (IntrospectionException e1) {
                    ImaTrace.trace(2, "Failure on attempt to add subscriptionName to list of invalid property descriptors");
                    ImaTrace.traceException(2, e1);
                }
                ImaTrace.trace(1, "Constraint Violation: subscriptionName must be set if subscriptionDurability is Durable or subscriptionShared is Shared");
            }

        
            /* Validate clientId setting */
            if ((clientId == null) && (subscriptionShared.equals(NON_SHARED) && subscriptionDurability.equals(DURABLE))) {
                try {
                    PropertyDescriptor pd = new PropertyDescriptor("clientId", this.getClass());
                    if (invalidPDs == null)
                        invalidPDs = new HashSet<PropertyDescriptor>();
                    invalidPDs.add(pd);
                } catch (IntrospectionException e1) {
                    ImaTrace.trace(2, "Failure on attempt to add clientId to list of invalid property descriptors");
                    ImaTrace.traceException(2, e1);
                }
                ImaTrace.trace(1, "Constraint Violation: clientId must be set for nonshared durable subscriptions");
            }
        
            /* Validate concurrentConsumers setting */
            if (concurrentConsumers > 1) {
                if (!subscriptionShared.equals("Shared")) {
                    try {
                        PropertyDescriptor pd = new PropertyDescriptor("concurrentConsumers", this.getClass());
                        if (invalidPDs == null)
                            invalidPDs = new HashSet<PropertyDescriptor>();
                        invalidPDs.add(pd);
                    } catch (IntrospectionException e1) {
                        ImaTrace.trace(2, "Failure on attempt to add concurrentConsumers to list of invalid property descriptors");
                        ImaTrace.traceException(2, e1);
                    }
                    ImaTrace.trace(1, "Constraint Violation: concurrentConsumers must not exceed 1 for a topic destination unless subscriptionShared is set to Shared");
                }
            }
        }

        if (invalidPDs != null) {
            InvalidPropertyException ipe = new ImaInvalidPropertyException("CWLNC2340", "Validation of an activation specification failed due to one or more errors in the property settings. Activation specification: {0}. The property names and errors are provided in the exception trace.", this);
            ipe.setInvalidPropertyDescriptors(invalidPDs.toArray(new PropertyDescriptor[invalidPDs.size()]));
            ImaTrace.traceException(1, ipe);
            throw ipe;
        }
        validated.set(true);
    }
    
    /*
     * Reports whether the validate() method has run successfully since
     * the last invocation of a setter.
     */
    public boolean validated() {
        return validated.get();
    }
    
    public String toString() {
        return toString(false);
    }
    
    public String toString(boolean onlyConnInfo) {
        StringBuffer sb = new StringBuffer("ImaActivationSpec={");
        sb.append("Connection info: server=" + getServer() +" port=" + getPort() + "protocol=" + getProtocol() + " ");
        if (!onlyConnInfo) {
            sb.append("Destination info: " + ((getDestinationLookup() != null)? "destinationLookup=" + getDestinationLookup()
                    :"destination=" + getDestination() + " destinationType=" + getDestinationType()));
            if (getSubscriptionName() != null) {
                /* Add: subscrptionName(clientId):subscriptionDurability:subscriptionShared */
                sb.append(" Subscription info: " + getSubscriptionName() + "(" + ((getClientId() == null)?"":getClientId() + ")"));
                sb.append(":" + getSubscriptionDurability() + ":" + getSubscriptionShared());
            }
        }
        sb.append("}");
        return sb.toString();
    }
    
    /* 
     * Unit test helper methods 
     */
    public void setLoopbackConn(ImaConnection conn) {
        validated.set(false);
        loopbackConn = conn;
    }

    /*
     *  Test only
     */
    public ImaConnection getLoopbackConn() {
        return loopbackConn;
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
