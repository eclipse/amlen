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

import com.ibm.ima.jms.impl.ImaTraceImpl;

import junit.framework.TestCase;



public class ImaTraceTest extends TestCase {

    public ImaTraceTest() {
        super("IMA Trace Test");
    }
    
    public void testTraceInitialization() {
    	String traceFile = "stdout";
    	int traceLevel = 0; 
    	System.setProperty("IMATraceFile", traceFile);
    	System.setProperty("IMATraceLevel", traceLevel+"");
    	
    	ImaTraceImpl ismTrace = new ImaTraceImpl();
    	
    	assertEquals(traceFile, ismTrace.destination);
    	assertEquals(traceLevel, ismTrace.traceLevel);
    	
    	try{
    		System.setProperty("IMATraceLevel", "invalid");
    		ismTrace = new ImaTraceImpl();
    		assert(false);
    	}catch(RuntimeException e){
    		assert(true);
    	}
    	
    	
    	
    }
    public void testTraceInitialization2() {
    	String traceFile = "stdout";
    	int traceLevel = 0; 
    	
    	ImaTraceImpl ismTrace = new ImaTraceImpl(traceFile, traceLevel);
    	
    	assertEquals(traceFile, ismTrace.destination);
    	assertEquals(traceLevel, ismTrace.traceLevel);
    	
    	try{
    		ismTrace = new ImaTraceImpl(traceFile, 1000);
    		assert(false);
    	}catch(RuntimeException e){
    		assert(true);
    	}
    }
   
    public void testSetTraceLevel() {
    	String traceFile = "stdout";
    	int traceLevel = 0; 
    	System.setProperty("IMATraceFile", traceFile);
    	System.setProperty("IMATraceLevel", traceLevel+"");
    	
    	ImaTraceImpl ismTrace = new ImaTraceImpl();
    	
    	assertEquals(traceFile, ismTrace.destination);
    	assertEquals(traceLevel, ismTrace.traceLevel);
    	
    	assertEquals(ismTrace.setTraceLevel(traceLevel+1),1);
    	assertEquals(traceLevel+1, ismTrace.traceLevel);
    	
    	ismTrace.setTraceLevel(-1);
    	assertEquals(traceLevel+1, ismTrace.traceLevel);
    	
    	assertEquals(ismTrace.setTraceLevel(-1),0);
    	assertEquals(traceLevel+1, ismTrace.traceLevel);
    	
    	assertEquals(ismTrace.setTraceLevel(10),0);
    	assertEquals(traceLevel+1, ismTrace.traceLevel);
    
    }
    
    public void testTrace() {
        @SuppressWarnings("unused")
    	final String msg = "Test Message";
    	String traceFile = "stdout";
    	int traceLevel = 0; 
    	System.setProperty("IMATraceFile", traceFile);
    	System.setProperty("IMATraceLevel", traceLevel+"");
    	
    	
    	
    	
    	
    }
    
    public static void main(String args[]){
        new ImaTraceTest().testTraceInitialization();
        new ImaTraceTest().testTraceInitialization2();
        new ImaTraceTest().testSetTraceLevel();
        new ImaTraceTest().testTrace();
        
    }
}
