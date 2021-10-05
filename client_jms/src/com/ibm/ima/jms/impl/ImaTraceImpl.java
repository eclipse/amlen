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

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.Charset;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.security.PrivilegedExceptionAction;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

import javax.jms.JMSException;

/**
 * This is the implementation of the IMA Trace. The purpose of all the trace points are to help the developer to debug
 * problems.
 * 
 * There are 2 configurations that need to set:
 * 
 * 1. IMATraceFile: the values are stdout for System.out, stderr for System.err, or the valid path to a file. The
 * default value is stderr (Sysmtem.err)
 * 
 * 2. IMATraceLevel: the trace level. the valid values are from 0-9. Default is 0 (DISABLE).
 * 
 * Here are the descriptions for the levels:
 * 
 * 1 = Trace events which normally occur only once Events which happen at init, start, and stop. This level can also be
 * used when reporting errors but these are commonly logged as well. 2 = Configuration information and details of
 * startup and changes to server state. This level can also be used when reporting external warnings, but these are
 * commonly logged as well. 3 = Infrequent actions as well as rejection of connection and client errors. This level can
 * also be used when reporting external informational conditions and security issues but these are commonly logged as
 * well. We put these in the trace as well since the user will commonly disable this level of log. 4 = Details of
 * infrequent action including connections, statistics, and actions which change states but are normal. 5 = Frequent
 * actions which do not commonly occur at a per-message rate. 7 = Trace events which commonly occur on a per-message
 * basis and call/return from external functions which are not called on a per-message basis. 8 = Call/return from
 * internal functions 9 = Detailed trace used to diagnose a problem.
 * 
 * To trace, call the ImaTrace.trace(level, message). For example:
 * 
 * ImaTrace.trace(4, "Trace Message");
 * 
 * Currently, trace method only accept the message string. all the format are done at the caller level.
 * 
 */
public class ImaTraceImpl {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static final SimpleDateFormat iso8601 = new SimpleDateFormat("[yyyy-MM-dd HH:mm:ss.SSSZ] ");

    static {
        iso8601.setTimeZone(TimeZone.getDefault());
    }

    public int              traceLevel = 0;

    public String           destination = "stderr";

    PrintWriter             writer;
    
    final ImaTraceImpl      parent;

    private final Date      date    = new Date();
    
    private boolean         suppressTimestamps    = false;

    /**
     * Construction with parameters
     * 
     * @param destination the destination (stdout, stdin, or valid file path)
     * @param level the trace level. The valid values are 0-9
     * @throws RuntimeException if the level is not valid.
     */
    public ImaTraceImpl(String destination, int level) {
        parent = null;
        /* Get the Properties from the java properties */
        if (level == -1) {
            String slevel = "0";
            try {
                destination = (String) AccessController.doPrivileged(new PrivilegedAction<Object>() {
                    public Object run() {
                        return System.getProperty("IMATraceFile", "stderr");
                    }
                });  
                slevel = (String) AccessController.doPrivileged(new PrivilegedAction<Object>() {
                    public Object run() {
                        return System.getProperty("IMATraceLevel", "0");
                    }
                });  
            } catch (Exception e) {
            }

            level = 0;
            try {
                level = Integer.parseInt(slevel);
            } catch (Exception e) {
                throw new ImaRuntimeException("CWLNC0350", e, "An attempt to set the IMA trace level failed because {0} is not an integer. Valid values are 0 to 9.", slevel);
            }
        }    
        IMATraceInit(destination, level, false);
    }

    /**
     * Construction with parameters
     * 
     * @param destination the destination (stdout, stdin, or valid file path)
     * @param level the trace level. The valid values are 0-9
     * @param suppressTimestamps whether timestamps should be suppressed in trace output
     * @throws RuntimeException if the level is not valid.
     */
    public ImaTraceImpl(String destination, int level, boolean suppressTimestamps) {
        parent = null;
        /* Get the Properties from the java properties */
        if (level == -1) {
            String slevel = "0";
            try {
                destination = (String) AccessController.doPrivileged(new PrivilegedAction<Object>() {
                    public Object run() {
                        return System.getProperty("IMATraceFile", "stderr");
                    }
                });  
                slevel = (String) AccessController.doPrivileged(new PrivilegedAction<Object>() {
                    public Object run() {
                        return System.getProperty("IMATraceLevel", "0");
                    }
                });  
            } catch (Exception e) {
            }

            level = 0;
            try {
                level = Integer.parseInt(slevel);
            } catch (Exception e) {
                throw new ImaRuntimeException("CWLNC0350", e, "An attempt to set the IMA trace level failed because {0} is not an integer. Valid values are 0 to 9.", slevel);
            }
        }    
        IMATraceInit(destination, level, suppressTimestamps);
    }
    
    /**
     * Construction
     * 
     * @throws RuntimeException if the level is not valid.
     */
    public ImaTraceImpl() {
        this((String)null, -1, false);
    }
    
    /*
     * Constructor with parent
     */
    public ImaTraceImpl(ImaTraceImpl parent, int level) {
        this.parent = parent;
        if (level >= 0 && level <= 9)
            setTraceLevel(level);
        if (parent != null)
            this.suppressTimestamps = parent.suppressTimestamps;
    }
    

    /**
     * Initialize the Trace
     * 
     * @param destination the destination (stdout, stdin, or valid file path)
     * @param level the trace level. The valid values are 0-9
     * @throws RuntimeException if the level is not valid.
     */
    private void IMATraceInit(String destination, int level, boolean suppressTimestamps) {
        if (level < 0 || level > 9) {
            throw new ImaRuntimeException("CWLNC0351", "An attempt to set the IMA trace level failed because {0} is not in the supported range. Valid values are 0 to 9.", level);
        }
        setTraceLevel(level);
        this.suppressTimestamps = suppressTimestamps;
        if (destination != null)
            setTraceFile(destination);
    }

    /**
     * Invoke the trace function to log the message
     * 
     * @param message the message string. If the message is null, nothing to be logged.
     */
    public void trace(String message) {
        if (message == null)
            return;
        if (parent != null) {
            parent.trace(message);
        } else {
            PrintWriter pw = writer;
            if (pw != null) {
                if (!suppressTimestamps) {
                    date.setTime(System.currentTimeMillis());
                    pw.println(iso8601.format(date) + message);
                } else {
                    pw.println("ImaTrace: " + message);
                }
                pw.flush();
            }
        }    
    }

    /*
     * Put an exception stack trace to the trace file.
     * 
     * @param ex The exception
     */
    public JMSException traceException(Throwable ex) {
        if (ex != null) {
            if (parent != null) {
                parent.traceException(ex);
            } else {
                PrintWriter pw = writer;
                if (pw != null)
                    ex.printStackTrace(pw);
            }    
        }
        return (ex instanceof JMSException) ? (JMSException)ex : null;
    }

    /**
     * Set the trace destination stdout: the message will go to System.out stderr: the message will go to System.err
     * filepath: the message will go to the file.
     * 
     * @param filename the destination
     * @return 1 for success. Otherwise, failure.
     * @throws RuntimeException if fail to open the destination.
     */
    private int setTraceFile(String filename) {
        if (filename == null) {
            throw new ImaRuntimeException("CWLNC0352", "An attempt to set the IMA trace destination failed because it is null.");
        }
        destination = filename.trim();
        if (writer != null) {
            writer.close();
            writer = null;
        }
        final Charset ch = Charset.forName("UTF-8");
        OutputStreamWriter osw;

        try {
            if (destination.equals("stdout")) {
                osw = new OutputStreamWriter(System.out, ch);
            } else if (destination.equals("stderr")) {
                osw = new OutputStreamWriter(System.err, ch);
            } else {
                /* Trace to a file */
                FileOutputStream fos; 
                fos = (FileOutputStream) AccessController.doPrivileged(new PrivilegedExceptionAction<Object>() {
                    public Object run() throws FileNotFoundException {
                        return new FileOutputStream(destination, false);
                    }
                });
                osw = new OutputStreamWriter(fos, ch);
            }
            writer = new PrintWriter(osw);
        } catch (Exception e) {
            throw new ImaRuntimeException("CWLNC0353", e, "An attempt to set the IMA trace destination to {0} failed due to an exception.", filename);
        }
        return 1;
    }
    
    synchronized int setPrintWriter(PrintWriter pWriter) {
        if (writer != null)
            writer.flush();
        writer = pWriter;
        return 1;
    }

    
    /**
     * Set the Trace Level
     * 
     * @param level the valid values are from 0 - 9
     * @return 1 for success, 0 for failure
     */
    public synchronized int setTraceLevel(int level) {
        if (level >= 0 && level <= 9) {
            traceLevel = level;
            return 1;
        }      
        return 0;
    }
    
    /**
     * Check if trace is allowed at the specified level.
     * @param level the trace level
     * @return true if the level is allowed to trace
     */
    public boolean isTraceable(int level) {
        return level <= traceLevel;
    }

    /**
     * Get the destination
     * 
     * @return the destination string.
     */
    public String getTraceDestination() {
        return destination;
    }
}
