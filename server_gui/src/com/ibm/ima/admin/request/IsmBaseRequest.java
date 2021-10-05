/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.admin.request;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;

import com.ibm.ima.admin.IsmClientType;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;

public abstract class IsmBaseRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

	
    private final static String CLAS = IsmBaseRequest.class.getCanonicalName();
    protected final static IsmLogger logger = IsmLogger.getGuiLogger();


    @JsonSerialize
    protected String Action;
    @JsonSerialize
    protected String User;
    @JsonSerialize
    protected String Locale;
    @JsonIgnore
    protected IsmClientType clientType;
    @JsonIgnore
    protected String serverInstance = null;
    @JsonIgnore
    protected int port = IsmClientType.ISM_ClientType_Configuration.getPort();
    @JsonIgnore
    protected String serverIPAddress = "127.0.0.1";
    
    @JsonIgnore 
    public void setClientType(IsmClientType clientType) {
    	this.clientType = clientType;
    	if (clientType != null) {
    		port = clientType.getPort();
    	}
    	
    	if (clientType.equals(IsmClientType.ISM_ClientType_Configuration) || clientType.equals(IsmClientType.ISM_ClientType_Monitoring)) {
    		/*
    		 * Get Server IP and Port from Environment variable
    		 * NOTE: This code is temporarily added to continue prototype
    		 *       work for WebUI docker image
    		 */
    		String portstr = System.getenv("IMA_SERVER_PORT");
    		if (portstr != null && !portstr.isEmpty()) {
    			try {
    				port = Integer.parseInt(portstr);
    			} catch (NumberFormatException nfe) {
    				logger.log(LogLevel.LOG_ERR, CLAS, "setClientType", "CWLNA5054", new Object[] {portstr});
    			}
    		}
    		String ipstr = System.getenv("IMA_SERVER_HOST");
    		if (ipstr != null && !ipstr.isEmpty()) {
    			serverIPAddress = ipstr;
    		}
    	}
    }
    
    /**
     * @return the clientType
     */
    @JsonIgnore
    public IsmClientType getClientType() {
        return clientType;
    }
    
    /**
     * 
     * @return the server instance or null if none set
     */
    @JsonIgnore
    public String getServerInstance() {
    	return serverInstance;
    }
    
    /**
     * Set server instance
     * @param serverInstance
     */
    @JsonIgnore
    public void setServerInstance(String serverInstance) {
    	logger.traceEntry(CLAS, "setServerInstance", new Object[] {serverInstance});
    	if (serverInstance == null || serverInstance.isEmpty() || serverInstance.equals("0") || (serverInstance.startsWith("127.0.0.1") && !Utils.getUtils().isDockerContainer())) {
    		logger.trace(CLAS, "setServerInstance", "setting to null");
    		serverInstance = null;
    	} else {
    		// parse out port and IP address
    		// is it an IPv6 address? format is [address]:port
    		String[] parts = serverInstance.split("]");
    		String portString = null;
    		String ipString = null;
    		if (parts.length > 1 && !parts[1].isEmpty()) {
    			// last part is the port
    	        if (parts[1] != null) {
    	        	portString = parts[1];
    	        }
    			// now get the IP address    	        
    	        ipString = parts[0].startsWith("[") ? parts[0].substring(1) : parts[0];
    		} else {
    			ipString = parts[0];
    			parts = ipString.split(":");
    			if (parts.length == 2) {
    				ipString = parts[0];
    				portString = parts[1];
    			}
    		}
    		serverIPAddress = ipString;
    		if (portString != null) {
	        	try {
	        		port = Integer.parseInt(portString);
	        	} catch (NumberFormatException nfe) {
	        		logger.log(LogLevel.LOG_ERR, CLAS, "setServerInstance", "CWLNA5054", new Object[] {parts[1]});
	        	}
    		}    		
    	}
    	this.serverInstance = serverInstance;
    	logger.trace(CLAS, "setServerInstance", "serverInstance: " + serverInstance + "; serverIPAddress: " + serverIPAddress + "; port: " + port);
    }    
    @JsonIgnore
    public int getPort() {
		return port;
	}
    @JsonIgnore
	public String getServerIPAddress() {
		return serverIPAddress;
	}

    /**
     * Returns true if the server being managed is the localhost, which is only the
     * case when the Web UI is running on a virtual or physical appliance...
     * UPDATE:  If all containers are on the same host, it might be localhost even though
     * not an appliance...
     * @return
     */
    @JsonIgnore
	public boolean isLocalServer() {
    	boolean isLocal = false;
    	if (!Utils.getUtils().isDockerContainer()) {
    		 isLocal = serverInstance == null || serverIPAddress == null || serverIPAddress.equals("127.0.0.1");
    	}
    	logger.traceExit(CLAS, "isLocal", isLocal);
		return isLocal;
	}

	/**
	 * @return the user
	 */
    @JsonProperty("User")
	public String getUser() {
		return User;
	}

	/**
	 * @param user the user to set
	 */
	public void setUser(String user) {
		User = user;
	}
	
	/**
	 * Set the locale prefered by the client.
	 * 
	 * @param locale the user client locale
	 */
	public void setLocale(String locale) {
		Locale = locale;
	}
	
    @JsonProperty("Action")
    public String getAction() {
        return Action;
    }

    @JsonProperty("Action")
    public void setAction(String action) {
        this.Action = action;
    }


}
