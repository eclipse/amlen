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

import java.util.Locale;

import javax.transaction.xa.XAException;

import junit.framework.TestCase;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.impl.ImaIOException;
import com.ibm.ima.jms.impl.ImaIllegalStateException;
import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;
import com.ibm.ima.jms.impl.ImaNamingException;
import com.ibm.ima.jms.impl.ImaXAException;
import com.ibm.ima.ra.common.ImaCommException;
import com.ibm.ima.ra.common.ImaException;
import com.ibm.ima.ra.common.ImaInvalidPropertyException;
import com.ibm.ima.ra.common.ImaResourceException;

public class ImaTestLocalizedEx extends TestCase {
	
	String server = "myserver";
	int port = 0;
	int protocol = 1;
	
	public ImaTestLocalizedEx() {
		super("IMA JMS Client localized exception test");
	}
	
	public void testExceptions() {
		ImaJmsException ex;
		String name = "durableName";
		String cid = "myClientId";		
		int rc = 1;;		
		String className = "myClass";		
		String topicName = "myTopic";

// TODO: Test property validation error messages!  These messages do not appear as exceptions.
//		ex = new ImaJmsExceptionImpl("CWLNC2001", "The traceFile property value must be stdout, stderr or a file name.");
//		testLocalizedMessages(ex);
//		
//		ex = new ImaJmsExceptionImpl("CWLNC2002", "The dynamicTraceEnabled property value must be set to True or False.");
//		testLocalizedMessages(ex);

		
		ex = new ImaResourceException("CWLNC2101", "Endpoint activation failed because the ActivationSpec class is not valid: {0}", className);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2103", (Exception)null, "Failed to create XAResource for recovery.");
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2104", "Failed to create XAResource for recovery because the ActivationSpec class is not valid: {0}", className);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2140", "Failed to look up JNDI destination ({0}) because the JNDI context is not defined.", topicName);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2141", (Exception)null, "Failed to find JNDI destination: {0}", topicName);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2142", (Exception)null, "Failed to create JMS destination: {0}", topicName);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2143", "Failed to retrieve the destination ({0}) because the destination type is not supported: {1}", topicName, className);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2144", "Failed to create {0} because the \"onMessage\" method was not found.", className);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2146", (Exception)null, "Error initializing work for activation specification: {0}",
                name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2147", "Failed to retrieve destination object {0} because the class ({1}) is not a valid Eclipse Amlen destination class.  Assure that the \"name\" property has been set for {0}.", topicName, className);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2160", (Exception)null, "Failure initializing managed connection. Connection configuration: {0}", name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2161","Failed to create a connection handle for a managed connection because the \"{0}\" content does not match the expected content. ({1})", name, name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2162", "Failed to create physical connection to Eclipse Amlen for a managed connection because {0} is not a valid \"ConnectionRequestInfo\" type. ({1})",className, name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2163", (Exception)null, "Failed to close the physical connection to Eclipse Amlen while destroying a managed connection. ({0})", name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2164", "Failed to associate connection handle {0} with a managed connection because {2} is not an Eclipse Amlen connection proxy. ({1})", name, className);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2165", (Exception)null, "Failed to create XAResource for managed connection. ({0})", name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2166", (Exception)null, "Failed to create physical connection to Eclipse Amlen for a managed connection because \"createConnection\" failed. Connection configuration: {0}", name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2167", (Exception)null, "Transaction begin failed for a managed connection because the physical session could not be created. ({0})", name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2168", "Transaction commit failed for managed connection because the local transaction was not started. ({0})", name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2169", (Exception)null, "Transaction commit failed for a managed connection because commit on the physical session failed. ({0})", name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2170", "Transaction rollback failed for a managed connection because the local transaction was not started. ({0})", name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2171", (Exception)null, "Transaction rollback failed for managed connection because rollback on the physical session failed. ({0})", name);
		testLocalizedMessages(ex);
		
		ex = new ImaResourceException("CWLNC2172","Failed to set \"ResourceAdapter\" object for a managed connection because {1} is not supported. ({0})", className);
		testLocalizedMessages(ex);
		
		ex = new ImaCommException("CWLNC2220", "A session is not available to create an XA resource.");
		testLocalizedMessages(ex);
		
		ex = new ImaCommException("CWLNC2260", "Failed to create connection handle for a managed connection because a physical connection to Eclipse Amlen is not available. ({0})", (Throwable)null, name);
		testLocalizedMessages(ex);
		
		ex = new ImaCommException("CWLNC2261", "Transaction begin failed for a managed connection because a physical session is not available. ({0})", (Throwable)null, this);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidPropertyException("CWLNC2321", "Errors found in the property settings for connection configuration: {0}", name);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidPropertyException("CWLNC2340", "Errors found in the property settings for an activation specification: {0}", name);
		testLocalizedMessages(ex);
				
		ex = new ImaXAException("CWLNC2401", XAException.XAER_PROTO, "The method, {0}, is not allowed to be called on an XAResource for recovery.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC2401", XAException.XAER_PROTO, null);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC2402", XAException.XAER_RMFAIL, (Exception)null, "Failure initializing XAResource for recovery.");
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC2420", XAException.XA_RBROLLBACK, "Rollback on commit because \"rollbackOnly\" was specified");
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC2421", XAException.XAER_RMFAIL, "\"rollbackOnly\" was specified on commit but rollback failed because the state is not valid.");
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC2422", XAException.XA_RBROLLBACK, "Rollback on prepare because \"rollbackOnly\" was specified.");
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC2423", XAException.XAER_RMFAIL, (Exception)null, "\"rollbackOnly\" was specified on prepare but rollback failed.");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC2520", "Eclipse Amlen version {0} is not compatible with the resource adapater.", rc);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC2560", (Exception)null, "Failed to create a " +
        		"managed connection because connection allocation has failed.");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC2561", "Multiple sessions are not allowed on a Java EE client application connection.  Connection: {0} Existing session: {1}", cid, name);
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC2562","The {0} method cannot be called on a Java EE client application connection.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC2563", "Failed to create Java EE client application session because the physical session is not available.");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC2564","The {0} method cannot be called on a Java EE client application consumer.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC2565","The {0} method cannot be called on a Java EE client application session.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC2566", "Failed to create Java EE client application producer because the physical JMS session is not available");
		testLocalizedMessages(ex);
		
		ex = new ImaNamingException("CWLNC2660", "Failed to retrieve the connection factory because the reference is null.");
		testLocalizedMessages(ex);
		
		ex = new ImaRARuntimeException("CWLNC2901", "Endpoint deactivation failed because the ActivationSpec class is not valid: {0}", className);
		testLocalizedMessages(ex);
		
		ex = new ImaIOException("CWLNC2921", (Exception)null, "Unable to connect to Eclipse Amlen server for recovery.");
		testLocalizedMessages(ex);
		
		ex = new ImaException("CWLNC2941","Failed to create {0} because the MessageEndpointFactory type is not supported: {1}",
                className,name);
		testLocalizedMessages(ex);
		
//		ex = 
//		testLocalizedMessages(ex);
	}

	public void testLocalizedMessages(ImaJmsException ex) {
	    
	    Locale frlo = new Locale("fr","FR");
	    Locale delo = new Locale("de", "DE");
	    Locale zhlo = new Locale("zh");
	    Locale zhTWlo = new Locale("zh","TW");
	    Locale jalo = new Locale("ja");
	    
	    assertTrue(ex.getMessage(frlo).endsWith("fr]"));
	    assertTrue(ex.getMessage(delo).endsWith("de]"));
	    assertTrue(ex.getMessage(zhlo).endsWith("zh]"));
	    assertTrue(ex.getMessage(zhTWlo).endsWith("zh_TW]"));
	    assertTrue(ex.getMessage(jalo).endsWith("ja]"));
	    
	    Locale.setDefault(frlo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("fr]"));
	    
	    Locale.setDefault(delo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("de]"));
	    
	    Locale.setDefault(zhlo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("zh]"));
	    
	    Locale.setDefault(zhTWlo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("zh_TW]"));
	    
	    Locale.setDefault(jalo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("ja]"));
	    

	}
	
    /*
     * Main test 
     */
    public void main(String args[]) {
        try {
        	new ImaTestLocalizedEx().testExceptions();
        } catch (Exception ex) {
        	ex.printStackTrace();
        }
    }
    
}
