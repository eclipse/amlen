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
package com.ibm.ima.proxy.impl;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;

import com.ibm.ima.proxy.ImaAuthConnection;
import com.ibm.ima.proxy.ImaAuthenticator;
import com.ibm.ima.proxy.ImaDeviceUpdater;
import com.ibm.ima.proxy.ImaJson;
import com.ibm.ima.proxy.ImaMHubCredentialManager;
import com.ibm.ima.proxy.ImaProxyListener;
import com.ibm.ima.proxy.ImaProxyResult;

public class ImaProxyImpl implements ImaProxyListener, Runnable {
    
    static final int config    = 1;
    static final int endpoint  = 2;
    static final int tenant    = 3;
    static final int server    = 4;
    static final int user      = 5;
    static final int cert      = 6;
    static final int item      = 7;
    static final int acl       = 8;
    static final int device     = 9;
    static final int list      = 0x10;
    static final int mhub_credentials     = 90;
    static final int mhub_bindings     	 = 91; 
    
    static Object   config_obj;
    static Class<?> config_cls; 
    ImaAuthenticator auth;
    static  LinkedBlockingQueue<ImaAuthConnImpl> authQueue;
    
    ImaAuthenticator authenticator = null;
    ImaDeviceUpdater deviceUpdater = null;
    ImaMHubCredentialManager mHubCredMgr = null;
    private final  boolean aaaEnabled;
    private final  boolean sgEnabled;
    native String  setJSONn(int type, String name, String name2, String json);
    native String  setBinary(int type, String name, String name2, byte [] data);
    native String  getJSONn(int type, String name, String name2);
    native byte [] getCert(int type, String name, String name2);
    native boolean deleteObj(int type, String name, String name2, boolean force);
    native String  getStats(int type, String name);
    native String  getObfus(String user, String pw, int type);
    native String  startMsg();
    native String  quitProxy(int rc);
    native int     doDisconnect(String endpoint, String server, String tenant, String user, String clientid, int operationbits);
    native String  doLog(String msgid, String msgformat, String filen, int lineno, String repl);
    native void    doAuth(long obj, boolean good, int rc);
    native void    doAuthorized(long obj, int rc, String reason);
    native void    doSetAuth(boolean b);
    native void    doSetDeviceUpdater(boolean b);
    native void    doSetMHUBCredentialManager(boolean b);
    native void    doSetAuthorize(boolean b);
    native int     getThreadCount();

    /*
     * Load libimaproxy.so which should be in libpath as most of proxy is implemented there.
     */
    static {
    		//Get the system property the name of the main proxy lib
    		String proxyLibName = System.getProperty("PROXYLIB_NAME");
    		
    		if(proxyLibName==null) {
    			//Get the environment variable for proxy lib
    			proxyLibName = System.getenv("PROXYLIB_NAME");
    			if(proxyLibName==null) {
    				proxyLibName = "imaproxy";
    			}
    		}
    		
        System.loadLibrary(proxyLibName);
        authQueue = new LinkedBlockingQueue<ImaAuthConnImpl>();
    }
    
    public ImaProxyImpl(int aaaEnabled, int sgEnabled) {
        this.aaaEnabled = (aaaEnabled != 0);
        this.sgEnabled = (sgEnabled != 0);
    }
    /*
     * Call initializer with error handling sent to the log.
     */
    public void init(String classname) {
        try {
            config_cls = Class.forName(classname);
            Class<?> [] parm = new Class[1];
            parm[0] = ImaProxyListener.class;
            Constructor<?> ctor  = config_cls.getConstructor(parm);
            config_obj = ctor.newInstance(this);
        } catch (Exception x) {
            StringWriter sw = new StringWriter();
            x.printStackTrace(new PrintWriter(sw));
            x.printStackTrace(System.err);
            log("CWLNA0999", "Exception loading class \"{0|\": ", sw.toString());
        }
    }
    
    /*
     * Run the config with exception handling 
     */
    public void dorun() {
        try {
            Method do_run = config_cls.getMethod("run");
            do_run.invoke(config_obj);
        } catch (Throwable tx) {
            StringWriter sw = new StringWriter();
            tx.printStackTrace(new PrintWriter(sw));
            tx.printStackTrace(System.err);
            log("CWLNA0998", "Exception running config: {0}", sw.toString());  
        }
    }
    
    /*
     * Proc for the auth thread
     * @see java.lang.Runnable#run()
     */
    public void run() {
    	ImaAuthConnImpl connect=null;
        for (;;) {
            try {
                connect = authQueue.take();
                if(connect.action==ImaAuthConnection.ACTION_AUTHENTICATION){
                	authenticator.authenticate((ImaAuthConnection)connect);
                }else{
                	authenticator.authorize((ImaAuthConnection)connect);
                }
            } catch (InterruptedException iex) {
            	StringWriter sw = new StringWriter();
            	iex.printStackTrace(new PrintWriter(sw));
            	iex.printStackTrace(System.err);
            	log("CWLNA0995", "Exception in auth thread run: {0} ", sw.toString());
            	if(connect!=null){
            		if(connect.action==ImaAuthConnection.ACTION_AUTHENTICATION){
	            		try{
	            			connect.authenticate(false, 100);
	            		}catch(IllegalStateException ise){}
            		}else{
            			try{
	            			connect.authorize(ImaAuthConnection.NOT_AUTHORIZED);
	            		}catch(IllegalStateException ise){}
            		}
            	}
            } catch (Exception e) {
            	StringWriter sw = new StringWriter();
            	e.printStackTrace(new PrintWriter(sw));
            	e.printStackTrace(System.err);
            	log("CWLNA0995", "Exception in auth thread run(): {0}", sw.toString());
            	if(connect!=null) {
            		if(connect.action==ImaAuthConnection.ACTION_AUTHENTICATION){
	            		try{
	            			connect.authenticate(false, 100);
	            		}catch(IllegalStateException ise){}
            		}else{
            			try{
	            			connect.authorize(ImaAuthConnection.NOT_AUTHORIZED);
	            		}catch(IllegalStateException ise){}
            		}
            	}
                try {
                	Thread.sleep(1000);
                } catch(InterruptedException ie) {
                	sw = new StringWriter();
                	e.printStackTrace(new PrintWriter(sw));
                	e.printStackTrace(System.err);
                	log("CWLNA0995", "Exception in auth thread run(): {0}", sw.toString());
                }
            }
        }
    }
    
    
    /*
     * Post the authenticate request
     */
    public int authenticate(String clientID, String userID, int rlacAppEnabled, int rlacAppDefaultGroup, int isSecure, int rmPolicies, int sgEnabled,
            String cert_name, String issuer, int crlStatus, int which, 
            String client_addr, String password, long transport, int port, long conId) {
        System.err.flush();
        int  rc = 5; 
        try {
            ImaAuthConnImpl connect = new ImaAuthConnImpl(clientID, userID, rlacAppEnabled, rlacAppDefaultGroup, isSecure, rmPolicies, sgEnabled, cert_name, 
            		issuer, crlStatus, which, client_addr, password, transport, port, conId, this);
            rc = authQueue.isEmpty() ? 0 : 1;
            authQueue.put(connect);
        } catch (Exception ex) {
            StringWriter sw = new StringWriter();
            ex.printStackTrace(System.err);
            log("CWLNA0996", "Exception in authenticate: {0} ", sw.toString()); 
        }
        return rc;
    }
    
    
    /*
     * Post the authorize request
     */
    public int authorize(String clientID, String client_addr, int action, int operations, String deviceType, String deviceID, String uid, byte[] authToken, int authToken_len, long correlation) {
        System.err.flush();
        int  rc = 5;
             
        try {
        	ImaAuthConnImpl connect = new ImaAuthConnImpl(clientID, client_addr, action, operations, deviceType, deviceID, uid, authToken, authToken_len, correlation,this);
	        rc = authQueue.isEmpty() ? 0 : 1;
	        authQueue.put(connect);
        }catch (Exception ex) {
            StringWriter sw = new StringWriter();
            ex.printStackTrace(System.err);
            log("CWLNA0996", "Exception in authenticate: {0} ", sw.toString()); 
	    }
        
        
        return rc;
    }
    

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#postJSON(java.lang.String, java.lang.String, java.lang.String)
     */
    public ImaProxyResult postJSON(String path, String query, String json) {
        String rstr;
        ImaProxyResult ret = null;
        if (json == null)
            return null;
        try {
            rstr = setJSONn(config, path, query, json);
            ret = new ImaProxyResult(rstr);
        } catch (Exception e) {
            ret = new ImaProxyResult(e);
        }
        return ret;
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#postCertificate(java.lang.String, java.lang.String, byte[])
     */
    public ImaProxyResult postCertificate(String path, String query, byte[] certval) {
        String  rstr;
        ImaProxyResult ret = null;
        if (certval == null)
            return ret;
        try {
            rstr = setBinary(cert, path, query, certval);
            ret = new ImaProxyResult(rstr);
        } catch (Exception e) {
            ret = new ImaProxyResult(e);
        }
        return ret;
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getJSON(java.lang.String, java.lang.String)
     */
    public ImaProxyResult getJSON(String path, String query) {
        String s;
        ImaProxyResult ret = null;
        try {
            s = getJSONn(config, path, query);
            if (s != null && s.charAt(0) != '{' && s.charAt(0) != '[')
                ret = makeResult(s);
            else
                ret = new ImaProxyResult(0, "appliction/json", s);
        } catch (Exception e) {
            ret = new ImaProxyResult(100, "plain/text", e.getMessage());
        }
        return ret;
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getCertificate(java.lang.String, java.lang.String)
     */
    public ImaProxyResult getCertificate(String path, String query) {
        byte [] s;
        ImaProxyResult ret = null;
        try {
            s = getCert(config, path, query);
            ret = new ImaProxyResult(0, "appliction/json", s);
        } catch (Exception e) {
            ret = new ImaProxyResult(100, "plain/text", e.getMessage());
        }
        return ret;
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#startMessaging()
     */
    public void startMessaging() {
        startMsg();
    }
    
    /**
     * Terminate the proxy.
     * Calling this will cause the proxy to terminate.
     * This allows an orderly shutdown of the proxy.
     * @param rc  The return code traced and sent to the operating system.
     * 
     * @see com.ibm.ima.proxy.ImaProxyListener#terminate(int)
     */
    public void terminate(int rc) {
        quitProxy(rc);
    }
    
    /**
     * Log a message to the proxy system log.
     * @param msgid  The message identifier.  If this is a message in a catalog known to the proxy
     *               is will use the message from the message catalog instead of the specified 
     *               message format.
     * @param msgformat  The message to log as a Java MessageFormat string.
     * @param repl   Zero or more replacement values for the message format.             
     * 
     * @see com.ibm.ima.proxy.ImaProxyListener#log(java.lang.String, java.lang.String, java.lang.Object[])
     */
    public void log(String msgid, String msgformat, Object ... repl) {
        Exception e = new Exception();
        StackTraceElement [] elem = e.getStackTrace();
        String filen = "java";
        int    lineno = 0;
        if (elem.length > 1) {
            filen = elem[1].getFileName();
            lineno = elem[1].getLineNumber();
        }    
        StringBuffer sb = new StringBuffer();
        sb.append('[');
        for (int i=0; i<repl.length; i++) {
            if (i != 0)
                sb.append(',');
            ImaJson.put(sb, repl[i]);
        }
        sb.append(']');
        if (msgid == null)
            msgid = "";
        doLog(msgid, msgformat, filen, lineno, sb.toString());
    }
    
    /**
     * Disconnect any connections matching the selected criteria.
     * <p>
     * This form of disconnect is commonly used when on object is to be deleted 
     * to ensure that there are no open connections when the object is deleted.
     * The common sequence is to disable an object which stops new connections,
     * then disconnect any existing connections using that object, and then delete
     * the object.  Users have no disable, but can be deleted while there are active
     * connections, so to delete a user it can be deleted and then all connections 
     * for that user disconnected.
     * <p>
     * To be disconnected a connection must match all specified values.
     * A null match string does not participate in the selection.
     * Any of the selection strings can contain an asterisk which will match 0 or more
     * characters.
     * <p>
     * This method returns when all of the connections have been selected to be disabled
     * so the connections might still exist when this method returns.  
     * 
     * @param endpoint  The name of the endpoint 
     * @param server    The name of the server
     * @param tenant    The name of the tenant
     * @praam user      The name of the user
     *    
     * @see com.ibm.ima.proxy.ImaProxyListener#disconnect(java.lang.String, java.lang.String, java.lang.String, java.lang.String)
     */
    public int disconnect(String endpoint, String server, String tenant, String user) {
        return doDisconnect(endpoint, server, tenant, user, (String)null, 0);
    }
    
    /**
     * Disconnect any connection which matches the clientID specification.
     * <p>
     * This form of disconnect is used to disconnect a particular client or group of clients
     * with a known clientID.  
     * <p>
     * This method returns when all of the connections have been selected to be disabled
     * so the connections might still exist when this method returns.
     *  
     * @param clientID  The clientID which can contain asterisks which match 0 or more characters.
     */
    public int disconnect(String clientID) {
        return doDisconnect((String)null, (String)null, (String)null, (String)null, clientID, 0);
    }
    
    /**
     * Disconnect any connection which matches the clientid specification.
     * <p>
     * This form of disconnect is used to disconnect a particular client or group of clients
     * with a known userid.  
     * <p>
     * This method returns when all of the connections have been selected to be disabled
     * so the connections might still exist when this method returns.
     *  
     * @param userID  The client of the application
     * @param operationBits  The operation bits
     */
    public int disconnect(String clientID, int operationBits) {
        return doDisconnect((String)null, (String)null, (String)null, (String)null, clientID, operationBits);
    }
    
    /**
     * Disconnect any connection which matches the userid specification for Application
     * <p>
     * This form of disconnect is used to disconnect a particular client or group of clients
     * with a known userid.  
     * <p>
     * This method returns when all of the connections have been selected to be disabled
     * so the connections might still exist when this method returns.
     *  
     * @param userID  The userID or username of the application
     * @param operationBits  The operation bits
     * @return the closed connections
     */
    public int disconnectApp(String userID, int operationBits) {
        return doDisconnect((String)null, (String)null, (String)null, userID, (String)null, operationBits);
    }
    
    /**
     * Get a global configuration item.
     * @param name  The name of the config item
     * @return The value of the config item
     */
    public Object getConfigItem(String name) {
        String json = getJSONn(item, name, null);
        if (json != null && json.length() > 0 && json.charAt(0)=='{') {
            ImaJson jparse = new ImaJson();
            jparse.parse(json);
            return jparse.getValue(1);
        }
        throw makeException(json);
    }
    
    /**
     * Set a global configuration item.
     * Only a subset of the global configuration items can be set at runtime.
     * @param name   The name of the config item
     * @param value  The value of the config item
     */
    public void setConfigItem(String name, Object value) {
        StringBuffer sb = new StringBuffer();
        sb.append("{ ");
        ImaJson.putString(sb, name);
        sb.append(": ");
        ImaJson.put(sb, value);
        sb.append(" }");
        String ret = setJSONn(config, null, null, sb.toString());
    }
    
    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getEndpoint(java.lang.String)
     */
    public Map<String, Object> getEndpoint(String name) {
        String json = getJSONn(endpoint, name, null);
        if (json != null && json.length() > 0 && json.charAt(0)=='{') {
            ImaJson jparse = new ImaJson();
            jparse.parse(json);
            return jparse.toMap(0);
        }
        throw makeException(json);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getTenant(java.lang.String)
     */
    public Map<String, Object> getTenant(String name) {
        String json = getJSONn(tenant, name, null);
        if (json != null && json.length() > 0 && json.charAt(0)=='{') {
            ImaJson jparse = new ImaJson();
            jparse.parse(json);
            return jparse.toMap(0);
        }
        throw makeException(json);
    }


    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getServer(java.lang.String)
     */
    public Map<String, Object> getServer(String name) {
        String json = getJSONn(server, name, null);
        if (json != null && json.length() > 0 && json.charAt(0)=='{') {
            ImaJson jparse = new ImaJson();
            jparse.parse(json);
            return jparse.toMap(0);
        }
        throw makeException(json);
    }


    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getUser(java.lang.String, java.lang.String)
     */
    public Map getUser(String name, String tenant) {
        String json = getJSONn(user, name, tenant);
        if (json != null && json.length() > 0 && json.charAt(0)=='{') {
            ImaJson jparse = new ImaJson();
            jparse.parse(json);
            return jparse.toMap(0);
        }
        throw makeException(json);
    }
    
    /**
     * Get a list of the defined endpoints.
     * @param match  A string to match against the endpoint name,  This can contain one or more asterisks (*)
     *               to match 0 or more characters.
     * @return An array of endpoint names
     * 
     * @see com.ibm.ima.proxy.ImaProxyListener#getEndointList(java.lang.String)
     */
    public String [] getEndpointList(String match) {
        String json = getJSONn(endpoint + list, match, null);
        if (json != null && json.length() > 0 && json.charAt(0)=='[') {
            ImaJson jparse = new ImaJson();
            jparse.parse(json);
            return jparse.toArray(0);
        }
        throw makeException(json);    
    }
    
    /**
     * Get a list of the defined tenants.
     * @param match  A string to match against the tenant name,  This can contain one or more asterisks (*)
     *               to match 0 or more characters.
     * @return An array of tenant names
     *
     * @see com.ibm.ima.proxy.ImaProxyListener#getTenantList(java.lang.String)
     */
    public String [] getTenantList(String match) {
        String json = getJSONn(tenant + list, match, null);
        if (json != null && json.length() > 0 && json.charAt(0)=='[') {
            ImaJson jparse = new ImaJson();
            jparse.parse(json);
            return jparse.toArray(0);
        }
        throw makeException(json);    
    }
    
    /**
     * Get a list of the defined servers.
     * @param match  A string to match against the sever name,  This can contain one or more asterisks (*)
     *               to match 0 or more characters.
     * @return An array of server names
     * 
     * @see com.ibm.ima.proxy.ImaProxyListener#getServerList(java.lang.String)
     */
    public String [] getServerList(String match) {
        String json = getJSONn(server + list, match, null);
        if (json != null && json.length() > 0 && json.charAt(0)=='[') {
            ImaJson jparse = new ImaJson();
            jparse.parse(json);
            return jparse.toArray(0);
        }
        throw makeException(json);     
    }
    
    /**
     * Get a list of the defined users.
     * @param match  A string to match against the user name,  This can contain one or more asterisks (*)
     *               to match 0 or more characters.
     * @param tenant The name of the tenant.  If this is null then global users are returned.               
     * @return An array of user names
     * 
     * @see com.ibm.ima.proxy.ImaProxyListener#getUserList(java.lang.String, java.lang.String)
     */
    public String [] getUserList(String match, String tenant) {
        String json = getJSONn(user + list, match, tenant);
        if (json != null && json.length() > 0 && json.charAt(0)=='[') {
            ImaJson jparse = new ImaJson();
            jparse.parse(json);
            return jparse.toArray(0);
        }
        throw makeException(json);     
    }
    
    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getErrorString(int)
     */
    public String getErrorString(int rc) {
        return ImaProxyResult.getErrorString(rc);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getErrorString(int, java.util.Locale)
     */
    public String getErrorString(int rc, Locale locale) {
        return getErrorString(rc);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#setEndpoint(java.lang.String, java.util.Map)
     */
    public void setEndpoint(String name, Map<String, Object> value) {
        String json = ImaJson.fromMap(value);
        String rstr = setJSONn(endpoint, name, null, json);
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#setTenant(java.lang.String, java.util.Map)
     */
    public void setTenant(String name, Map<String, Object> value) {
        String json = ImaJson.fromMap(value);
        String rstr = setJSONn(tenant, name, null, json);
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#setServer(java.lang.String, java.util.Map)
     */
    public void setServer(String name, Map<String, Object> value) {
        String json = ImaJson.fromMap(value);
        String rstr = setJSONn(server, name, null, json);
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#setUser(java.lang.String, java.lang.String, java.util.Map)
     */
    public void setUser(String name, String tenant, Map<String, Object> value) {
        String json = ImaJson.fromMap(value);
        String rstr = setJSONn(user, name, tenant, json);
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }

    /**
     * @see com.ibm.ima.proxy.ImaProxyListener@setACL(java.lang.String data)
     */
    public void setACL(String data) {
        String rstr = setJSONn(acl, null, null, data);    
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }
    
    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#deleteEndpoint(java.lang.String, boolean)
     */
    public boolean deleteEndpoint(String name, boolean force) {
        return deleteObj(endpoint, name, null, force);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#deleteTenant(java.lang.String, boolean)
     */
    public boolean deleteTenant(String name, boolean force) {
        return deleteObj(tenant, name, null, force);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#deleteServer(java.lang.String, boolean)
     */
    public boolean deleteServer(String name, boolean force) {
        return deleteObj(server, name, null, force);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#deleteUser(java.lang.String, java.lang.String, boolean)
     */
    public boolean deleteUser(String name, String tenant, boolean force) {
        return deleteObj(user, name, tenant, force);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getObfuscatedPassword(java.lang.String, java.lang.String, int)
     */
    public String getObfuscatedPassword(String user, String pw, int type) {
        return getObfus(user, pw, type);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getEndpointStats(java.lang.String)
     */
    public String getEndpointStats(String name) {
        return getStats(endpoint, name);
    }

    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#getServerStats(java.lang.String)
     */
    public String getServerStats(String name) {
        return getStats(server, name);
    }
    
    /*
     * Make an exception from a string return
     */
    RuntimeException makeException(String s) {
        int  rcval = -1;
        boolean  replonly = true;
        RuntimeException ret;
        int colon1 = -1;
        int colon2 = -1;
        if (s != null) {
            colon1 = s.indexOf(':');
            colon2 = s.indexOf(':', colon1+1);
        } 
        if (colon1<0 || colon2<0) {
            ret = new IllegalArgumentException("Return code not valid");
        } else {
            String rc;
            if (s.charAt(0)=='+') {
                replonly = false;
                rc = s.substring(1, colon1);
            } else {
                rc = s.substring(0, colon1);
            }
                
            try { 
                rcval = Integer.parseInt(rc); 
            } catch (Exception e) {
                ret = new IllegalArgumentException("Return code not valid");
                return ret;
            }
            String except = s.substring(colon1+1, colon2);
            String reason = s.substring(colon2+1);
            
            if (reason.length() > 0) {
                if (replonly)
                    reason = getErrorString(rcval) + ": " + reason;
            } else {
                reason = getErrorString(rcval);
            }
            
            if (except.equals("IllegalArgumentException")) {
                ret = new IllegalArgumentException(reason + " (" + rc + ')');
            } else if (except.equals("IllegalStateException")) {
                ret = new IllegalArgumentException(reason + " (" + rc + ')');
            } else {
                ret = new RuntimeException(reason + " (" + rc + ')');   
            }
        }
        return ret;
    }

    /*
     * Make a result proxy in the case of an error
     */
    ImaProxyResult makeResult(String s) {
        ImaProxyResult ret;
        int colon1 = -1;
        int colon2 = -1;
        if (s != null) {
            colon1 = s.indexOf(':');
            colon2 = s.indexOf(':', colon1+1);
        }    
        if (colon1<0 || colon2<0) { 
            ret = new ImaProxyResult(100, "plain/text", "Return code not valid");
        } else {
            String rc = s.substring(0, colon1);
            String reason = s.substring(colon2+1);
            ret = new ImaProxyResult(Integer.valueOf(rc), "plain/text", reason);
        }
        return ret;
    }

    static int thread_cnt = 0;
    
    /**
     * Register an authenticator.
     * @param auth  The authenticator
     */
    public void setAuthenticator(ImaAuthenticator auth) {

        if (auth != null) {
            synchronized (authQueue) {
                if (thread_cnt == 0) {
                    thread_cnt = getThreadCount();
                    if (thread_cnt <= 0)
                        thread_cnt = 1;
                    for (int i=0; i<thread_cnt; i++) {
                        Thread th = new Thread(this, "AuthThread"+i);
                        th.start();
                    }
                }    
                doSetAuth(true);
                
                doSetAuthorize(true);
                authenticator = auth;
            }
        } else {
            doSetAuth(false);
            doSetAuthorize(false);
        }   
    }
    
    
    /**
     * Return from authenticate.
     * 
     * @param correlate  An object sent on the authenticate call
     * @param good       true if the user is authenticated, false otherwise
     */
    public void authenticated(ImaAuthConnImpl correlate, boolean good) {
        doAuth(correlate.transport, good, 0);
    }
    
    /**
     * Return authenticate result with rc
     * @param correlate
     * @param good
     * @param rc
     */
    public void authenticated(ImaAuthConnImpl correlate, boolean good, int rc) {
        doAuth(correlate.transport, good, rc);
    }
    
    /**
     * set Authorize result with rc and result
     * @param correlate correlation object
     * @param rc the return code
     * @param reason the reason value
     */
    public void authorized(ImaAuthConnImpl correlate, int rc, String reason) {
    	//System.out.println("Authorized: ");
        doAuthorized(correlate.correlation, rc, reason);
    }
    
    /**
     * set Authorize result with rc
     * @param correlate correlation object
     * @param rc the return code
     */
    public void authorized(ImaAuthConnImpl correlate, int rc) {
        doAuthorized(correlate.correlation, rc, null);
    }
    
    public void setDeviceUpdater(ImaDeviceUpdater deviceUpdater) {
    		this.deviceUpdater = deviceUpdater;
    		doSetDeviceUpdater(true);
    }
    
    
    public int deviceStatusUpdate(String deviceStatusJson) {
    	
    	/*Form the status and submit to a thread*/
    	//System.out.println("deviceStatusUpdate: json="+deviceStatusJson);
    	ImaDeviceStatusImpl deviceStatus = new ImaDeviceStatusImpl(deviceStatusJson);
    	deviceUpdater.deviceStatusUpdate(deviceStatus);
    	return 0;
    }
    
    boolean getAaaEnabled() {
        return aaaEnabled ;
    }
    boolean getSgEnabled() {
        return sgEnabled ;
    }
    
    
    /*
     * @see com.ibm.ima.proxy.ImaProxyListener#setDevice(java.lang.String, java.util.Map)
     */
    public void setDevice(String name, String json) {
        String rstr = setJSONn(device, name, null, json);
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }
    
    /**
     * @see com.ibm.ima.proxy.ImaProxyListener@setMHubBindings(java.lang.String json)
     */
    public void setMHubBindings(String json) {
        String rstr = setJSONn(mhub_bindings, null, null, json);    
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }
    
    /**
     * @see com.ibm.ima.proxy.ImaProxyListener@setMHubBindings(java.lang.String name, java.lang.String json)
     */
    public void setMHubBindings(String name, String json) {
        String rstr = setJSONn(mhub_bindings, name, null, json);    
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }
    
    /**
     * @see com.ibm.ima.proxy.ImaProxyListener@setMHubCredentials(java.lang.String json)
     */
    public void setMHubCredentials(String json) {
        String rstr = setJSONn(mhub_credentials, null, null, json);    
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }
    
    /**
     * @see com.ibm.ima.proxy.ImaProxyListener@setMHubCredentials(java.lang.String name, java.lang.String json)
     */
    public void setMHubCredentials(String name, String json) {
        String rstr = setJSONn(mhub_credentials, name, null, json);    
        if (rstr != null && rstr.length() > 0)
            throw makeException(rstr);
    }
    
    public void setMHUBCredentialManager(ImaMHubCredentialManager mHubCredMgr) {
		this.mHubCredMgr = mHubCredMgr;
		doSetMHUBCredentialManager(true);
    }
    
    public int getMHubCredential(String orgId, String serviceId) {
	 	this.mHubCredMgr.getMHubCredential(orgId, serviceId);
	    	return 0;
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
}
