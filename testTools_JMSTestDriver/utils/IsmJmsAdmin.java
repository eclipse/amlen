/*
 * Copyright (c) 2011-2021 Contributors to the Eclipse Foundation
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

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.naming.CompositeName;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NameClassPair;
import javax.naming.NamingEnumeration;
import javax.naming.NamingException;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


/*
 * This utility takes a ISM JMS client configuration as a flat file
 * with stanzas and populates a JNDI repository.  The properties
 * are validated and the JNDI repository is only updated if the
 * properties are valid.
 * 
 * Each stanza in the configuration file consists of an introducer
 * line in square brackets.  This contains the object type and
 * the object name.  The type can be "connection", "destination",
 * or "topic".
 * 
 * The properties are in the form of key = value.  Lines beginning
 * with hash (#) or asterisk (*) are ignored.
 * 
 * The JNDI provider URI, and optional userid and password can be
 * specified as options on the command, or as java properties or
 * environment variables.  The type of the provider is determined
 * by the scheme of the provider URI.  The default file scheme
 * uses fscontext, and an ldap scheme uses LDAP.
 * 
 * This utility also allows the contents on ISM JMS admin objects
 * within the JNDI repository to be printed out.  This is output
 * in the same format as the input config file.
 * 
 * An simple example config file:
 * # Sample config
 * [connection fred]
 * group = 239.1.2.3
 * NetworkInterface = rmm://9.0.0.0/8 rmmib://10.12.1.0/24
 * Port 3200 3211?priority=high
 * 
 * [destination sam]
 * destination = 127.0.0.1:32000
 * 
 */
public class IsmJmsAdmin {
    static HashMap<String,Object> objs = new HashMap<String,Object>();
    static String  provider;
    static String  providerctx;
    static String  userid;
    static String  password;
    static String  nameprefix;
    static String  cfgfile;
    static int     errcount;
    static boolean no_warnings;
    static boolean verbose;
    static boolean dolist;
    
    /*
     * Print out syntax help. 
     */
    static void help() {
        System.out.println("java IsmJmsAdmin [options] configfile");
        System.out.println("options: ");
        System.out.println("-c url        The JNDI provider URL");
        System.out.println("-l            List ISM JMS objects in the JNDI repository");
        System.out.println("-u userid     The JNDI provider userid");
        System.out.println("-p password   The JNDI provider password");
        System.out.println("-n nameprefix Name prefix for creating and looking up objects");
        System.out.println("-v            Verbose output");
        System.out.println("-w            Do not show warnings");
        System.out.println();
        System.out.println("The JNDI provider must be specified either with the -c option, or by from");
        System.out.println("the value of the jndi.provider.url system property or the JNDI_PROVIDER_URL");
        System.out.println("environment variable.");
    }
    
    
    /*
     * Main entry point.
     */
    public static void main(String[] args) {
        /*
         * Get the arguments
         */
        getEnvProperties();
        if (!processArgs(args)) {
            help();
            return;
        }    
        
        /*
         * Do the specified function
         */
        if (dolist) {
            listRepository();
        } else {
            processFile();
            if (errcount == 0) {
                InitialContext ctx = getInitialContext(provider, providerctx, userid, password);
                if (ctx != null) 
                    bindContext(ctx);
                if (errcount == 0) {
                    System.out.println("The bind of the config file was successful");
                }    
            }
         }   
    }
    
    
    /*
     * Print an error message with a line number
     */
    static void printError(String err, int lineno) {
        if (lineno != 0)
            System.out.println("["+lineno+"] "+err);
        else
            System.out.println(err);
        errcount++;
    }
    
    
    /*
     * Get the initial context
     */
    @SuppressWarnings("unchecked")
    static InitialContext getInitialContext(String provider, String providerctx, String userid, String password) {
        InitialContext ctx = null;;
        Hashtable env = new Hashtable();
        env.put(Context.INITIAL_CONTEXT_FACTORY, providerctx);
        env.put(Context.PROVIDER_URL, provider);
        if (userid != null)
            env.put(Context.SECURITY_PRINCIPAL, userid);
        if (password != null)
            env.put(Context.SECURITY_CREDENTIALS, password);
        try {
            ctx = new InitialContext(env);
            if (verbose)
                System.out.println("Initial context = " + ctx);
        } catch (Exception e) {
            System.out.println("Unable to open an initial context: " + provider);
        }   
        return ctx;
    }
    
    /*
     * Bind the objects to the context 
     */
    static void bindContext(InitialContext ctx) {
        Iterator <String>it = objs.keySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            Object val = objs.get(key);
            try {
            	if(nameprefix != null)
	                ctx.rebind(new CompositeName().add(nameprefix+key), val);
            	else
            		ctx.rebind(key, val);
                if (verbose)
                    System.out.println("Add to JNDI context: " + key);
            } catch (NamingException ne) {
                 printError("Error binding " + key + ": " + ne.getMessage(), 0);
            }
        }    
    }    
    
    
    /*
     * Get the default for provider, userid, and password.
     * These can come from either system properties or
     * environment variables.
     */
    static void getEnvProperties() {
        provider = System.getProperty("jndi.provider.uri");
        if (provider == null) {
            provider = System.getenv("JNDI_PROVIDER_URI");
        }
        userid = System.getProperty("jndi.userid");
        if (userid == null) {
            userid = System.getenv("JNDI_USERID");
        }
        password = System.getProperty("jndi.password");
        if (password == null) {
            password = System.getenv("JNDI_PASSWORD");
        }
    }
    
    
    /*
     * Process the config file
     */
    static void processFile() {
        int    lineno = 0;
        int    objlineno = 0;
        boolean inobject = false;
        String  name = null;
        ImaProperties props = null;
        
        /*
         * Open the config file
         */
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(cfgfile));
        } catch (IOException ioe) {
            System.out.println("Unable to open config file: " + cfgfile);
            errcount++;
            return;
        }
        
        /*
         * Read the config file
         */
        try {
            String line = in.readLine();
            while (line != null) {
                line = line.trim();
                lineno++;
                if (line.length()<1 || line.charAt(0)=='#' || line.charAt(0)=='*') {
                } else {    
                    if (line.charAt(0) == '[') {
                        if (inobject)
                            completeObject(name, props, objlineno);
                        name = getName(line, lineno);
                        props = getObject(line, name, lineno);
                        inobject = props != null && name != null;
                        objlineno = lineno;
                    } else {
                        processLine(props, line, lineno);
                    }
                    
                }
                line = in.readLine();
            }
            if (inobject)
                completeObject(name, props, objlineno);
        } catch (IOException ioe) {
            printError("Error reading config file: " + cfgfile + ": " + ioe.getMessage(), lineno);    
        }
    }
    
    
    /*
     * Process one property line in the config file.
     */
    static void processLine(ImaProperties props, String line, int lineno) {
        if (props == null)     /* Ignore line if the object is bad */
            return;
        int pos = 0;
        int startpos = pos;
        while (pos<line.length() && !Character.isWhitespace(line.charAt(pos)) && 
               line.charAt(pos) != '=')
            pos++;   
        String key = line.substring(startpos, pos);
        while (pos<line.length() && 
               (Character.isWhitespace(line.charAt(pos)) || line.charAt(pos) == '='))
            pos++;
        String value = line.substring(pos);
        if (key.length()==0 || line.indexOf('=')<0) {
            printError("The property is not valid: " + line, lineno);
        } else {
            try {
                props.put(key, value);
                if (verbose)
                    System.out.println("Add property:  " + key + " = " + value);
            } catch (JMSException jmse) {
                printError("The property cannot be set: " + key + ": " + jmse.getMessage(), lineno);
            }
        }
    }

    
    /*
     * Get the object name
     */
    static String getName(String line, int lineno) {
        int startpos;
        int pos = 1;
        while (pos<line.length() && Character.isWhitespace(line.charAt(pos)))
            pos++;
        while (pos<line.length() && !Character.isWhitespace(line.charAt(pos)))
            pos++;
        while (pos<line.length() && Character.isWhitespace(line.charAt(pos)))
            pos++;
        startpos = pos;
        while (pos<line.length() && !Character.isWhitespace(line.charAt(pos)) &&
                        line.charAt(pos) != ']')
            pos++;
        return pos>startpos ? line.substring(startpos, pos) : null;
    }
    
    
    /*
     * Get the object from a line 
     */
    static ImaProperties getObject(String line, String name, int lineno) {
        int startpos;
        int pos = 1;
        while (pos<line.length() && Character.isWhitespace(line.charAt(pos)))
            pos++;
        startpos = pos;
        while (pos<line.length() && !Character.isWhitespace(line.charAt(pos)))
            pos++;
        if (pos == startpos) {
            printError("Object type not found", lineno);
        } else {
            String objtype = line.substring(startpos, pos).toLowerCase();
            if (objtype.equals("connection")) {
                try {
                    if (verbose)
                        System.out.println("Create connection factory: " + name);
                    return (ImaProperties)ImaJmsFactory.createConnectionFactory();
                } catch(JMSException jmse) {
                    printError("Unable to create ConnectionFactory: " + name +
                                    ": " + jmse.getMessage(), lineno);
                }
            } else if (objtype.equals("topic")) {
                try {
                    if (verbose)
                        System.out.println("Create destination: " + name);
                    return (ImaProperties)ImaJmsFactory.createTopic((String)null);
                } catch(JMSException jmse) {
                    printError("Unable to create Destination: " + name + 
                                    ": " + jmse.getMessage(), lineno);
                }          
            } else if (objtype.equals("queue")) {
                try {
                    if (verbose)
                        System.out.println("Create destination: " + name);
                    return (ImaProperties)ImaJmsFactory.createQueue((String)null);
                } catch(JMSException jmse) {
                    printError("Unable to create Destination: " + name + 
                                    ": " + jmse.getMessage(), lineno);
                }          
            } else {
                printError("Unknown object type: " + objtype, lineno);
            }
        }    
        return null;
    }
    
    
    /*
     * Finish processing of an object.
     * The object is ended by the start of another object,
     * or by the end of the file.
     * We use the line number of the start of the object when
     * reporting property errors.
     */
    static void completeObject(String name, ImaProperties props, int lineno) {
        ImaJmsException [] err = props.validate(no_warnings);
        if (err == null) {
            if (props instanceof Destination) {
                if (props.get("Name")==null) { 
                    printError("The destination must have a name: " + name, lineno);
                    errcount++;
                    return;
                }
            }      
            objs.put(name, props);    
        } else {
            for (int i=0; i<err.length; i++) {
                printError(err[i].toString(), lineno);
                errcount++;
            }   
        }    
    }
    
    
    /*
     * Process and check arguments 
     */
    static boolean processArgs(String [] args) {
        int   i;
        for (i=0; i<args.length; i++) {
            String argp = args[i];
            if (argp.length()>1 && argp.charAt(0)=='-') {
                switch (Character.toLowerCase(argp.charAt(1))) {
                case 'c':    /* Context */
                    if (i+1 < args.length)
                        provider = args[++i];
                    break;
                case 'h':    /* Help */
                case '?':    
                    return false;
                case 'l':    /* List the repository */
                    dolist = true;
                    break;
                case 'p':    /* Password */ 
                    if (i+1 < args.length)
                        password = args[++i];
                    break;
                case 'u':    /* Userid */
                    if (i+1 < args.length)
                        userid = args[++i];
                    break;
                case 'n':    /* Nameprefix */
                    if (i+1 < args.length)
                        nameprefix = args[++i];
                    break;
                case 'v':    /* Verbose */ 
                    verbose = true;
                    break;
                case 'w':    /* Warning */
                    no_warnings = true;
                    break;
                default:  
                    System.out.println("There is an unknown option: " + argp);
                    return false;
                } 
            } else {
                if (cfgfile != null) { 
                    System.out.println("There is an extra command argument: " + argp);
                    return false;
                }    
                cfgfile = argp;
            }
        }
        
        /*
         * Check the config and provider 
         */
        if (cfgfile == null && !dolist) {
            System.out.println("The config file is not specified.");
            return false;
        } else {
            try {
                URI uri = new URI(provider);
                String scheme = uri.getScheme();
                if (scheme == null) {
                    provider = "file:"+provider;
                    providerctx = "com.sun.jndi.fscontext.RefFSContextFactory";
                } else if (scheme.equals("file")) {
                    providerctx = "com.sun.jndi.fscontext.RefFSContextFactory";
                } else if (scheme.equals("ldap")) {
                    providerctx = "com.sun.jndi.ldap.LdapCtxFactory";
                } else {
                    System.out.println("The provider type is unknown: "+scheme);
                    return false;
                }
            } catch (URISyntaxException e) {
                System.out.println("The provider URL is not valid: " + provider);
                return false;
            }
        }
        return true;
    }
    
    
    /*
     * Dump the repository
     */
    static void listRepository() {
        InitialContext ctx = getInitialContext(provider, providerctx, userid, password);
        try {
            NamingEnumeration<NameClassPair> list = ctx.list("");
            while (list.hasMore()) {
                NameClassPair nc = list.next();
                if (nc.getClassName().endsWith("Destination")) {
                    Object obj = ctx.lookup(nc.getName());
                    if (obj != null && obj instanceof ImaProperties)
                        listObject("destination", nc.getName(), (ImaProperties)obj);
                } else if (nc.getClassName().endsWith("ConnectionFactory")) {
                    Object obj = ctx.lookup(nc.getName());
                    if (obj != null && obj instanceof ImaProperties)
                        listObject("connection", nc.getName(), (ImaProperties)obj);
                }
                // System.out.println(nc);
            }  
        } catch (NamingException ne) {
            ne.printStackTrace(System.err);
        }
    }
    
    
    /*
     * List an ISM JMS object
     */
    static void listObject(String typename, String name, ImaProperties props) {
    	if (nameprefix != null) {
           if (name.toLowerCase().startsWith(nameprefix.toLowerCase())) {
               System.out.println("["+typename+" " + name.substring(nameprefix.length()) + "]");
           }
    	} else {
    		System.out.println("["+typename+" " + name + "]");
    	}	
        Iterator <String> it = props.propertySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            Object val = props.get(key);
            if (val != null)
                System.out.println(key + " = " + val);            
        }
        System.out.println();
    }
}
