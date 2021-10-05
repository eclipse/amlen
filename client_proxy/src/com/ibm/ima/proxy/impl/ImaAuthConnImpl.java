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

import com.ibm.ima.proxy.ImaAuthConnection;

public class ImaAuthConnImpl implements ImaAuthConnection {
    final String clientID;
    final String userID;
    String protocolUserID;
    String password;
    String cert_name;
    String issuer;
    boolean isSecure;
    boolean isNameMatch;
    boolean isExactNameMatch;
    CrlState crlState;
    String client_addr;
    long   transport;
    int clientPort;
    long connectionId;
    boolean isAuthenticated; 
    ImaProxyImpl impl;
    /*Authorization Attributes*/
    long correlation;
    int action;
    int permissions;
    String deviceType;
    String deviceID;
    boolean isAuthorized;
    boolean  sgEnabled;
    byte[] authToken;
    int authToken_len;
    boolean rlacAppEnabled;
    boolean rlacAppDefaultGroup;
    int rmPolicies;
    
    
    /*
     * Package private constructor
     */
    ImaAuthConnImpl(String clientID, String userID, int rlacAppEnabled, int rlacAppDefaultGroup, int isSecure, int rmPolicies,  int sgEnabled, String cert_name, String issuer, int crlStatus, int which, String client_addr, 
            String password, long transport, int port, long conId, ImaProxyImpl impl) {
    	this.action=ImaAuthConnection.ACTION_AUTHENTICATION;
        this.clientID = clientID != null ? clientID : "";
        this.protocolUserID = userID;
        this.password = password != null ? password : "";
        this.transport = transport;
        this.client_addr = client_addr != null ? client_addr : "";
        this.cert_name = cert_name;
        this.issuer = issuer;
        this.isSecure = (isSecure != 0);
        this.isNameMatch = (isSecure > 1);
        this.isExactNameMatch = (isSecure > 3);
        this.rmPolicies = rmPolicies;
        this.crlState = CrlState.valueOf(crlStatus);
        this.clientPort = port;
        this.connectionId = conId;
        switch (which) {
        case 1:   this.userID = clientID;   break;
        case 2:   this.userID = cert_name == null ? cert_name : "";  break;
        default:  this.userID = userID;
        }
        this.impl = impl;
        if(impl.getSgEnabled() && (sgEnabled != 0)) {
            this.sgEnabled = true;
        } else {
            this.sgEnabled = false;
        }
        this.rlacAppEnabled = (rlacAppEnabled!=0);
        this.rlacAppDefaultGroup = (rlacAppDefaultGroup!=0);
    }
    
    /*
     * Package private constructor
     */
    ImaAuthConnImpl(String clientID, String client_addr, int action, int permissions, String deviceType, String deviceID, String uid, byte[] authToken, 
            int authToken_len, long correlation, ImaProxyImpl impl) {
    		this.clientID = clientID;
    		this.client_addr = client_addr != null ? client_addr : "";
    		this.action = action;
    		this.permissions = permissions;
        this.deviceID = deviceID;
        this.deviceType = deviceType;
        this.userID = uid;
        this.authToken = authToken;
        this.authToken_len = authToken_len;
        this.correlation = correlation;
        this.impl = impl;
    }
    
    /*
     * Gets the client ID of the connection.
     * @return the client ID
     * 
     * @see com.ibm.ima.proxy.ImaAuthConnection#getClientID()
     */
    public String getClientID() {
        return clientID;
    }
    
    /**
     * Gets the user ID of the connection.
     * @return the user ID.  If the user ID is not set, return a zero length string.
     * 
     * @see com.ibm.ima.proxy.ImaAuthConnection#getUserID()
     */
    public String getUserID() {
        return userID;
    }
    
    /**
     * Gets the password of the connection.
     * @return the password.  If the password is not set, return a zero length string.
     *
     * @see com.ibm.ima.proxy.ImaAuthConnection#getPassword()
     */
    public String getPassword() {
        return password;
    }
    
    /**
     * Gets the user ID of the connection as specified by the protocol.
     * @return the protocol user ID.  If the client does not supply a userID this will be null.
     *
     * @see com.ibm.ima.proxy.ImaAuthConnection#getProtocolUserID()
     */
    public String getProtocolUserID() {
        return protocolUserID;
    }
    
    /**
     * Gets the client IP address of the connection.
     * @return the client IP address.  If this is IPv6 is will be within square brackets.
     *
     * @see com.ibm.ima.proxy.ImaAuthConnection#getClientAddress()
     */
    public String getClientAddress() {
        return client_addr;
    }
    
    /**
     * Gets the certificate name the connection.
     * @return the certificate common name.  If there is not certificate or the certificate does not have a name this will be null.
     * 
     * @see com.ibm.ima.proxy.ImaAuthConnection#getCertificateName()
     */
    public String getCertificateName() {
        return cert_name;
    }
    

    /**
     * Gets the Risk Management Policy Bits
     *  ISM_TENANT_RMPOLICY_REQSECURE	0x0001
     *  ISM_TENANT_RMPOLICY_REQCERT      0x0002
     *  ISM_TENANT_RMPOLICY_REQUSER      0x0004
     *  ISM_TENANT_RMPOLICY_REQSNI       0x0008
     * @return policy bits
     * @see com.ibm.ima.proxy.ImaAuthConnection#getCertificateName()
     */
    public int getRMPolicies() {
        return rmPolicies;
    }

    
    /**
     * Authenticate the connection.
     * <p>
     * A connection must be authenticated once and only once.  The authenticator must keep
     * this object and invoke the authenticate method to indicate either success or failure
     * of the authentication. 
     * 
     * @param good  true if the connection is authenticated, false otherwise
     * @throws IllegalStateException if authenticate is called more than once
     * 
     * @see com.ibm.ima.proxy.ImaAuthConnection#authenticate(boolean)
     */
    public void authenticate(boolean good) {
        authenticate(good, 0);
    }
    
    /**
     * Authenticate the connection with return code.
     *
     * A connection must be authenticated once and only once.  The authenticator must keep
     * this object and invoke the authenticate method to indicate either success or failure
     * of the authentication. 
     * 
     * @param good  true if the connection is authenticated, false otherwise
     * @param rc the return code. 
     * @throws IllegalStateException if authenticate is called more than once
     *
     * @see com.ibm.ima.proxy.ImaAuthConnection#authenticate(boolean, int)
     */
    public void authenticate(boolean good, int rc) {
        if (isAuthenticated) {
            throw new IllegalStateException("The connection is already authenticated");
        }
        isAuthenticated = true;
        impl.authenticated(this, good, rc);
    }
    
    /**
     * Authorize the connection with return code.
     *
     * @param rc  return code
     * @param result result value 
     * @throws IllegalStateException if authenticate is called more than once
     * 
     * @see com.ibm.ima.proxy.ImaAuthConnection#authorize(int, java.lang.String)
     */
    public void authorize(int rc, String result) {
        impl.authorized(this, rc, result);
    }
    
    /**
     * Authorize the connection with return code.
     * @see com.ibm.ima.proxy.ImaAuthConnection#authorize(int)
     */
    public void authorize(int rc) {
        impl.authorized(this, rc);
    }
    
    
	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#getDeviceId()
	 */
	public String getDeviceId() {
		
		return deviceID;
	}

	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#getDeviceType()
	 */
	public String getDeviceType() {
		
		return deviceType;
	}

	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#getAuthToken()
	 */
	public byte[] getAuthToken() {
		
		return authToken;
	}

	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#getAuthTokenLenghth()
	 */
	public int getAuthTokenLenghth() {
		return authToken_len;
	}

	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#getAction()
	 */
	public int getAction() {
		return action;
	}
	
	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#getPermissions()
	 */
	public int getPermissions() {
		return permissions;
	}
	
	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#getAaaEnabled()
	 */
	public boolean getAaaEnabled() {
		return impl.getAaaEnabled();
	}
	
	

    /*
     * @see java.lang.Object#toString()
     */
    public String toString() {
        String ret = "ImaAuthConnection ClientID=\"" + clientID + "\" UserID=\"" + userID +
               "\" ClientAddr=\"" + client_addr + "\"";
        if (protocolUserID != null && protocolUserID.length() > 0 && !protocolUserID.equals(userID)) {
            ret += " ProtocolUserID=\"" + protocolUserID + "\"";
        }
        if (cert_name != null) {
            ret += " CertificateName=\"" + cert_name + "\"";
        }
        if (password.length() > 0) {
            ret += " Password=\"****\"";
        }
        if (action >=0) {
        	ret += " action=\"" + action + "\"";
        }
        if (deviceID != null) {
        	ret += " deviceID=\"" + deviceID + "\"";
        }
        if (deviceType != null) {
        	ret += " deviceType=\"" + deviceType + "\"";
        }
        return ret;
        
    }

    /*
     * @see com.ibm.ima.proxy.ImaAuthConnection#getCertificateIssuer()
     */
	public String getCertificateIssuer() {
		return issuer;
	}

	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#isSecure()
	 */
	public boolean isSecure() {
		return isSecure;
	}

	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#isNameMatch()
	 */
	public boolean isNameMatch() {
	    return isNameMatch;
	}
	
	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#isExactNameMatch()
	 */
    public boolean isExactNameMatch() {
        return isExactNameMatch;    
    }
    
	/*
	 * @ see com.ibm.ima.proxy.ImaAuthConnection#getSecGuardianEnabled
	 */
	public boolean getSecGuardianEnabled() {
	    return sgEnabled;
	}    

	/*
	 * @see com.ibm.ima.proxy.ImaAuthConnection#getCrlState()
	 */
	public CrlState getCrlState() {
		return crlState;
	}

	@Override
	public boolean getRLACAppEnabled() {
		return rlacAppEnabled;
	}

	@Override
	public boolean getRLACAppDefaultGroup() {
		return rlacAppDefaultGroup;
	}

	@Override
	public int getClientPort() {
		// TODO Auto-generated method stub
		return clientPort;
	}

	@Override
	public long getConnectionId() {
		// TODO Auto-generated method stub
		return connectionId;
	}

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

	
}
