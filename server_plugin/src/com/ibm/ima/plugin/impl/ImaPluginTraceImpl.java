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
package com.ibm.ima.plugin.impl;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;
import java.security.AccessController;
import java.security.PrivilegedExceptionAction;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;


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
public class ImaPluginTraceImpl {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static final SimpleDateFormat iso8601 = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z' ");
    private static final Charset utf8;
    static {
        iso8601.setTimeZone(TimeZone.getDefault());
        utf8 = Charset.forName("UTF-8");
    }

    public int              traceLevel = 0;
    public String           destination = "stdout";
    PrintWriter             writer;
    ImaPluginTraceImpl      parent;
    FileChannel             fchannel;
    int                     maxSize = 100 * 1024 * 1024;
    private final Date      date    = new Date();

    /**
     * Construction with parameters
     * 
     * @param destination the destination (stdout, stdin, or valid file path)
     * @param level the trace level. The valid values are 0-9
     * @throws RuntimeException if the level is not valid.
     */
    public ImaPluginTraceImpl(String destination, int level, boolean append) {
        parent = null;
        if (destination == null)
            destination = "stdout";
        IMATraceInit(destination, level, append);
    }
    
    /*
     * Constructor with parent
     */
    public ImaPluginTraceImpl(ImaPluginTraceImpl parent, int level) {
        this.parent = parent;
        if (level >= 0 && level <= 9)
            setTraceLevel(level);
    }
    
    /*
     * Set the file max
     * @param max the value in bytes. 
     */
    public void setFileMax(int max) {
        maxSize = max;
    }

    /**
     * Initialize the Trace
     * 
     * @param destination the destination (stdout, stdin, or valid file path)
     * @param level the trace level. The valid values are 0-9
     * @throws RuntimeException if the level is not valid.
     */
    private void IMATraceInit(String destination, int level, boolean append) {
        if (level < 0 || level > 9) {
            throw new RuntimeException("The IMA trace level is not valid. Valid values are 0 to 9.");
        }
        setTraceLevel(level);
        if (destination != null)
            setTraceFile(destination, append);
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
                date.setTime(System.currentTimeMillis());
                pw.println(iso8601.format(date) + message);
                pw.flush();
            }
        }  
        if (fchannel != null)
            checkRoll();
    }

    /*
     * Put an exception stack trace to the trace file.
     * 
     * @param ex The exception
     */
    public void traceException(Throwable ex) {
        if (ex != null) {
            if (parent != null) {
                parent.traceException(ex);
            } else {
                PrintWriter pw = writer;
                if (pw != null) {
                    ex.printStackTrace(pw);
                    pw.flush();
                }    
            }    
        }
        if (fchannel != null)
            checkRoll();
    }
    
    /*
     * Roll the trace file
     */
    public synchronized void checkRoll() {
        if (fchannel == null)
            return;
        OutputStreamWriter osw;
        try {
            if (fchannel.position() > maxSize) {
                final String backup;
                int dot = destination.lastIndexOf('.');
                if (dot >= 0) {
                    backup = destination.substring(0, dot) + "_back" + destination.substring(dot);
                } else {
                    backup = destination + "_back";
                }
                FileOutputStream fos = null; 
                fos = (FileOutputStream) AccessController.doPrivileged(new PrivilegedExceptionAction<Object>() {
                    public Object run() throws IOException {
                        File destFile = new File(destination);
                        File backupFile = new File(backup);
                        writer.close();
                        writer = null;
                        destFile.renameTo(backupFile);
                        FileOutputStream fos = new FileOutputStream(destination, false);
                        fchannel = fos.getChannel();
                        return fos;
                    }
                });  
                osw = new OutputStreamWriter(fos, utf8);
                writer = new PrintWriter(osw);
            }
        } catch (Exception e) {
            osw = new OutputStreamWriter(System.out, utf8);
            fchannel = null;
            writer = new PrintWriter(osw);
            e.printStackTrace(writer);    
        }
    }

    /**
     * Set the trace destination stdout: the message will go to System.out stderr: the message will go to System.err
     * filepath: the message will go to the file.
     * 
     * @param filename the destination
     * @return 1 for success. Otherwise, failure.
     * @throws RuntimeException if fail to open the destination.
     */
    public synchronized int setTraceFile(String filename, boolean append) {
        if (filename == null) {
            throw new RuntimeException("The trace Destination is not valid");
        }
        destination = filename.trim();
        if (writer != null) {
            writer.close();
            writer = null;
        }
        OutputStreamWriter osw;
        final boolean appendx = append;

        try {
            if (destination.equals("stdout")) {
                osw = new OutputStreamWriter(System.out, utf8);
                fchannel = null;
            } else if (destination.equals("stderr")) {
                osw = new OutputStreamWriter(System.err, utf8);
                fchannel = null;
            } else {
                /* Trace to a file */
                FileOutputStream fos; 
                fos = (FileOutputStream) AccessController.doPrivileged(new PrivilegedExceptionAction<Object>() {
                    public Object run() throws FileNotFoundException {
                        FileOutputStream fos = new FileOutputStream(destination, appendx);
                        fchannel = fos.getChannel();
                        return fos;
                    }
                });
                osw = new OutputStreamWriter(fos, utf8);
            }
            writer = new PrintWriter(osw);
        } catch (Exception e) {
            osw = new OutputStreamWriter(System.out, utf8);
            fchannel = null;
            writer = new PrintWriter(osw);
            e.printStackTrace(writer);
        }
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
