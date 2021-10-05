/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
/**
 */
package com.ibm.ism.util.test;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;

/**
 *
 */
public class IsmLoggerTest {
    
    private static IsmLogger logger = null;
    private static Logger parentLogger = null;
    private static FileHandler fh = null;

    /**
     * @throws java.lang.Exception
     */
    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        if (logger == null) {
            //logFile = new File("testLogger.log");
            parentLogger = Logger.getLogger("com.ibm.ima");
            fh = new FileHandler("testLogger.log", false);
            fh.setFormatter(new SimpleFormatter());
            parentLogger.addHandler(fh);
            logger = IsmLogger.getGuiLogger();
        }
    }

    /**
     * @throws java.lang.Exception
     */
    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        logger = null;
        parentLogger.removeHandler(fh);
        fh.flush();
        fh.close();
        fh = null;
        parentLogger = null;
    }

    /**
     * Test method for {@link com.ibm.ima.util.IsmLogger#isLoggable(com.ibm.ima.util.IsmLogger.LogLevel)}.
     */
    @Test
    public final void testIsLoggable() {
        parentLogger.setLevel(Level.ALL);
        assertTrue(logger.isLoggable(LogLevel.LOG_INFO));
        assertTrue(logger.isLoggable(LogLevel.LOG_NOTICE)); 
        assertTrue(logger.isLoggable(LogLevel.LOG_WARNING));
        assertTrue(logger.isLoggable(LogLevel.LOG_ERR));
        assertTrue(logger.isLoggable(LogLevel.LOG_CRIT));
        
        parentLogger.setLevel(Level.WARNING);
        assertFalse(logger.isLoggable(LogLevel.LOG_INFO));
        assertFalse(logger.isLoggable(LogLevel.LOG_NOTICE)); 
        assertTrue(logger.isLoggable(LogLevel.LOG_WARNING));
        assertTrue(logger.isLoggable(LogLevel.LOG_ERR));
        assertTrue(logger.isLoggable(LogLevel.LOG_CRIT));

    }

    /**
     * Test method for {@link com.ibm.ima.util.IsmLogger#log(com.ibm.ima.util.IsmLogger.LogLevel, java.lang.String, java.lang.String, java.lang.String)}.
     */
    @Test
    public final void testLogLogLevelStringStringString() {
        parentLogger.setLevel(Level.ALL);
        logger.log(LogLevel.LOG_ERR, "IsmLoggerTest", "1", "CWLNA5003");
    }

    /**
     * Test method for {@link com.ibm.ima.util.IsmLogger#log(com.ibm.ima.util.IsmLogger.LogLevel, java.lang.String, java.lang.String, java.lang.String, java.lang.Object[])}.
     */
    @Test
    public final void testLogLogLevelStringStringStringObjectArray() {
        parentLogger.setLevel(Level.ALL);
        logger.log(LogLevel.LOG_WARNING, "IsmLoggerTest", "2", "CWLNA5001", new Object[] {"one", "two"});
    }

    /**
     * Test method for {@link com.ibm.ima.util.IsmLogger#log(com.ibm.ima.util.IsmLogger.LogLevel, java.lang.String, java.lang.String, java.lang.String, java.lang.Throwable)}.
     */
    @Test
    public final void testLogLogLevelStringStringStringThrowable() {
        parentLogger.setLevel(Level.ALL);
        logger.log(LogLevel.LOG_ERR, "IsmLoggerTest", "3", "CWLNA5000", new Exception("test exception"));
    }

    /**
     * Test method for {@link com.ibm.ima.util.IsmLogger#traceEntry(java.lang.String, java.lang.String)}.
     */
    @Test
    public final void testTraceEntryStringString() {
        parentLogger.setLevel(Level.FINER);
        logger.traceEntry("IsmLoggerTest", "4");
       
    }

    /**
     * Test method for {@link com.ibm.ima.util.IsmLogger#traceEntry(java.lang.String, java.lang.String, java.lang.Object[])}.
     */
    @Test
    public final void testTraceEntryStringStringObjectArray() {
        parentLogger.setLevel(Level.FINER);
        logger.traceEntry("IsmLoggerTest", "5",new Object[] {"one", "two"});
    }

    /**
     * Test method for {@link com.ibm.ima.util.IsmLogger#traceExit(java.lang.String, java.lang.String)}.
     */
    @Test
    public final void testTraceExitStringString() {
        parentLogger.setLevel(Level.FINER);
        logger.traceExit("IsmLoggerTest", "6");
    }

    /**
     * Test method for {@link com.ibm.ima.util.IsmLogger#traceExit(java.lang.String, java.lang.String, java.lang.Object)}.
     */
    @Test
    public final void testTraceExitStringStringObject() {
        parentLogger.setLevel(Level.FINER);
        logger.traceExit("IsmLoggerTest", "7", "result");
    }

    /**
     * Test method for {@link com.ibm.ima.util.IsmLogger#trace(java.lang.String, java.lang.String, java.lang.String)}.
     */
    @Test
    public final void testTrace() {
        parentLogger.setLevel(Level.FINE);
        logger.trace("IsmLoggerTest", "8","test trace message");
    }
    
    @Test
    public final void testMissingMessageIdAndVariousLevels() {
        parentLogger.setLevel(Level.WARNING);        
        logger.log(LogLevel.LOG_NOTICE, "IsmLoggerTest", "DontDisplay", "Shouldn't print, wrong level");
        logger.log(LogLevel.LOG_WARNING, "IsmLoggerTest", "9", "Warning no messageID");
        parentLogger.setLevel(Level.INFO);
        logger.log(LogLevel.LOG_INFO, "IsmLoggerTest", "10", "Info, no messageID, but has param {0}", new Object[] {"myParam"});        
    }

}
