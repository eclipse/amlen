/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Map;


/*
 * This class represents the plug-in process.
 * 
 * The plug-in process runs as a Java process separate from imaserver.  
 * When imaserver starts is starts the plug-in process which accepts connections
 * from imaserver.
 * 
 * The imaserver is configured to start up a set of plug-ins.  Each plug-in
 * defines a set of jar files, a set of plug-in properties, a set of plug-in
 * configuration, and a data directory (with initial content).  This configuration
 * is sent to the plug-in process when it requests configuration.
 * 
 * The plug-in properties define what a plug-in is allowed to do including
 * its security manager environment and which endpoints it can interact with.
 * 
 * The plug-in configuration is a set of configuration properties which can be
 * modified dynamically.
 * 
 * In an HA environment, the same plug-ins must be installed on both peers, and
 * MessageSight synchronizes configuration changes between the two peers.  Files 
 * in the data directories are not synchronized. 
 *  
 */
public class ImaPluginMain {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    static String ifname = "127.0.0.1";
    static int    port = 9103;
    static int    trclevel = 6;
    static String trcdest = "stdout";
    static volatile boolean stopping = false;
    static Map<String,Object>    props;
    static private ServerSocket serverSocket;
    static ImaPluginTraceImpl trace;
    static boolean daemon = false;
    
    public static void help() {
        System.out.println("imaplugin: ");
        System.out.println("   -0 ... -9 = Set the trce level.  0=off  default=5");
        System.out.println("   -i ifname = Restrict to a single IP address. @=all, default=127.0.0.1");
        System.out.println("   -p        = Set the port number (1-65535). default=9103");
        System.out.println("   -t file   = Set the trace destination.  default=stdout");
        System.out.println("   -d        = Set daemon mode, trace is overridden by server config after startup");
        System.exit(255);
    }
    
    /*
     * Utility to return a string property
     */
    static String getStringProperty(Map<String,Object>map, String name) {
        Object obj = map.get(name);
        if (obj == null)
            return null;
        if (obj instanceof String)
            return (String)obj;
        return ""+obj;
    }
    
    /*
     * Utility to return an int property
     */
    static int getIntProperty(Map<String,Object>map, String name, int defval) {
        Object obj = map.get(name);
        if (obj == null)
            return defval;
        if (obj instanceof Number)
            return ((Number)obj).intValue();
        return defval;
    }
    
    /*
     * Set the properties
     */
    public static void setConfig(Map<String,Object> xprops) {
        props = xprops;

        /*
         * If in daemon mode (as opposed to test mode) set the trace location from the server config
         */
        int    tracemax   = getIntProperty(props, "TraceMax", -1);
        if (tracemax > 0) {
            // System.out.println("tracemax=" + tracemax);
            if (tracemax < 2)
                tracemax = 2;
            if (tracemax > 2000)
                tracemax = 2000;
            tracemax = tracemax * 1024 * 1024;
            trace.setFileMax(tracemax);
        }
        
        if (daemon) {
            String tracefile  = getStringProperty(props, "TracePluginFile");
            int    tracelevel = getIntProperty(props, "TracePluginLevel", -1);
            
            if(tracelevel<0){
            	tracelevel = getIntProperty(props, "TraceLevel", -1);
            }

            if (tracefile != null || tracelevel >= 0) {
                if (tracelevel >= 0) {
                    trace.setTraceLevel(tracelevel);
                }    

                if (tracefile != null && !tracefile.equals(trace.destination)) {
                    trace.setTraceFile(tracefile, true);
                }  
                ImaTrace.trace(2, "Configure IBM MessageSight plug-in process: Port=" + port + " Interface=" + ifname + 
                        " TraceLevel=" + trclevel);
            }

        }
    }
    
    /*
     * Get the properties
     */
    public static Map<String,Object> getConfig() {
        return props;
    }
    
    /*
     * Return a config item in string form
     */
    public static String getStringConfig(String name) {
        if (props == null)
            return null;
        Object obj = props.get(name);
        if (obj == null)
            return null;
        return ""+obj;
    }
    
    /*
     * Return a config item as an integer
     */
    public static int getIntConfig(String name, int deflt) {
        int ret = deflt;
        Object obj = props.get(name);
        if (obj != null) {
            if (obj instanceof Boolean) {
                ret = ((Boolean) obj) ? 1 : 0;
            } else if (obj instanceof Number) {
                ret = ((Number) obj).intValue();
            } else {
                try {
                    ret = Integer.parseInt("" + obj);
                } catch (Exception e) {
                }
            }
        }
        return ret; 
    }
    
    /*
     * Return a config item as a boolean
     */
    public static boolean getBooleanConfig(String name, boolean deflt) {
        int val = getIntConfig(name, deflt ? 1 : 0);
        return val != 0;
    }
    
    /*
     * Terminate
     */
    public static void terminate(int rc, String reason) {
        // TODO: Trace
        try {
            stopping = true;
            serverSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    
    
    /*
     * Main funtion of plug-in process
     */
    public static void main(String [] args) {
        /*
         * Process args
         */
        processArgs(args);
        
        System.out.println("Eclipse Amlen plug-in process");
        System.out.println("Copyright (C) 2014-2021 Contributors to the Eclipse Foundation.");
        
        /*
         * Initialize trace
         */
        trace = ImaTrace.init(trcdest, trclevel, false);
        ImaTrace.trace(2, "Start Eclipse Amlen plug-in process: Port=" + port + " Interface=" + ifname + 
                " TraceLevel=" + trclevel);

        InetAddress iface = null;

        /*
         * Open listen socket
         */
        try {
            if ("@".equals(ifname)) {
                serverSocket = new ServerSocket(port, 1024);
            } else {    
                iface = InetAddress.getByName(ifname);
                serverSocket = new ServerSocket(port, 1024, iface);
            }   
            serverSocket.setReuseAddress(true);
        } catch (IOException iex) {
            ImaTrace.trace(1, "Error creating plug-in server socket.");
            ImaTrace.traceException(1, iex);
            stopping = true;
        }    
        
        
        /*
         * Accept connections
         */
        while (!stopping) {
            try {
                Socket clientSock = serverSocket.accept();
                new ImaChannel(clientSock).start();
            } catch (Exception iex) {
                if (!stopping) {
                    ImaTrace.trace(1, "Error accepting plug-in server socket.");
                    ImaTrace.traceException(1, iex);
                    stopping = true;
                }
            }
        }
        ImaTrace.trace(2, "End IBM MessageSight plugin processor");
        System.exit(0);
    }
    
    /*
     * Process arguments
     */
    static void processArgs(String [] args) {
        int i;
        String argp = "";
        for (i=0; i<args.length; i++) {
            argp = args[i];
            if (argp.length() > 1 && argp.charAt(0)=='-') {
                switch(argp.charAt(1)) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    trclevel = argp.charAt(1)-'0';
                    break;
                case 'i':   /* Inteface */
                    if (++i < args.length) {
                        ifname = args[i];
                    }
                    break;
                case 'p':    /* Port */
                    if (++i < args.length){
                        port = Integer.parseInt(args[i]);
                        if (port < 1 || port > 35575) {
                            System.out.println("Port not valid: " + argp);
                            help();
                        }
                    }
                    break;
                case 't':    /* Trace file */
                    if (++i < args.length){
                        trcdest = args[i];
                    }
                    break;
                case 'd':    /* Daemon mode */
                    daemon = true;
                    break;
                default:
                    System.out.println("Unknown switch: " + argp);
                    help();
                }
            } else {
                System.out.println("Unknown argument: " + argp);
                help();
            }
        }
        if (i > args.length){
            System.out.println("Trailing switch not valid: " + argp);
        }
    }
}
