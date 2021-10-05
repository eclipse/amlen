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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Vector;

import javax.jms.JMSException;

/*
 * This is a static only class used by other classes in the package.
 * The enumerations and property validators are kept in this class so that they are
 * not in the classes which are administered objects and are serialized.
 */
public class ImaJms {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /*
     * This class is never instantiated
     */
    private ImaJms() {}

    /*
     * Static initialization of valid property and casemaps
     */
    static HashMap <String, String> connProps;
    static HashMap <String, String> connCasemap;
    static HashMap <String, String> destProps;
    static HashMap <String, String> destCasemap;

    static final int PTYPE_Connection = 1;
    static final int PTYPE_Destination = 2;

    public static final int Common = 0;
    public static final int Queue = 1;
    public static final int Topic = 2;
    
    /* For internal use */
    public static final int Topic_False            = 7;
    public static final int Topic_Shared           = 8;
    public static final int Topic_SharedNonDurable = 9;
    public static final int Topic_GlobalShared     = 10;
    public static final int Topic_GlobalNoConsumer = 11;

    /*
     * Enumeration for boolean
     */
    public static ImaEnumeration Boolean;
    public static ImaEnumeration PollPolicyEnum;
    public static ImaEnumeration ObjectTypeEnum;
    public static ImaEnumeration ProtocolEnum;
    public static ImaEnumeration ConvertEnum;
    public static ImaEnumeration SubSharedEnum;


    /*
     * Initialize the enumerations
     */
    static {
        String [] booldef = {"true", "1", "false", "0", "on", "1", "off", "0", "1", "1", "0", "0", "enabled", "1", "disabled", "0"};
        Boolean = new ImaEnumeration(false, booldef);

        String [] otdef = {"common", "0", "queue", "1", "topic", "2" };
        ObjectTypeEnum = new ImaEnumeration(true, otdef);

        String [] cvdef = {"automatic", "0", "auto", "0", "bytes", "1", "text", "2" };
        ConvertEnum = new ImaEnumeration(true, cvdef);

        String [] pdef = {"tcp", "1", "tcps", "2"};
        ProtocolEnum = new ImaEnumeration(true, pdef);
        
        String [] shdef = {"True", "1", "Shared", "1", "False", "0", "NonShared", "0", "NonDurable", "2"};
        SubSharedEnum = new ImaEnumeration(true, shdef);
    }


    /*
     * Create a non-implemented exception
     */
    public static JMSException notImplemented(String method) {
        ImaJmsExceptionImpl jmsex;
        jmsex = new ImaJmsExceptionImpl("CWLNC0091", "A call to {0} failed because the IBM MessageSight JMS client does not support this method.", method);
        return jmsex;
    }

    /*
     * Return the map of valid properties for this property type.
     */
    public static HashMap <String, String> getValidProps(int proptype) {
        switch (proptype) {
        case PTYPE_Connection:
            return connProps;
        case PTYPE_Destination:
            return destProps;
        }
        return null;
    }

    static int unique = 0;

    /*
     * Return the map of lowercase string to preferred string for property keys.
     * This is used to allow known properties to be named in a case independent manor.
     */
    public static HashMap <String, String> getCasemap(int proptype) {
        switch (proptype) {
        case PTYPE_Connection:
            return connCasemap;
        case PTYPE_Destination:
            return destCasemap;
        }
        return null;
    }


    /*
     * Return the value of the specified key.
     * If the key is not found, return null.
     */
    public static String getStringKeyValue(String [] options, String key) {
        if (options != null && key != null) {
            for (int i=0; i<options.length; i+=2) {
                if (key.equalsIgnoreCase(options[i]))
                    return options[i+1];
            }
        }
        return null;
    }


    /*
     * Return whether a specified key exists.
     */
    public static boolean stringKeyValueExists(String [] options, String key) {
        if (options != null && key != null) {
            for (int i=0; i<options.length; i+=2) {
                if (key.equalsIgnoreCase(options[i]))
                    return true;
            }
        }
        return false;
    }

    /*
     * Return an array of strings which are separated by any number of spaces or semicolons.
     */
    static String [] getStringList(String s) {
        ArrayList<String> al = new ArrayList<String>();
        int pos = 0;
        int len = s.length();
        int start = -1;
        while (pos < len) {
            char ch = s.charAt(pos);
            if (ch==' ' || ch=='\t' || ch==';') {
                if (start >= 0)
                    al.add(s.substring(start, pos));
                start = -1;
            } else {
                if (start < 0)
                    start = pos;
            }
            pos++;
        }
        if (start >= 0)
            al.add(s.substring(start, pos));
        return al.toArray(new String[0]);
    }


    /*
     * Add a property to the validator list.
     */
    public static void addValidProperty(int proptype, String name, String validator) {
        HashMap <String,String> validProps = getValidProps(proptype);
        HashMap <String,String> caseMap = getCasemap(proptype);

        if (name == null || caseMap.get(name.toLowerCase()) != null)
            return;
        if (validator == null)
            validator = "S";
        validProps.put(name, validator);
        caseMap.put(name.toLowerCase(), name);
    }



    /*
     * Parse the options into an array
     */
    static String [] parseOptions(String query) {
        Vector <String> v = new Vector<String>();
        int pos = 0;
        if (query != null) {
            boolean inname = true;
            int startpos = pos;
            while (pos <query.length()) {
                char ch = query.charAt(pos);
                if (ch == ',' || (inname && ch == '=')) {
                    v.add(query.substring(startpos, pos));
                    if (inname && ch == ',') {
                        v.add(null);
                        inname = true;
                    } else {
                        inname = !inname;
                    }
                    startpos = pos+1;
                }
                pos++;
            }
            if (!inname)
                v.add(query.substring(startpos, pos));
        }
        return (String [])(v.toArray(new String [0]));
    }

    /*
     * Parse the host from the authority.
     * IMA allows any IP address to be within the IPv6 like brackets
     */
    static String parseAuthorityIP(String portstr) {
        if (portstr == null || portstr.length()==0)
            return null;

        int pos = 0;
        int epos;
        int len = portstr.length();
        String ip = null;

        /* Handle an IPv6 style IP enclosed in brackets */
        if (portstr.charAt(pos)=='[') {
            epos = portstr.indexOf(']');
            if (epos<0)
                return null;
            ip = portstr.substring(1, epos);
            pos = epos+1;
            if (pos<len && portstr.charAt(pos) != ':')
                return null;
        }
        /* Handle a IP address without brackets */
        else {
            epos = portstr.indexOf(':');
            if (epos >= 0) {
                ip = portstr.substring(0, epos);
                pos = epos;
            } else {
                ip = portstr;
            }

        }
        return ip;
    }


    /*
     * Parse the port number.
     * IMA allows any IP address to be within the IPv6 like brackets
     * @return The port number, or 0 to indic
     */
    static int parseAuthorityPort(String portstr, int defaultport) {
        if (portstr == null || portstr.length()==0)
            return -1;

        int pos = 0;
        int epos;
        int len = portstr.length();

        /* Handle an IPv6 style IP enclosed in brackets */
        if (portstr.charAt(pos)=='[') {
            epos = portstr.indexOf(']');
            if (epos<0)
                return -1;
            pos = epos+1;
            if (pos<len && portstr.charAt(pos) != ':')
                return defaultport;
        }
        /* Handle a IP address without brackets */
        else {
            epos = portstr.indexOf(':');
            if (epos >= 0)
                pos = epos;
            else
                pos = len;
        }

        if (pos < len && portstr.charAt(pos)==':')
            pos++;

        int portnum  = 0;
        while (pos < len) {
            int ch = portstr.charAt(pos);
            if (ch < '0' || ch > '9')
                break;
            portnum = portnum*10 + (ch-'0');
            pos++;
        }
        return portnum==0 ? defaultport : portnum;
    }



    /*
     * Tables for valid properties.
     * This maps from a property name to a validator.
     */
    static {
        connProps = new HashMap<String, String>();

        /*
         * Base properties
         */
        connProps.put("AllowAsynchronousSend",         "B");
        connProps.put("AsyncTransactionSend",          "B");
        connProps.put("CipherSuites",                  "S");
        connProps.put("ClientID",                      "S");
        connProps.put("ClientMessageCache",            "U");
        connProps.put("ConvertMessageType",            "E:ConvertEnum");
        connProps.put("DefaultTimeToLive",             "U");     /* Default JMS time to live in seconds */
        connProps.put("DisableACK",                    "B");
        connProps.put("DisableMessageID",              "B");
        connProps.put("DisableMessageTimestamp",       "B");
        connProps.put("DisableTimeout",                "B");
        connProps.put("Port",                          "I:1:65535");
        connProps.put("Protocol",                      "E:ProtocolEnum");
        connProps.put("RecvSockBuffer",                "K");
        connProps.put("Server",                        "S");
        connProps.put("SecurityProtocols",             "S");
        connProps.put("SecuritySocketFactory",         "S");
        connProps.put("SecurityConfiguration",         "S");
        connProps.put("TemporaryQueue",                "S");
        connProps.put("TemporaryTopic",                "S");
        connProps.put("UserData",                      "S");
        connProps.put("Verbose",                       "B");     /* Allow debug output */

        /*
         * Current properties of Connection and Session
         */
        connProps.put("Connection",                    "Z");
        connProps.put("isClosed",                      "Z");
        connProps.put("isStopped",                     "Z");
        connProps.put("ObjectType",                    "E:ObjectTypeEnum");
        connProps.put("userid",                        "Z");
        connProps.put("password",                      "Z");

        /*
         * Create the case map.  This allows us to make the properties
         * for known properties case independent.
         */
        connCasemap = new HashMap<String, String>();
        Iterator<String> it = connProps.keySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            connCasemap.put(key.toLowerCase(), key);
        }

        /*
         * Set up the destination properties
         */
        destProps = new HashMap<String, String>();
        destProps.put("Name",                          "S");
        destProps.put("DisableACK",                    "B");
        destProps.put("ClientMessageCache",            "U");
        destProps.put("UserData",                      "S");

        /*
         * Current properties of MessageConsumer and MessageProducer
         */
        destProps.put("ObjectType",                    "E:ObjectTypeEnum");
        destProps.put("Connection",                    "Z");
        destProps.put("isDurable",                     "Z");
        destProps.put("subscriptionName",                   "Z");
        destProps.put("isClosed",                      "Z");
        destProps.put("noLocal",                       "Z");
        destProps.put("Session",                       "Z");
        destProps.put("SubscriptionShared",            "Z");

        /*
         * Create the destination case map
         */
        destCasemap = new HashMap<String, String>();
        it = destProps.keySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            destCasemap.put(key.toLowerCase(), key);
        }
    }
}
