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

import java.util.Locale;

import com.ibm.ima.jms.impl.ImaIllegalStateException;
import com.ibm.ima.jms.impl.ImaInvalidClientIDException;
import com.ibm.ima.jms.impl.ImaInvalidDestinationException;
import com.ibm.ima.jms.impl.ImaInvalidSelectorException;
import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;
import com.ibm.ima.jms.impl.ImaJmsPropertyException;
import com.ibm.ima.jms.impl.ImaMessageEOFException;
import com.ibm.ima.jms.impl.ImaMessageNotReadableException;
import com.ibm.ima.jms.impl.ImaMessageNotWriteableException;

import junit.framework.TestCase;

/*
 * Test the IMAJmsException methods for the exception implementing classes.
 */
public class ImaJmsExceptionTest extends TestCase {
    static final String errcode  = "IMAJE9988";
    static final String errcode2 = "IMARE9922";
    static final String reason0 = "Reason0";
    static final String reason1 = "Reason1: {0}";
    static final String reason2 = "Reason2: {0}, {1}";
    static final String msg1 = "Reason1: repl";
    static final String msg2 = "Reason2: repl, repl2";
    static final String repl = "repl";
    static final String repl2 = "repl2";
    static final Object [] replarray = { repl, repl2 };
    static final Exception e = new Exception("My exception");
   
    /*
     * test IMAJmsExceptionImpl
     */
    public void testIMAJmsExceptionImpl() throws Exception {
        ImaJmsExceptionImpl ex1 = new ImaJmsExceptionImpl(errcode, reason0);
        ImaJmsExceptionImpl ex2 = new ImaJmsExceptionImpl(errcode, reason1, repl);
        ImaJmsExceptionImpl ex3 = new ImaJmsExceptionImpl(errcode2, e, reason1, repl);
        ImaJmsExceptionImpl ex4 = new ImaJmsExceptionImpl(errcode2, e, reason2, replarray);
        ImaJmsExceptionImpl ex5 = new ImaJmsExceptionImpl(errcode, reason2, replarray);
        
        assertEquals(errcode, ex1.getErrorCode());  
        assertEquals(errcode, ex2.getErrorCode());  
        assertEquals(errcode2, ex3.getErrorCode());  
        assertEquals(errcode2, ex4.getErrorCode());  
        assertEquals(errcode, ex5.getErrorCode());
        /* Test message text */
        assertEquals(reason0, ex1.getMessageFormat());
        assertEquals(reason1, ex2.getMessageFormat());
        assertEquals(reason1, ex3.getMessageFormat());
        assertEquals(reason2, ex4.getMessageFormat());
        assertEquals(reason2, ex5.getMessageFormat());
        assertEquals(reason0, ex1.getMessageFormat(null));
        assertEquals(reason1, ex2.getMessageFormat(null));
        assertEquals(reason1, ex3.getMessageFormat(null));
        assertEquals(reason2, ex4.getMessageFormat(null));
        assertEquals(reason2, ex5.getMessageFormat(null));
        assertEquals(reason0, ex1.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex2.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex3.getMessageFormat(Locale.FRENCH));
        assertEquals(reason2, ex4.getMessageFormat(Locale.FRENCH));
        assertEquals(reason2, ex5.getMessageFormat(Locale.FRENCH));
        
        assertEquals(reason0, ex1.getMessage());
        assertEquals(msg1, ex2.getMessage());
        assertEquals(msg1, ex3.getMessage());
        assertEquals(msg2, ex4.getMessage());
        assertEquals(msg2, ex5.getMessage());
        
        assertEquals(null, ex1.getParameters());
        assertEquals(repl, ex2.getParameters()[0]);
        assertEquals(repl, ex3.getParameters()[0]);
        assertEquals(repl, ex4.getParameters()[0]);
        assertEquals(repl, ex5.getParameters()[0]);
        assertEquals(repl2, ex4.getParameters()[1]);
        assertEquals(repl2, ex5.getParameters()[1]);
        
        assertEquals(null, ex1.getCause());
        assertEquals(null, ex2.getCause());
        assertEquals(e,    ex3.getCause());
        assertEquals(e,    ex4.getCause());
        assertEquals(null, ex5.getCause());
        
        
        /* Test null parameters */
        ex1 = new ImaJmsExceptionImpl(null, null);
        ex2 = new ImaJmsExceptionImpl(null, null, (Object)null);
        ex3 = new ImaJmsExceptionImpl(null, (Throwable)null, null, (Object)null);
        ex4 = new ImaJmsExceptionImpl(null, (Throwable)null, null, (Object [])null);
        ex5 = new ImaJmsExceptionImpl(null, null, (Object [])null);
        assertEquals(null, ex1.getErrorCode()); 
        assertEquals(null, ex5.getErrorCode()); 
        assertEquals(null, ex1.getMessage()); 
        assertEquals(null, ex5.getMessage()); 
        assertEquals(null, ex1.getMessageFormat()); 
        assertEquals(null, ex5.getMessageFormat()); 
        assertEquals(null, ex1.getParameters()); 
        assertEquals(null, ex5.getParameters()); 
        assertEquals(0, ex1.getErrorType());
        assertEquals(null, ex3.getCause());
    }
    
    /*
     * test IMAIllegalStateException
     */
    public void testIMAIllegalStateException() throws Exception {
        ImaIllegalStateException ex1 = new ImaIllegalStateException(errcode, reason0);
        ImaIllegalStateException ex2 = new ImaIllegalStateException(errcode, reason1, repl);
        ImaIllegalStateException ex3 = new ImaIllegalStateException(errcode2, e, reason1, repl);
        assertEquals(errcode, ex1.getErrorCode());  
        assertEquals(errcode, ex2.getErrorCode());  
        assertEquals(errcode2, ex3.getErrorCode());
        /* Test message text */
        assertEquals(reason0, ex1.getMessageFormat());
        assertEquals(reason1, ex2.getMessageFormat());
        assertEquals(reason1, ex3.getMessageFormat());
        assertEquals(reason0, ex1.getMessageFormat(null));
        assertEquals(reason1, ex2.getMessageFormat(null));
        assertEquals(reason1, ex3.getMessageFormat(null));
        assertEquals(reason0, ex1.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex2.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex3.getMessageFormat(Locale.FRENCH));
        
        assertEquals(reason0, ex1.getMessage());
        assertEquals(msg1, ex2.getMessage());
        assertEquals(msg1, ex3.getMessage()); 
        
        assertEquals(null, ex1.getParameters());
        assertEquals(repl, ex2.getParameters()[0]);
        assertEquals(repl, ex3.getParameters()[0]);
        
        assertEquals(9988, ex1.getErrorType());
        assertEquals(9922, ex3.getErrorType());
        
        assertEquals(null, ex1.getCause());
        assertEquals(null, ex2.getCause());
        assertEquals(e,    ex3.getCause());
        
        /* Test null parameters */
        ex1 = new ImaIllegalStateException(null, null);
        ex2 = new ImaIllegalStateException(null, (String)null, (Object)null);
        ex3 = new ImaIllegalStateException(null, (Throwable)null, (String)null, (Object)null);
        assertEquals(null, ex1.getErrorCode()); 
        assertEquals(null, ex3.getErrorCode()); 
        assertEquals(null, ex1.getMessage()); 
        assertEquals(null, ex3.getMessage()); 
        assertEquals(null, ex1.getMessageFormat()); 
        assertEquals(null, ex3.getMessageFormat()); 
        assertEquals(null, ex1.getParameters()); 
        assertEquals(0, ex1.getErrorType());
        assertEquals(null, ex3.getCause());
    }

    /*
     * test IMAIMAInvalidClientIDException
     */
    public void testIMAInvalidClientIDException() throws Exception{
        ImaInvalidClientIDException ex1 = new ImaInvalidClientIDException(errcode, reason1, repl);
        ImaInvalidClientIDException ex2 = new ImaInvalidClientIDException(errcode2, e, reason1, repl);
        assertEquals(errcode, ex1.getErrorCode());  
        assertEquals(errcode2, ex2.getErrorCode()); 
        /* Test message text */
        assertEquals(reason1, ex1.getMessageFormat());
        assertEquals(reason1, ex2.getMessageFormat());
        assertEquals(reason1, ex1.getMessageFormat(null));
        assertEquals(reason1, ex2.getMessageFormat(null));
        assertEquals(reason1, ex1.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex2.getMessageFormat(Locale.FRENCH));
        
        assertEquals(msg1, ex1.getMessage());
        assertEquals(msg1, ex2.getMessage());
        
        assertEquals(repl, ex1.getParameters()[0]);
        assertEquals(repl, ex2.getParameters()[0]);
        
         assertEquals(9988, ex1.getErrorType());
        assertEquals(9922, ex2.getErrorType());
        
        assertEquals(null, ex1.getCause());
        assertEquals(e,    ex2.getCause());
        
        /* Test null parameters */
        ex1 = new ImaInvalidClientIDException((String)null, (String)null, (Object)null);
        ex2 = new ImaInvalidClientIDException(null, (Throwable)null, (String)null, (Object)null);
        assertEquals(null, ex1.getErrorCode()); 
        assertEquals(null, ex2.getErrorCode()); 
        assertEquals(null, ex1.getMessage()); 
        assertEquals(null, ex2.getMessage()); 
        assertEquals(null, ex1.getMessageFormat()); 
        assertEquals(null, ex2.getMessageFormat()); 
        assertEquals(0, ex1.getErrorType());
        assertEquals(null, ex2.getCause());
    }

    /*
     * test IMAIMAInvalidDestinationException
     */
    public void testIMAInvalidDestinationException() throws Exception {
        ImaInvalidDestinationException ex1 = new ImaInvalidDestinationException(errcode, reason0);
        ImaInvalidDestinationException ex2 = new ImaInvalidDestinationException(errcode, reason1, repl);
        ImaInvalidDestinationException ex3 = new ImaInvalidDestinationException(errcode2, e, reason1, repl);
        assertEquals(errcode, ex1.getErrorCode());  
        assertEquals(errcode, ex2.getErrorCode());  
        assertEquals(errcode2, ex3.getErrorCode());  
        
        assertEquals(reason0, ex1.getMessageFormat());
        assertEquals(reason1, ex2.getMessageFormat());
        assertEquals(reason1, ex3.getMessageFormat());
        assertEquals(reason0, ex1.getMessageFormat(null));
        assertEquals(reason1, ex2.getMessageFormat(null));
        assertEquals(reason1, ex3.getMessageFormat(null));
        assertEquals(reason0, ex1.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex2.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex3.getMessageFormat(Locale.FRENCH));
        
        assertEquals(reason0, ex1.getMessage());
        assertEquals(msg1, ex2.getMessage());
        assertEquals(msg1, ex3.getMessage());
        
        assertEquals(null, ex1.getParameters());
        assertEquals(repl, ex2.getParameters()[0]);
        assertEquals(repl, ex3.getParameters()[0]);
        
        assertEquals(9922, ex3.getErrorType());
        
        assertEquals(null, ex1.getCause());
        assertEquals(null, ex2.getCause());
        assertEquals(e,    ex3.getCause());
        
        /* Test null parameters */
        ex1 = new ImaInvalidDestinationException(null, null);
        ex2 = new ImaInvalidDestinationException(null, (String)null, (Object)null);
        ex3 = new ImaInvalidDestinationException(null, (Throwable)null, (String)null, (Object)null);
        assertEquals(null, ex1.getErrorCode()); 
        assertEquals(null, ex3.getErrorCode()); 
        assertEquals(null, ex1.getMessage()); 
        assertEquals(null, ex3.getMessage()); 
        assertEquals(null, ex1.getMessageFormat()); 
        assertEquals(null, ex3.getMessageFormat()); 
        assertEquals(null, ex1.getParameters()); 
        assertEquals(0, ex1.getErrorType());
        assertEquals(null, ex3.getCause());
    }

    /*
     * test IMAInvalidSelectorException
     */
    public void testIMAInvalidSelectorException() throws Exception {
        ImaInvalidSelectorException ex1 = new ImaInvalidSelectorException(errcode, reason0);
        ImaInvalidSelectorException ex2 = new ImaInvalidSelectorException(errcode, reason1, repl);
        ImaInvalidSelectorException ex3 = new ImaInvalidSelectorException(errcode2, e, reason1, repl);
        assertEquals(errcode, ex1.getErrorCode());  
        assertEquals(errcode, ex2.getErrorCode());  
        assertEquals(errcode2, ex3.getErrorCode());
        
        assertEquals(reason0, ex1.getMessageFormat());
        assertEquals(reason1, ex2.getMessageFormat());
        assertEquals(reason1, ex3.getMessageFormat());
        
        assertEquals(reason0, ex1.getMessage());
        assertEquals(msg1, ex2.getMessage());
        assertEquals(msg1, ex3.getMessage());
        
        assertEquals(null, ex1.getParameters());
        assertEquals(repl, ex2.getParameters()[0]);
        assertEquals(repl, ex3.getParameters()[0]);
        
        assertEquals(9988, ex1.getErrorType());
        assertEquals(9922, ex3.getErrorType());
        
        assertEquals(null, ex1.getCause());
        assertEquals(null, ex2.getCause());
        assertEquals(e,    ex3.getCause());
        
        /* Test null parameters */
        ex1 = new ImaInvalidSelectorException(null, null);
        ex2 = new ImaInvalidSelectorException(null, (String)null, (Object)null);
        ex3 = new ImaInvalidSelectorException(null, (Throwable)null, (String)null, (Object)null);
        assertEquals(null, ex1.getErrorCode()); 
        assertEquals(null, ex3.getErrorCode()); 
        assertEquals(null, ex1.getMessage()); 
        assertEquals(null, ex3.getMessage()); 
        assertEquals(null, ex1.getMessageFormat()); 
        assertEquals(null, ex3.getMessageFormat()); 
        assertEquals(null, ex1.getParameters()); 
        assertEquals(0, ex1.getErrorType());
        assertEquals(null, ex3.getCause());
    }

    /*
     * test IMAJmsPropertyException
     */
    public void testIMAJmsPropertyException() throws Exception {
        ImaJmsPropertyException ex1 = new ImaJmsPropertyException(errcode, reason0);
        ImaJmsPropertyException ex2 = new ImaJmsPropertyException(errcode, reason1, repl);
        ImaJmsPropertyException ex3 = new ImaJmsPropertyException(errcode2, reason2, repl, repl2);
        ImaJmsPropertyException ex4 = new ImaJmsPropertyException(errcode2, e, reason1, repl);
        ImaJmsPropertyException ex5 = new ImaJmsPropertyException(errcode, e, reason2, replarray);
        assertEquals(errcode, ex1.getErrorCode());  
        assertEquals(errcode, ex2.getErrorCode());  
        assertEquals(errcode2, ex3.getErrorCode()); 
        assertEquals(errcode2, ex4.getErrorCode());  
        assertEquals(errcode, ex5.getErrorCode());
        
        assertEquals(reason0, ex1.getMessageFormat());
        assertEquals(reason1, ex2.getMessageFormat());
        assertEquals(reason2, ex3.getMessageFormat());
        assertEquals(reason1, ex4.getMessageFormat());
        assertEquals(reason2, ex5.getMessageFormat());
        assertEquals(reason0, ex1.getMessageFormat(null));
        assertEquals(reason1, ex2.getMessageFormat(null));
        assertEquals(reason2, ex5.getMessageFormat(null));
        assertEquals(reason0, ex1.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex2.getMessageFormat(Locale.FRENCH));
        assertEquals(reason2, ex5.getMessageFormat(Locale.FRENCH));
        
        assertEquals(reason0, ex1.getMessage());
        assertEquals(msg1, ex2.getMessage());
        assertEquals(msg2, ex3.getMessage());
        assertEquals(msg1, ex4.getMessage());
        assertEquals(msg2, ex5.getMessage());
        
        assertEquals(null, ex1.getParameters());
        assertEquals(repl, ex2.getParameters()[0]);
        assertEquals(repl, ex3.getParameters()[0]);
        assertEquals(repl, ex4.getParameters()[0]);
        assertEquals(repl, ex5.getParameters()[0]);
        assertEquals(repl2, ex3.getParameters()[1]);
        assertEquals(repl2, ex5.getParameters()[1]);
        
        assertEquals(9988, ex1.getErrorType());
        assertEquals(9922, ex3.getErrorType());
        
        assertEquals(null, ex1.getCause());
        assertEquals(null, ex2.getCause());
        assertEquals(null, ex3.getCause());
        assertEquals(e,    ex4.getCause());
        assertEquals(e,    ex5.getCause());
        
        /* Test null parameters */
        ex1 = new ImaJmsPropertyException(null, null);
        ex2 = new ImaJmsPropertyException(null, null, (Object)null);
        ex3 = new ImaJmsPropertyException(null, null, (Object)null, (Object)null);
        ex4 = new ImaJmsPropertyException(null, (Throwable)null, null, (Object)null);
        ex5 = new ImaJmsPropertyException(null,(Throwable)null, null, (Object [])null);
        assertEquals(null, ex1.getErrorCode()); 
        assertEquals(null, ex3.getErrorCode());
        assertEquals(null, ex1.getMessage()); 
        assertEquals(null, ex3.getMessage()); 
        assertEquals(null, ex1.getMessageFormat()); 
        assertEquals(null, ex3.getMessageFormat()); 
        assertEquals(null, ex1.getParameters()); 
        assertEquals(null, ex3.getParameters()[0]);
        assertEquals(null, ex3.getParameters()[1]);
        assertEquals(0, ex1.getErrorType());
        assertEquals(null, ex5.getCause());
    }

    /*
     * test IMAMessageEOFException
     */
    public void testIMAMessageEOFException() throws Exception {
        ImaMessageEOFException ex1 = new ImaMessageEOFException(errcode, reason0);
        ImaMessageEOFException ex2 = new ImaMessageEOFException(errcode, reason1, repl);
        ImaMessageEOFException ex3 = new ImaMessageEOFException(errcode2, e, reason1, repl);
        assertEquals(errcode, ex1.getErrorCode());  
        assertEquals(errcode, ex2.getErrorCode());  
        assertEquals(errcode2, ex3.getErrorCode());
        
        assertEquals(reason0, ex1.getMessageFormat());
        assertEquals(reason1, ex2.getMessageFormat());
        assertEquals(reason1, ex3.getMessageFormat());
        assertEquals(reason0, ex1.getMessageFormat(null));
        assertEquals(reason1, ex2.getMessageFormat(null));
        assertEquals(reason1, ex3.getMessageFormat(null));
        assertEquals(reason0, ex1.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex2.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex3.getMessageFormat(Locale.FRENCH));
        
        assertEquals(reason0, ex1.getMessage());
        assertEquals(msg1, ex2.getMessage());
        assertEquals(msg1, ex3.getMessage());
        
        assertEquals(null, ex1.getParameters());
        assertEquals(repl, ex2.getParameters()[0]);
        assertEquals(repl, ex3.getParameters()[0]);
        
        assertEquals(9988, ex1.getErrorType());
        assertEquals(9922, ex3.getErrorType());
        
        assertEquals(null, ex1.getCause());
        assertEquals(null, ex2.getCause());
        assertEquals(e,    ex3.getCause());
        
        ex1 = new ImaMessageEOFException(null, null);
        ex2 = new ImaMessageEOFException(null, (String)null, (Object)null);
        ex3 = new ImaMessageEOFException(null, (Throwable)null, (String)null, (Object)null);
        assertEquals(null, ex1.getErrorCode()); 
        assertEquals(null, ex3.getErrorCode());
        assertEquals(0, ex1.getErrorType());
        assertEquals(null, ex3.getCause());
    }

    /*
     * test IMAMessageNotReadableException
     */
    public void testIMAMessageNotReadableException() throws Exception {
        ImaMessageNotReadableException ex1 = new ImaMessageNotReadableException(errcode, reason0);
        ImaMessageNotReadableException ex2 = new ImaMessageNotReadableException(errcode, reason1, repl);
        ImaMessageNotReadableException ex3 = new ImaMessageNotReadableException(errcode2, e, reason1, repl);
        assertEquals(errcode, ex1.getErrorCode());  
        assertEquals(errcode, ex2.getErrorCode());  
        assertEquals(errcode2, ex3.getErrorCode());
        
        assertEquals(reason0, ex1.getMessageFormat());
        assertEquals(reason1, ex2.getMessageFormat());
        assertEquals(reason1, ex3.getMessageFormat());
        assertEquals(reason0, ex1.getMessageFormat(null));
        assertEquals(reason1, ex2.getMessageFormat(null));
        assertEquals(reason1, ex3.getMessageFormat(null));
        assertEquals(reason0, ex1.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex2.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex3.getMessageFormat(Locale.FRENCH));
        
        assertEquals(reason0, ex1.getMessage());
        assertEquals(msg1, ex2.getMessage());
        assertEquals(msg1, ex3.getMessage());
        
        assertEquals(null, ex1.getParameters());
        assertEquals(repl, ex2.getParameters()[0]);
        assertEquals(repl, ex3.getParameters()[0]);
        
        assertEquals(9988, ex1.getErrorType());
        assertEquals(9922, ex3.getErrorType());
        
        assertEquals(null, ex1.getCause());
        assertEquals(null, ex2.getCause());
        assertEquals(e,    ex3.getCause());
        
        /* Test null parameters */
        ex1 = new ImaMessageNotReadableException(null, null);
        ex2 = new ImaMessageNotReadableException(null, (String)null, (Object)null);
        ex3 = new ImaMessageNotReadableException(null, (Throwable)null, (String)null, (Object)null);
        assertEquals(null, ex1.getErrorCode()); 
        assertEquals(null, ex3.getErrorCode()); 
        assertEquals(null, ex1.getMessage()); 
        assertEquals(null, ex3.getMessage()); 
        assertEquals(null, ex1.getMessageFormat()); 
        assertEquals(null, ex3.getMessageFormat()); 
        assertEquals(null, ex1.getParameters()); 
        assertEquals(0, ex1.getErrorType());
        assertEquals(null, ex3.getCause());
    }

    /*
     * test IMAMessageNotWritableException
     */
    public void testIMAMessageNotWriteableException() throws Exception {
        ImaMessageNotWriteableException ex1 = new ImaMessageNotWriteableException(errcode, reason0);
        ImaMessageNotWriteableException ex2 = new ImaMessageNotWriteableException(errcode, reason1, repl);
        ImaMessageNotWriteableException ex3 = new ImaMessageNotWriteableException(errcode2, e, reason1, repl);
        assertEquals(errcode, ex1.getErrorCode());  
        assertEquals(errcode, ex2.getErrorCode());  
        assertEquals(errcode2, ex3.getErrorCode());
        
        assertEquals(reason0, ex1.getMessageFormat());
        assertEquals(reason1, ex2.getMessageFormat());
        assertEquals(reason1, ex3.getMessageFormat());
        assertEquals(reason0, ex1.getMessageFormat(null));
        assertEquals(reason1, ex2.getMessageFormat(null));
        assertEquals(reason1, ex3.getMessageFormat(null));
        assertEquals(reason0, ex1.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex2.getMessageFormat(Locale.FRENCH));
        assertEquals(reason1, ex3.getMessageFormat(Locale.FRENCH));
        
        assertEquals(reason0, ex1.getMessage());
        assertEquals(msg1, ex2.getMessage());
        assertEquals(msg1, ex3.getMessage());
        
        assertEquals(null, ex1.getParameters());
        assertEquals(repl, ex2.getParameters()[0]);
        assertEquals(repl, ex3.getParameters()[0]);
        
        assertEquals(9988, ex1.getErrorType());
        assertEquals(9922, ex3.getErrorType());
        
        assertEquals(null, ex1.getCause());
        assertEquals(null, ex2.getCause());
        assertEquals(e,    ex3.getCause());
        
        /* Test null parameters */
        ex1 = new ImaMessageNotWriteableException(null, null);
        ex2 = new ImaMessageNotWriteableException(null, (String)null, (Object)null);
        ex3 = new ImaMessageNotWriteableException(null, (Throwable)null, (String)null, (Object)null);
        assertEquals(null, ex1.getErrorCode()); 
        assertEquals(null, ex3.getErrorCode()); 
        assertEquals(null, ex1.getMessage()); 
        assertEquals(null, ex3.getMessage()); 
        assertEquals(null, ex1.getMessageFormat()); 
        assertEquals(null, ex3.getMessageFormat()); 
        assertEquals(null, ex1.getParameters()); 
        assertEquals(0, ex1.getErrorType());
        assertEquals(null, ex3.getCause());
    }
    
    /*
     * Test cases of not specifying enough replacement values
     */
    public void testUnspecifiedReplacement() throws Exception {
        ImaJmsExceptionImpl ex5 = new ImaJmsExceptionImpl(errcode, reason2, repl);
        assertEquals("Reason2: repl, {1}", ex5.getMessage());
        ex5 = new ImaJmsExceptionImpl(errcode, reason2, new Object [] {repl});
        assertEquals("Reason2: repl, {1}", ex5.getMessage());
        ex5 = new ImaJmsExceptionImpl(errcode, reason2);
        assertEquals("Reason2: {0}, {1}", ex5.getMessage());
    }

}
